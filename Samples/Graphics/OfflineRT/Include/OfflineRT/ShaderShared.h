//--------------------------------------------------------------------------------------
// ShaderShared.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#ifdef __cplusplus
using float4x4 = DirectX::XMMATRIX;
using float4 = DirectX::XMFLOAT4;
using float3 = DirectX::XMFLOAT3;
using float2 = DirectX::XMFLOAT2;
using uint = uint32_t;
#endif

struct SceneConstants
{
    float4x4    projectionViewWorld;
    float3      cameraWorldPos;
    float       rayMaxLength;
    float3      lightWorldPos;
    uint        pad0;
    float4      lightDiffuseColor;
    float4      lightAmbientColor;
};

struct Vertex
{
    float3      position;
    float3      normal;
};

struct MaterialConstants
{
    float4      albedo;
};

