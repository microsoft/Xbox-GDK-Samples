//--------------------------------------------------------------------------------------
// PointSpritesMS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"

#define VERT_COUNT GROUP_SIZE
#define PRIM_COUNT VERT_COUNT / 2
#define PARTICLE_COUNT VERT_COUNT / 4


static const float2 s_vertOffsets[4] =
{
    float2(-1,  1),
    float2(-1, -1),
    float2( 1,  1),
    float2( 1, -1),
};

static const float2 s_vertUVs[4] =
{
    float2(0, 1),
    float2(1, 1),
    float2(0, 0),
    float2(1, 0),
};

static const uint3 s_indices[2] =
{
    uint3(0, 1, 2),
    uint3(1, 3, 2)
};

ConstantBuffer<Constants> Constants : register(b0);
StructuredBuffer<Particle> Particles : register(t0);

[RootSignature(ROOT_SIG)]
[NumThreads(GROUP_SIZE, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint dtid : SV_DispatchThreadID,
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out vertices VertexOut verts[VERT_COUNT],
    out indices uint3 tris[PRIM_COUNT]
)
{
    // Calculate the number of particles this thread group must process
    uint numParticles = min(Constants.ParticleCount - gid * PARTICLE_COUNT, PARTICLE_COUNT);

    // Dervie our vertex & primitive counts based on particle counts & geometry type (two-triangle quads)
    uint numVerts = numParticles * 4;
    uint numPrims = numParticles * 2;

    // Set our mesh counts
    SetMeshOutputCounts(numVerts, numPrims);

    // Process vertices
    if (gtid < numVerts)
    {
        // Load vertex data
        uint particleIndex = dtid >> 2;
        Particle p = Particles[particleIndex];

        // Derive camera properties
        float3 up = Constants.CameraUp;
        float3 forward = normalize(p.Position - Constants.ViewPosition);
        float3 right = cross(forward, up);

        // Particle lifetime properties
        float alpha = ((Constants.SimulationTime - p.InitTime) / p.Lifetime);
        float oneMinusAlpha = 1.0 - alpha;
        float alphaSq = alpha * alpha;

        // Translate/scale vertices to proper billboard positions
        uint vertexIndex = dtid & 0x3;

        float2 size = (-alphaSq + 1) * 0.5 * (p.Size * 0.5);
        float3 rightOffset = right * size.x * s_vertOffsets[vertexIndex].x;
        float3 upOffset = up * size.y * s_vertOffsets[vertexIndex].y;

        float3 position = p.Position + upOffset + rightOffset;

        // Write out vertex
        VertexOut vout;
        vout.Position = mul(float4(position, 1), Constants.ViewProj);
        vout.Color = float4(Constants.Color, oneMinusAlpha * 0.5);
        vout.UV = s_vertUVs[vertexIndex];

        verts[gtid] = vout;
    }

    // Process primitives
    if (gtid < numPrims)
    {
        // Offset the local sprite indices to the proper vertex array index.
        tris[gtid] = ((gtid & ~0x1) * 2) + s_indices[gtid & 0x1];
    }
}
