//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#define THREADGROUP_X 64
#define TOTAL_MEM_OPS 64

RWBuffer<uint> rwBuf : register(u0);

[numthreads(THREADGROUP_X, 1, 1)]
[ROOT_SIGNATURE]
void main(uint3 idGroup : SV_GroupID, uint3 idGroupThread : SV_GroupThreadID)
{
    uint offset = idGroup.x * THREADGROUP_X + idGroupThread.x;
    uint mask = 1;

#pragma warning(disable:3583)
    [unroll]
    for (uint i = 0; i < TOTAL_MEM_OPS; ++i)
    {                                                                                                                                        
        InterlockedAnd(rwBuf[offset], mask);
        offset += TOTAL_MEM_OPS;
    }
}