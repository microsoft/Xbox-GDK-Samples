//--------------------------------------------------------------------------------------
// Sharpen.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define HLSL_INCLUDE
#include "../SharedDataTypes.h"

#include "RootSignatures.hlsli"

ConstantBuffer<SharpenPassCB> constants : register(b0);
Texture2D<float4> inTexture : register(t0);
RWTexture2D<float4> outTexture : register(u0);

// https://github.com/TheRealMJP/MSAAFilter/blob/master/MSAAFilter/PostProcessing.hlsl
// Approximates luminance from an RGB value
float CalculateLuminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

// https://github.com/TheRealMJP/MSAAFilter/blob/master/MSAAFilter/PostProcessing.hlsl
[RootSignature(SharpenPassRS)]
[numthreads(TEMPORAL_RESOLVE_THREAD_X, TEMPORAL_RESOLVE_THREAD_Y, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    float3 inputColor = inTexture[DTid.xy].xyz;

    if (constants.skipSharpen == false)
    {
        float inputLuminance = CalculateLuminance(inputColor);

        float avgLuminance = 0.0f;

        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                avgLuminance += CalculateLuminance(inTexture[DTid.xy + int2(x, y)].xyz);
            }
        }
        avgLuminance /= 9.0f;

        float sharpenedLuminance = inputLuminance - avgLuminance;
        float finalLuminance = inputLuminance + sharpenedLuminance * constants.SharpeningAmount;
        inputColor = inputColor * (finalLuminance / inputLuminance);
    }

    outTexture[DTid.xy] = float4(inputColor, 1.0f);
}
