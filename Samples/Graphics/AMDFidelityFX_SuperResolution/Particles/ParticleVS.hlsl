//--------------------------------------------------------------------------------------
// ParticleVS.hlsl
//
// Modifications Copyright (C) 2022. Advanced Micro Devices, Inc. All Rights Reserved.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ParticleCommon.hlsli"

static const float2 g_billboardPositions[4] =
{
    float2(-1.0f, -1.0f),
    float2(-1.0f,  1.0f),
    float2(1.0f, -1.0f),
    float2(1.0f,  1.0f),
};

static const float2 g_billboardUVs[4] =
{
    float2(0.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f),
};

RWStructuredBuffer<float4>    g_particlePositions : register(u0);

[RootSignature(ParticleRS)]
ParticleInterpolants main(uint BillboardVertex : SV_VertexID, uint ParticleIdx : SV_InstanceID)
{
    ParticleInterpolants Output = (ParticleInterpolants)0;

    // Get the particle's world position.
    float4 Particle = g_particlePositions[ParticleIdx];
    float3 WorldPosition = Particle.xyz;

    // Transform the world position into clip space.
    float4 ClipSpacePos = mul(float4(WorldPosition, 1), ViewProj);

    // Now expand (in clip-space) based on the vertex in the quad we are processing. g_ParticleScale
    // contains the _11 and _22 components of the projection matrix, which we use to ensure we
    // expand by the right amount in clip-space.  
    ClipSpacePos.xy += ClipSpaceScale.xy * g_billboardPositions[BillboardVertex] * g_particleScale* max(0.1f, Particle.w* 0.275);

    // Output final position...
    Output.Position = ClipSpacePos;

    // And pass through texture UV...
    Output.TextureUV = g_billboardUVs[BillboardVertex];


    float falloff = 1.0f;
    if (Particle.w < 0.2)
    {
        falloff = (Particle.w / 0.2);
    }
    Output.Color = falloff * float4(0.5+ 0.4 * sqrt(Particle.w), 0.25 + 0.5 * Particle.w * Particle.w, 0.1 + 0.9 * Particle.w * Particle.w, 0.8);

    return Output;
}
