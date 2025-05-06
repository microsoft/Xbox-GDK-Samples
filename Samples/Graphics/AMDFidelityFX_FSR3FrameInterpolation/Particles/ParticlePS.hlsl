//--------------------------------------------------------------------------------------
// ParticlePS.hlsl
//
// Pixel shader for rendering a particle.
//
// Modifications Copyright (C) 2022. Advanced Micro Devices, Inc. All Rights Reserved.
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
    return input.Color * (min(1, input.Color.a + 0.5f)) * SampleColor.a;
}
