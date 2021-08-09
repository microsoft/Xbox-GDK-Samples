//--------------------------------------------------------------------------------------
// Shared.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#define MAX_MESHLET_SIZE 128
#define GROUP_SIZE MAX_MESHLET_SIZE

#ifdef __cplusplus 
using float4x4 = DirectX::XMFLOAT4X4;
using uint     = uint32_t;
#endif

struct Constants
{
    float4x4 View;
    float4x4 ViewProj;
    uint     RenderMode;
};

struct DrawParams
{
    uint InstanceCount;
    uint InstanceOffset;
};

struct Instance
{
    float4x4 World;
    float4x4 WorldInvTranspose;
};
