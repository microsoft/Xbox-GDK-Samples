//--------------------------------------------------------------------------------------
// Shared.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define GROUP_SIZE 128

#ifdef __cplusplus 
using float4x4  = DirectX::XMFLOAT4X4;
using float4    = DirectX::XMFLOAT4;
using float3    = DirectX::XMFLOAT3;
using float2    = DirectX::XMFLOAT2;
using uint      = uint32_t;
#endif

struct Constants
{
    float4x4 ViewProj;
    float3   ViewPosition;
    uint     ParticleCount;
    float3   CameraUp;
    float    SimulationTime;
    float3   Color;
};

struct Particle
{
    float3 Position;
    float  Size;
    float  InitTime;
    float  Lifetime;
    float3 Velocity;
};
