//--------------------------------------------------------------------------------------
// MeshletCullAS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "CommonMS.hlsli"

// The groupshared payload data to export to dispatched mesh shader threadgroups
groupshared Payload s_Payload;

RWByteAddressBuffer Counts : register(u0);

// Some enum values for a thread's final visibility
#define INVALID 0
#define VISIBLE 1
#define CULLED_FRUSTUM 2
#define CULLED_NORMALCONE 3

uint IsVisible(CullData c, float4x4 world, float scale, float3 viewPos)
{
    if (!Constants.Cull)
        return VISIBLE;

    // Do a cull test of the bounding sphere against the view frustum planes.
    float4 center = mul(float4(c.BoundingSphere.xyz, 1), world);
    float radius = c.BoundingSphere.w * scale;

    for (int i = 0; i < 6; ++i)
    {
        if (dot(center, Constants.Planes[i]) < -radius)
        {
            return CULLED_FRUSTUM;
        }
    }

    // Do normal cone culling
    if (IsConeDegenerate(c))
        return VISIBLE; // Cone is degenerate - spread is wider than a hemisphere.

    // Unpack the normal cone from its 8-bit uint compression
    float4 normalCone = UnpackCone(c.NormalCone);

    // Transform axis to world space
    float3 axis = normalize(mul(float4(normalCone.xyz, 0), world)).xyz;

    // Offset the normal cone axis from the meshlet center-point - make sure to account for world scaling
    float3 apex = center.xyz - axis * c.ApexOffset * scale;
    float3 view = normalize(viewPos - apex);

    // The normal cone w-component stores -cos(angle + 90 deg)
    // This is the min dot product along the inverted axis from which all the meshlet's triangles are backface
    if (dot(view, -axis) > normalCone.w)
    {
        return CULLED_NORMALCONE;
    }

    // All tests passed - it will merit pixels
    return VISIBLE;
}


[RootSignature(ROOT_SIG)]
[NumThreads(AS_GROUP_SIZE, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint dtid : SV_DispatchThreadID, uint gid : SV_GroupID)
{
    uint cullCode = INVALID;

    // Check bounds of meshlet cull data resource
    if (dtid < MeshInfo.MeshletCount)
    {
        // Do visibility testing for this thread
        cullCode = IsVisible(MeshletCullData[dtid], Instance.World, Instance.Scale, Constants.CullViewPosition);
    }

    // Compact visible meshlets into the export payload array
    if (cullCode == VISIBLE)
    {
        uint index = PrefixCountBits(cullCode == VISIBLE);
        s_Payload.MeshletIndices[index] = dtid;
    }

    // Write our cull counts out to UAV for readback to the CPU
    Counts.Store((gid * 2 + 0) * 4, ActiveCountBits(cullCode == CULLED_FRUSTUM));
    Counts.Store((gid * 2 + 1) * 4, ActiveCountBits(cullCode == CULLED_NORMALCONE));

    // Dispatch the required number of MS threadgroups to render the visible meshlets
    uint visibleCount = ActiveCountBits(cullCode == VISIBLE);
    DispatchMesh(visibleCount, 1, 1, s_Payload);
}
