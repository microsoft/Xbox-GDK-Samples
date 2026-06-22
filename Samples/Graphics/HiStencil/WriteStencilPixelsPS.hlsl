//--------------------------------------------------------------------------------------
// WriteStencilPixelsPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Shader to write pixels if it passes Stencil test
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
	psOut.color = float4(0, 0.5, 0, 0.1f);
	return psOut;
}