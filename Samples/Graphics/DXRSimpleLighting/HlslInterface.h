//--------------------------------------------------------------------------------------
// HlslInterface.h
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#ifdef HLSL

typedef float2      Vector2;
typedef float3      Vector3;
typedef float4      Vector4;
typedef float4x4    Matrix;
typedef uint        uint32_t;

#else
using namespace DirectX;
using namespace DirectX::SimpleMath;
#endif

struct SceneConstants
{
    Matrix  projectionToWorld;
    Matrix  rotate;
    Vector4 cameraPosition;
    Vector4 lightPosition; // Position of rotating point light.
    Vector4 directionalLightColor;
    Vector4 pointLightColor;
    Vector4 staticLightDirection;
    Vector4 backgroundColor;
    float   minRayExtent;
    float   maxRayExtent;
};

struct CubeConstants
{
    Vector4 albedo;
};

struct ScreenConstants
{
    Vector2 dimensions;
};

struct Vertex
{
    Vector3 position;
    Vector3 normal;
};

#define THREAD_GROUP_X 8
#define THREAD_GROUP_Y 4
