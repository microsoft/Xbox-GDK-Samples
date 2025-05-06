//--------------------------------------------------------------------------------------
// Shared.h
//
// Modifications Copyright (C) 2022. Advanced Micro Devices, Inc. All Rights Reserved.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef _SHARED_H
#define _SHARED_H

#ifdef CPP_SHARED
#define cbuffer struct
typedef DirectX::XMFLOAT4 float4;
typedef DirectX::XMFLOAT4X4 float4x4;
typedef DirectX::XMFLOAT3 float3;
typedef DirectX::XMFLOAT2 float2;
typedef UINT uint;
#endif

static const uint g_threadGroupSize = 128;

//--------------------------------------------------------------------------------------
// Name: PerComponent
// Desc: constant buffer containing per-component data.
//--------------------------------------------------------------------------------------

cbuffer PerComponent
{
    float4x4 ViewProj;
    float4   ClipSpaceScale;
};

// Count of spheres in the sdkmesh.
static const uint g_sphereCount = 1;

//--------------------------------------------------------------------------------------
// Name: ParticleUpdateConstants
// Desc: Constants used to advance particle simulation one frame.
//--------------------------------------------------------------------------------------

cbuffer ParticleUpdateConstants
{
    float4x4 CameraTransform;
    float4   EmitterRotation;
    float4   ParticleData;
    float3   EmitterPosition;
    uint     pad0;
    float4   Plane;
    uint     ActiveCount;
};

//--------------------------------------------------------------------------------------
// Name: ParticleResetData
// Desc: struct used in the buffer storing the data used to reset a particle.
//--------------------------------------------------------------------------------------

struct ParticleResetData
{
    float AllottedLife;
    float Speed;
    float InitLife;
    float3 Direction;
};

//--------------------------------------------------------------------------------------
// Name: ParticleMotionData
// Desc: struct used in the buffer storing the particle's current state.
//--------------------------------------------------------------------------------------

struct ParticleMotionData
{
    float3 LastPosition;
    float3 Velocity;
    float  RemainingLife;
    float  Mass;
};

static const float g_particleScale = 0.1f;
static const float g_particleRadius = g_particleScale / 2.0f;
static const float g_gravitationalConstant = 1.58f;

#endif
