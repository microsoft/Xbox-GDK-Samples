//--------------------------------------------------------------------------------------
// RenderMS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.hlsli"

#define __XBOX_ENABLE_WAVE32 1

#define GROUP_SIZE 32
#define MAX_VERTS 32
#define MAX_PRIMS 16

// Hardcoded vertes indices for each primitive.
static const uint3 Primitives[16] =
{
    uint3(0, 1, 2),    uint3(2, 3, 0),
    uint3(4, 5, 6),    uint3(6, 7, 4),
    uint3(8, 9, 10),   uint3(10, 11, 8),
    uint3(12, 13, 14), uint3(14, 15, 12),
    uint3(16, 17, 18), uint3(18, 19, 16),
    uint3(20, 21, 22), uint3(22, 23, 20),
    uint3(24, 25, 26), uint3(26, 27, 24),
    uint3(28, 29, 30), uint3(30, 31, 28),
};

// Normally use a root signature without ALLOW_INPUT_ASSEMBLER flag for mesh shader pipelines.
// Omitted in this case as it adds complexity to the sample without improving performance.
[RootSignature(CommonRS)]
[OutputTopology("triangle")]
[NumThreads(GROUP_SIZE, 1, 1)]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupIndex,
    uint dtid : SV_DispatchThreadID,
    out vertices VSOut verts[MAX_VERTS],
    out indices uint3 tris[MAX_PRIMS]
)
{
    // Limit our vertex and primitive count to the number required for the particles.
    uint vertCount = min(particleCount * 4 - gid * MAX_VERTS, MAX_VERTS);
    uint primCount = min(particleCount * 2 - gid * MAX_PRIMS, MAX_PRIMS);

    SetMeshOutputCounts(vertCount, primCount);

    // Vertex phase
    if (gtid < vertCount)
    {
        const uint sourceIndex = dtid / 4;
        const uint vi = dtid % 4;

        VSIn vertex = ReadVertex(sourceIndex);

        verts[gtid] = VSRenderNativeQuadHelper(vertex, vi);
    }

    // Primitive phase
    if (gtid < primCount)
    {
        tris[gtid] = Primitives[gtid];
    }
}
