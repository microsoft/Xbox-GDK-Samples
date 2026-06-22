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
};

struct DebugConstants
{
    float heatmapScale;
    uint mode;
};

#define DEGUG_MODE_ITERATIONS 0
#define DEBUG_MODE_TIME 1
#define DEBUG_MODE_ITERATIONS_SHADOW 2
#define DEBUG_MODE_GEOMETRY_INDEX 3
#define DEBUG_MODE_INSTANCES 4

#define THREAD_GROUP_X 8
#define THREAD_GROUP_Y 4

#define SPHERE_RADIUS 2000.0f
#define PI 3.14159265f
