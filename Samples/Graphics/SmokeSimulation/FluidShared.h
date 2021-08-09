//--------------------------------------------------------------------------------------
// FluidShared.h
//
// Shared declarations between shaders (HLSL) and C++ source.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef FLUID_SHARED_H
#define FLUID_SHARED_H

#ifdef CPP_SHARED
#define cbuffer struct
using float4x4 = DirectX::XMFLOAT4X4;
using float4 = DirectX::XMFLOAT4;
using float3 = DirectX::XMFLOAT3;
using uint = uint32_t;
#endif

static const unsigned int VolumeSize = 128;      // The X, Y, Z dimension of the simulation domain
static const unsigned int GroupSize = 8;         // The X, Y, Z dimension of the group size in CS

// Constant buffer for vertex shader
cbuffer cbRender
{
    float4x4 World;
    float4x4 WorldViewProj;
    float3   EyePosition;
};

cbuffer cbCS
{
    float DeltaTime;
    float Forward;
    float Modulate;
    float Epsilon;

    float3 EmitterCenter;
    uint   pad;
    float3 EmitterDir;
};

#endif //SHARED_H