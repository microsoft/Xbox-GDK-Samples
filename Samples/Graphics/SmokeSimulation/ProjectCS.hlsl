//--------------------------------------------------------------------------------------
// ProjectCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

// tex0 is m_pPressure
// tex1 is m_pVelocity[1]
// result4 is m_pVelocity[0]

[RootSignature(ComputeRS)]
[numthreads(GroupSize, GroupSize, GroupSize)]
void main(uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    float pL, pR, pB, pT, pD, pU;
    ReadLRBTDU1(GTid, DTid, pL, pR, pB, pT, pD, pU);

    float4 velocity = float4(0, 0, 0, 0);
    float3 vMask = float3(1, 1, 1);
    float3 v;

    v = (tex1[DTid].xyz - (0.5*Modulate*float3(pR - pL, pT - pB, pU - pD)));

    velocity.xyz = (vMask * v);

    result4[DTid] = velocity;
}
