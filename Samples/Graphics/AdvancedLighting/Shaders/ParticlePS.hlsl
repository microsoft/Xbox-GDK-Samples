//--------------------------------------------------------------------------------------
// ParticlePS.hlsl
//
// Shaders for rendering particle effects.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ParticleCommon.hlsli"

Texture2D<float4>   g_texParticle   : register(t0);

[RootSignature(ParticleRS)]
float4 main(in ParticleInterpolants input) : SV_Target
{
    return input.Color;
}
