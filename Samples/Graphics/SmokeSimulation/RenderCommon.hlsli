//--------------------------------------------------------------------------------------
// RenderCommon.hlsli
//
// Simple vertex shader and pixel shader for rendering volumetric data
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FluidShared.h"

Texture3D g_tex3D : register(t0);     // The smoke volume
Texture3D g_tex3Dmip : register(t1);  // The smoke volume, scaled down to 1/8 size on each axis

SamplerState g_sam : register(s0);    // Linear sampler

struct PS_INPUT
{
    float4 Pos      : SV_POSITION;
    float3 WorldPos : POSITION;
};

struct Ray
{
    float3 o;                           // Origin of the ray
    float3 d;                           // Direction of the ray
};

#define RenderRS \
    "RootFlags(0),\
    CBV(b0),\
    DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_PIXEL),\
    DescriptorTable(SRV(t1, numDescriptors=1), visibility=SHADER_VISIBILITY_PIXEL),\
    StaticSampler(s0,\
                    addressU = TEXTURE_ADDRESS_CLAMP,\
                    addressV = TEXTURE_ADDRESS_CLAMP,\
                    addressW = TEXTURE_ADDRESS_CLAMP,\
                    comparisonFunc = COMPARISON_ALWAYS,\
                    borderColor = STATIC_BORDER_COLOR_TRANSPARENT_BLACK,\
                    filter = FILTER_MIN_MAG_MIP_LINEAR,\
                    visibility=SHADER_VISIBILITY_PIXEL)"
