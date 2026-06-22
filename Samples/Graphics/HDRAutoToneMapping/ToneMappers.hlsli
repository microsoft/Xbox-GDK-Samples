//--------------------------------------------------------------------------------------
// ToneMap.hlsl
//
// Define some tone map operators
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

// Simple Reinhard tone mapper
float3 ToneMapReinhard(float3 x)
{
    return x / (1.0f + x);
}

// Filmic tone mapper
float3 ToneMapFilmic(float3 x)
{
    return pow((x * (6.2f * x + 0.5f)) / (x * (6.2f * x + 1.7f) + 0.06f), 2.2f);
}

// Average between Reinhard and Filmic
float3 CustomToneMapper(float3 x)
{
    return (ToneMapReinhard(x) + ToneMapFilmic(x)) / 2.0f;
}