//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Common.hlsli"
#include "DebugVisCommon.hlsli"

#if defined(__XBOX_ENABLE_WAVE32)
    #define TG_SIZE_H 4
#else
    #define TG_SIZE_H 8
#endif

#define TG_SIZE_W 8

RWTexture2D<uint>   InKeys : register(u1);
RWTexture2D<float3> Output : register(u2);

[RootSignature(DebugVisRootSig)]
[numthreads(TG_SIZE_W, TG_SIZE_H, 1)]
void main(uint2 Id : SV_DispatchThreadID)
{
    const uint2 TouchedOncePixelMask = __XB_Ballot64(InKeys[dispatch2dOffset + Id] == DEBUG_VIS_VERIFY_PIXEL_TOUCHED_SEED_KEY);
    if (CountBits64(TouchedOncePixelMask) != TG_SIZE_W * TG_SIZE_H)
    {
        const float3 ColorKey = float3(1.0, 0.0, 1.0);
#if 0   // Set to 1 to highlight only top-left 16x16/16x8 tile
        // hightlight top left square area
        uint2 Coord = (Id % uint2(TG_SIZE_W, TG_SIZE_H)) << 1;

        Output[Coord] = ColorKey;
        Output[Coord + uint2(1, 0)] = ColorKey;
        Output[Coord + uint2(0, 1)] = ColorKey;
        Output[Coord + uint2(1, 1)] = ColorKey;
#else
        // highlight buggy tile
        Output[dispatch2dOffset + Id] = ColorKey;
#endif
    }
}
