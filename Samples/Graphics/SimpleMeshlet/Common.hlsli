//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.h"

#define ROOT_SIG "CBV(b0), \
                  CBV(b1), \
                  CBV(b2), \
                  SRV(t0), \
                  SRV(t1), \
                  SRV(t2), \
                  SRV(t3)"


ConstantBuffer<Constants> Globals : register(b0);

struct Vertex
{
    float3 Position;
    float3 Normal;
};

struct VertexOut
{
    float3 PositionVS : POSITION0;
    uint MeshletIndex : COLOR0;
    float3 Normal     : NORMAL0;
    uint _unused      : COLOR1;
    float4 PositionHS : SV_Position;
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

uint GetLaneIndex(uint gtid)
{
#ifdef __XBOX_SCARLETT
    return __XB_GetLaneID();
#else
    //return WaveGetLaneIndex(); // Broken
    return gtid % THREADS_PER_WAVE;
#endif
}

uint GetWaveIndex(uint gtid)
{
#ifdef __XBOX_SCARLETT
    //return __XB_GetWaveID(); // Broken
    return gtid / THREADS_PER_WAVE;
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

bool IsFirstLane(uint gtid)
{
#ifdef __XBOX_SCARLETT
    return __XB_GetLaneID() == 0;
#else
    //return WaveIsFirstLane(); // Broken - returns gtid == 0
    return (gtid % THREADS_PER_WAVE) == 0;
#endif
}
