//--------------------------------------------------------------------------------------
// VisualiseDepth.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define VisualizeDepthRS \
    "RootFlags ( ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |" \
    "            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
    "            DENY_HULL_SHADER_ROOT_ACCESS )," \
    "DescriptorTable ( SRV(t0), visibility = SHADER_VISIBILITY_PIXEL ),"\
    "CBV(b0), "\
    "CBV(b1), "\
    "StaticSampler(s0,"\
    "           filter = FILTER_MIN_MAG_MIP_POINT,"\
    "           addressU = TEXTURE_ADDRESS_CLAMP,"\
    "           addressV = TEXTURE_ADDRESS_CLAMP,"\
    "           addressW = TEXTURE_ADDRESS_CLAMP,"\
    "           visibility = SHADER_VISIBILITY_PIXEL )"

cbuffer cbRemapping : register(b1)
{
	float g_fFarNearRatio;  // needed for re-linearizing depth
	float g_remapMin;
	float g_remapDelta;
};

Texture2D depthTexture : register(t0);
SamplerState s0 : register(s0);


//-------------------------------------------------------------------------------------------------------------
// Convert perspective depth back into normalized linear depth
//-------------------------------------------------------------------------------------------------------------
float LinearizeDepth(float fIn)
{
	return fIn / (g_fFarNearRatio - (g_fFarNearRatio - 1.0) * fIn);
}


//-------------------------------------------------------------------------------------------------------------
// Pixel shader which displays depth as color
//-------------------------------------------------------------------------------------------------------------
[RootSignature(VisualizeDepthRS)]
float4 main(float4 color : COLOR0, float2 texCoord : TEXCOORD0) : SV_Target0
{
	float depth = LinearizeDepth(depthTexture.Sample(s0, texCoord).x);

	depth = (depth - g_remapMin) * g_remapDelta;

	return depth;
}

