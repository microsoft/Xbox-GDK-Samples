//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

struct s_Frustum
{
    float4 m_plane[6];
};
cbuffer cbFrustum : register(b0)
{
    s_Frustum   m_cameraFrustum;
};

struct s_Sphere
{
    float4 m_centerAndRadius;
};
StructuredBuffer<s_Sphere> bufBoundingSphere : register(t0);

bool CullSphereFrustum(const s_Sphere sphere, const s_Frustum frustum)
{
    bool possiblyIntersect = true;

    [unroll]
    for (uint i = 0; i < 6; ++i)
    {
        if (dot(frustum.m_plane[i], float4(sphere.m_centerAndRadius.xyz, 1.0f)) < -sphere.m_centerAndRadius.w)
        {
            possiblyIntersect = false;
        }
    }

    return possiblyIntersect;
}

// Parallel indirect args declarations to d3d12_x.h
typedef uint2 D3D12_GPU_VIRTUAL_ADDRESS;
struct D3D12_DRAW_INDEXED_ARGUMENTS
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int  BaseVertexLocation;
    uint StartInstanceLocation;
};
struct s_IndirectArgs
{
    D3D12_GPU_VIRTUAL_ADDRESS       m_cb0;
    D3D12_GPU_VIRTUAL_ADDRESS       m_cb1;
    D3D12_DRAW_INDEXED_ARGUMENTS    m_drawIndexedArgs;
};
StructuredBuffer<s_IndirectArgs> bufIndirectArgsIn : register(t1);
AppendStructuredBuffer<s_IndirectArgs> bufIndirectArgsOut : register(u0);

#define ROOT_SIGNATURE_CULL \
    RootSignature\
    (\
        "CBV(b0),"\
        "SRV(t0),"\
        "SRV(t1),"\
        "DescriptorTable(UAV(u0)), " \
    )

[ROOT_SIGNATURE_CULL]
[numthreads(64, 1, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint i = dispatchThreadID.x;
    s_IndirectArgs indirectArgs = bufIndirectArgsIn[i];

    if (CullSphereFrustum(bufBoundingSphere[i], m_cameraFrustum))
    {
        bufIndirectArgsOut.Append(indirectArgs);
    }
}
