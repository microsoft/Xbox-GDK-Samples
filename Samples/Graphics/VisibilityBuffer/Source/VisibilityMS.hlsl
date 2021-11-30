//------------------------------------------------------------------------------------
// VisibilityPS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "CommonHLSL.h"
#include "common.hlsli"

struct Interpolants
{
    float4 position     : SV_Position;
};

struct PrimitiveAttributes
{
    uint primitiveID : SV_PrimitiveID;
};

ConstantBuffer<ConstantsVis> constantInfo : register(b0);

[RootSignature(MainRSVis)]
[outputtopology("triangle")]
[numthreads(MESHLET_MAX_PRIMITIVES, 1, 1)]
void main(
    in uint dtid   : SV_DispatchThreadID,
    in uint gindex : SV_GroupIndex,
    in uint gid    : SV_GroupID,
    out vertices Interpolants verts[MESHLET_MAX_VERTICES],
    out indices uint3 triangles[MESHLET_MAX_PRIMITIVES],
    out primitives PrimitiveAttributes sharedPrimitives[MESHLET_MAX_PRIMITIVES])
{
    StructuredBuffer<MeshletDesc> MeshletBuffer          = ResourceDescriptorHeap[Descriptors::MeshletBuffer + constantInfo.vertexBufferIndex];
    StructuredBuffer<uint>        UniqueIndicesBuffer    = ResourceDescriptorHeap[Descriptors::UniqueIndices + constantInfo.vertexBufferIndex];
    StructuredBuffer<uint3>   PrimitiveIndicesBuffer = ResourceDescriptorHeap[Descriptors::PrimitiveIndices + constantInfo.vertexBufferIndex];

    MeshletDesc curMeshlet = MeshletBuffer[gid];

    StructuredBuffer<VertexElement> VertexBuffer = ResourceDescriptorHeap[Descriptors::VertexBuffer + constantInfo.vertexBufferIndex];

    // Set number of outputs
    SetMeshOutputCounts(curMeshlet.numVertices, curMeshlet.numPrimitives);

    // Transform the vertices and write them
    if (gindex < curMeshlet.numVertices)
    {
        uint index = UniqueIndicesBuffer[curMeshlet.startUniqueVertex + gindex];
        VertexElement vertex = VertexBuffer[index];

        Interpolants Out;
        Out.position = mul(constantInfo.mvpMatrix, float4(vertex.position, 1.0f));

        verts[gindex] = Out;
    }

    // Now write the primitives
    if (gindex < curMeshlet.numPrimitives)
    {
        triangles[gindex] = PrimitiveIndicesBuffer[curMeshlet.startPrimitiveIndex + gindex];

        PrimitiveAttributes Attributes;

        if (constantInfo.drawOverlay == OverlayModes::MeshletID)
        {
            // Use primitiveID field to pass through group ID (same as meshlet ID). Saves export fields.
            Attributes.primitiveID = gid;
        }
        else
        {
            // PrimitiveID ordering is maintained in meshlets, so can use this value with normal index buffer.
            Attributes.primitiveID = curMeshlet.startPrimitiveIndex + gindex;
        }

        sharedPrimitives[gindex] = Attributes;
    }
}
