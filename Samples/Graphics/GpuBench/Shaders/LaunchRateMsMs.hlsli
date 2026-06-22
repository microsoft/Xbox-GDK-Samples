//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#ifdef __XBOX_SCARLETT
struct VertexOut
{
    float4 position : SV_Position;
};

cbuffer Constant : register(b0)
{
    uint numPrims;
}
static const uint numVerts = 3;

[RootSignature("RootConstants(num32BitConstants=1, b0)")]
[numthreads(GROUP_SIZE, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    out indices uint3 tris[GROUP_SIZE],
    out vertices VertexOut verts[GROUP_SIZE]
)
{
    SetMeshOutputCounts(numVerts, numPrims); // In this benchmark, numPrims is defined at runtime intentionally
                                             // Otherwise, compiler optimizes subgroup with one wave regardless of numthreads                                             

    if (gtid < numVerts)
    {
        verts[gtid].position = 0;
    }

    if (gtid < numPrims)
    {
        tris[gtid] = uint3(0, 1, 2);
    }    
}
#endif

