//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// The "Nearest sample" visualization
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
// Desc: For each pixel, draw the color values of the nearest samples, by brute force iteration.
//-------------------------------------------------------------------------------------------------------------
ROOT_SIGNATURE_GRAPHICS
Pixel main(InterpolantsTexture In)
{
    Pixel Out;

    uint2 dims;
    uint samples;
    GetDimensions(texMS, dims.x, dims.y, samples);

    // Coordinates of pixel 
    int2 texcoord = floor(In.texcoord * dims);

    // Find the nearest sample location to the texcoords
    int nearestSample = 0;
    float nearestSampleDistanceSq = 1e10f; // Largest distance between texcoords is sqrt(2)
    for (uint sample = 0; sample < samples; ++sample)
    {
        // Can't do EvaluateAttributeAtSample here, because the RT is not multisampled!!!
        //float2 sampleTexcoord = EvaluateAttributeAtSample( In.texcoord, sample );
        float2 sampleTexcoord = texcoord + 0.5f + GetSamplePosition(texMS, sample);
        float2 diff = sampleTexcoord - In.texcoord * float2(dims);
        float distSq = dot(diff, diff);
        if (distSq < nearestSampleDistanceSq)
        {
            nearestSampleDistanceSq = distSq;
            nearestSample = sample;
        }
    }

    Out.color = LoadFromTexMS(texcoord, nearestSample, In.position.xy);

    return Out;
}

