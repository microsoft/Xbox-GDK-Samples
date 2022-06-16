//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.h"

#define ROOT_SIG "CBV(b0), \
                  SRV(t0)"

struct VertexOut
{
    float2 UV       : TEXCOORD0;
    float4 Color    : COLOR0;
    float4 Position : SV_Position;
};


uint PrefixCountBits(bool flag)
{
#ifdef __XBOX_SCARLETT
    return __XB_MBCNT64(__XB_Ballot64(flag));
#else
    return WavePrefixCountBits(flag);
#endif
}

uint GetWavesPerGroup(uint groupSize)
{
#ifdef __XBOX_SCARLETT
    return (groupSize + 63) / 64; // Subject to change with Wave32 - should have Wave Intrinsics on Scarlett by then.
#else
    return (groupSize + WaveGetLaneCount() - 1) / WaveGetLaneCount();
#endif
}

uint GetWaveIndex(uint gtid)
{
#ifdef __XBOX_SCARLETT
    return __XB_GetWaveID();
#else
    return WaveReadLaneFirst(gtid / WaveGetLaneCount());
#endif
}

uint ActiveCountBits(bool flag)
{
#ifdef __XBOX_SCARLETT
    return __XB_S_BCNT1_U64(__XB_Ballot64((uint)flag));
#else
    return WaveActiveCountBits(flag);
#endif
}

bool IsFirstLane()
{
#ifdef __XBOX_SCARLETT
    return __XB_GetLaneID() == 0;
#else
    return WaveIsFirstLane();
#endif
}
