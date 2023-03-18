//--------------------------------------------------------------------------------------
// SceneVertexShader.hlsl
//
// Simple vertex shader for rendering a textured model
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Scene.hlsli"

[RootSignature(MainRS)]
Interpolants main(uint vertexIndex : SV_VertexId)
{
    Interpolants Out = (Interpolants)0.0f;

    // Output position
    uint vertexIndexBytes = vertexInfo.vertexStride * vertexIndex;
    float3 vertexPos = asfloat(Vertices.Load3(vertexIndexBytes));
    Out.position = mul(float4(vertexPos, 1.0f), constants.mWorldViewProj);

    // Propagate tex coord
    Out.texcoord = asfloat(Vertices.Load2(vertexIndexBytes + vertexInfo.texOffset));

    return Out;
}
