//--------------------------------------------------------------------------------------
// Lighting.hlsli
//
// Common code for rendering lit geometry
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Constants.h"

struct Interpolants
{
    float4 position     : SV_Position;
    float3 normal       : NORMAL0;
    float2 texcoord     : TEXCOORD0;
};

struct Pixel
{
    float4 color    : SV_Target;
};

ConstantBuffer<Constants> constants : register(b0);
Texture2D texDiffuse : register(t0);
Texture2D<float> texAO : register(t1);
SamplerState samLinear : register(s0);

#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_AMPLIFICATION_SHADER_ROOT_ACCESS" \
    "          | DENY_MESH_SHADER_ROOT_ACCESS" \
    "          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "CBV(b0),"\
    "DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
    "DescriptorTable (SRV(t1), visibility=SHADER_VISIBILITY_PIXEL),"\
    "StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL)"
