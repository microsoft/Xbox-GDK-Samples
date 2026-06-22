//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifndef FETCH_PATTERN
#define FETCH_PATTERN FETCH_PATTERN_COHERENT
#endif

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
};

Texture2D tex                       : register(t0);
SamplerState samp                   : register(s0);

// Create a draw which is bandwidth bound, so that TC compat has a chance of improving performance
[ROOT_SIGNATURE]
float4 main(in InterpolantsTexcoord In) : SV_Target0
{
#if FETCH_PATTERN == FETCH_PATTERN_COHERENT
    float2 texcoord = In.texcoord;
#elif FETCH_PATTERN == FETCH_PATTERN_SCATTERED
    float2 texcoord = In.texcoord * 0x00005555;
#endif

    return tex.Sample(samp, texcoord);
}