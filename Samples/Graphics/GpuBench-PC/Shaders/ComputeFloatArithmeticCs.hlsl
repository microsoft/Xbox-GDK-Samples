//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

RWStructuredBuffer<float> buf : register(u0);

[numthreads(8, 8, 1)]
[ROOT_SIGNATURE]
void main( uint3 id : SV_GroupThreadID )
{
    float x = asfloat(id.x);
    float y = asfloat(id.y);

    [unroll]
    for (uint i = 0; i < 512; ++i)
    {
        x = x * x + y;
    }

#pragma warning(disable:3583)
    buf[id.z] = x;
}