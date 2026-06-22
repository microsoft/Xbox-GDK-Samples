//--------------------------------------------------------------------------------------
// OpaquePS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define USE_HLSL
#include "SharedDefinitions.h"
#include "HLSLCommon.hlsli"

Texture2D<float4> diffuse      : register(t0);
sampler texSampler             : register(s0);

[RootSignature(MAIN_RS)]
float4 main(Interpolators IN) : SV_TARGET
{
    float4 color = diffuse.Sample(texSampler, IN.texcoords);
	return color;
}
