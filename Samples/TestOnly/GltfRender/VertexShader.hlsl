//--------------------------------------------------------------------------------------
// VertexShader.hlsl
//
// Simple vertex shader for rendering a textured model
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

uint GetIndices(uint bufferIndex, uint vertexID)
{
    Buffer<uint> indexBuffer = ResourceDescriptorHeap[bufferIndex];
    return indexBuffer[vertexID];
}

[RootSignature(MainRS)]
Interpolants main(uint vertexIndex : SV_VertexId)
{
    Interpolants Out = (Interpolants) 0.0f;

    // Output position
    MeshInfo currentMeshInfo = g_meshInfo[g_meshCB.meshInfoIndex];
    const uint index = GetIndices(currentMeshInfo.indicesIndex, vertexIndex);
    uint vertexIndexBytes = currentMeshInfo.vertexStride * index;
    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[currentMeshInfo.vbIndex];
    float3 vertexPos = asfloat(vertexBuffer.Load3(vertexIndexBytes));
    Out.worldPos = mul(float4(vertexPos, 1.0f), g_meshCB.world).xyz;
    Out.position = mul(float4(Out.worldPos, 1.0f), g_sceneCB.projectionViewWorld);

    // texcoord
    Out.texcoord = asfloat(vertexBuffer.Load2(vertexIndexBytes + currentMeshInfo.texOffset));

    // normal
    float3 objectSpaceNormal = asfloat(vertexBuffer.Load3(vertexIndexBytes + +currentMeshInfo.normalOffset));
    Out.normal = mul(normalize(objectSpaceNormal), float3x3(g_meshCB.world[0].xyz, g_meshCB.world[1].xyz, g_meshCB.world[2].xyz));

    return Out;
}
