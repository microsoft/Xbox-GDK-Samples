//--------------------------------------------------------------------------------------
// Shared.h
//
// Shared declarations between shaders (HLSL) and C++ source.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#ifndef SHARED_H
#define SHARED_H

#ifdef SHARED_CPP
using float4x4 = DirectX::XMFLOAT4X4;
using float4 = DirectX::XMFLOAT4;
using float3 = DirectX::XMFLOAT3;
using uint = uint32_t;
#endif

static const unsigned int g_MaxFlashLights = 16;

struct cbModel
{
    float4x4 WorldViewProj;
    float4x4 WorldRot;
    float4x4 World;
};

struct cbScene
{
    float3 LightDir;
    uint   pad0;
    float4 Color;
    float3 CameraPos;
    uint   pad1;
};

struct cbParticle
{
    float3      LightDirection;
    uint        pad0;
    float3      Right;
    uint        pad1;
    float3      Up;
    uint        pad2;
    float3      Forward;
    uint        pad3;
    float4x4    ViewProj;
};


struct cbGlowLights
{
    float4 GlowLightPosIntensity[g_MaxFlashLights];
    float4 GlowLightColor[g_MaxFlashLights];

    float4 GlowLightAttenuation;
    float4 MeshLightAttenuation;
    uint   NumGlowLights;
};

#endif //SHARED_H