//--------------------------------------------------------------------------------------
// BasicMeshletMS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "CommonMS.hlsli"

[RootSignature(ROOT_SIG)]
[NumThreads(MS_GROUP_SIZE, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint dtid : SV_DispatchThreadID,
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    in payload Payload payload,
    out indices uint3 tris[MS_GROUP_SIZE],
    out vertices VertexOut verts[MS_GROUP_SIZE]
)
{
    // Load the meshlet from the AS payload data
    uint meshletIndex = payload.MeshletIndices[gid];

    // Catch any out-of-range indices (in case too many MS threadgroups were dispatched from AS)
    if (meshletIndex >= MeshInfo.MeshletCount)
        return;

    // Load the meshlet
    Meshlet m = Meshlets[meshletIndex];

    // Our vertex and primitive counts come directly from the meshlet
    SetMeshOutputCounts(m.VertCount, m.PrimCount);

    //--------------------------------------------------------------------
    // Export Primitive & Vertex Data

    if (gtid < m.VertCount)
    {
        uint vertexIndex = GetVertexIndex(m, gtid);
        verts[gtid] = GetVertexAttributes(meshletIndex, vertexIndex);
    }

    if (gtid < m.PrimCount)
    {
        tris[gtid] = GetPrimitive(m, gtid);
    }
}
