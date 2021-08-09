//--------------------------------------------------------------------------------------
// ParticlePS.hlsl
//
// Pixel shader for rendering a particle.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ParticleCommon.hlsli"

// Particle texture
Texture2D<float4> g_particleTex : register(t0);

[RootSignature(ParticleRS)]
float4 main(in ParticleInterpolants input) : SV_Target
{
    float4 SampleColor = g_particleTex.Sample(g_sampLinear, input.TextureUV);
    return SampleColor * SampleColor.a * input.Color * (min(1, input.Color.a + 0.5f));
}
