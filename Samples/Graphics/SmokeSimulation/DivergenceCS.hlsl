//--------------------------------------------------------------------------------------
// Divergence.hlsl
//
// Demonstrates basic Navier-Stokes flow simulation using Compute Shader 5 and 3D textures
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

// tex0 is m_pVelocity[1]
// result4 is m_pTempVector

[RootSignature(ComputeRS)]
[numthreads(GroupSize, GroupSize, GroupSize)]
void main(uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    float4 fieldL, fieldR, fieldB, fieldT, fieldD, fieldU;
    ReadLRBTDU(GTid, DTid, fieldL, fieldR, fieldB, fieldT, fieldD, fieldU);

    float divergence = 0.5 *
        ((fieldR.x - fieldL.x) + (fieldT.y - fieldB.y) + (fieldU.z - fieldD.z));

    result4[DTid] = float4(divergence, divergence, divergence, divergence);
}
