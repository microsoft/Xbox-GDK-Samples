//--------------------------------------------------------------------------------------
// MagnifyingGlassPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignatures.hlsli"

struct Interpolators
{
    float4 Position : SV_POSITION;
    float2 texCoords : TEXCOORD;
};

// GBuffer shader resources
Texture2D<float4> texTAARender  : register(t0);
sampler pointSampler            : register(s0);

[RootSignature(MagnifyingGlassPassRS)]
float4 main(Interpolators IN) : SV_TARGET
{
    return texTAARender.Sample(pointSampler, IN.texCoords);
}
