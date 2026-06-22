//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

RWStructuredBuffer<uint> buf : register(u0);

// define Xbox intrinsic replacement on PC
#if !defined(__XBOX_SCARLETT) && !defined(__XBOX_ONE) && (__DXC_VERSION_RELEASE >= 2110)
uint16_t2 __XB_AsUInt16(uint x)
{
    return uint16_t2(x, x >> 16);
}

uint __XB_AsUInt(uint16_t2 x)
{
    return uint(x.x) | (uint(x.y) << 16);
}
#endif

[numthreads(8, 8, 1)]
[ROOT_SIGNATURE]
void main(uint3 id : SV_GroupThreadID)
{
#if defined(__XBOX_ONE) || (__DXC_VERSION_RELEASE < 2110)
    // Do nothing as 16-bit types are not supported
#else
    uint16_t2 x16 = __XB_AsUInt16(id.x);

    [unroll]
    for (uint i = 0; i < 512; ++i)
    {
        x16 *= x16;
    }

#pragma warning(disable:3583)
    buf[id.z] = __XB_AsUInt(x16);
#endif
}
