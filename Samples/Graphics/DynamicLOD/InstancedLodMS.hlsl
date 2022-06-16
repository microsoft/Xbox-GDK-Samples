//--------------------------------------------------------------------------------------
// InstancedLodMS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "Common.hlsli"
#include "CommonMS.hlsli"

[RootSignature(ROOT_SIG)]
[NumThreads(MS_GROUP_SIZE, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupIndex,
    uint gid : SV_GroupID,
    in payload Payload payload,
    out vertices VertexOut verts[MAX_MESHLET_SIZE],
    out indices uint3 tris[MAX_MESHLET_SIZE])
{
    // Find the LOD to which this threadgroup is assigned.
    // Each wave does this independently to avoid groupshared memory & sync.
    uint offsetCheck = 0;
    uint laneIndex = GetLaneIndex(gtid);

    if (laneIndex <= MAX_LOD_LEVELS)
    {
        offsetCheck = ActiveCountBits(gid >= payload.GroupOffsets[laneIndex]) - 1;
    }
    uint lodIndex = ReadLaneFirst(offsetCheck);

    // Load our LOD meshlet offset & LOD instance count
    uint lodOffset = payload.GroupOffsets[lodIndex];
    uint lodCount = payload.InstanceCounts[lodIndex];

    // Calculate and load our meshlet.
    uint meshletIndex = (gid - lodOffset) / lodCount;
    Meshlet m = Meshlets[lodIndex][meshletIndex];

    // Determine instance count - only 1 instance per threadgroup in the general case
    uint instanceCount = 1;

    // Last meshlet in mesh may be be packed - multiple instances rendered from a single threadgroup.
    if (meshletIndex == MeshInfo[lodIndex].MeshletCount - 1)
    {
        // Determine how many packed instances there are in this group
        uint unpackedGroupCount = (MeshInfo[lodIndex].MeshletCount - 1) * lodCount;
        uint packedIndex = gid - (unpackedGroupCount + lodOffset);

        uint instancesPerGroup = MS_GROUP_SIZE / MeshInfo[lodIndex].LastMeshletSize;
        uint startInstance = packedIndex * instancesPerGroup;

        instanceCount = min(lodCount - startInstance, instancesPerGroup);
    }

    // Compute our total vertex & primitive counts
    uint totalVertCount = m.VertCount * instanceCount;
    uint totalPrimCount = m.PrimCount * instanceCount;

    SetMeshOutputCounts(totalVertCount, totalPrimCount);

    //--------------------------------------------------------------------
    // Export Primitive & Vertex Data

    if (gtid < totalVertCount)
    {
        uint readIndex = gtid % m.VertCount;  // Wrap our reads for packed instancing.
        uint vertexIndex = GetVertexIndex(lodIndex, m, readIndex);

        // Determine our instance index
        uint instanceId = gtid / m.VertCount; // Instance index into this threadgroup's instances (only non-zero for packed threadgroups.)

        uint lodInstance = (gid - lodOffset) % lodCount + instanceId;           // Instance index into this LOD level's instances
        uint instanceOffset = payload.InstanceOffsets[lodIndex] + lodInstance;  // Instance index into the payload instance list

        uint instanceIndex = payload.InstanceList[instanceOffset]; // The final instance index of this vertex.

        verts[gtid] = GetVertexAttributes(lodIndex, meshletIndex, vertexIndex, instanceIndex);
    }

    if (gtid < totalPrimCount)
    {
        uint readIndex = gtid % m.PrimCount;  // Wrap our reads for packed instancing.
        uint instanceId = gtid / m.PrimCount; // Instance index within this threadgroup (only non-zero in last meshlet threadgroups.)

        // Must offset the vertex indices to this thread's instanced verts
        tris[gtid] = GetPrimitive(lodIndex, m, readIndex) + (m.VertCount * instanceId);
    }
}
