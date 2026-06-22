//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// The "All samples" visualization
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

//-------------------------------------------------------------------------------------------------------------
// Name: PSTextureAllSamples()
// Desc: For each pixel, draw the color values for all samples, in a rectangular array.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
Pixel main(InterpolantsTexture In)
{
    Pixel Out;

    uint2 dims;
    uint samples;
    GetDimensions(texMS, dims.x, dims.y, samples);

    // Build a roughly square array of samples for each pixel
    int2 subDims;
    subDims.y = exp2(int(log2(samples)) >> 1);
    subDims.x = samples / subDims.y;

    int2 texcoord = floor(In.texcoord * dims);
    int2 subTexcoord = floor(In.texcoord * dims * subDims) % subDims;
    int sample = subTexcoord.x + subDims.x * subTexcoord.y;
    Out.color = LoadFromTexMS(texcoord, sample, In.position.xy);

    return Out;
}
