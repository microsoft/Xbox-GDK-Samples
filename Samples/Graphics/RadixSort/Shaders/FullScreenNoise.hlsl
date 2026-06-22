//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#if defined(__XBOX_ENABLE_WAVE32)
#   define THREAD_GROUP_W 8
#   define THREAD_GROUP_H 4
#   define LANES_PER_WAVE 32
#else
#   define THREAD_GROUP_W 8
#   define THREAD_GROUP_H 8
#   define LANES_PER_WAVE 64
#endif

RWByteAddressBuffer OutRadix : register(u0);

cbuffer ShaderParams
{
    uint resW;
    uint resH;
};

// Ported to HLSL from https://www.shadertoy.com/view/Msf3WH
typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;

#define mat2 float2x2

#define USE_GLSL_FIXES 1

static float2 iResolution;
static float2 iMouse;

#define fract frac

static float3 make_vec3(float v)
{
    return v.xxx;
}

#include "ShaderToy_Import_Msf3WH_Noise.hlsli"

static float4 call_mainImage(float2 PixelPos, float2 Resolution, float2 MousePos)
{
    float4 fragColor;
    iResolution = Resolution;
    iMouse = MousePos;
    mainImage(fragColor, PixelPos);
    return fragColor;
}

#define RootSig \
    "UAV(u0),"                                       \
    "RootConstants(b0, num32bitconstants=2)"

[RootSignature(RootSig)]
[numthreads(THREAD_GROUP_W, THREAD_GROUP_H, 1)]
void main( uint2 ThreadGlobalId : SV_DispatchThreadID)
{
    float2 UV = float2(ThreadGlobalId) / float2(resW, resH);

    float4 fragColor = call_mainImage(float2(ThreadGlobalId), float2(resW, resH), float2(1920.0, 1080.0));

    OutRadix.Store((ThreadGlobalId.y * resW + ThreadGlobalId.x) << 2, asuint(fragColor.x));
}
