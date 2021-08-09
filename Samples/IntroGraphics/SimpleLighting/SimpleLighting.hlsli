//--------------------------------------------------------------------------------------
// SimpleLighting.hlsl
//
// Shader demonstrating Lambertian lighting from multiple sources
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define rootSig "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT" \
	"| DENY_DOMAIN_SHADER_ROOT_ACCESS " \
	"| DENY_GEOMETRY_SHADER_ROOT_ACCESS " \
	"| DENY_HULL_SHADER_ROOT_ACCESS), " \
    "CBV(b0, space = 0)"


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer Constants : register( b0 )
{
	float4x4 mWorld;
	float4x4 mView;
	float4x4 mProjection;
	float4   lightDir[ 2 ];
	float4   lightColor[ 2 ];
	float4   outputColor;
};

 
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Normal : NORMAL;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 Normal : TEXCOORD0;
};


