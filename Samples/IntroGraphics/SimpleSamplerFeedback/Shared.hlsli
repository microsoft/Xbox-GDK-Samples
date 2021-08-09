//--------------------------------------------------------------------------------------
// Shared.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define MainRS \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
	"| DENY_DOMAIN_SHADER_ROOT_ACCESS " \
	"| DENY_GEOMETRY_SHADER_ROOT_ACCESS " \
	"| DENY_HULL_SHADER_ROOT_ACCESS), " \
    "CBV(b0, space = 0), "\
    "DescriptorTable (SRV(t0), visibility = SHADER_VISIBILITY_PIXEL),"\
    "DescriptorTable (UAV(u0), visibility = SHADER_VISIBILITY_PIXEL),"\
    "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_LINEAR, visibility = SHADER_VISIBILITY_PIXEL)"

struct VS_INPUT
{
    float4 Pos      : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct PS_INPUT
{
    float4 Pos      : SV_POSITION;
    float2 TexCoord : TEXCOORD;
};
