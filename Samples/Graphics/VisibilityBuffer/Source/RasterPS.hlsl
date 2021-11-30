//--------------------------------------------------------------------------------------
// RasterPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "common.hlsli"

struct Interpolants
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

Texture2D<float4> diffuseTexture : register(t0);

SamplerState samLinear : register(s0);

[RootSignature(MainRSRaster)]
float4 main(Interpolants In) : SV_Target
{
    float4 texColour = diffuseTexture.Sample(samLinear, In.uv);

    for (int i = 0; i < 15; i++)
    {
        texColour.a += diffuseTexture.Sample(samLinear, In.uv + float2(i/15.0, i/15.0)).r;
    }
    
    return float4(texColour);
}
