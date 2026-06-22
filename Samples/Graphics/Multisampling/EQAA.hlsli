//--------------------------------------------------------------------------------------
// EQAA.hlsli
//
// Routines for extending GetDimensions and GetSamplePosition for the case of EQAA
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#if ENABLE_EQAA

#include "FMaskConstants.hlsli"   // For LOG_NUM_SAMPLES

void GetDimensions(uniform Texture2DMS< float4 > texMS, out uint width, out uint height, out uint numberOfSamples)
{
    texMS.GetDimensions(width, height, numberOfSamples);
    numberOfSamples = NUM_SAMPLES;
}

cbuffer cbEQAA : register(b3)
{
    int2 g_EQAASamplePositions[MAX_LOG_NUM_FRAGMENTS + 1][MAX_QUALITY + 1][1U << MAX_LOG_NUM_SAMPLES];
};
#define SAMPLE_POSITION_UNIT 16.0f

float2 GetSamplePosition(uniform Texture2DMS< float4 > texMS, uniform in int sampleindex)
{
    return g_EQAASamplePositions[LOG_NUM_FRAGMENTS][QUALITY][sampleindex] / SAMPLE_POSITION_UNIT;
}

#else

void GetDimensions(uniform Texture2DMS< float4 > texMS, out uint width, out uint height, out uint numberOfSamples)
{
    texMS.GetDimensions(width, height, numberOfSamples);
}

float2 GetSamplePosition(uniform Texture2DMS< float4 > texMS, uniform in int sampleindex)
{
    return texMS.GetSamplePosition(sampleindex);
}

#endif
