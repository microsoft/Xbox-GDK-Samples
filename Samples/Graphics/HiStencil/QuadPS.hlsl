//--------------------------------------------------------------------------------------
// QuadPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Simple shader to output a quad
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct PSInput
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

struct PSOutput
{
	float4 color    : SV_TARGET0;
};

Texture2D<float4> tex  : register(t0);

sampler simpleSampler  : register(s0);

[ROOT_SIGNATURE_MAIN]
PSOutput main(PSInput psIn)
{
	PSOutput psOut;
	psOut.color = tex.Sample(simpleSampler, psIn.texCoord);
	return psOut;
}