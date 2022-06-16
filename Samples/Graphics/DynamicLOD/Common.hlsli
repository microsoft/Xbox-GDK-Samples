//--------------------------------------------------------------------------------------
// Common.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once
#include "Shared.h"

#ifdef __XBOX_SCARLETT
#define ROOT_SIG \
    "RootFlags(XBOX_FORCE_MEMORY_BASED_ABI), \
     CBV(b0), \
     RootConstants(b1, num32BitConstants = 2), \
     SRV(t32), \
     DescriptorTable( CBV(b2, numDescriptors = 8), \
                     SRV(t0, numDescriptors = 32)), \
     UAV(u0)"
#else
#define ROOT_SIG \
    "CBV(b0), \
     RootConstants(b1, num32BitConstants = 2), \
     SRV(t32), \
     DescriptorTable( CBV(b2, numDescriptors = 8), \
                      SRV(t0, numDescriptors = 32)), \
     UAV(u0)"
#endif

ConstantBuffer<Constants> Constants : register(b0);

struct Vertex
{
    float3 Position;
    float3 Normal;
};

struct VertexOut
{
    float3 PositionVS : POSITION0;
    uint   MeshletIndex : COLOR0;
    float3 Normal : NORMAL0;
    uint   _unused : COLOR1;
    float4 Color : COLOR2;
    float4 PositionHS : SV_Position;
};


uint GetThreadsPerWave()
{
#ifdef __XBOX_SCARLETT
    return THREADS_PER_WAVE;
#else
    return WaveGetLaneCount();
#endif
}

uint PrefixCountBits(bool flag)
{
#ifdef __XBOX_SCARLETT // Remove when wave intrinsics are added to Scarlett
    return __XB_MBCNT64(__XB_Ballot64(flag));
#else
    return WavePrefixCountBits(flag);
#endif
}

uint GetWavesPerGroup(uint groupSize)
{
#ifdef __XBOX_SCARLETT // Remove when wave intrinsics are added to Scarlett
    return (groupSize + THREADS_PER_WAVE - 1) / THREADS_PER_WAVE;
#else
    return (groupSize + WaveGetLaneCount() - 1) / WaveGetLaneCount();
#endif
}

uint ReadLaneFirst(uint value)
{
#ifdef __XBOX_SCARLETT // Remove when wave intrinsics are added to Scarlett
    return __XB_MakeUniform(value);
#else
    return WaveReadLaneFirst(value);
#endif
}

uint GetWaveIndex(uint gtid)
{
#ifdef __XBOX_SCARLETT // Remove when wave intrinsics are added to Scarlett
    return __XB_GetWaveID();
#else
    return WaveReadLaneFirst(gtid / WaveGetLaneCount());
#endif
}

uint GetLaneIndex(uint gtid)
{
#ifdef __XBOX_SCARLETT // Remove when wave intrinsics are added to Scarlett
    return __XB_GetLaneID();
#else
    return gtid % WaveGetLaneCount();
#endif
}

uint ActiveCountBits(bool flag)
{
#ifdef __XBOX_SCARLETT // Remove when wave intrinsics are added to Scarlett
    return __XB_S_BCNT1_U64(__XB_Ballot64((uint)flag));
#else
    return WaveActiveCountBits(flag);
#endif
}

bool IsFirstLane()
{
#ifdef __XBOX_SCARLETT // Remove when wave intrinsics are added to Scarlett
    return __XB_GetLaneID() == 0;
#else
    return WaveIsFirstLane();
#endif
}
