//--------------------------------------------------------------------------------------
// SamplerFeedbackEmulation.hlsli
//
// HLSL function that emulates one form of the FeedbackTexture2D::WriteSamplerFeedback
// method. The MinLOD feedback map is passed as an image UAV here.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

void FeedbackTexture2D_WriteSamplerFeedback(RWTexture2D<uint> MinLODFeedbackMap, Texture2D SampledTexture, SamplerState Sampler, float2 Location)
{
    // Calculate LOD from a single sample location (crude approximation of sampler pattern)
    float LODValue = SampledTexture.CalculateLevelOfDetailUnclamped(Sampler, Location);

    // Convert the float LOD to an unsigned fixed point value with 3 bits of fractional precision
    uint EncodedLODValue = (uint)(LODValue * 8);

    // Convert the normalized texture coordinates into absolute texel coordinates in the feedback map
    uint2 FeedbackDimensions;
    MinLODFeedbackMap.GetDimensions(FeedbackDimensions.x, FeedbackDimensions.y);

    uint2 WriteLocation;
    WriteLocation.x = (uint)(Location.x * FeedbackDimensions.x);
    WriteLocation.y = (uint)(Location.y * FeedbackDimensions.y);

    // Write the LOD value to the feedback map with an atomic MIN operation
    InterlockedMin(MinLODFeedbackMap[WriteLocation], EncodedLODValue);
}
