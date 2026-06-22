//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RootSignature.hlsli"

#ifndef BYTES_PER_CHANNEL
#define BYTES_PER_CHANNEL 1
#endif

[ROOT_SIGNATURE]
void main(float4 position : SV_Position, out uint4 color : SV_Target)
{
    // This needs to vary per pixel enough that no compression happens.
    // We are only guaranteed to have a single 8-bit channel
    uint2 microPosition = ((uint2) (position.xy - 0.5f)) & 0xf;
    color.x =
          ((microPosition.x & 0x1) << 7)
        | ((microPosition.y & 0x1) << 6)
        | ((microPosition.x & 0x2) << 4)
        | ((microPosition.y & 0x2) << 3)
        | ((microPosition.x & 0x4) << 1)
        | ((microPosition.y & 0x4) << 2)
        | ((microPosition.x & 0x8) >> 2)
        | ((microPosition.y & 0x8) >> 3);
    color.y = ((color.x & 0x03) << 6) | ((color.x & 0xfc) >> 2);
    color.z = ((color.y & 0x03) << 6) | ((color.y & 0xfc) >> 2);
    color.w = ((color.z & 0x03) << 6) | ((color.z & 0xfc) >> 2);

    if (BYTES_PER_CHANNEL > 1)
    {
        color |= color << 8;
    }
    if (BYTES_PER_CHANNEL > 2)
    {
        color |= color << 16;
    }
}
