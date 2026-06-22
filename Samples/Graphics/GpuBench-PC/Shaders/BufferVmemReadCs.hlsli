//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#define THREADGROUP_X 64
#define TOTAL_MEM_OPS 64

#ifndef DATA_TYPE
#define DATA_TYPE uint4
#endif

#ifndef CHANNEL_MASK
#define CHANNEL_MASK xyzw
#endif

Buffer buf : register(t0);
RWBuffer<DATA_TYPE> rwBuf : register(u0);

[numthreads(THREADGROUP_X, 1, 1)]
[ROOT_SIGNATURE]
void main(uint3 idGroup : SV_GroupID, uint3 idGroupThread : SV_GroupThreadID)
{
    uint offset = idGroup.x * THREADGROUP_X + idGroupThread.x;
    uint4 value = 0;

    [unroll]
    for (uint i = 0; i < TOTAL_MEM_OPS; ++i)
    {     
        // This line will generate buffer_load_format_* instructions
        value += buf[offset];
        offset += TOTAL_MEM_OPS;
    }

#pragma warning(disable:3583)
    if (value.x == 12345678) // the buffer is initialized to 0, so this is impossible
    {
        // By using .CHANNEL_MASK here, we force the channel mask for the fetches
        rwBuf[0] = value.CHANNEL_MASK;
    }
}
