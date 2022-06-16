//--------------------------------------------------------------------------------------
// Shared.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#define MAX_MESHLET_SIZE 128

#if defined(_GAMING_XBOX_SCARLETT) || defined(__XBOX_SCARLETT)

#ifdef __XBOX_ENABLE_WAVE32
#define THREADS_PER_WAVE 32
#else
#define THREADS_PER_WAVE 64
#endif

#else
#define THREADS_PER_WAVE 32
#endif

#define AS_GROUP_SIZE THREADS_PER_WAVE
#define MS_GROUP_SIZE MAX_MESHLET_SIZE

#ifdef __cplusplus
using float4x4 = DirectX::XMFLOAT4X4;
using float4   = DirectX::XMFLOAT4;
using float3   = DirectX::XMFLOAT3;
using float2   = DirectX::XMFLOAT2;
using uint     = uint32_t;
#endif

struct Payload
{
    uint MeshletIndices[AS_GROUP_SIZE];
};

struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};

struct CullData
{
    float4 BoundingSphere;
    uint   NormalCone;
    float  ApexOffset;
};

struct MeshInfo
{
    uint IndexBytes;
    uint MeshletCount;
    uint LastMeshletSize;
};

struct Instance
{
    float4x4    World;
    float4x4    WorldInvTrans;
    float       Scale;
};

struct Constants
{
    float4x4    View;
    float4x4    ViewProj;
    float4      Planes[6];

    float3      CullViewPosition;
    uint        Cull;

    float3      ViewPosition;
    uint        DebugCull;

    uint        RenderMode;
    uint        HighlightedIndex;
    uint        SelectedIndex;
};
