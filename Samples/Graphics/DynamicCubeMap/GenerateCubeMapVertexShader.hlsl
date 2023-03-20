//--------------------------------------------------------------------------------------
// GenerateCubeMapVertexShader.hlsl
//
// Vertex shader for generating a cube map
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "GenerateCubeMap.hlsli"

#ifdef __XBOX_SCARLETT
#define __XBOX_PRECOMPILE_VS_GS 1
#define __XBOX_PRECOMPILE_VS_PS 0
#endif

[RootSignature(MainRS)]
GS_CUBEMAP_IN main(uint vertexIndex : SV_VertexId)
{
    GS_CUBEMAP_IN output = (GS_CUBEMAP_IN)0.0f;

    // Compute world position
    uint vertexIndexBytes = vertexInfo.vertexStride * vertexIndex;
    float3 vertexPos = asfloat(Vertices.Load3(vertexIndexBytes));
    output.Pos = mul(float4(vertexPos, 1.0f), constants.mWorld);

    // Propagate tex coord
    float2 texCoord = asfloat(Vertices.Load2(vertexIndexBytes + vertexInfo.texOffset));
    output.Tex = texCoord;

    return output;
}
