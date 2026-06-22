//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifndef __XBOX_SCARLETT
#undef DEST_16_BIT
#endif

#ifndef CHANNEL_MASK
#define CHANNEL_MASK xyzw
#endif

#ifdef FILTER_ALLOWED
    #define TEXCOORD_TYPE float2
    #define TEXCOORD_OFFSET 1e-10
    #ifdef USE_GATHER
        #define TEX_METHOD Gather
    #else
        #define TEX_METHOD Sample
    #endif
    #ifdef DEST_16_BIT
        #define DATA_TYPE float16_t4
    #else
        #define DATA_TYPE float4
    #endif
#else
    #define TEXCOORD_TYPE int2
    #define TEXCOORD_OFFSET 1
    #ifdef USE_GATHER
        #define TEX_METHOD Gather
    #else
        #define TEX_METHOD Load
    #endif
    #ifdef DEST_16_BIT
        #define DATA_TYPE uint16_t4
    #else
        #define DATA_TYPE uint4
    #endif
#endif

Texture2D<DATA_TYPE> tex            : register(t0);
SamplerState samp                   : register(s0);

struct InterpolantsTexcoord
{
    float4 position         : SV_POSITION0;
    TEXCOORD_TYPE texcoord  : TEXCOORD0;
};

[ROOT_SIGNATURE]
DATA_TYPE main(in InterpolantsTexcoord In) : SV_Target
{
    TEXCOORD_TYPE texcoord = In.texcoord;
    DATA_TYPE color = 0;

    [unroll]
    for(uint i = 0; i < 64; ++i)
    {
#ifdef FILTER_ALLOWED
        color.CHANNEL_MASK += tex.TEX_METHOD(samp, texcoord).CHANNEL_MASK;
#else
        color.CHANNEL_MASK |= tex.TEX_METHOD(int3(texcoord, 0)).CHANNEL_MASK;
#endif
        texcoord += TEXCOORD_OFFSET;
    }

    return color;
}
