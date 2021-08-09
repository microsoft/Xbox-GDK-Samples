//--------------------------------------------------------------------------------------
// Gaussian1CS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ComputeCommon.hlsli"

// Emitter size
static const float size = 0.15f;

// Group shared memory used for downsampling to 1/8 size of the 3D volume
groupshared float smem1[GroupSize * GroupSize * GroupSize];

// tex0 is m_pColor[1]
// result1 is m_pColor[0]
// result_mip is m_pColorOneEighth

[RootSignature(ComputeRS)]
[numthreads(GroupSize, GroupSize, GroupSize)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GID : SV_GroupID, uint GI : SV_GroupIndex)
{
    float old = tex0[DTid].x;

    float dist = length(DTid - EmitterCenter.xyz) * size;
    float a = exp(-dist * dist);

    result1[DTid] = smem1[GI] = old * (1 - a) + 1.0f * a;

    GroupMemoryBarrierWithGroupSync();

    if (GI < 256)
        smem1[GI] += smem1[GI + 256];

    GroupMemoryBarrierWithGroupSync();

    if (GI < 128)
        smem1[GI] += smem1[GI + 128];

    GroupMemoryBarrierWithGroupSync();

    if (GI < 64)
        smem1[GI] += smem1[GI + 64];

    GroupMemoryBarrierWithGroupSync();

    if (GI < 32)
        smem1[GI] += smem1[GI + 32];

    GroupMemoryBarrierWithGroupSync();

    if (GI < 16)
        smem1[GI] += smem1[GI + 16];

    GroupMemoryBarrierWithGroupSync();

    if (GI < 8)
        smem1[GI] += smem1[GI + 8];

    GroupMemoryBarrierWithGroupSync();

    if (GI < 4)
        smem1[GI] += smem1[GI + 4];

    GroupMemoryBarrierWithGroupSync();

    if (GI < 2)
        smem1[GI] += smem1[GI + 2];

    GroupMemoryBarrierWithGroupSync();

    // All values in the group shared memory is reduced down to this single value
    // which is an average of all values in the group shared memory
    if (GI == 0)
    {
        smem1[GI] += smem1[GI + 1];
        result_mip[GID] = smem1[0] / (GroupSize * GroupSize * GroupSize);
    }
}
