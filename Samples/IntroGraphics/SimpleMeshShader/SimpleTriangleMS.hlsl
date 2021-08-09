//--------------------------------------------------------------------------------------
// SimpleTriangleMS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

// Hardcoded screen-space triangle positions, colors, & indices
static const float4 s_Positions[3] =
{
    float4(-0.5f, -0.5f, 0.5f, 1.0f),
    float4( 0.0f,  0.5f, 0.5f, 1.0f),
    float4( 0.5f, -0.5f, 0.5f, 1.0f),
};

static const float4 s_Color[3] =
{
    float4(1.0, 0.0, 0.0, 1.0f),
    float4(0.0, 1.0, 0.0, 1.0f),
    float4(0.0, 0.0, 1.0, 1.0f),
};

static const uint3 s_Triangle = uint3(0, 1, 2);

[RootSignature("")]
[NumThreads(32, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    out indices uint3 tris[1],
    out vertices VertexOut verts[3]
)
{
    // SetMeshOutputCounts(numVerts, numPrims) 
    // - Mandatory intrinsics function call in Mesh Shaders
    // - Must be called from non-divergent codepath
    // - Assumes threadgroup constant values (reads from lane 0)
    // - Must be <= length of indices & vertices arrays.
    SetMeshOutputCounts(3, 1); // Set the correct counts for a triangle: 3 verts & 1 primitive

    // Only 3 verts so use first 3 threads to process & export.
    if (gtid < 3)
    {
        VertexOut vout;
        vout.Position = s_Positions[gtid];
        vout.Color = s_Color[gtid];

        // Write out vertex data to the 'vertices' array.
        verts[gtid] = vout;
    }

    // Only 1 prim so use first thread to process & export.
    if (gtid < 1)
    {
        // Write out index data to the 'indices' array.
        tris[gtid] = s_Triangle;
    }
}
