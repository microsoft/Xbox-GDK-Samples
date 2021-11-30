//--------------------------------------------------------------------------------------
// common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "CommonHLSL.h"

// Enable dynamic indexing into descriptor heap with CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED.
// This enables use of the ResourceDescriptorHeap[] keyword in HLSL SM 6.6,
// which allows "bindless" access of resources directly from the descriptor heap,
// without needing them to be bound in the root signature.
#define MainRSVis \
"RootFlags(  DENY_DOMAIN_SHADER_ROOT_ACCESS" \
"          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
"          | DENY_HULL_SHADER_ROOT_ACCESS" \
"          | ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
"          | CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"RootConstants(num32BitConstants=19, b0)"

#define MainRSRaster \
"RootFlags(  ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT "\
"          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
"          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
"          | DENY_HULL_SHADER_ROOT_ACCESS)," \
"RootConstants(num32BitConstants=19, b0)," \
"DescriptorTable(SRV(t0)), " \
"DescriptorTable(Sampler(s0))"

struct VertexElement
{
    float3 position;
    float3 normal;
    float2 uv;
};

typedef uint IndexElement;

// Returns true if ray would hit plane.
// hitT stores distance along ray, or -1 if missed.
// Parallel ray/plane returns a hit, with hitT = 0.
bool RayPlaneIntersection(out float hitT, float3 rayOrigin, float3 rayDirection, float3 planeSurfacePoint, float3 planeNormal)
{
    float denominator = dot(rayDirection, planeNormal);

    float numerator = dot(planeSurfacePoint - rayOrigin, planeNormal);

    if (denominator == 0)
    {
        if (numerator == 0)
        {
            // parallel, choose ray-origin as hit point. 
            hitT = 0;
            return true;
        }
        else
        {
            hitT = -1;
            return false;
        }
    }

    hitT = numerator / denominator;
    return true;
}

/*
    REF: https://gamedev.stackexchange.com/questions/23743/whats-the-most-efficient-way-to-find-barycentric-coordinates
    From "Real-Time Collision Detection" by Christer Ericson
*/
float3 GetBarycentricsFromPlanePoint(float3 pt, float3 v0, float3 v1, float3 v2)
{
    float3 e0 = v1 - v0;
    float3 e1 = v2 - v0;
    float3 e2 = pt - v0;
    float d00 = dot(e0, e0);
    float d01 = dot(e0, e1);
    float d11 = dot(e1, e1);
    float d20 = dot(e2, e0);
    float d21 = dot(e2, e1);
    float denom = 1.0 / (d00 * d11 - d01 * d01);
    float v = (d11 * d20 - d01 * d21) * denom;
    float w = (d00 * d21 - d01 * d20) * denom;
    float u = 1.0 - v - w;
    return float3(u, v, w);
}
