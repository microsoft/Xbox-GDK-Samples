//--------------------------------------------------------------------------------------
// CommonMS.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct Meshlet
{
    uint VertCount;
    uint VertOffset;
    uint PrimCount;
    uint PrimOffset;
};

struct MeshInfo
{
    uint IndexBytes;
    uint MeshletCount;
    uint LastMeshletSize;
};

RWByteAddressBuffer        LodCounts : register(u0);

ConstantBuffer<DrawParams> DrawParams : register(b1);
StructuredBuffer<Instance> Instances : register(t32);

ConstantBuffer<MeshInfo>   MeshInfo[MAX_LOD_LEVELS] : register(b2);
StructuredBuffer<Vertex>   Vertices[MAX_LOD_LEVELS] : register(t0);
StructuredBuffer<Meshlet>  Meshlets[MAX_LOD_LEVELS] : register(t8);
ByteAddressBuffer          UniqueVertexIndices[MAX_LOD_LEVELS] : register(t16);
StructuredBuffer<uint>     PrimitiveIndices[MAX_LOD_LEVELS] : register(t24);


// Packs/unpacks a 10-bit index triangle primitive into/from a uint.
uint3 UnpackPrimitive(uint primitive) 
{ 
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF); 
}

uint PackPrimitive(uint i0, uint i1, uint i2) 
{ 
    return (i0 & 0x3FF) | ((i1 & 0x3FF) << 10) | ((i2 & 0x3FF) << 20); 
}

// Computes visiblity of an instance
// Performs a simple world-space bounding sphere vs. frustum plane check.
bool IsVisible(float4 boundingSphere)
{
    if (Constants.ForceVisible)
        return true;

    float4 center = float4(boundingSphere.xyz, 1.0);
    float radius = boundingSphere.w;

    for (int i = 0; i < 6; ++i)
    {
        if (dot(center, Constants.Planes[i]) < -radius)
        {
            return false;
        }
    }

    return true;
}

// Computes the LOD for a given instance.
// Calculates the spread of the instance's world-space bounding sphere in screen space.
uint ComputeLOD(float4 boundingSphere)
{
    if (Constants.ForceLOD0)
        return 0;

    float3 v = boundingSphere.xyz - Constants.ViewPosition;
    float r = boundingSphere.w;

    // Sphere radius in screen space
    float size = Constants.RecipTanHalfFovy * r / sqrt(dot(v, v) - r * r);
    size = min(size, 1.0);

    return (1.0 - size) * (Constants.LODCount - 1);
}

//--------------------------------
// Data Loaders

uint GetVertexIndex(uint lodIndex, Meshlet m, uint localIndex)
{
    localIndex = m.VertOffset + localIndex;

    if (MeshInfo[lodIndex].IndexBytes == 4)
    {
        return UniqueVertexIndices[lodIndex].Load(localIndex * 4);
    }
    else // Global vertex index width is 16-bit
    {
        // Byte address must be 4-byte aligned.
        uint wordOffset = (localIndex & 0x1);
        uint byteOffset = (localIndex / 2) * 4;

        // Grab the pair of 16-bit indices, shift & mask off proper 16-bits.
        uint indexPair = UniqueVertexIndices[lodIndex].Load(byteOffset);
        uint index = (indexPair >> (wordOffset * 16)) & 0xffff;

        return index;
    }
}

uint3 GetPrimitive(uint lodIndex, Meshlet m, uint index)
{
    return UnpackPrimitive(PrimitiveIndices[lodIndex][m.PrimOffset + index]);
}

float4 LODColor(float4 boundingSphere)
{
    uint lodLevel = ComputeLOD(boundingSphere);
    float alpha = float(lodLevel) / (Constants.LODCount - 1);

    return lerp(float4(1, 0, 0, 1), float4(0, 1, 0, 1), alpha);
}

VertexOut GetVertexAttributes(uint lodIndex, uint meshletIndex, uint vertexIndex, uint instanceIndex)
{
    Instance n = Instances[instanceIndex];
    Vertex v = Vertices[lodIndex][vertexIndex];

    float4 positionWS = mul(float4(v.Position, 1), n.World);

    VertexOut vout;
    vout.PositionVS   = mul(positionWS, Constants.View).xyz;
    vout.PositionHS   = mul(positionWS, Constants.ViewProj);
    vout.Normal       = mul(float4(v.Normal, 0), n.WorldInvTranspose).xyz;
    vout.Color        = LODColor(n.BoundingSphere);
    vout.MeshletIndex = meshletIndex;

    return vout;
}
