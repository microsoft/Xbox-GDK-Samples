#include "Common.hlsli"
#include "BlockColours.hlsli"

RWTexture2D<float4> output : register(u0);

ByteAddressBuffer blockIDBuffer : register(t2);

TextureCube<float4> skyRadianceTexture : register(t3);
TextureCube<float4> skyIrradianceTexture : register(t4);

SamplerState skySampler : register(s0);

static const uint STACK_DEPTH = 24;
static const uint STACK_SIZE = WAVE_SIZE * STACK_DEPTH;

// Determined experimentally
static const float SHADOW_EPSILON = 0.0004f;

groupshared uint g_stack[STACK_SIZE];

struct RaytraceParams
{
    float ray_extent;
    float3 ray_origin;
    float3 ray_direction;
};

struct RaytraceResults
{
    bool hit;
    uint primitiveIndex;
    uint3 blockCoord;
    uint numIterations;
    uint blockIDHit;
};

float3 GetSkyColour(float3 rayDir, float mip = 0.0f)
{
    return skyRadianceTexture.SampleLevel(skySampler, rayDir, mip).rgb;
}

float3 GetDirectionAOColour(float3 rayDir)
{
    return skyIrradianceTexture.SampleLevel(skySampler, rayDir, 0).rgb;
}

RaytraceResults Raytrace(in RaytraceParams input)
{
#if ENABLE_SCARLETT_INTERSECTION_TEST_INTRINSICS
    GpuVA va = vaBVH;
#else
    GpuVA va = 0;
#endif

    float3 inv_ray_dir = rcp(input.ray_direction);
    uint nodePtr = 0 | NODE_TYPE_BOX16;
    uint prevNodePtr = InvalidNodePtr;
    uint stackPtr = __XB_GetLaneID();
    uint4 childPtrs = 0;

    uint numIterations = 0;
    uint timesUniform = 0;



    // The core traversal loop.
    // More details in Readme.docx to save polluting the code with comments on every line
    do
    {
        prevNodePtr = nodePtr;
        childPtrs = asuint(BvhIntersectRay(va, nodePtr, input.ray_extent, input.ray_origin, input.ray_direction, inv_ray_dir, bvhSize, true));
        nodePtr = childPtrs.x;

        if (nodePtr != InvalidNodePtr)
        {
            for (int i = 3; i > 0; i--)         // Add them in reverse order so the nearest ones pop first
            {
                if (childPtrs[i] < 0x40000000)  // Not invalid and not a leaf. Don't add leaves to the stack!
                {
                    g_stack[stackPtr] = childPtrs[i];
                    stackPtr += WAVE_SIZE;
                }
            }
        }
        else    // Need to go back up the stack...
        {
            stackPtr -= WAVE_SIZE;
            nodePtr = (stackPtr < STACK_SIZE) ? g_stack[stackPtr] : InvalidNodePtr;
        }

        numIterations++;
    } while (nodePtr < 0x40000000);





    uint x = nodePtr & 0x7FF;
    uint y = (nodePtr >> 11) & 0xFF;
    uint z = (nodePtr >> 19) & 0x7FF;

    RaytraceResults results;
    results.hit = (nodePtr != InvalidNodePtr);
    results.blockCoord = uint3(x, y, z);
    results.numIterations = numIterations;
    results.blockIDHit = 0;
    results.primitiveIndex = 0;

    if (results.hit)
    {
        // Calculate which leaf node we hit and produce a 0-based index relative to the first leaf node.
        uint leafNodeIndex = (prevNodePtr >> 3) - firstLeafNodeIndex;
        // Four AABBs per leaf FP16 AABB node
        uint boxIndex = leafNodeIndex * 4;

        // Fetch a DWORD containing the four block IDs for the four AABBs in our leaf node
        uint fourBlockIDs = blockIDBuffer.Load(boxIndex);

        // Need to work out whether the box we hit was box 0, 1, 2 or 3 in the node.
        // Just do a quick search through childPtrs to see which one we were.
        GpuVA vaOfParentNode = NodePointerToVA(0, prevNodePtr);
        uint3 pointers = GpuVALoad3(vaOfParentNode);

        // In order to determine whether the block we hit was the 1st, 2nd, 3rd or 4th AABB in the node
        // we have to compare the node pointer of the node we hit with the 4 node pointers in our parent's
        // set of 4 child pointers.
        uint subNodeIndex = 3;
        subNodeIndex = (pointers.x == nodePtr) ? 0 : subNodeIndex;
        subNodeIndex = (pointers.y == nodePtr) ? 1 : subNodeIndex;
        subNodeIndex = (pointers.z == nodePtr) ? 2 : subNodeIndex;

        uint blockIDHit = (fourBlockIDs >> (8 * subNodeIndex)) & 0xFF;  // Extract our byte from the DWORD.

        results.blockIDHit = blockIDHit;
        results.primitiveIndex = boxIndex + subNodeIndex;
    }

    return results;
}

