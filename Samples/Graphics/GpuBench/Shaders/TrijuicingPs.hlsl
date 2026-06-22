//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

Texture2D<float4> tex               : register(t0);
SamplerState samp                   : register(s0);

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
    float lod           : LOD;
};

[ROOT_SIGNATURE]
float4 main(in InterpolantsTexcoord In) : SV_Target
{
    float2 texcoord = In.texcoord;
    float2 offset = 1e-10;  // small offset to ensure most fetches are satisfied from the L1 cache
    float lod = In.lod;
    float4 color = 0;

    for(uint i = 0; i < 64; ++i)
    {
        color += tex.SampleLevel(samp, texcoord, lod);
        texcoord += offset;
    }

    return color;
}
