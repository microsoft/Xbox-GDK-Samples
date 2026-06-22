//--------------------------------------------------------------------------------------
// AoTex.hlsli
//
// Common code for rendering the AO texture
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Constants.h"

struct Interpolants
{
    float4 position     : SV_Position;
    float2 texcoord     : TEXCOORD0;
};

struct Pixel
{
    float4 color    : SV_Target;
};

Texture2D<float> texAO : register(t0);

#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_AMPLIFICATION_SHADER_ROOT_ACCESS" \
    "          | DENY_MESH_SHADER_ROOT_ACCESS" \
    "          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "DescriptorTable (SRV(t0), visibility=SHADER_VISIBILITY_PIXEL)"
