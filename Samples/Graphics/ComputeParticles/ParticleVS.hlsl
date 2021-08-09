//--------------------------------------------------------------------------------------
// ParticleVS.hlsl
//
// Vertex shader that transforms a particle to its instanced position (based
// on the AppendBuffer we filled in the compute step). 
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

// This StructuredBuffer is the same buffer as the AppendStructuredBuffer that's filled
// in the ComputeShader Advance phase. Our instance count is the count of items 
// in this buffer.
StructuredBuffer<float4>    g_particlePositions : register(t0);
Texture1D<float4>           g_blackbodyLUT : register(t1); // Blackbody radiation color lookup

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
    ClipSpacePos.xy += ClipSpaceScale.xy * g_billboardPositions[BillboardVertex] * g_particleScale;

    // Output final position...
    Output.Position = ClipSpacePos;

    // And pass through texture UV...
    Output.TextureUV = g_billboardUVs[BillboardVertex];

    // Finally, grab the particle color based on the normalized life value looking up into our blackbody 
    // 1D lookup.
    Output.Color = g_blackbodyLUT.SampleLevel(g_sampLinear, max(0.01, Particle.w - 0.5f), 0);

    return Output;
}