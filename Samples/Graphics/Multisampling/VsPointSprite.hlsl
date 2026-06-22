//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Vertex shader for rendering sample dots
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// This VS will be followed by GS and not PS 
#ifdef __XBOX_SCARLETT
#define __XBOX_PRECOMPILE_VS_GS 1
#define __XBOX_PRECOMPILE_VS_PS 0
#endif

#include "RootSignature.hlsli"

cbuffer cbTransform : register(b0)
{
    float4x4    g_mWorldViewProj;
    float4x4    g_mWorld;
};

cbuffer cbPointSprite : register(b2)
{
    uint        selectedSample;
}

struct InterpolantsPointSprite
{
    float4 position     : SV_POSITION0;
    float2 texcoord     : TEXCOORD0;
    nointerpolation bool fragment : FRAGMENT;
};

// This is a different HLSL type for each MSAA_LEVEL, which means we need a separate instance of the shader.
// The only uses of the texture are for GetDimensions and GetSamplePosition; we never sample from it.
Texture2DMS<float4> texMS : register(t0);

#include "EQAA.hlsli"

//-------------------------------------------------------------------------------------------------------------
// Name: VSPointSprite()
// Desc: Emit points at every sample location.  These are expanded into point sprites by the
// geometry shader.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
InterpolantsPointSprite main(uint instance : SV_InstanceID, uint vertex : SV_VertexID)
{
    InterpolantsPointSprite Out;

    uint2 dims;
    uint samples;
    GetDimensions(texMS, dims.x, dims.y, samples);

    if (selectedSample != 0xffffffff)
    {
        vertex = selectedSample;   // override
    }

    Out.position.xy = float2(instance % dims.x, instance / dims.x);   // in pixels
    Out.position.xy += 0.5f;                                                  // half-pixel offset
    Out.position.xy += GetSamplePosition(texMS, vertex) * int2(1, -1);    // sub-pixel offset
    Out.position.xy /= (float2) dims.xy;                                    // normalized
    Out.position.xy *= 2.0f;                                                  // to screen-space
    Out.position.xy -= 1.0f;
    Out.position.zw = float2(0.5f, 1.0f);
    Out.position = mul(Out.position, g_mWorldViewProj);

    Out.texcoord = float2(0.0f, 0.0f);    // this would be used for atlased textures

#if ENABLE_EQAA
    Out.fragment = vertex < NUM_FRAGMENTS;
#else
    Out.fragment = true;
#endif

    return Out;
}
