//--------------------------------------------------------------------------------------
// MeshletCullAS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "GenerateCubeMap.hlsli"

#ifdef __XBOX_SCARLETT
#define __XBOX_ENABLE_WAVE32 1
#endif

bool IsConeDegenerate(CullData c)
{
    return (c.NormalCone >> 24) == 0xff;
}

float4 UnpackCone(uint packed)
{
    float4 v;
    v.x = float((packed >> 0) & 0xFF);
    v.y = float((packed >> 8) & 0xFF);
    v.z = float((packed >> 16) & 0xFF);
    v.w = float((packed >> 24) & 0xFF);

    v = v / 255.0;
    v.xyz = v.xyz * 2.0 - 1.0;

    return v;
}

bool IsFrontface(CullData c, float4x4 world, float scale, float3 viewPos)
{
    // Do normal cone culling
    if (IsConeDegenerate(c))
        return true; // Cone is degenerate - spread is wider than a hemisphere.

    // Unpack the normal cone from its 8-bit uint compression
    float4 normalCone = UnpackCone(c.NormalCone);

    // Transform axis to world space
    float3 axis = normalize(mul(float4(normalCone.xyz, 0), world)).xyz;

    // Offset the normal cone axis from the meshlet center-point - make sure to account for world scaling
    float4 center = mul(float4(c.BoundingSphere.xyz, 1), world);
    float3 apex = center.xyz - axis * c.ApexOffset * scale;
    float3 view = normalize(viewPos - apex);

    // The normal cone w-component stores -cos(angle + 90 deg)
    // This is the min dot product along the inverted axis from which all the meshlet's triangles are backface
    if (dot(view, -axis) > normalCone.w)
    {
        return false;
    }

    return true;
}

bool IsInFrustum(CullData c, float4x4 world, float scale, float3 viewPos, uint face)
{
    // Do a cull test of the bounding sphere against the view frustum planes.
    float4 center = mul(float4(c.BoundingSphere.xyz, 1), world);
    float radius = c.BoundingSphere.w * scale;

    for (int i = 0; i < 6; ++i)
    {
        float4 plane = cubeMapConstants.planesPerCube[face][i];
        float dotProd = dot(center, plane);
        if (dotProd < -radius)
        {
            return false;
        }
    }

    return true;
}

// The groupshared payload data to export to dispatched mesh shader threadgroups
groupshared Payload s_Payload;


[RootSignature(MainRS)]
[NumThreads(6, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint dtid : SV_DispatchThreadID, uint gid : SV_GroupID)
{
    uint meshletIndex = gid + subMeshletInfo.offset;
    s_Payload.meshletIndex = meshletIndex;
    CullData c = MeshletCullData[meshletIndex];

    bool isFrontFace = IsFrontface(c, constants.mWorld, constants.scale, cubeMapConstants.cubeViewPos);
    bool inFrustum = IsInFrustum(c, constants.mWorld, constants.scale, cubeMapConstants.cubeViewPos, gtid);
    bool visible = isFrontFace && inFrustum;
    uint index = WavePrefixCountBits(visible);
    s_Payload.faceIndex[index] = gtid;
    uint faces = WaveActiveCountBits(visible);

    DispatchMesh(faces, 1, 1, s_Payload);
}
