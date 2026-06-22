//--------------------------------------------------------------------------------------
// RaytraceTriangles.hlsl
//
// Traversal of a BVH contains FP16 AABB nodes and Triangle leaf nodes.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

RWTexture2D<float4> output : register(u0);

// Stack depth decided based on the content being rendered (LOD 0 Dragon)
static const uint STACK_DEPTH = 28;
static const uint STACK_SIZE = WAVE_SIZE * STACK_DEPTH;

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
    uint leafIndex;
    float nearestT;
    uint numIterations;
    uint triIndexInLeaf;
};

RaytraceResults Raytrace(in RaytraceParams input, uint3 DTid)
{
#if ENABLE_SCARLETT_INTERSECTION_TEST_INTRINSICS
    GpuVA va = vaBVH;
#else
    GpuVA va = 0;
#endif

    float3 inv_ray_dir = rcp(input.ray_direction);
    uint nodePtr = 0 | NODE_TYPE_BOX16;     // Root node pointer is the first node and is BOX16 type.
    uint stackPtr = __XB_GetLaneID();
    uint numIterations = 0;
    uint triangleReturnMode = 0;

    uint4 childPtrs;

    float nearestT = input.ray_extent;
    uint nearestNodePtr = InvalidNodePtr;
    uint nearestTriangleID = InvalidNodePtr;

    // The core traversal loop.
    // More details in Readme.docx to save polluting the code with comments on every line
    do
    {
        childPtrs = asuint(BvhIntersectRay(va, nodePtr, nearestT, input.ray_origin, input.ray_direction, inv_ray_dir, bvhSize, true, triangleReturnMode));

        if ((nodePtr & NODE_TYPE_MASK) <= NODE_TYPE_TRI3)   // If we just tested a triangle node...
        {
            float tNum = asfloat(childPtrs.x);
            float tDenom = asfloat(childPtrs.y);
            float t = tNum / tDenom;                        // Calculate t

            bool newNearest = (t < nearestT);               // Is this the nearest hit so far?
            nearestT = (newNearest ? t : nearestT);         // If so, update nearestT.
            nearestNodePtr = (newNearest ? nodePtr : nearestNodePtr);   // Update nearest node ptr.
            nearestTriangleID = (newNearest ? childPtrs.z : nearestTriangleID);
            nodePtr = InvalidNodePtr;                       // Pretend we missed so we pull the next one off the stack.
        }
        else
        {
            nodePtr = childPtrs.x;                          // We'll descend into this node next (if valid)

            if (childPtrs.y != InvalidNodePtr)
            {
                for (int i = 3; i > 0; i--)                 // Add them in reverse order so the nearest ones pop first
                {
                    if (childPtrs[i] != InvalidNodePtr)     // Not invalid and not a leaf. Don't add leaves to the stack!
                    {
                        g_stack[stackPtr] = childPtrs[i];
                        stackPtr += WAVE_SIZE;
                    }
                }
            }
        }
                
        if (nodePtr == InvalidNodePtr)                      // If we missed the children (or hit a triangle), pull the next one off the stack
        {
            stackPtr -= WAVE_SIZE;
            nodePtr = (stackPtr < STACK_SIZE) ? g_stack[stackPtr] : InvalidNodePtr;
        }

        numIterations++;
    } while (nodePtr != InvalidNodePtr);                    // Terminate when the stack is empty
    
    RaytraceResults results;
    results.hit = (nearestNodePtr != InvalidNodePtr);
    results.numIterations = numIterations;
    results.nearestT = nearestT;
    results.triIndexInLeaf = nearestNodePtr & 0x3;
    results.leafIndex = nearestTriangleID - results.triIndexInLeaf;
    
    return results;
}

[RootSignature("DescriptorTable(SRV(t0)), DescriptorTable(UAV(u0)), SRV(t1), RootConstants(b0, num32bitconstants=28), UAV(u1), SRV(t2)")]
[numthreads(TG_WIDTH, TG_HEIGHT, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    RaytraceParams params = { FLT_INF, cameraPosition.xyz, GetRayDirection(DTid.xy + 0.5f) };
    RaytraceResults results = Raytrace(params, DTid);

    RecordRayStats(results.hit, 0);
    
    float3 colour;

    switch (renderMode)
    {
    case RENDER_MODE_NORMAL:
    {
        // This isn't the real "Triangle Index", it's just some unique identifier for a triangle
        uint triIndex = results.leafIndex * 4;  // Because Max Triangles per leaf is 4
        triIndex += (results.triIndexInLeaf);   // + 0-3
        colour = (triIndex % 16) / 15.0f;
    }break;
    case RENDER_MODE_PRIMITIVE:
    {
        // Colour each 'leaf' by some colour based on the index of the leaf
        float r = ((results.leafIndex >> 0) & 3) * (85.0f / 255.0f);
        float g = ((results.leafIndex >> 2) & 3) * (85.0f / 255.0f);
        float b = ((results.leafIndex >> 4) & 3) * (85.0f / 255.0f);
        colour = float3(r, g, b);
    }break;
    case RENDER_MODE_DEPTH:
    {
        // Reverse linear depth.
        colour = (1 - saturate(results.nearestT / 256.0f)) * UintToColor(color);
    }break;
    case RENDER_MODE_TRAVERSAL_COST:
    {
        // Per pixel brightness based on number of steps taken through the BVH up to a maximum of 128 == brightest colour.
        colour = (results.numIterations / 128.0f) * UintToColor(color);
    }break;
    default: colour = float3(1, 0, 1); break;
    }

    output[DTid.xy] = float4(results.hit ? colour : 0, 1);
}
