//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

RWStructuredBuffer<float> rwBuf : register(u0);

[numthreads(8, 8, 1)]
[ROOT_SIGNATURE]
void main( uint3 id : SV_GroupID )
{
    uint x = id.x;
    uint y = id.y;

    [unroll]
    for (uint i = 0; i < 256; ++i)
    {
        x += y;
        y += x;
    }
    x += y;

    // There will be some VALU to move 0 and x into VGPRs, but that should occur in parallel with SALU
#pragma warning(disable:3583)
    rwBuf[0] = x;
}
