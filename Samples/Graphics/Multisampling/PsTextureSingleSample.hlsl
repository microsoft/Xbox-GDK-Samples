//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// The "Single sample" visualization
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

struct Pixel
{
    float4 color            : SV_TARGET0;
};

struct InterpolantsTexture
{
    float4 position         : SV_POSITION0;
    float2 texcoord         : TEXCOORD0;
};

#include "PsCommon.hlsli"

// The sample selected for viewing in the single-sample view mode
cbuffer cbViewMode : register(b0)
{
    unsigned int    selectedSample;
};

//-------------------------------------------------------------------------------------------------------------
// Name: PSTextureSingleSample()
// Desc: For each pixel, draw the color value for the selected sample.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
Pixel main(InterpolantsTexture In)
{
    Pixel Out;

    uint2 dims;
    uint samples;
    GetDimensions(texMS, dims.x, dims.y, samples);

    int2 texcoord = floor(In.texcoord * dims);
    Out.color = LoadFromTexMS(texcoord, selectedSample, In.position.xy);

    return Out;
}
