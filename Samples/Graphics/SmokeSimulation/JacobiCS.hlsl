//--------------------------------------------------------------------------------------
// JacobiCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

// alternating pass 0:
//      tex0 is Texture_pressure				
//      tex1 is Texture_tempvector			
//      result1 is Texture_tempscalar		
//
// alternating pass 1:
//      tex0 is m_pTempScalar				
//      tex1 is Texture_tempvector			
//      result1 is Texture_pressure		

[RootSignature(ComputeRS)]
[numthreads(GroupSize, GroupSize, GroupSize)]
void main(uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    // Texture_tempvector contains the "divergence" computed by PS_DIVERGENCE
    float bC = tex1[DTid].x;

    float pL, pR, pB, pT, pD, pU;
    ReadLRBTDU1(GTid, DTid, pL, pR, pB, pT, pD, pU);

    result1[DTid] = (pL + pR + pB + pT + pU + pD - bC) / 6.0;
}
