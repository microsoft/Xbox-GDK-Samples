//--------------------------------------------------------------------------------------
// InlineRT.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#define __XBOX_ENABLE_WAVE32 1
#include "XDXRStandaloneTraverseShortStack.hlsli"
#include "Constants.h"

#define MainRS \
    "RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | SAMPLER_HEAP_DIRECTLY_INDEXED)," \
    "CBV(b0),"\
    "SRV(t0),"\
    "DescriptorTable(UAV(u0)),"\
    "DescriptorTable(SRV(t1)),"\
    "DescriptorTable(SRV(t2)),"\
    "DescriptorTable(SRV(t3)),"\
    "CBV(b1),"\
    "StaticSampler(s0," \
        "addressU = TEXTURE_ADDRESS_CLAMP,"\
        "addressV = TEXTURE_ADDRESS_CLAMP,"\
        "addressW = TEXTURE_ADDRESS_CLAMP,"\
        "filter = FILTER_MIN_MAG_MIP_LINEAR),"\
    "StaticSampler(s1," \
        "addressU = TEXTURE_ADDRESS_WRAP,"\
        "addressV = TEXTURE_ADDRESS_WRAP,"\
        "addressW = TEXTURE_ADDRESS_WRAP,"\
        "filter = FILTER_MIN_MAG_MIP_LINEAR)"

RWTexture2D<float4> g_rtOutput : register(u0);
RaytracingAccelerationStructure g_tlas : register(t0);
StructuredBuffer<MeshInfo> g_meshInfo : register(t1);
Texture2D<float4> g_skybox : register(t2);
Texture1D<float4> g_heatmap : register(t3);
ConstantBuffer<SceneConstants> g_sceneCB : register(b0);
ConstantBuffer<DebugConstants> g_debugCB : register(b1);
SamplerState samLinearClamp : register(s0);
SamplerState samLinearWrap : register(s1);

#ifdef DEBUG_MODE
static const uint numUniqueColors = 11;
static const float4 uniqueColors[numUniqueColors] =
{
    float4(0.2f, 0.5f, 0.7f, 1.f),
    float4(0.8f, 0.2f, 0.7f, 1.f),
    float4(0.8f, 0.9f, 0.1f, 1.f),
    float4(0.5f, 0.1f, 0.1f, 1.f),
    float4(0.1f, 0.8f, 0.1f, 1.f),
    float4(0.6f, 0.4f, 0.6f, 1.f),
    float4(0.3f, 0.4f, 0.2f, 1.f),
    float4(0.9f, 0.7f, 0.4f, 1.f),
    float4(0.1f, 0.9f, 0.8f, 1.f),
    float4(0.1f, 0.1f, 0.2f, 1.f),
    float4(0.7f, 0.2f, 0.2f, 1.f)
};
#endif

uint3 GetIndices(uint index, uint primitiveID)
{
    Buffer<uint> indexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(index)];
    uint indexID = primitiveID * 3;
    return uint3(indexBuffer[indexID], indexBuffer[indexID + 1], indexBuffer[indexID + 2]);
}

float3 GetVertexNormal(MeshInfo currentMeshInfo, uint primitiveID, float2 barycentrics)
{
    const uint3 indices = GetIndices(currentMeshInfo.indicesIndex, primitiveID);
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(currentMeshInfo.vbIndex)];
    uint normalIndexBytes0 = currentMeshInfo.vertexStride * indices[0] + currentMeshInfo.normalOffset;
    uint normalIndexBytes1 = currentMeshInfo.vertexStride * indices[1] + currentMeshInfo.normalOffset;
    uint normalIndexBytes2 = currentMeshInfo.vertexStride * indices[2] + currentMeshInfo.normalOffset;
    float3 vertexNormals[3] = { asfloat(vertexBuffer.Load3(normalIndexBytes0)), asfloat(vertexBuffer.Load3(normalIndexBytes1)), asfloat(vertexBuffer.Load3(normalIndexBytes2)) };

    // Compute the triangle's interpolated normal using the hit's barycentrics
    float3 objectSpaceNormal = vertexNormals[0] + barycentrics.x * (vertexNormals[1] - vertexNormals[0]) + barycentrics.y * (vertexNormals[2] - vertexNormals[0]);
    return objectSpaceNormal;
}

