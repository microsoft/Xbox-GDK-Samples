//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifndef SAMPLE_TYPE
#define SAMPLE_TYPE Sample
#endif

#ifndef SAMPLE_ARGS
#define SAMPLE_ARGS
#endif

#ifndef SAMPLER_STATE_TYPE
#define SAMPLER_STATE_TYPE SamplerState
#endif

#ifndef TEXTURE_TYPE
#define TEXTURE_TYPE Texture2D
#endif

#ifndef TEXCOORD_CHANNELS
#define TEXCOORD_CHANNELS xy
#endif

TEXTURE_TYPE<float4> tex            : register(t0);
SAMPLER_STATE_TYPE samp             : register(s0);

struct InterpolantsTexcoord
{
    float4 position     : SV_POSITION0;
    float4 texcoord     : TEXCOORD0;
};

[ROOT_SIGNATURE]
float4 main(in InterpolantsTexcoord In) : SV_Target
{
    float4 texcoord = In.texcoord;
    float4 offset = 1e-10;  // small offset to ensure most fetches are satisfied from the L1 cache
    float4 color = 0.0f;
    uint globalStatus = true;

    for(uint i = 0; i < 64; ++i)
    {
        // We must try to use as little VALU as possible, so as to remain VMEM bound always
        uint status = true;
        color.x += tex.SAMPLE_TYPE(samp, texcoord.TEXCOORD_CHANNELS SAMPLE_ARGS).x;
        texcoord.x += offset.x;

        // It's not stricly valid to use 'status' directly, but we do this only
        // to obtain the desired GPU instruction for all cases.
        globalStatus &= status;
    }

    if (!globalStatus)
    {
        color.x = 0.0f;
    }

    return color;
}
