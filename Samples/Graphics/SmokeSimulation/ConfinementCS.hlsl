//--------------------------------------------------------------------------------------
// ConfinementCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

// tex0 is m_pTempVector
// tex1 is m_pVelocity[1]
// result4 is m_pVelocity[0]

[RootSignature(ComputeRS)]
[numthreads(GroupSize, GroupSize, GroupSize)]
void main(uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    // Texture_tempvector contains the vorticity computed by VorticityCS
    float4 omega = tex0[DTid];

    float4 L = 0, R = 0, B = 0, T = 0, D = 0, U = 0;
    ReadLRBTDU(GTid, DTid, L, R, B, T, D, U);
    float omegaL = L.w;
    float omegaR = R.w;
    float omegaB = B.w;
    float omegaT = T.w;
    float omegaD = D.w;
    float omegaU = U.w;

    float4 oldVel = tex1[DTid];

    float3 eta = 0.5 * float3(omegaR - omegaL,
        omegaT - omegaB,
        omegaU - omegaD); // grid size is 1, so 0.5 = 1 / (gridsize * 2)

    eta = normalize(eta + float3(0.0001, 0.0001, 0.0001));

    float4 force = float4(0, 0, 0, 0);
    force.xyz = Epsilon /** gridsize*/ * cross(eta, omega.xyz); /*float3( eta.y * omega.z - eta.z * omega.y,
                                                                         eta.z * omega.x - eta.x * omega.z,
                                                                         eta.x * omega.y - eta.y * omega.x ); */ // grid size is 1

    result4[DTid] = oldVel + DeltaTime * force;
}
