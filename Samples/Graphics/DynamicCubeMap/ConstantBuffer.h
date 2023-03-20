//--------------------------------------------------------------------------------------
// ConstantBuffer.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef __cplusplus 
using float4x4 = DirectX::XMFLOAT4X4;
using float3x3 = DirectX::XMFLOAT3X3;
using float4 = DirectX::XMFLOAT4;
using float3 = DirectX::XMFLOAT3;
using float2 = DirectX::XMFLOAT2;
using uint = uint32_t;
#endif

#define MIPS_IN_ONE_SHADER 4

struct CBMultiPerFrame
{
    float4x4 mWorldViewProj;
    float4x4 mWorld;
    float4x4 mView;
    float4x4 mProj;
    float3   vEye;                    // Eye point in world space
    float    scale;
};

struct CBPerCubeRender
{
    float4x4 mViewCBM[6];            // View matrices for cube map rendering
    float4 planesPerCube[6][6];
    float3 cubeViewPos;
};

struct GenerateMipsConstants
{
    float2 InvOutTexelSize; // texel size for OutMip (NOT SrcMip)
    uint SrcMipIndex;
    uint numMips;
};

struct SubMeshlet
{
    uint offset;
    uint vertexStride;
    uint texOffset;
};

struct VBLayout
{
    uint vertexStride;
    uint texOffset;
};
