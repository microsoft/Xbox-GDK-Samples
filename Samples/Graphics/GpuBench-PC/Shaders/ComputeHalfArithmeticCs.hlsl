//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

RWStructuredBuffer<float> buf : register(u0);

// define Xbox intrinsic replacement on PC
#if !defined(__XBOX_SCARLETT) && !defined(__XBOX_ONE) && (__DXC_VERSION_RELEASE >= 2110)
float16_t2 __XB_AsHalf(uint x)
{
    return float16_t2(asfloat16((uint16_t)x), asfloat16((uint16_t)(x >> 16)));
}

float __XB_AsFloat(float16_t2 x)
{
    return asfloat(uint(asuint16(x.x)) | (uint(asuint16(x.y)) << 16));
}
#endif

[numthreads(8, 8, 1)]
[ROOT_SIGNATURE]
void main(uint3 id : SV_GroupThreadID)
{
#if defined(__XBOX_ONE) || (__DXC_VERSION_RELEASE < 2110)
    // Do nothing as 16-bit types are not supported
#else
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
