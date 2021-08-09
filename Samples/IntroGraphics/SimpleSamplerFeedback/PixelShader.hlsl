//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// Diffuse texture
Texture2D DiffuseTexture : register(t0);
SamplerState TriLinearSampler : register(s0);

// MinMip feedback map paired to the diffuse texture
FeedbackTexture2D<SAMPLER_FEEDBACK_MIN_MIP> MinMipFeedbackMap : register(u0);

[RootSignature(MainRS)]
float4 main(PS_INPUT input) : SV_Target
{
    float4 color = DiffuseTexture.Sample(TriLinearSampler, input.TexCoord);

    // Use sampler feedback to determine and record the requested mip level of the diffuse texture into the feedback map
    // Note: This has to use the same sampler as the paired texture, in this case the trilinear sampler of the diffuse texture
    MinMipFeedbackMap.WriteSamplerFeedback(DiffuseTexture, TriLinearSampler, input.TexCoord);

    return color;
}


// HLSL function that emulates the SM 6.5 method FeedbackTexture2D::WriteSamplerFeedback
// for MinMip feedback maps. MinMipFeedbackMap is passed as an image UAV here.
void FeedbackTexture2D_WriteSamplerFeedback(RWTexture2D<uint> MinMipFeedbackMap, Texture2D PairedTexture, SamplerState Sampler, float2 Location)
{
    // Calculate LOD from a single sample location (crude approximation of sampler pattern)
    float LODValue = PairedTexture.CalculateLevelOfDetailUnclamped(Sampler, Location);

    // Convert the float LOD to an unsigned fixed point value with 3 bits of fractional precision
    uint EncodedLODValue = (uint)(LODValue * 8);

    // Convert the normalized texture coordinates into absolute texel coordinates in the feedback map
    uint2 FeedbackDimensions;
    MinMipFeedbackMap.GetDimensions(FeedbackDimensions.x, FeedbackDimensions.y);

    uint2 WriteLocation;
    WriteLocation.x = (uint)(Location.x * FeedbackDimensions.x);
    WriteLocation.y = (uint)(Location.y * FeedbackDimensions.y);

    // Write the LOD value to the feedback map with an atomic MIN operation
    InterlockedMin(MinMipFeedbackMap[WriteLocation], EncodedLODValue);
}
