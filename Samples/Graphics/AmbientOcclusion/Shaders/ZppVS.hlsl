//--------------------------------------------------------------------------------------
// ZppVS.hlsl
//
// Simple vertex shader for rendering position only
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Constants.h"

struct VSInput
{
    float4 Position : SV_Position;
    float3 Normal   : NORMAL0;
    float2 TexCoord : TEXCOORD0;
};

struct Output
{
    float4 Position : SV_Position;
};

ConstantBuffer<Constants> constants : register(b0);

#ifdef __XBOX_SCARLETT
#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_AMPLIFICATION_SHADER_ROOT_ACCESS" \
    "          | DENY_MESH_SHADER_ROOT_ACCESS" \
    "          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
    "          | DENY_PIXEL_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "CBV(b0)"
#else
#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
    "          | DENY_DOMAIN_SHADER_ROOT_ACCESS" \
    "          | DENY_GEOMETRY_SHADER_ROOT_ACCESS" \
    "          | DENY_PIXEL_SHADER_ROOT_ACCESS" \
    "          | DENY_HULL_SHADER_ROOT_ACCESS)," \
    "CBV(b0)"
#endif

[RootSignature(MainRS)]
Output main(VSInput In)
{
    Output output = (Output)0.0f;
    output.Position = mul(In.Position, constants.WorldViewProj);
    return output;
}