float2 GetTexCoords(MeshInfo currentMeshInfo, uint primitiveID, float2 barycentrics)
{
    const uint3 indices = GetIndices(currentMeshInfo.indicesIndex, primitiveID);
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(currentMeshInfo.vbIndex)];
    uint texIndexBytes0 = currentMeshInfo.vertexStride * indices[0] + currentMeshInfo.texOffset;
    uint texIndexBytes1 = currentMeshInfo.vertexStride * indices[1] + currentMeshInfo.texOffset;
    uint texIndexBytes2 = currentMeshInfo.vertexStride * indices[2] + currentMeshInfo.texOffset;
    float2 vertexUVs[3] = { asfloat(vertexBuffer.Load2(texIndexBytes0)), asfloat(vertexBuffer.Load2(texIndexBytes1)), asfloat(vertexBuffer.Load2(texIndexBytes2)) };

    // Compute the triangle's interpolated UVs using the hit's barycentrics
    return vertexUVs[0] + barycentrics.x * (vertexUVs[1] - vertexUVs[0]) + barycentrics.y * (vertexUVs[2] - vertexUVs[0]);
}

float4 ShadeClosestHit(float3 worldHitPosition, uint instanceID, uint geometryID, uint primitiveID,
                       float2 barycentrics, float3x3 objectToWorld, float shadowMultiplier, bool lit)
{
    MeshInfo currentMeshInfo = g_meshInfo[instanceID + geometryID];
    float3 objectSpaceNormal = GetVertexNormal(currentMeshInfo, primitiveID, barycentrics);

    // Diffuse lighting calculation
    Texture2D materialTex = ResourceDescriptorHeap[NonUniformResourceIndex(currentMeshInfo.texIndex)];
    float2 texCoord = GetTexCoords(currentMeshInfo, primitiveID, barycentrics);
    SamplerState texSamp = SamplerDescriptorHeap[NonUniformResourceIndex(currentMeshInfo.samplerIndex)];
    float4 albedoColor = materialTex.Sample(texSamp, texCoord);

    if (lit)
    {
        float3 lightDir = normalize(g_sceneCB.lightPosition - worldHitPosition);
        float3 worldSpaceNormal = normalize(mul(normalize(objectSpaceNormal), objectToWorld)); // Assume orthogonal transform (i.e. 'N = N * M^(-1T) = N * M)
        float nDotL = max(0.0f, dot(lightDir, worldSpaceNormal));
        float4 diffuseColor = g_sceneCB.lightDiffuseColor * shadowMultiplier * nDotL;

        return (g_sceneCB.lightAmbientColor + diffuseColor) * albedoColor;
    }
    else
    {
        return albedoColor;
    }
}

// Function to return nDotL for closest hit
float GetNDotL(float3 worldHitPosition, uint instanceID, uint geometryID, uint primitiveID, float2 barycentrics, float3x3 objectToWorld)
{
    MeshInfo currentMeshInfo = g_meshInfo[instanceID + geometryID];
    float3 objectSpaceNormal = GetVertexNormal(currentMeshInfo, primitiveID, barycentrics);

    float3 lightDir = normalize(g_sceneCB.lightPosition - worldHitPosition);
    float3 worldSpaceNormal = normalize(mul(normalize(objectSpaceNormal), objectToWorld)); // Assume orthogonal transform (i.e. 'N = N * M^(-1T) = N * M)
    return max(0.0f, dot(lightDir, worldSpaceNormal));
}

// Function that returns the intersection of a ray and a sphere with center at (0,0,0)
// and radius SPHERE_RADIUS. Returns true if the ray intersects the sphere, and false otherwise.
bool IntersectSphere(float3 origin, float3 direction, out float3 hitPos)
{
    float3 oc = origin;
    float a = dot(direction, direction);
    float b = 2.0 * dot(oc, direction);
    float c = dot(oc, oc) - SPHERE_RADIUS * SPHERE_RADIUS;
    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
    {
        return false;
    }
    else
    {
        float t0 = (-b - sqrt(discriminant)) / (2.0 * a);
        float t1 = (-b + sqrt(discriminant)) / (2.0 * a);
        if (t0 < 0 && t1 < 0)
        {
            return false;
        }
        else if (t0 < 0)
        {
            hitPos = origin + t1 * direction;
        }
        else
        {
            hitPos = origin + t0 * direction;
        }
        
        return true;
    }
}


