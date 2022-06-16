//--------------------------------------------------------------------------------------
// CullMeshletMS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"
#include "CommonMS.hlsli"

// This layout should generate a single 128bit load and store to and from it (64x2 op)
groupshared float4 s_Position[MAX_MESHLET_SIZE];

// Stores/loads transformed positions to/from LDS memory.
void StorePosition(uint loc, float4 val)
{
    s_Position[loc] = val;
}

float4 LoadPosition(uint loc)
{
    return s_Position[loc];
}


// Primitive culling routine.
bool CullPrimitive(float4 p0, float4 p1, float4 p2)
{
    // Back-face culling
    float3x3 posMatrix = { p0.xyw, p1.xyw, p2.xyw };
    if (determinant(posMatrix) > 0)
    {
        return true;
    }

    // Convert to NDC coordinates
    p0 = float4(p0.xyz / p0.w, p0.w);
    p1 = float4(p1.xyz / p1.w, p1.w);
    p2 = float4(p2.xyz / p2.w, p2.w);

    // Viewport culling
    if (min(p0.x, min(p1.x, p2.x)) > 1 || max(p0.x, max(p1.x, p2.x)) < -1 ||
        min(p0.y, min(p1.y, p2.y)) > 1 || max(p0.y, max(p1.y, p2.y)) < -1)
    {
        return true;
    }

    // Across far plane.
    if (min(p0.z, min(p1.z, p2.z)) > 1)
    {
        return true;
    }

    // Behind Camera
    if (max(p0.w, max(p1.w, p2.w)) <= 0)
    {
        return true;
    }

    return false;
}


// Mask for active vertices (1 dword per vertex to prevent memory lane access collision.)
groupshared uint s_VertexDwordMask[MAX_MESHLET_SIZE];

groupshared uint s_PrimCount[WAVES_PER_GROUP + 1];
groupshared uint s_VertCount[WAVES_PER_GROUP + 1];

groupshared uint s_RemappedPrims[MAX_MESHLET_SIZE];
groupshared uint s_RemappedVerts[MAX_MESHLET_SIZE];

groupshared uint s_VertLookup[MAX_MESHLET_SIZE];


[RootSignature(ROOT_SIG)]
[NumThreads(GROUP_SIZE, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint3 tris[MAX_MESHLET_SIZE],
    out vertices VertexOut verts[MAX_MESHLET_SIZE]
)
{
    s_VertexDwordMask[gtid] = 0;

    Meshlet m = Meshlets[gid];

    //--------------------------------------------------------------------
    // Do vertex transformations 
    // Would be better to de-interleave position data to optimize cache efficiency and reduce bandwidth.

    if (gtid < m.VertCount)
    {
        uint vertexIndex = GetVertexIndex(m, gtid);

        float3 positionWS = GetWorldPosition(vertexIndex);
        float4 positionHS = mul(float4(positionWS, 1), Constants.ViewProj);

        StorePosition(gtid, positionHS);
    }
    GroupMemoryBarrierWithGroupSync();


    //--------------------------------------------------------------------
    // Culling

    uint3 indices = 0;

    bool cullPrim = true;
    bool cullVert = true;

    if (gtid < m.PrimCount)
    {
        // Load the primitive's vertex indices
        indices = GetPrimitive(m, gtid);

        float4 p0, p1, p2;
        if (Constants.DebugCull)
        {
            // Cull against the debug camera - we'll have to reload & transform vertices against the debug view-projection.
            uint i0 = GetVertexIndex(m, indices.x);
            uint i1 = GetVertexIndex(m, indices.y);
            uint i2 = GetVertexIndex(m, indices.z);

            p0 = mul(float4(GetWorldPosition(i0), 1), Constants.DebugViewProj);
            p1 = mul(float4(GetWorldPosition(i1), 1), Constants.DebugViewProj);
            p2 = mul(float4(GetWorldPosition(i2), 1), Constants.DebugViewProj);
        }
        else
        {
            // Cull against the main camera, for which we've already transformed the vertices.
            p0 = LoadPosition(indices.x);
            p1 = LoadPosition(indices.y);
            p2 = LoadPosition(indices.z);
        }

        cullPrim = CullPrimitive(p0, p1, p2);

        if (!cullPrim)
        {
            s_VertexDwordMask[indices.x] = 1;
            s_VertexDwordMask[indices.y] = 1;
            s_VertexDwordMask[indices.z] = 1;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    // Generate an active vertex mask for the wave.
    if (gtid < m.VertCount)
    {
        cullVert = s_VertexDwordMask[gtid] == 0;
    }


    //--------------------------------------------------------------------
    // Primitive & Vertex Compaction

    {
        uint count1 = ActiveCountBits(!cullPrim);
        uint count2 = ActiveCountBits(!cullVert);

        if (IsFirstLane(gtid))
        {
            uint waveIndex = GetWaveIndex(gtid);

            s_PrimCount[waveIndex + 1] = count1;
            s_VertCount[waveIndex + 1] = count2;
        }
        GroupMemoryBarrierWithGroupSync();
        
        if (gtid == 0)
        {
            s_PrimCount[0] = 0;
            s_VertCount[0] = 0;

            for (int i = 2; i < WAVES_PER_GROUP + 1; ++i)
            {
                s_PrimCount[i] += s_PrimCount[i - 1];
                s_VertCount[i] += s_VertCount[i - 1];
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }

    // Vertex remapping - only for active vertex lanes.
    if (!cullVert)
    {
        uint waveIndex = GetWaveIndex(gtid);
        uint waveDestOffset = s_VertCount[waveIndex];

        uint threadIndex = PrefixCountBits(!cullVert);
        uint remapIndex = waveDestOffset + threadIndex;

        s_RemappedVerts[remapIndex] = gtid;
        s_VertLookup[gtid] = remapIndex;
    }

    // Primitive fix-up & remapping - only for active primitive lanes.
    if (!cullPrim)
    {
        uint waveIndex = GetWaveIndex(gtid);
        uint waveDestOffset = s_PrimCount[waveIndex];

        uint threadIndex = PrefixCountBits(!cullPrim);
        uint remapIndex = waveDestOffset + threadIndex;

        // We need to remap the primitive's vertex indices
        s_RemappedPrims[remapIndex] = PackPrimitive(indices);
    }
    GroupMemoryBarrierWithGroupSync();


    //--------------------------------------------------------------------
    // Read & declare our actual vertex & primitive counts.

    uint wavesPerGroup = GetWavesPerGroup(GROUP_SIZE);
    uint numVerts = s_VertCount[wavesPerGroup];
    uint numPrims = s_PrimCount[wavesPerGroup];

    SetMeshOutputCounts(numVerts, numPrims);

    // Kill all lanes that have no export work - hopefully results in entire waves retiring
    if (gtid >= numVerts && gtid >= numPrims)
    {
        return;
    }


    //--------------------------------------------------------------------
    // Export Primitive & Vertex Data

    if (gtid < numPrims)
    {
        uint3 indices = UnpackPrimitive(s_RemappedPrims[gtid]);

        // Write the packed primitive indices.
        tris[gtid] = uint3(s_VertLookup[indices.x], s_VertLookup[indices.y], s_VertLookup[indices.z]);
    }

    if (gtid < numVerts)
    {
        // Read the compacted vertex index.
        uint localIndex = s_RemappedVerts[gtid];
        uint vertexIndex = GetVertexIndex(m, localIndex);

        verts[gtid] = GetVertexAttributes(gid, vertexIndex);
    }
}
