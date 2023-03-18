//--------------------------------------------------------------------------------------
// ParticleVS.hlsl
//
// Shaders for rendering particle effects.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ParticleCommon.hlsli"

static const float          g_ParticleScale = 1.0f;

StructuredBuffer<float4>                g_ParticlePositions : register(t0);
ConstantBuffer<ParticleSceneConstCB>    g_PerComponent : register(b0);

[RootSignature(ParticleRS)]
ParticleInterpolants main(in ParticleVertex input, uint vertexID : SV_VertexID, uint ParticleIdx : SV_InstanceID)
{
    ParticleInterpolants Output = (ParticleInterpolants) 0;

    float4 ParticlePos = g_ParticlePositions[ParticleIdx];

    // Add scale into the model matrix
    float s = 1.0f;
    float4x4 model =
    {
        s, 0.0f, 0.0f, 0.0f,
        0.0f, s, 0.0f, 0.0f,
        0.0f, 0.0f, s, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    // Add translation into the model matrix
    model._41 = ParticlePos.x;
    model._42 = ParticlePos.y;
    model._43 = ParticlePos.z;
    
    // Standard transform
    Output.Position = mul(float4(input.Position, 1.0f), model);
    Output.Position = mul(Output.Position, g_PerComponent.matViewProj);
    
    uint lightIndex = asuint(ParticlePos.w);
    Output.Color = CachedColors[lightIndex % COLOR_COUNT];;
    
    return Output;
}
