//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifndef MRT_COUNT
#error MRT_COUNT must be defined
#endif

#if MRT_COUNT == 0

StructuredBuffer<uint> buf : register(t0);

[ROOT_SIGNATURE]
void main(void)
{
    if (buf[0] == 12345678) // the buffer is a null descriptor, so this is impossible
    {
        discard;
    }
}

#else

#ifndef RT_FORMAT
#error RT_FORMAT must be defined
#endif

struct PixelMRT
{
    RT_FORMAT color[MRT_COUNT]  : SV_Target0;
};

#pragma warning(disable:3578)
[ROOT_SIGNATURE]
void main(out PixelMRT Out)
{
    [unroll]
    for (uint rt = 0; rt < MRT_COUNT; ++rt)
    {
        // Avoid various hardware fast paths for 0.0 and 1.0
        Out.color[rt] = 0.5f;
    }
}

#endif
