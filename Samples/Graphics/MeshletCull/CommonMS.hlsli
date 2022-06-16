//--------------------------------------------------------------------------------------
// CommonMS.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

#ifdef __XBOX_SCARLETT
#define ROOT_SIG \
    "RootFlags(XBOX_FORCE_MEMORY_BASED_ABI), \
     CBV(b0), CBV(b1), CBV(b2), \
     SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), \
     UAV(u0)"
#else
#define ROOT_SIG \
    "CBV(b0), CBV(b1), CBV(b2), \
     SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4), \
     UAV(u0)"
#endif

struct Vertex
{
    float3 Position;
    float3 Normal;
};

struct VertexOut
{
    float3  PositionVS   : POSITION0;
    uint    MeshletIndex : COLOR0;
    float3  Normal       : NORMAL0;
    uint    Metadata     : COLOR1;
    float4  PositionHS   : SV_Position;
};

ConstantBuffer<Constants>   Constants           : register(b0);
ConstantBuffer<Instance>    Instance            : register(b1);
ConstantBuffer<MeshInfo>    MeshInfo            : register(b2);
StructuredBuffer<Vertex>    Vertices            : register(t0);
StructuredBuffer<Meshlet>   Meshlets            : register(t1);
ByteAddressBuffer           UniqueVertexIndices : register(t2);
StructuredBuffer<uint>      PrimitiveIndices    : register(t3);
StructuredBuffer<CullData>  MeshletCullData     : register(t4);


// Packs/unpacks a 10-bit index triangle primitive into/from a uint.
uint3 UnpackPrimitive(uint primitive) { return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF); }
uint  PackPrimitive(uint i0, uint i1, uint i2) { return (i0 & 0x3FF) | ((i1 & 0x3FF) << 10) | ((i2 & 0x3FF) << 20); }

//--------------------------------
// Data Loaders

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

uint3 GetPrimitive(Meshlet m, uint index)
{
    return UnpackPrimitive(PrimitiveIndices[m.PrimOffset + index]);
}

float3 GetWorldPosition(uint vertexIndex)
{
    Vertex v = Vertices[vertexIndex];

    return mul(float4(v.Position, 1), Instance.World).xyz;
}

VertexOut GetVertexAttributes(uint meshletIndex, uint vertexIndex)
{
    Vertex v = Vertices[vertexIndex];

    float4 positionWS = mul(float4(v.Position, 1), Instance.World);

    VertexOut vout;
    vout.PositionVS   = mul(positionWS, Constants.View).xyz;
    vout.PositionHS   = mul(positionWS, Constants.ViewProj);
    vout.Normal       = mul(float4(v.Normal, 0), Instance.WorldInvTrans).xyz;
    vout.MeshletIndex = meshletIndex;

    return vout;
}
