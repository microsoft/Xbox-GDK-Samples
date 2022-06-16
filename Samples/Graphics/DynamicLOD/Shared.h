//--------------------------------------------------------------------------------------
// Shared.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define __XBOX_ENABLE_WAVE32 1

#define MAX_MESHLET_SIZE 128
#define MAX_LOD_LEVELS 8

#if defined(_GAMING_XBOX_SCARLETT) || defined(__XBOX_SCARLETT)

#ifdef __XBOX_ENABLE_WAVE32 
#define THREADS_PER_WAVE 32
#else
#define THREADS_PER_WAVE 64
#endif

#else
#define THREADS_PER_WAVE 32
#endif

// Pre-defined threadgroup sizes for AS & MS stages
#define AS_GROUP_SIZE THREADS_PER_WAVE
#define MS_GROUP_SIZE MAX_MESHLET_SIZE


#ifdef __cplusplus 
using float4x4 = DirectX::XMFLOAT4X4;
using float4   = DirectX::XMFLOAT4;
using float3   = DirectX::XMFLOAT3;
using float2   = DirectX::XMFLOAT2;
using uint     = uint32_t;
#endif

struct Constants
{
    float4x4 View;
    float4x4 ViewProj;

    float4 Planes[6];
    float3 ViewPosition;
    float RecipTanHalfFovy;

    uint RenderMode;
    uint LODCount;
    uint ForceVisible;
    uint ForceLOD0;
};

struct DrawParams
{
    uint InstanceOffset;
    uint InstanceCount;
};

struct Instance
{
    float4x4 World;
    float4x4 WorldInvTranspose;
    float4   BoundingSphere;
};

// This is the data which will be exported from the Amplification Shader
// and supplied as an extra 'in' argument to its dispatched Mesh Shader
// children.
struct Payload
{
    uint InstanceCounts[MAX_LOD_LEVELS];   // The instance count for each LOD level.
    uint GroupOffsets[MAX_LOD_LEVELS + 1]; // The offset in threadgroups for each LOD level.

    // The list of instance indices after culling. Ordered as:
    // (list of LOD 0 instance indices), (list of LOD 1 instance indices), ... (list of LOD MAX_LOD_LEVELS-1 instance indices)                                            
    uint InstanceList[AS_GROUP_SIZE];
    uint InstanceOffsets[MAX_LOD_LEVELS + 1]; // The offset into the Instance List at which each LOD level begins.
};