// A little array containing the normal of the voxel based on which side of the voxel we hit (0-5)
static const float3 sideNormals[6] =
{
    float3(1, 0, 0),
    float3(-1, 0, 0),
    float3(0, 1, 0),
    float3(0, -1, 0),
    float3(0, 0, 1),
    float3(0, 0, -1),
};

[RootSignature("DescriptorTable(SRV(t0)), DescriptorTable(UAV(u0), SRV(t3), SRV(t4)), SRV(t1), RootConstants(b0, num32bitconstants=28), UAV(u1), SRV(t2), StaticSampler(s0)")]
[numthreads(TG_WIDTH, TG_HEIGHT, 1)]
void main( uint3 DTid : SV_DispatchThreadID)
{
    RaytraceParams params = { FLT_INF, cameraPosition.xyz, GetRayDirection(DTid.xy + 0.5f) };
    RaytraceResults results = Raytrace(params);

    RecordRayStats(results.hit, 0);
    
    const float3 sunColor = float3(1,1,1);
    float3 sunDirection = normalize(float3(sin(elapsedTime), 1.25f, cos(elapsedTime)));

    float3 finalColour = 0;
    float nearestT = 0;

    if (results.hit)
    {
        float3 diffuseComponent = 0;
        float3 specularComponent = 0;

        float3 aabbMin = results.blockCoord;
        float3 aabbMax = aabbMin + 1.0f;
        uint sideEntered;
        AABBIntersect(params.ray_origin, rcp(params.ray_direction), aabbMin, aabbMax, sideEntered, nearestT);

        float3 primaryRayEnd = params.ray_origin + (params.ray_direction * nearestT);
        float3 primaryRayAlbedo = BLOCK_COLORS[results.blockIDHit].rgb;
        float3 primaryRaySurfaceNormal = sideNormals[sideEntered];
        float primaryRaynDotL = saturate(dot(primaryRaySurfaceNormal, sunDirection));

        const float ambientAmount = 0.2f;
        float3 incomingLight = ambientAmount * GetDirectionAOColour(primaryRaySurfaceNormal);

        // Only fire a shadow ray if we're pointing towards the sun
        if(primaryRaynDotL > 0.0f)
        {
            RaytraceParams shadowParams = { FLT_INF, primaryRayEnd, sunDirection };
            shadowParams.ray_origin += (primaryRaySurfaceNormal * SHADOW_EPSILON);

            RaytraceResults shadowResults = Raytrace(shadowParams);
            RecordRayStats(shadowResults.hit, 2);

            primaryRaynDotL *= (shadowResults.hit ? 0 : 1);
            incomingLight += (primaryRaynDotL * sunColor);
        }

        diffuseComponent = (incomingLight * primaryRayAlbedo);

        // If we hit water, doing a reflection ray!
        bool isWater = (results.blockIDHit == 8 || results.blockIDHit == 9);
        bool isGlass = (results.blockIDHit == 20 || results.blockIDHit == 102);

        // Only fire a reflection ray for water and glass
        if (isWater || isGlass)
        {
            float3 reflectedDirection = reflect(params.ray_direction, primaryRaySurfaceNormal);

            RaytraceParams reflectionParams = { FLT_INF, primaryRayEnd, reflectedDirection };
            reflectionParams.ray_origin += (primaryRaySurfaceNormal * SHADOW_EPSILON);

            RaytraceResults reflectionResults = Raytrace(reflectionParams);
            RecordRayStats(reflectionResults.hit, 4);

            static const float IOR_WATER = 1.333;
            static const float F0_WATER = pow((1 - IOR_WATER) / (1 + IOR_WATER), 2);

            float f0 = F0_WATER;
            float3 V = -params.ray_direction;
            float3 L = -reflectionParams.ray_direction;
            float3 H = normalize(V + L);
            float specVal = pow(saturate(dot(H, primaryRaySurfaceNormal)), 50);
            float base = 1 - dot(V, primaryRaySurfaceNormal);
            float exponential = pow(base, 5);
            float fresnel = exponential * (1 - f0) + f0;

            if (reflectionResults.hit)
            {
                // Do nDotL here too
                float reflectionNearestT;
                aabbMin = reflectionResults.blockCoord;
                aabbMax = aabbMin + 1.0f;
                AABBIntersect(reflectionParams.ray_origin, rcp(reflectionParams.ray_direction), aabbMin, aabbMax, sideEntered, reflectionNearestT);

                float3 reflectionRaySurfaceNormal = sideNormals[sideEntered];
                float specularRaynDotL = saturate(dot(reflectionRaySurfaceNormal, sunDirection));

                float3 reflectedRayAlbedo = BLOCK_COLORS[reflectionResults.blockIDHit].rgb;
                float3 specularIncomingLight = ambientAmount * GetDirectionAOColour(reflectionRaySurfaceNormal);

                // Only first a shadow ray for reflections if the surface points towards the sun
                if (specularRaynDotL > 0.0f)
                {
                    float3 reflectedRayEnd = reflectionParams.ray_origin + (reflectionParams.ray_direction * reflectionNearestT);

                    RaytraceParams reflectedShadowParams = { FLT_INF, reflectedRayEnd, sunDirection };
                    reflectedShadowParams.ray_origin += (reflectionRaySurfaceNormal * SHADOW_EPSILON);

                    RaytraceResults reflectedShadowResults = Raytrace(reflectedShadowParams);
                    RecordRayStats(reflectedShadowResults.hit, 6);

                    float isLit = reflectedShadowResults.hit ? 0 : 1;
                    specularIncomingLight += (specularRaynDotL * isLit * sunColor);
                }

                specularComponent = (specularIncomingLight * reflectedRayAlbedo);
            }
            else
            {
                // Reflection ray "miss shader"
                specularComponent = GetSkyColour(reflectedDirection, 1.0f);
            }

            diffuseComponent *= (1 - fresnel);
            specularComponent = specularComponent * fresnel;
        }

        finalColour = diffuseComponent + specularComponent;
    }
    else
    {
        // Primary ray "miss shader"
        finalColour = GetSkyColour(params.ray_direction);
    }

    float3 colour;

    switch (renderMode)
    {
    case RENDER_MODE_NORMAL: colour = finalColour; break;
    case RENDER_MODE_PRIMITIVE: colour = results.hit ? (results.blockCoord % 2) : 0; break;
    case RENDER_MODE_DEPTH: colour = (nearestT / 2048.0f) * UintToColor(color); break;
    case RENDER_MODE_TRAVERSAL_COST: colour = (results.numIterations / 80.0f) * UintToColor(color); break;
    default: colour = float3(1, 0, 1); break;
    }

    output[DTid.xy] = float4(colour, 1);
}
