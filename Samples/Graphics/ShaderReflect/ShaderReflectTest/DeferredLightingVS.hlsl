//--------------------------------------------------------------------------------------
// DeferredLightingVS.hlsl
//
// Vertex shader used for full-screen composition pass.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

static const float2 g_positions[] = {
    float2(-1, 1),
    float2(3, 1),
    float2(-1, -3)
};

[RootSignature(ParticleRS)]
ScreenInterpolants main(in uint VertexId : SV_VertexID)
{
    ScreenInterpolants Output;
    Output.Position = float4(g_positions[VertexId], 0, 1);
    return Output;
}
