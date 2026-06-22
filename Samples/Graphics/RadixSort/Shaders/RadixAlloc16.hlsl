//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "RadixShared.hlsli"

#define NUM_THREADS (512)

#define WAVE_SIZE (64)
#define NUM_WAVES (NUM_THREADS / WAVE_SIZE)

ByteAddressBuffer InSubKeys : register(t0);
ByteAddressBuffer PrefixL1 : register(t1);
ByteAddressBuffer PrefixL2 : register(t2);
RWByteAddressBuffer OutPrefix : register(u0);

#define NUM_64BIT_MASKS (NUM_WAVES * 4)

groupshared uint SharedBitCnt64[NUM_64BIT_MASKS];

uint LoadBitCnt64(uint BitCnt64Index)
{
    return SharedBitCnt64[BitCnt64Index];
}

uint4x2 ComputeIsBitSetAndStoreBitCnt64(uint4 Data, uint SubKeyBitIndex, uint LaneId, uint WaveId)
{
    const uint Shift = SubKeyShift + SubKeyBitIndex;
    const uint4x2 IsBitSetMask = uint4x2(
        __XB_Ballot64(v_bfe_u32(Data.x, Shift, 1u)),
        __XB_Ballot64(v_bfe_u32(Data.y, Shift, 1u)),
        __XB_Ballot64(v_bfe_u32(Data.z, Shift, 1u)),
        __XB_Ballot64(v_bfe_u32(Data.w, Shift, 1u))
    );

    const uint4 BitCnt64 = uint4(
        __XB_S_BCNT1_U64(IsBitSetMask[0]),
        __XB_S_BCNT1_U64(IsBitSetMask[1]),
        __XB_S_BCNT1_U64(IsBitSetMask[2]),
        __XB_S_BCNT1_U64(IsBitSetMask[3])
    );

    if (LaneId == 0)
    {
        SharedBitCnt64[WaveId + NUM_WAVES * 0] = BitCnt64.x;
        SharedBitCnt64[WaveId + NUM_WAVES * 1] = BitCnt64.y;
        SharedBitCnt64[WaveId + NUM_WAVES * 2] = BitCnt64.z;
        SharedBitCnt64[WaveId + NUM_WAVES * 3] = BitCnt64.w;
    }
    return IsBitSetMask;
}

uint Compute32WayPrefix(uint WavePrefix)
{
    uint
    Tmp = __XB_LaneSwizzle(WavePrefix, 0x001e);
    if (__XB_V_CNDMASK_B32(0xaaaaaaaa, uint2(0, 1))) WavePrefix += Tmp;

    Tmp = __XB_LaneSwizzle(WavePrefix, 0x003c);
    if (__XB_V_CNDMASK_B32(0xcccccccc, uint2(0, 1))) WavePrefix += Tmp;

    Tmp = __XB_LaneSwizzle(WavePrefix, 0x0078);
    if (__XB_V_CNDMASK_B32(0xf0f0f0f0, uint2(0, 1))) WavePrefix += Tmp;

    Tmp = __XB_LaneSwizzle(WavePrefix, 0x00f0);
    if (__XB_V_CNDMASK_B32(0xff00ff00, uint2(0, 1))) WavePrefix += Tmp;

    Tmp = __XB_LaneSwizzle(WavePrefix, 0x01e0);
    if (__XB_V_CNDMASK_B32(0xffff0000, uint2(0, 1))) WavePrefix += Tmp;

    return WavePrefix;
}

groupshared uint ShuffleStore[KERNEL_SIZE];

void ShuffleData(uint4x2 IsBitSetMask, uint LaneId, uint WaveId, uint4 Data)
{
    const uint NumBitsSetInWave = LoadBitCnt64(LaneId & (NUM_64BIT_MASKS - 1u));

#if NUM_64BIT_MASKS == 32
    uint CurrentWaveInclusivePrefix = Compute32WayPrefix(NumBitsSetInWave);
#else
    #error NUM_64BIT_MASKS defines prefix length, currently set to 32
#endif
    const uint NumBitsSetInTG = __XB_ReadLane(CurrentWaveInclusivePrefix, NUM_64BIT_MASKS - 1);

    const uint CurrentWaveExclusivePrefix = CurrentWaveInclusivePrefix - NumBitsSetInWave;

    const uint4 WaveIndices = uint4(
        WaveId,
        WaveId + NUM_WAVES * 1,
        WaveId + NUM_WAVES * 2,
        WaveId + NUM_WAVES * 3
    );

    uint4 ZeroBitExclusivePrefix = WAVE_SIZE * WaveIndices + (NumBitsSetInTG + LaneId);

    const uint4 OneBitExclusivePrefix = uint4(
        __XB_ReadLane(CurrentWaveExclusivePrefix, WaveIndices.x),
        __XB_ReadLane(CurrentWaveExclusivePrefix, WaveIndices.y),
        __XB_ReadLane(CurrentWaveExclusivePrefix, WaveIndices.z),
        __XB_ReadLane(CurrentWaveExclusivePrefix, WaveIndices.w)
    );

    uint4 InWaveIndex;
    InWaveIndex.x = __XB_MBCNT64(IsBitSetMask[0]);
    InWaveIndex.y = __XB_MBCNT64(IsBitSetMask[1]);
    InWaveIndex.z = __XB_MBCNT64(IsBitSetMask[2]);
    InWaveIndex.w = __XB_MBCNT64(IsBitSetMask[3]);

    uint4 OneBitLoc = OneBitExclusivePrefix + InWaveIndex;

    const uint4 DstIndex = uint4(
        __XB_V_CNDMASK_B32(IsBitSetMask[0], uint2(ZeroBitExclusivePrefix.x - OneBitLoc.x, OneBitLoc.x)),
        __XB_V_CNDMASK_B32(IsBitSetMask[1], uint2(ZeroBitExclusivePrefix.y - OneBitLoc.y, OneBitLoc.y)),
        __XB_V_CNDMASK_B32(IsBitSetMask[2], uint2(ZeroBitExclusivePrefix.z - OneBitLoc.z, OneBitLoc.z)),
        __XB_V_CNDMASK_B32(IsBitSetMask[3], uint2(ZeroBitExclusivePrefix.w - OneBitLoc.w, OneBitLoc.w))
    );
    ShuffleStore[DstIndex.x] = Data.x;
    ShuffleStore[DstIndex.y] = Data.y;
    ShuffleStore[DstIndex.z] = Data.z;
    ShuffleStore[DstIndex.w] = Data.w;
}

