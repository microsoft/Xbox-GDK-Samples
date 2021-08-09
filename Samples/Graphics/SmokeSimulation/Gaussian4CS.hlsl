//--------------------------------------------------------------------------------------
// Gaussian4CS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

// Emitter size
static const float size = 0.15f;

// tex0 is m_pVelocity[0]
// result4 is m_pVelocity[1]

[RootSignature(ComputeRS)]
[numthreads(GroupSize, GroupSize, GroupSize)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    float4 old = tex0[DTid];

    float dist = length(DTid - EmitterCenter.xyz) * size;
    float4 result;
    result.rgb = EmitterDir.xyz;
    result.a = exp(-dist * dist);

    result4[DTid] = float4(old.xyz * (1 - result.a) + result.xyz * result.a, 1.0f);
}
