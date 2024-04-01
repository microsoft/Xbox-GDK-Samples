//--------------------------------------------------------------------------------------
// NormalConeMS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "Common.hlsli"

#define GROUP_SIZE 128

// Resource bindings
cbuffer Globals : register(b0)
{
    float4x4 World         : packoffset(c0);
    float4x4 ViewProj      : packoffset(c4);
    float4   Color         : packoffset(c8);
    float3   ViewUp        : packoffset(c9);
    float3   ViewForward   : packoffset(c10);
    float    Scale         : packoffset(c10.w);
    uint     MeshletOffset : packoffset(c11.x);
    uint     MeshletCount  : packoffset(c11.y);
};

ConstantBuffer<MeshInfo> MeshInfo : register(b1);

StructuredBuffer<CullData> MeshletCullData : register(t0);


//---------------------------------------------
// Main

[RootSignature(ROOT_SIG_DEBUG)]
[NumThreads(GROUP_SIZE, 1, 1)]
[OutputTopology("line")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out vertices DebugVertex verts[GROUP_SIZE],
    out indices uint2 prims[GROUP_SIZE]
)
{
    // Ensure not too many threadgroups were dispatched.
    if (gid >= MeshletCount)
        return;

    // Validate the meshlet index against the meshlet count of the mesh.
    uint meshletIndex = MeshletOffset + gid;
    if (meshletIndex >= MeshInfo.MeshletCount)
        return;

    CullData c = MeshletCullData[meshletIndex];

    // Creating half a UV sphere - a UV hemisphere, if you will.
    const uint n = 14;
    const uint m = 8;

    uint vertCount = n * m + 2;
    uint primCount = n * (m + 1);

    if (IsConeDegenerate(c)) // Degenerate cone
    {
        vertCount = 0;
        primCount = 0;
    }

    SetMeshOutputCounts(vertCount, primCount);

    // Procedurally generate some vertices in the shape of a cone or hemisphere (degenerate cone).
    if (gtid < vertCount)
    {
        float3 center = mul(float4(c.BoundingSphere.xyz, 1), World).xyz; // Center point of cone
        float3 axis;                                                     // Central orientational axis
        float angle;                                                     // Half pi for sphere; angular spread of normal cone.
        float radius = c.BoundingSphere.w * Scale;                       // Radius of bounding sphere

        float4 normalCone = UnpackCone(c.NormalCone);
        float apexOffset = c.ApexOffset * Scale;

        axis = normalize(mul(float4(normalCone.xyz, 0), World)).xyz;
        angle = acos(-normalCone.w) - (3.1415926 / 2); // Angle is stored as -cos(a + 90) to optimize for cull-tests.
        radius += apexOffset;

        float3 pointWS; // Final output position in world-space.

        if (gtid < n * m) // The rings of vertices which compose the UV squares around the hemisphere.
        {
            uint i = gtid % n;
            uint j = gtid / n;

            float3 up = float3(0, 1, 0); // Arbitrary reference vector

            float3 u = normalize(up - dot(axis, up) * axis); // Calculate some orthogonal unit vector
            u = RotateVector(u, axis, i * (3.1415926 * 2) / n);       // Rotate vector about axis by proper theta

            float3 w = RotateVector(axis, u, (m - j) * angle / m); // Rotate axis about vector to get proper phi (angle away from axis.)

            pointWS = center + w;
        }
        else if (gtid < n * m + 1) // Top center point
        {
            pointWS = center + axis;
        }
        else // Bottom center point
        {
            pointWS = center;
        }

        DebugVertex v;
        v.Position = mul(float4(pointWS, 1), ViewProj);
        v.Color = Color;

        verts[gtid] = v;
    }

    // Procedurally generate primitives from the submitted vertices.
    if (gtid < primCount)
    {
        uint2 indices = uint2(0, 0);

        if (gtid < n)
        {
            uint i = gtid % n;

            indices = uint2(i, (i + 1) % n);
        }
        else if (gtid < n * (m - 1))
        {
            uint i = gtid % n;
            uint j = gtid / n - 1;

            indices = uint2(j * n + i, n * (j + 1) + i);
        }
        else if (gtid < n * m)
        {
            uint i = gtid % n;
            uint j = gtid / n - 1;

            indices = uint2(j * n + i, vertCount - 2);
        }
        else
        {
            uint i = gtid % n;

            indices = uint2(i, vertCount - 1);
        }

        prims[gtid] = indices;
    }
}
