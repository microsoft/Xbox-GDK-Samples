//--------------------------------------------------------------------------------------
// GenerateCubeMapMeshShader.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "GenerateCubeMap.hlsli"

#define MAX_MESHLET_SIZE 128
#define GROUP_SIZE MAX_MESHLET_SIZE

uint GetVertexIndex(Meshlet m, uint localIndex)
{
    localIndex = m.VertOffset + localIndex;

    if (meshInfo.IndexBytes == 4) // 32-bit Vertex Indices
    {
        return UniqueVertexIndices.Load(localIndex * 4);
    }
    else // 16-bit Vertex Indices
    {
        // Byte address must be 4-byte aligned.
        uint wordOffset = (localIndex & 0x1);
        uint byteOffset = (localIndex / 2) * 4;

        // Grab the pair of 16-bit indices, shift & mask off proper 16-bits.
        uint indexPair = UniqueVertexIndices.Load(byteOffset);
        uint index = (indexPair >> (wordOffset * 16)) & 0xffff;

        return index;
    }
}

// Packs/unpacks a 10-bit index triangle primitive into/from a uint.
uint3 UnpackPrimitive(uint primitive)
{
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

uint3 GetPrimitive(Meshlet m, uint index)
{
    return UnpackPrimitive(PrimitiveIndices[m.PrimOffset + index]);
}

[RootSignature(MainRS)]
[NumThreads(GROUP_SIZE, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint dtid : SV_DispatchThreadID,
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload Payload payload,
    out indices uint3 tris[MAX_MESHLET_SIZE],
    out vertices MS_CUBEMAP_VERT_OUT verts[MAX_MESHLET_SIZE],
    out primitives MS_CUBEMAP_PRIM_OUT prims[MAX_MESHLET_SIZE]
)
{
    Meshlet m = Meshlets[payload.meshletIndex];
    uint faceIndex = payload.faceIndex[gid];

    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    if (gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid);
        MS_CUBEMAP_PRIM_OUT output;
        output.RTIndex = faceIndex;
        prims[gtid] = output;
    }

    if (gtid < m.VertCount)
    {
        uint vertexIndex = GetVertexIndex(m, gtid);
        uint vertexIndexBytes = subMeshletInfo.vertexStride * vertexIndex;
        float3 vertexPos = asfloat(Vertices.Load3(vertexIndexBytes));
        MS_CUBEMAP_VERT_OUT output;
        output.Pos = mul(float4(vertexPos, 1.0f), constants.mWorld);
        output.Pos = mul(output.Pos, cubeMapConstants.mViewCBM[faceIndex]);
        output.Pos = mul(output.Pos, constants.mProj);
        float2 texCoord = asfloat(Vertices.Load2(vertexIndexBytes + subMeshletInfo.texOffset));
        output.Tex = texCoord;
        verts[gtid] = output;
    }
}
