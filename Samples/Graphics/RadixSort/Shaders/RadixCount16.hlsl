//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RadixShared.hlsli"

#define NUM_THREADS (512)
#define WAVE_SIZE (64)
#define NUM_WAVES (NUM_THREADS / WAVE_SIZE)
#define NUM_SUBKEYS_PER_WAVE (NUM_SUBKEYS / NUM_WAVES)

ByteAddressBuffer InSubKeys : register(t0);
RWByteAddressBuffer OutPrefix : register(u0);

groupshared uint Counters[NUM_SUBKEYS];

[RootSignature(RootSig)]
[numthreads(NUM_THREADS, 1, 1)]
void main(uint GroupId : SV_GroupId, uint ThreadId : SV_GroupThreadId)
{
    uint Offset = ThreadId;
    uint TGBase = GroupId * KERNEL_SIZE;
    uint RawKey = InSubKeys.Load((TGBase + Offset) << 2);

    const uint InWaveThreadId = ThreadId & (WAVE_SIZE - 1);

    const uint InGroupWaveId = __XB_MakeUniform(ThreadId / WAVE_SIZE);
    const uint ThreadCounter = NUM_SUBKEYS_PER_WAVE * InGroupWaveId + (InWaveThreadId & (NUM_SUBKEYS_PER_WAVE - 1));

#define ALL_WAVES_ACCESS_LDS 0

    #if ALL_WAVES_ACCESS_LDS
    if (InWaveThreadId < NUM_SUBKEYS_PER_WAVE)
        Counters[ThreadCounter] = 0;
    #else
    if (ThreadId < NUM_SUBKEYS)
        Counters[ThreadId] = 0;
    #endif

#if 1

    uint2 Mask05, Mask6, Mask7;
    uint i;

    GroupMemoryBarrierWithGroupSync();

    #define Update8BitCounters(SubKey, InWaveId, Cnt064, Cnt128, Cnt192, Cnt256)\
        Mask05 = ~0u;                                                           \
        [unroll] for (i = 0; i < min(NUM_SUBKEY_BITS, 6); ++i)                  \
        {                                                                       \
            const uint XorMask = v_bfe_u32(InWaveId, i, 0x1u) + ~0u;            \
            Mask05 &= __XB_Ballot64(v_bfe_u32(SubKey, i, 0x1u)) ^ XorMask.xx;   \
        }                                                                       \
        Mask6 = __XB_Ballot64(v_bfe_u32(SubKey, 6, 0x1u));                      \
        Mask7 = __XB_Ballot64(v_bfe_u32(SubKey, 7, 0x1u));                      \
        InterlockedAdd(Counters[InWaveThreadId + WAVE_SIZE * 0], CountBits64(Mask05 & ~Mask6 & ~Mask7));                          \
        InterlockedAdd(Counters[InWaveThreadId + WAVE_SIZE * 1], CountBits64(Mask05 &  Mask6 & ~Mask7));                          \
        InterlockedAdd(Counters[InWaveThreadId + WAVE_SIZE * 2], CountBits64(Mask05 & ~Mask6 &  Mask7));                          \
        InterlockedAdd(Counters[InWaveThreadId + WAVE_SIZE * 3], CountBits64(Mask05 &  Mask6 &  Mask7));

    uint Sum0 = 0, Sum1 = 0, Sum2 = 0, Sum3 = 0;

    uint SubKey = v_bfe_u32(RawKey, SubKeyShift, NUM_SUBKEY_BITS);
    Offset = AdvanceOffset(Offset, NUM_THREADS);
    RawKey = InSubKeys.Load((TGBase + Offset) << 2);

    Update8BitCounters(SubKey, InWaveThreadId, Sum0, Sum1, Sum2, Sum3)

    SubKey = v_bfe_u32(RawKey, SubKeyShift, NUM_SUBKEY_BITS);
    Offset = AdvanceOffset(Offset, NUM_THREADS);
    RawKey = InSubKeys.Load((TGBase + Offset) << 2);

    Update8BitCounters(SubKey, InWaveThreadId, Sum0, Sum1, Sum2, Sum3)

    SubKey = v_bfe_u32(RawKey, SubKeyShift, NUM_SUBKEY_BITS);
    Offset = AdvanceOffset(Offset, NUM_THREADS);
    RawKey = InSubKeys.Load((TGBase + Offset) << 2);

    Update8BitCounters(SubKey, InWaveThreadId, Sum0, Sum1, Sum2, Sum3)

    SubKey = v_bfe_u32(RawKey, SubKeyShift, NUM_SUBKEY_BITS);
    Update8BitCounters(SubKey, InWaveThreadId, Sum0, Sum1, Sum2, Sum3)

#else
    GroupMemoryBarrierWithGroupSync();

    uint SubKey = v_bfe_u32(RawKey, SubKeyShift, NUM_SUBKEY_BITS);
    Offset = AdvanceOffset(Offset, NUM_THREADS);
    RawKey = InSubKeys.Load((TGBase + Offset) << 2);
    InterlockedAdd(Counters[SubKey], 1);

    SubKey = v_bfe_u32(RawKey, SubKeyShift, NUM_SUBKEY_BITS);
    Offset = AdvanceOffset(Offset, NUM_THREADS);
    RawKey = InSubKeys.Load((TGBase + Offset) << 2);
    InterlockedAdd(Counters[SubKey], 1);

    SubKey = v_bfe_u32(RawKey, SubKeyShift, NUM_SUBKEY_BITS);
    Offset = AdvanceOffset(Offset, NUM_THREADS);
    RawKey = InSubKeys.Load((TGBase + Offset) << 2);
    InterlockedAdd(Counters[SubKey], 1);

    SubKey = v_bfe_u32(RawKey, SubKeyShift, NUM_SUBKEY_BITS);
    Offset = AdvanceOffset(Offset, NUM_THREADS);
    RawKey = InSubKeys.Load((TGBase + Offset) << 2);
    InterlockedAdd(Counters[SubKey], 1);
#endif

    GroupMemoryBarrierWithGroupSync();

    #if ALL_WAVES_ACCESS_LDS
        if (InWaveThreadId < NUM_SUBKEYS_PER_WAVE)
            OutPrefix.Store((ThreadCounter * NumCountersPerSubKey + GroupId) << 2, Counters[ThreadCounter]);
    #else
        if (ThreadId < NUM_SUBKEYS)
            OutPrefix.Store((ThreadId * NumCountersPerSubKey + GroupId) << 2, Counters[ThreadId]);
    #endif
}
