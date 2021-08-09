//--------------------------------------------------------------------------------------
// AdvectColorCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

// tex0 is m_pVelocity[0], tex1 is m_pColor[0]
// result1 is m_pColor[1] 

[RootSignature(ComputeRS)]
[numthreads(GroupSize, GroupSize, GroupSize)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 npos = GetAdvectedPosTexCoords(DTid);
    result1[DTid] = tex1.SampleLevel(samLinear, npos, 0).x * 0.995f;
}
