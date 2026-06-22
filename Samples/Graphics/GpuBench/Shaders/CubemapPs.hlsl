//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

TextureCube<float4> tex             : register(t0);
SamplerState samp                   : register(s0);

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
    float3 texcoord     : TEXCOORD0;
};

[ROOT_SIGNATURE]
float4 main(in InterpolantsTexcoord In) : SV_Target
{
    float3 texcoord = In.texcoord;
    float3 offset = 1e-10;  // small offset to ensure most fetches are satisfied from the L1 cache
    float4 color = 0;

    for(uint i = 0; i < 64; ++i)
    {
        color += tex.Sample(samp, texcoord);
        texcoord += offset;
    }

    return color;
}
