//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
};

Texture2D tex                       : register(t0);
SamplerState samp                   : register(s0);

// Create a draw which is bandwidth bound, so that dcc has a chance of improving performance
[ROOT_SIGNATURE]
float4 main(in InterpolantsTexcoord In) : SV_Target0
{
    return tex.Sample(samp, In.texcoord);
}