//--------------------------------------------------------------------------------------
// SamplerFeedbackEmulationBuffer.hlsli
//
// HLSL function that emulates one form of the FeedbackTexture2D::WriteSamplerFeedback
// method. The MinLOD feedback map is passed as a buffer UAV here, along with dimensions
// of a 2D texture to be used when encoding the buffer elements.
// 
// On some older GPU hardware, there is a performance benefit to emulating the feedback map 
// as a linear texture stored within a buffer UAV instead of as a proper texture 2D image UAV.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


void FeedbackTexture2D_WriteSamplerFeedback(RWBuffer<uint> MinLODFeedbackMapBuffer, uint4 MinLODFeedbackMapBufferDimensions, Texture2D SampledTexture, SamplerState Sampler, float2 Location)
{
    // Calculate LOD from a single sample location (crude approximation of sampler pattern)
    float LODValue = SampledTexture.CalculateLevelOfDetailUnclamped(Sampler, Location);

    // Convert the float LOD to an unsigned fixed point value with 3 bits of fractional precision
    uint EncodedLODValue = (uint)(LODValue * 8);

    // Convert the normalized texture coordinates into absolute texel coordinates in the feedback map
    uint2 WriteLocation;
    WriteLocation.x = (uint)(Location.x * MinLODFeedbackMapBufferDimensions.x);
    WriteLocation.y = (uint)(Location.y * MinLODFeedbackMapBufferDimensions.y);

    // Convert absolute texel coordinates to a linear texture layout buffer offset
    uint BufferOffset = WriteLocation.y * MinLODFeedbackMapBufferDimensions.x + WriteLocation.x;

    // Write the LOD value to the feedback map buffer with an atomic MIN operation
    InterlockedMin(MinLODFeedbackMapBuffer[BufferOffset], EncodedLODValue);
}
