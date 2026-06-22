//--------------------------------------------------------------------------------------
// AoTexVS.hlsl
//
// Simple vertex shader for rendering the AO texture
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "AoTex.hlsli"

static const float4 positions[4] =
{
    float4(-1.0f, -1.0f, 0.5, 1.0f),
    float4(-1.0f, 1.0f, 0.5, 1.0f),
    float4(1.0f, -1.0f, 0.5, 1.0f),
    float4(1.0f, 1.0f, 0.5, 1.0f)
};

static const float2 texCoords[4] =
{
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 1.0f)
};

[RootSignature(MainRS)]
Interpolants main(uint vertexIndex : SV_VertexId)
{
    Interpolants Out = (Interpolants)0.0f;

    // Output position
    Out.position = positions[vertexIndex];
    Out.texcoord = texCoords[vertexIndex];

    return Out;
}
