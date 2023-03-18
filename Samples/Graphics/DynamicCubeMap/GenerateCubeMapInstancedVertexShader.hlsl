//--------------------------------------------------------------------------------------
// GenerateCubeInstancedMapVertexShader.hlsl
//
// Vertex shader for generating a cube map
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "GenerateCubeMap.hlsli"

[RootSignature(MainRS)]
PS_CUBEMAP_IN main(uint vertexIndex : SV_VertexId, uint cubeSide : SV_InstanceID)
{
    PS_CUBEMAP_IN output = (PS_CUBEMAP_IN)0.0f;

    // Compute world position
    uint vertexIndexBytes = vertexInfo.vertexStride * vertexIndex;
    float3 vertexPos = asfloat(Vertices.Load3(vertexIndexBytes));
    output.Pos = mul(float4(vertexPos, 1.0f), constants.mWorld);
    output.Pos = mul(output.Pos, cubeMapConstants.mViewCBM[cubeSide]);
    output.Pos = mul(output.Pos, constants.mProj);
    output.RTIndex = cubeSide;

    // Propagate tex coord
    float2 texCoord = asfloat(Vertices.Load2(vertexIndexBytes + vertexInfo.texOffset));
    output.Tex = texCoord;

    return output;
}