groupshared uint LeadingThreads[NUM_SUBKEYS];

[RootSignature(RootSig)]
[numthreads(NUM_THREADS, 1, 1)]
void main(uint GroupId : SV_GroupId, uint ThreadId : SV_GroupThreadId)
{
    const uint WaveId = __XB_MakeUniform(ThreadId / WAVE_SIZE);
    const uint LaneId = ThreadId % WAVE_SIZE;

    uint Index = (GroupId * KERNEL_SIZE + ThreadId) * 4;

    uint4 Data;
    Data.x = InSubKeys.Load(Index + NUM_THREADS * 0 * 4);
    Data.y = InSubKeys.Load(Index + NUM_THREADS * 1 * 4);
    Data.z = InSubKeys.Load(Index + NUM_THREADS * 2 * 4);
    Data.w = InSubKeys.Load(Index + NUM_THREADS * 3 * 4);

    uint4x2 IsBitSetMask;
    uint4 DstIndex;
    uint4 InWaveIndex;
#define ShufflePass(PassIndex)                                                  \
    IsBitSetMask = ComputeIsBitSetAndStoreBitCnt64(Data, PassIndex, LaneId, WaveId);\
    GroupMemoryBarrierWithGroupSync();                                          \
    \
    ShuffleData(IsBitSetMask, LaneId, WaveId, Data);                            \
    GroupMemoryBarrierWithGroupSync();                                          \
    Data.x = ShuffleStore[ThreadId + NUM_THREADS * 0];                          \
    Data.y = ShuffleStore[ThreadId + NUM_THREADS * 1];                          \
    Data.z = ShuffleStore[ThreadId + NUM_THREADS * 2];                          \
    Data.w = ShuffleStore[ThreadId + NUM_THREADS * 3];                          \

    ShufflePass(0u);
    ShufflePass(1u);
    ShufflePass(2u);
    ShufflePass(3u);
    ShufflePass(4u);
    ShufflePass(5u);
    ShufflePass(6u);
    ShufflePass(7u);

    const uint4 SubKey = uint4(
        UnpackSubKey(Data.x),
        UnpackSubKey(Data.y),
        UnpackSubKey(Data.z),
        UnpackSubKey(Data.w)
    );

    if (UnpackSubKey(ShuffleStore[ThreadId - 1]) != SubKey.x || ThreadId == 0)
        LeadingThreads[SubKey.x] = ThreadId;

    if (UnpackSubKey(ShuffleStore[ThreadId + NUM_THREADS * 1 - 1]) != SubKey.y)
        LeadingThreads[SubKey.y] = ThreadId + NUM_THREADS * 1;

    if (UnpackSubKey(ShuffleStore[ThreadId + NUM_THREADS * 2 - 1]) != SubKey.z)
        LeadingThreads[SubKey.z] = ThreadId + NUM_THREADS * 2;

    if (UnpackSubKey(ShuffleStore[ThreadId + NUM_THREADS * 3 - 1]) != SubKey.w)
        LeadingThreads[SubKey.w] = ThreadId + NUM_THREADS * 3;

    GroupMemoryBarrierWithGroupSync();

    uint4 WithinSubGroupIndex = uint4(
        ThreadId + NUM_THREADS * 0 - LeadingThreads[SubKey.x],
        ThreadId + NUM_THREADS * 1 - LeadingThreads[SubKey.y],
        ThreadId + NUM_THREADS * 2 - LeadingThreads[SubKey.z],
        ThreadId + NUM_THREADS * 3 - LeadingThreads[SubKey.w]
    );
    uint LocalOffset = 0;

    const uint4 PrefixL1Idx = NumCountersPerSubKey * SubKey + GroupId;
    const uint4 PrefixL2Idx = PrefixL1Idx >> PrefixBlockSizeLog2L1;

    const uint4 GOffsetL1 = uint4(
        PrefixL1.Load(PrefixL1Idx.x << 2),
        PrefixL1.Load(PrefixL1Idx.y << 2),
        PrefixL1.Load(PrefixL1Idx.z << 2),
        PrefixL1.Load(PrefixL1Idx.w << 2)
    );
    const uint4 GOffsetL2 = uint4(
        PrefixL2.Load(PrefixL2Idx.x << 2),
        PrefixL2.Load(PrefixL2Idx.y << 2),
        PrefixL2.Load(PrefixL2Idx.z << 2),
        PrefixL2.Load(PrefixL2Idx.w << 2)
    );

    DstIndex = GOffsetL1 + GOffsetL2 + WithinSubGroupIndex;

    OutPrefix.Store(DstIndex.x << 2, Data.x);
    OutPrefix.Store(DstIndex.y << 2, Data.y);
    OutPrefix.Store(DstIndex.z << 2, Data.z);
    OutPrefix.Store(DstIndex.w << 2, Data.w);
}
