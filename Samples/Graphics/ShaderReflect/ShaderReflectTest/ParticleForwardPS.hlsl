//--------------------------------------------------------------------------------------
// ParticleForwardPS.hlsl
//
// Pixel shader for forward particle rendering.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

[RootSignature(ParticleRS)]
float4 main(in ParticleInterpolants input) : SV_Target
{
    float4 diffuse = texDiffuse.Sample(sampLinear, input.TextureUVI.xy);

    // unbias
    float3 norm = diffuse.xyz * 2 - 1;

    // rotate
    float3 rotnorm;
    float fSinTheta = input.SinCosThetaLife.x;
    float fCosTheta = input.SinCosThetaLife.y;

    rotnorm.x = fCosTheta * norm.x - fSinTheta * norm.y;
    rotnorm.y = fSinTheta * norm.x + fCosTheta * norm.y;
    rotnorm.z = norm.z;

    // rebias
    norm = rotnorm;

    // Fade
    float alpha = diffuse.a * (1.0f - input.SinCosThetaLife.z);

    // rebias	
    float intensity = input.TextureUVI.z * alpha;

    // move normal into world space
    float3 worldnorm;
    worldnorm = -norm.x * g_particle.Right;
    worldnorm += norm.y * g_particle.Up;
    worldnorm += -norm.z * g_particle.Forward;

    // Light the particle.
    float lighting = max(0.1, dot(worldnorm, g_particle.LightDirection));

    float3 flashcolor = input.Color.xyz * intensity;
    float3 lightcolor = input.Color.xyz * lighting;
    float3 lerpcolor = lerp(lightcolor, flashcolor, intensity);
    float4 color = float4(lerpcolor, alpha);

    return color;
}