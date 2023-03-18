//--------------------------------------------------------------------------------------
// GenerateCubeMap.hlsli
//
// Common code for rendering the scene
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ConstantBuffer.h"

struct Interpolants
{
    float4 position     : SV_Position;
    float2 texcoord     : TEXCOORD0;
};

struct Pixel
{
    float4 color    : SV_Target;
};

ConstantBuffer<CBMultiPerFrame> constants : register(b0);
ConstantBuffer<VBLayout> vertexInfo : register(b1);
ByteAddressBuffer    Vertices : register(t0);
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);

#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "CBV(b0),"\
    "DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
    "SRV(t0, visibility=SHADER_VISIBILITY_VERTEX),"\
    "RootConstants(num32BitConstants=2, b1)," \
    "StaticSampler(s0, visibility=SHADER_VISIBILITY_PIXEL)"