[RootSignature(MainRS)]
[numthreads(THREAD_GROUP_X, THREAD_GROUP_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    float2 xy = DTid.xy + 0.5f; // center in the middle of the pixel.
    float2 screenPos = xy * g_sceneCB.invScreenDimensions * 2.0 - 1.0;

    // Invert Y for DirectX-style coordinates.
    screenPos.y = -screenPos.y;

    // Unproject the pixel coordinate into a ray.
    float4 world = mul(g_sceneCB.projectionViewWorld, float4(screenPos, 0, 1));

    world.xyz /= world.w;
    float3 origin = g_sceneCB.cameraWorldPos;
    float3 direction = normalize(world.xyz - origin);

    // Trace the ray
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = direction;

    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = g_sceneCB.rayMaxLength;

#ifdef DEBUG_MODE
    uint64_t startTimePrimaryRay = __XB_s_memrealtime();
#endif
    XboxRayQuery q;
    q.TraceRayInlineNoAHSNoIS(g_tlas, RAY_FLAG_NONE, 0xFF, ray);
    q.Proceed();
#ifdef DEBUG_MODE 
    uint64_t endTimePrimaryRay = __XB_s_memrealtime();
#endif

    float4 color;

#ifdef DEBUG_MODE
    float scaledValue = 0;
    if (g_debugCB.mode == DEGUG_MODE_ITERATIONS)
    {
        scaledValue = q.CurrentNumIterations() / g_debugCB.heatmapScale;
    }
    else if (g_debugCB.mode == DEBUG_MODE_TIME)
    {
        float elapsedTime = float(endTimePrimaryRay - startTimePrimaryRay);
        scaledValue = elapsedTime / g_debugCB.heatmapScale;
    }
    color = g_heatmap.Sample(samLinearClamp, scaledValue);

#endif

    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        float3 worldHitPosition = q.WorldRayOrigin() + q.CommittedRayT() * q.WorldRayDirection();
        uint instanceID = q.CommittedInstanceID();
        uint geometryID = q.CommittedGeometryIndex();
        uint primitiveID = q.CommittedPrimitiveIndex();
        bool lit = q.CommittedInstanceContributionToHitGroupIndex() == 1 ? true : false;
        float2 barycentrics = q.CommittedTriangleBarycentrics();
        float3x4 ow = q.CommittedObjectToWorld3x4();

#ifdef DEBUG_MODE
        uint instanceIndex = q.CommittedInstanceIndex();
#endif
        
        // transpose the matrix
        float3x3 objectToWorld = float3x3(ow[0].x, ow[1].x, ow[2].x, ow[0].y, ow[1].y, ow[2].y, ow[0].z, ow[1].z, ow[2].z);

        // trace a shadow ray
        ray.Origin = worldHitPosition;
        float3 lightDirection = g_sceneCB.lightPosition - worldHitPosition;
        ray.Direction = normalize(lightDirection);
        ray.TMax = length(lightDirection);
        float shadowMultiplier = 1.0f;
        q.TraceRayInlineNoAHSNoIS(g_tlas, RAY_FLAG_NONE, 0x1, ray);
        q.Proceed();
#ifdef DEBUG_MODE
        float scaledValue = 0;
        if (g_debugCB.mode == DEBUG_MODE_ITERATIONS_SHADOW)
        {
            scaledValue = q.CurrentNumIterations() / g_debugCB.heatmapScale;
            color = g_heatmap.Sample(samLinearClamp, scaledValue);
        }
#endif
        if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
        {
            // in shadow
            shadowMultiplier = 0.0f;
        }

#ifndef DEBUG_MODE
        color = ShadeClosestHit(worldHitPosition, instanceID, geometryID, primitiveID, barycentrics, objectToWorld, shadowMultiplier, lit);
#else
        if (g_debugCB.mode == DEBUG_MODE_GEOMETRY_INDEX)
        {
            color = uniqueColors[geometryID % numUniqueColors];
        }
        else if (g_debugCB.mode == DEBUG_MODE_INSTANCES)
        {
            color = uniqueColors[instanceIndex % numUniqueColors];
        }

        // apply partial lighting to see the geometry better
        float nDotL = GetNDotL(worldHitPosition, instanceID, geometryID, primitiveID, barycentrics, objectToWorld);
        color = lerp(color, color * nDotL, 0.5f);
#endif
    }
    else
    {
#ifndef DEBUG_MODE
        // If the ray does not hit any geometry, sample the skybox
        float3 worldPos = float3(0, 0, 0);
        bool isSkybox = IntersectSphere(origin, direction, worldPos);
        if (isSkybox)
        {
            float2 uv = float2(atan2(worldPos.x, worldPos.z) / (2 * PI) + 0.25f, acos(worldPos.y / SPHERE_RADIUS) / PI);
            color = g_skybox.Sample(samLinearWrap, uv);
        }
        else
        {
            color = float4(0.0f, 0.2f, 0.4f, 1.0f);
        }
#else
        if (g_debugCB.mode == DEBUG_MODE_ITERATIONS_SHADOW)
        {
            color = float4(0.0f, 0.0f, 0.0f, 1.0f);
        }
#endif
    }

    // Write the raytraced color to the output texture.
    g_rtOutput[DTid.xy] = color;
}
