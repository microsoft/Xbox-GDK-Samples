//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#define THREADGROUP_X 64
#define TOTAL_MEM_OPS 64

#ifndef ELEMENT_BYTES
#define ELEMENT_BYTES 4
#endif

#ifndef TYPEMASK
#include "BufferFormat.hlsli"
#endif

#ifndef NUM_FORMAT
#define NUM_FORMAT NUM_FORMAT_UINT
#endif

#ifndef DATA_FORMAT
#define DATA_FORMAT DATA_FORMAT_8
#endif

#ifndef ELEMENT_BYTES
#define STORE_OP XB_TypedStore4
#endif

#ifndef CHANNEL_MASK
#define CHANNEL_MASK xyzw
#endif

RWByteAddressBuffer rwBuf : register(u1);

[numthreads(THREADGROUP_X, 1, 1)]
[ROOT_SIGNATURE]
void main(uint3 idGroup : SV_GroupID, uint3 idGroupThread : SV_GroupThreadID)
{
    uint offset = idGroup.x * THREADGROUP_X + idGroupThread.x;
    uint typeMask = TYPEMASK(NUM_FORMAT, DATA_FORMAT);
	uint4 zero = 0;

    [unroll]
    for (uint i = 0; i < TOTAL_MEM_OPS; ++i)
    {                
        // We use XB_TypedStore here rather than a regular buffer write.
        // This requires that the instruction match the format of the buffer.
        //
        // With a regular buffer write, HLSL semantics requires a buffer_store_format_xyzw,
        // in order to support the case of swizzle. For instance, the buffer descriptor 
        // might specify that the .w element of the written value go into the .x 
        // element of the buffer. The typed store will be correct in the absence of a 
        // non-default swizzle.
        //
        // A full xyzw buffer store can be more expensive than a store with the native 
        // number of channels, due to loss of coalescing.  
        rwBuf.STORE_OP(ELEMENT_BYTES * offset, zero.CHANNEL_MASK, typeMask);
        offset += TOTAL_MEM_OPS;
    }
}

