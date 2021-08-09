//--------------------------------------------------------------------------------------
// VorticityCS.hlsl
//
// Simple pixel shader for rendering volumetric data
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

// tex0 is m_pVelocity[1]
// result4 is m_pTempVector

[RootSignature(ComputeRS)]
[numthreads(GroupSize, GroupSize, GroupSize)]
void main(uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID) // Curl operator on velocity
{
    float4 L, R, B, T, D, U;
    ReadLRBTDU(GTid, DTid, L, R, B, T, D, U);

    float4 vorticity = float4(0, 0, 0, 0);
    vorticity.xyz = 0.5 * float3(((T.z - B.z) - (U.y - D.y)),
        ((U.x - D.x) - (R.z - L.z)),
        ((R.y - L.y) - (T.x - B.x))); // http://en.wikipedia.org/wiki/Curl_(mathematics)
                                           // grid size is 1, so 0.5 = 1 / (gridsize * 2)
    vorticity.w = length(vorticity.xyz);
    result4[DTid] = vorticity;
}
