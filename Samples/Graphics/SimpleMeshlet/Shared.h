//--------------------------------------------------------------------------------------
// Shared.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define MAX_MESHLET_SIZE 128

#if defined(_GAMING_XBOX_SCARLETT) || defined(__XBOX_SCARLETT)
#define THREADS_PER_WAVE 64
#else
#define THREADS_PER_WAVE 32
#endif

#define GROUP_SIZE MAX_MESHLET_SIZE
#define WAVES_PER_GROUP ((GROUP_SIZE + THREADS_PER_WAVE - 1) / THREADS_PER_WAVE)

#ifdef __cplusplus 
using float4x4  = DirectX::XMFLOAT4X4;
using float4    = DirectX::XMFLOAT4;
using float3    = DirectX::XMFLOAT3;
using float2    = DirectX::XMFLOAT2;
using uint      = uint32_t;
#endif

struct Instance
{
    float4x4    World;
    float4x4    WorldInvTrans;
};

struct Constants
{
    float4x4    View;
    float4x4    ViewProj;
    float4x4    DebugViewProj;
    uint        DebugCull;
    uint        DrawMeshlets;
};
