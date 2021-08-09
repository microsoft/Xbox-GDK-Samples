//--------------------------------------------------------------------------------------
// InstancedMeshletMS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "Common.hlsli"

struct MeshInfo
{
    uint IndexBytes;
    uint MeshletCount;
    uint LastMeshletSize;
};

struct Vertex
{
    float3 Position;
    float3 Normal;
};

struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};

ConstantBuffer<DrawParams>  DrawParams : register(b1);
ConstantBuffer<MeshInfo>    MeshInfo : register(b2);
StructuredBuffer<Vertex>    Vertices : register(t0);
StructuredBuffer<Meshlet>   Meshlets : register(t1);
ByteAddressBuffer           UniqueVertexIndices : register(t2);
StructuredBuffer<uint>      PrimitiveIndices : register(t3);
StructuredBuffer<Instance>  Instances : register(t4);


// Unpacks a 10-bit index triangle primitive into/from a uint.
uint3 UnpackPrimitive(uint primitive)
{
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

// Packs a 10-bit index triangle primitive into/from a uint.
uint PackPrimitive(uint i0, uint i1, uint i2)
{
    return (i0 & 0x3FF) | ((i1 & 0x3FF) << 10) | ((i2 & 0x3FF) << 20);
}

// Loads a 16- or 32-bit vertex index from the unique vertex index buffer.
uint GetVertexIndex(Meshlet m, uint localIndex)
{
    localIndex = m.VertOffset + localIndex;

    if (MeshInfo.IndexBytes == 4)
    {
        return UniqueVertexIndices.Load(localIndex * 4);
    }
    else // Global vertex index width is 16-bit
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

// Loads a 3-index triangle from the primitive index buffer
uint3 GetPrimitive(Meshlet m, uint index)
{
    return UnpackPrimitive(PrimitiveIndices[m.PrimOffset + index]);
}

// Loads and transforms a vertex from the vertex buffer
VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex, uint instanceIndex)
{
    Instance n = Instances[DrawParams.InstanceOffset + instanceIndex];
    Vertex v = Vertices[vertexIndex];

    float4 positionWS = mul(float4(v.Position, 1), n.World);

    VertexOut vout;
    vout.PositionVS = mul(positionWS, Globals.View).xyz;
    vout.PositionHS = mul(positionWS, Globals.ViewProj);
    vout.Normal = mul(float4(v.Normal, 0), n.WorldInvTranspose).xyz;
    vout.MeshletIndex = meshletIndex;

    return vout;
}


//--------------------------------------------------------------------
// Main

[RootSignature(ROOT_SIG)]
[NumThreads(GROUP_SIZE, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupIndex,
    uint gid : SV_GroupID,
    out vertices VertexOut verts[MAX_MESHLET_SIZE],
    out indices uint3 tris[MAX_MESHLET_SIZE])
{
    uint meshletIndex = gid / DrawParams.InstanceCount;
    Meshlet m = Meshlets[meshletIndex];

    // Determine instance count - only 1 instance per threadgroup in the general case
    uint startInstance = gid % DrawParams.InstanceCount;
    uint instanceCount = 1;

    // Last meshlet in mesh may be be packed - multiple instances submitted by a single threadgroup.
    if (meshletIndex == MeshInfo.MeshletCount - 1)
    {
        const uint instancesPerGroup = GROUP_SIZE / MeshInfo.LastMeshletSize;

        // Determine how many packed instances there are in this group
        uint unpackedGroupCount = (MeshInfo.MeshletCount - 1) * DrawParams.InstanceCount;
        uint packedIndex = gid - unpackedGroupCount;

        startInstance = packedIndex * instancesPerGroup;
        instanceCount = min(DrawParams.InstanceCount - startInstance, instancesPerGroup);
    }

    // Compute our total vertex & primitive counts
    uint vertCount = m.VertCount * instanceCount;
    uint primCount = m.PrimCount * instanceCount;

    SetMeshOutputCounts(vertCount, primCount);

    //--------------------------------------------------------------------
    // Export Primitive & Vertex Data

    if (gtid < vertCount)
    {
        uint readIndex = gtid % m.VertCount;  // Wrap our reads for packed instancing.
        uint vertexIndex = GetVertexIndex(m, readIndex);

        // Determine our instance index
        uint instanceId = gtid / m.VertCount; // Instance index into this threadgroup's instances (only non-zero for packed threadgroups.)
        uint instanceIndex = startInstance + instanceId;

        verts[gtid] = GetVertexAttributes(meshletIndex, vertexIndex, instanceIndex);
    }

    if (gtid < primCount)
    {
        uint readIndex = gtid % m.PrimCount;  // Wrap our reads for packed instancing.
        uint instanceId = gtid / m.PrimCount; // Instance index within this threadgroup (only non-zero in last meshlet threadgroups.)

        // Must offset the vertex indices to this thread's instanced verts
        tris[gtid] = GetPrimitive(m, readIndex) + (m.VertCount * instanceId);
    }
}
