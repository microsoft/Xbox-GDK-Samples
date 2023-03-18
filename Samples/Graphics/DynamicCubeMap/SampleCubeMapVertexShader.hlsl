//--------------------------------------------------------------------------------------
// SampleCubeMapVertexShader.hlsl
//
// Vertex shader for sampling a cube map
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "SampleCubeMapHeader.hlsli"

struct Vertex
{
    float4 position     : SV_Position;
    float3 normal       : NORMAL0;
    float2 texcoord     : TEXCOORD0;
};

[RootSignature(MainRS)]
VS_OUTPUT_SCENEENV main(Vertex In)
{
    VS_OUTPUT_SCENEENV o = (VS_OUTPUT_SCENEENV)0.0f;

    // Output position
    o.Pos = mul(In.position, constants.mWorldViewProj);

    // Compute world space position
    o.wPos = mul(In.position, constants.mWorld);

    // Propogate the normal
    o.wN = In.normal;

    return o;
}
