//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

RWStructuredBuffer<uint> buf : register(u0);

[numthreads(8, 8, 1)]
[ROOT_SIGNATURE]
void main( uint3 id : SV_GroupThreadID )
{
    double x = asdouble(id.x, id.y);
    double y = asdouble(id.y, id.x);

    [unroll]
    for (uint i = 0; i < 256; ++i)
    {
        x += y;
        y += x;
    }

    asuint(x, id.x, id.x);
#pragma warning(disable:3583)
    buf[id.z] = id.x;
}
