//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

RWStructuredBuffer<float> buf : register(u0);

[numthreads(8, 8, 1)]
[ROOT_SIGNATURE]
void main(uint3 id : SV_GroupThreadID)
{
#if defined(__XBOX_SCARLETT) && (__DXC_VERSION_RELEASE >= 2110)
    float16_t2 x16 = __XB_AsHalf(id.x);
    float16_t2 y16 = __XB_AsHalf(id.y);

    [unroll]
    for (uint i = 0; i < 512; ++i)
    {
        x16 = x16 * x16 + y16;
    }

#pragma warning(disable:3583)
    buf[id.z] = __XB_AsFloat(x16);
#endif
}
