//--------------------------------------------------------------------------------------
// ShaderShared.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef __cplusplus
using float4x4 = DirectX::XMFLOAT4X4;
using float4 = DirectX::XMFLOAT4;
using float3 = DirectX::XMFLOAT3;
using float2 = DirectX::XMFLOAT2;
using uint = uint32_t;
#endif

struct SceneConstants
{
    float4x4 projectionViewWorld;
    float3 cameraWorldPos;
    float rayMaxLength;
    float4 lightDiffuseColor;
    float4 lightAmbientColor;
    float3 lightPosition;
    float pad0;
    float2 invScreenDimensions;
    float2 pad1;
};

struct MeshInfo
{
    uint vbIndex;
    uint indicesIndex;
    uint vertexStride;
    uint normalOffset;
    uint texOffset;
    uint texIndex;
    uint samplerIndex;
    float pad;
};

struct MeshConstants
{
    float4x4 world;
    uint meshInfoIndex;
    float3 pad;
};

#define THREAD_GROUP_X 8
#define THREAD_GROUP_Y 4
