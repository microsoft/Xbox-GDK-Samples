//--------------------------------------------------------------------------------------
// PixelShader.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

// Feedback map writes are highly redundant. The screen coverate of each 64KB tile is relative large.
// We can randomly discard feedback map writes and still get enough data through to the streaming system.
// In this sample we discard 99% of feedback map writes, giving a big perf win
static const float g_StochasticDiscard = 1.0f / 100.0f;

// Random generator
float NormalizedRandom()
{
    uint laneID = __XB_GetLaneID();
    uint swizzledLaneID = laneID ^ (laneID << 2) ^ (laneID << 5);
    uint randomBits = __XB_MemTime().x ^ swizzledLaneID;
    float randomValue = (float)(randomBits & 0x7F) / 128.0f;
    return randomValue;
}

// Randomly discard feedback map writes
bool DoSamplerFeedback()
{
    float randomValue = NormalizedRandom();
    return randomValue < g_StochasticDiscard;
}

// Tiled texture
Texture2D TiledTexture : register(t0);
sampler TriLinearSampler : register(s1);

// MinMip map holding info about tile residency
Texture2D MinMipMap : register(t1);

// Scarlett specific sampler for sampling the MinMip map
sampler MinMipSampler : register(s0);

// MinMip feedback map paired to the tiled texture
FeedbackTexture2D<SAMPLER_FEEDBACK_MIN_MIP> FeedbackMap : register(u0);

[RootSignature(MainRS)]
float4 main(PS_INPUT input) : SV_Target
{
    // Sample the MinMip map using the Scarlett specific MinMip map filter, producing a per-pixel LOD clamp value
    float minMipResident = MinMipMap.Sample(MinMipSampler, input.TexCoord).x;

    // Sample the tiled texture, clamping to the highest detail mip currently resident
    int2 NullOffset = int2(0, 0);
    float4 color = TiledTexture.Sample(TriLinearSampler, input.TexCoord, NullOffset, minMipResident);

    // Use sampler feedback to determine and record the requested mip level of the tiled texture into the feedback map
    // Note: This has to use the same sampler as the paired texture, in this case the trilinear sampler of the diffuse texture
    if (DoSamplerFeedback())
    {
        FeedbackMap.WriteSamplerFeedback(TiledTexture, TriLinearSampler, input.TexCoord);
    }

    return color;
}
