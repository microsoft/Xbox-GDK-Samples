//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef WAVE_SIZE
#define WAVE_SIZE 64
#endif

#include "PrefixSumIntrinsics.hlsli"

RWByteAddressBuffer InCounterOutPrefix : register(u0);
RWByteAddressBuffer OutThreadGroupSums : register(u1);

#ifndef NUM_WAVE64_PER_GROUP
    #define NUM_WAVE64_PER_GROUP 1
#endif

#define NUM_THREADS (WAVE_SIZE * NUM_WAVE64_PER_GROUP)

#if !defined(PER_THREAD_PREFIX_WIDTH)
    #error PER_THREAD_PREFIX_WIDTH should be defined
#endif

#if (PER_THREAD_PREFIX_WIDTH == 0 || PER_THREAD_PREFIX_WIDTH > 8)
    #error PER_THREAD_PREFIX_WIDTH should be non zero and <= 8
#endif

#if ((PER_THREAD_PREFIX_WIDTH & (PER_THREAD_PREFIX_WIDTH - 1)) != 0)
    #error PER_THREAD_PREFIX_WIDTH should be power of 2
#endif

#define PrefixRootSig \
    "UAV(u0)," \
    "UAV(u1)," \
    "RootConstants(b0, num32bitconstants=1)"

cbuffer ShaderParams
{
    uint ShouldOutputTGSum;
};

#if NUM_WAVE64_PER_GROUP > 1
groupshared uint PerWaveCount[NUM_WAVE64_PER_GROUP];
#endif

[RootSignature(PrefixRootSig)]
[numthreads(NUM_THREADS, 1, 1)]
void main(uint GroupId : SV_GroupId, uint LocalId : SV_GroupIndex)
{
    const uint ThreadId = GroupId * NUM_THREADS + LocalId;
    const uint Addr = ThreadId * PER_THREAD_PREFIX_WIDTH * 4;
#if PER_THREAD_PREFIX_WIDTH >= 4
        const uint4 Counters03 = InCounterOutPrefix.Load4(Addr);
#elif PER_THREAD_PREFIX_WIDTH == 2
        const uint4 Counters03 = InCounterOutPrefix.Load2(Addr).xyxy;
#elif PER_THREAD_PREFIX_WIDTH == 1
        const uint4 Counters03 = InCounterOutPrefix.Load(Addr).xxxx;
#endif

    uint4 Prefix03 = Counters03;
    Prefix03.yw += Prefix03.xz;
    Prefix03.zw += Prefix03.yy;

#if PER_THREAD_PREFIX_WIDTH == 8
    const uint4 Counters47 = InCounterOutPrefix.Load4(Addr + 16);

    // Compute prefix sum for upper 4 uints
    uint4 Prefix47 = Counters47;
    Prefix47.yw += Prefix47.xz;
    Prefix47.zw += Prefix47.yy;

    // Add prefix sum from lower 4 uints
    Prefix47 += Prefix03.wwww;

    const uint PerThreadSum = Prefix47.w;

    // Make per thread prefix sum exclusive instead of inclusive
    Prefix47 -= Counters47;

#elif PER_THREAD_PREFIX_WIDTH == 4
    const uint PerThreadSum = Prefix03.w;
#elif PER_THREAD_PREFIX_WIDTH == 2
    const uint PerThreadSum = Prefix03.y;
#elif PER_THREAD_PREFIX_WIDTH == 1
    const uint PerThreadSum = Prefix03.x;
#endif

    // Make per thread prefix sum exclusive instead of inclusive
    Prefix03 -= Counters03;

    uint InWavePrefix = PrefixSum_Inclusive(PerThreadSum);

#if NUM_WAVE64_PER_GROUP > 1
    const uint WaveId = LocalId / WAVE_SIZE;
    if (__XB_GetLaneID() == WAVE_SIZE - 1)
        PerWaveCount[WaveId] = InWavePrefix;

    GroupMemoryBarrierWithGroupSync();

    // knowing per wave counters, compute the exclusive prefix sum of those counters
    uint InThreadGroupExclusivePrefix = 0;
    for (uint iWave = 0; iWave < WaveId; ++iWave)
    {
        InThreadGroupExclusivePrefix += PerWaveCount[iWave];
    }

    // update in-wave inclusive prefix sum by the sum of counters processed by waves with lower id
    InWavePrefix += InThreadGroupExclusivePrefix;
#endif
    uint PerThreadGroupSum = InWavePrefix;

    // Make exclusive prefix sum instead of inclusive
    InWavePrefix -= PerThreadSum;

    Prefix03 += InWavePrefix;
#if PER_THREAD_PREFIX_WIDTH >= 4
    InCounterOutPrefix.Store4(Addr, Prefix03);
#elif PER_THREAD_PREFIX_WIDTH == 2
    InCounterOutPrefix.Store2(Addr, Prefix03.xy);
#elif PER_THREAD_PREFIX_WIDTH == 1
    InCounterOutPrefix.Store(Addr, Prefix03.x);
#endif

#if PER_THREAD_PREFIX_WIDTH == 8
    Prefix47 += InWavePrefix;
    InCounterOutPrefix.Store4(Addr + 16, Prefix47);
#endif

    // Output reduction result for the next pass if necessary (the last pass in the chain doesn't do this)
    if ((ShouldOutputTGSum & 0x1) && (LocalId == NUM_THREADS - 1))
    {
        OutThreadGroupSums.Store(GroupId * 4, PerThreadGroupSum);
    }
}
