//--------------------------------------------------------------------------------------
// DeferredLightingPS.hlsl
//
// Compose deferred particles back into the scene.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

[RootSignature(ParticleRS)]
float4 main(in ScreenInterpolants input) : SV_Target
{
    // Load the particle normal data, opacity, and color
    float3 loadpos = float3(input.Position.xy,0);
    float4 particleData = texNormal.Load(loadpos);
    float4 particleColor = texDiffuse.Load(loadpos);

    // Recreate z-component of the normal
    float nz = sqrt(1 - particleData.x*particleData.x + particleData.y*particleData.y);
    float3 normal = float3(particleData.xy, nz);
    float intensity = particleData.z;

    // move normal into world space
    float3 worldnorm;
    worldnorm = -normal.x * g_particle.Right;
    worldnorm += normal.y * g_particle.Up;
    worldnorm += -normal.z * g_particle.Forward;

    // light
    float lighting = max(0.1, dot(worldnorm, g_particle.LightDirection));

    float3 flashcolor = particleColor.xyz * intensity;
    float3 lightcolor = particleColor.xyz * lighting;
    float3 lerpcolor = lerp(lightcolor, flashcolor, intensity);
    float4 color = (float4(lerpcolor, particleData.a));

    return color;
}
