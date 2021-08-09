//--------------------------------------------------------------------------------------
// AdvectVelocityCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

// tex0 is m_pVelocity[0]
// result4 is m_pVelocity[1]

[RootSignature(ComputeRS)]
[numthreads(GroupSize, GroupSize, GroupSize)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float3 npos = GetAdvectedPosTexCoords(DTid);
    result4[DTid] = tex0.SampleLevel(samLinear, npos, 0) * Modulate;
}
