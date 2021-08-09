//--------------------------------------------------------------------------------------
// ParticleVS.hlsl
//
// Vertex shader for particle rendering.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

[RootSignature(ParticleRS)]
ParticleInterpolants main(in ParticleVertex input)
{
    ParticleInterpolants Output = (ParticleInterpolants)0;

    // Standard transform
    Output.Position = mul(float4(input.Position, 1), g_particle.ViewProj);
    Output.TextureUVI.xy = input.TextureUV;
    Output.Color = input.Color;

    // Loop over the glow lights (from the explosions) and light our particle
    float runningintensity = 0;
    uint count = g_glowLights.NumGlowLights;
    for (uint i = 0; i < count; i++)
    {
        float3 delta = g_glowLights.GlowLightPosIntensity[i].xyz - input.Position;
        float distSq = dot(delta, delta);
        float3 d = float3(1, 0, distSq);

        float fatten = 1.0 / dot(g_glowLights.GlowLightAttenuation.xyz, d);

        float intensity = fatten * g_glowLights.GlowLightPosIntensity[i].w * g_glowLights.GlowLightColor[i].w;
        runningintensity += intensity;
        Output.Color += intensity * g_glowLights.GlowLightColor[i];
    }
    Output.TextureUVI.z = runningintensity;

    // Rotate our texture coordinates
    float Rot = -input.Rot;
    Output.SinCosThetaLife.x = sin(Rot);
    Output.SinCosThetaLife.y = cos(Rot);
    Output.SinCosThetaLife.z = input.Life;

    return Output;
}