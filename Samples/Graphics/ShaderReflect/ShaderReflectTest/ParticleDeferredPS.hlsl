//--------------------------------------------------------------------------------------
// ParticleDeferredPS.hlsl
//
// Pixel shader for deferred particle rendering
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

[RootSignature(ParticleRS)]
DeferredPixel main(in ParticleInterpolants input)
{
    DeferredPixel output = (DeferredPixel)0;

    float4 normAndAlpha = texDiffuse.Sample(sampLinear, input.TextureUVI.xy);

    // unbias
    float3 norm = normAndAlpha.xyz * 2 - 1;

    // rotate our texture coordinate and our normal basis
    float3 rotnorm;
    float fSinTheta = input.SinCosThetaLife.x;
    float fCosTheta = input.SinCosThetaLife.y;

    rotnorm.x = fCosTheta * norm.x - fSinTheta * norm.y;
    rotnorm.y = fSinTheta * norm.x + fCosTheta * norm.y;
    rotnorm.z = norm.z;

    // rebias
    norm = rotnorm;

    // Fade
    float alpha = normAndAlpha.a * (1.0f - input.SinCosThetaLife.z);
    float4 normalpha = float4(norm.xy * alpha, input.TextureUVI.z * alpha, alpha);

    // Output final data to render targets.
    output.NormalAndAlpha = normalpha;
    output.Color = input.Color * alpha;

    return output;
}