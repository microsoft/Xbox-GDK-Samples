//--------------------------------------------------------------------------------------
// SampleCubeMapHeader.hlsli
//
// Common code for sampling a cube map
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ConstantBuffer.h"

struct VS_OUTPUT_SCENEENV
{
    float4 Pos : SV_POSITION;
    float4 wPos : TEXCOORD1; // World space position
    float3 wN : TEXCOORD3;       // World space normal
};

ConstantBuffer<CBMultiPerFrame> constants : register(b0);
TextureCube txEnvMap  : register(t0);
SamplerState samCube : register(s0);

#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "CBV(b0),"\
    "DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL),"\
    "StaticSampler(s0, \
            filter = FILTER_ANISOTROPIC, \
            addressU = TEXTURE_ADDRESS_CLAMP, \
            addressV = TEXTURE_ADDRESS_CLAMP, \
            addressW = TEXTURE_ADDRESS_CLAMP, \
            maxAnisotropy = 16,\
            comparisonFunc = COMPARISON_NEVER, \
            visibility = SHADER_VISIBILITY_PIXEL)"
