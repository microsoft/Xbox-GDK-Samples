//--------------------------------------------------------------------------------------
// SkyboxEffect_Common.hlsli
//
// A sky box effect for DirectX 12.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#ifndef __SKYBOXEFFECT_COMMON_HLSLI__
#define __SKYBOXEFFECT_COMMON_HLSLI__

#define SkyboxRS \
"RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
"            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
"            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
"            DENY_HULL_SHADER_ROOT_ACCESS )," \
"DescriptorTable ( SRV(t0) ),"\
"DescriptorTable ( Sampler(s0) )," \
"CBV(b0)"

cbuffer SkyboxConstants : register(b0)
{
    float4x4 WorldViewProj;
}

struct VSOutput
{
    float3 TexCoord : TEXCOORD0;
    float4 PositionPS : SV_Position;
};

#endif
