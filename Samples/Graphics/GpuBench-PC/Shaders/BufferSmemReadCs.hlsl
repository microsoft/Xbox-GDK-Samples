//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#define THREADGROUP_X 64
#define TOTAL_MEM_OPS 64

// The best version so far
ByteAddressBuffer buf : register(t0);
RWBuffer<uint> rwBuf : register(u0);

[numthreads(THREADGROUP_X, 1, 1)]
[ROOT_SIGNATURE]
void main()
{
    // We cannot use a dispatch input such as SV_GroupId for the fetch offset, 
    // because that would produce SALU ops, which compete for issue with the 
    // SMEM ops we are trying to measure.
    uint offset = 0;
    float value = 1.0f;

    uint memOps = asfloat(buf.Load(0)) != 1245.0 ? TOTAL_MEM_OPS : 1;

    for (uint i = 0; i < memOps; ++i)
    {
        value *= asfloat(buf.Load(offset)); // We must use the results, but not in an SALU instruction, and in at most one VALU instruction
        offset += THREADGROUP_X; // The exact stride has enormous implications on performance, for unknown reasons
    }

#pragma warning(disable:3583)
    if (asfloat(buf.Load(0)) != value) // the buffer is initialized to 0, so this is impossible
    {
        rwBuf[0] = value;
    }
}
