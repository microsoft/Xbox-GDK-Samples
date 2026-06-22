//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef ENABLE_PIXEL_ALLOC_PASS
#define ENABLE_PIXEL_ALLOC_PASS 0   /**< By default, compile pixel counting pass */
#endif

#if ENABLE_PIXEL_ALLOC_PASS
#define USE_COALESCED_COORD_STORE (NUM_PIXEL_BINS <= 32 ? 0 : 1)
#endif

#ifndef USE_32BIT_COUNTERS
#define USE_32BIT_COUNTERS 1        /**< The only supported option for now*/
#endif

#ifndef USE_IN_WAVE_COUNTING
#define USE_IN_WAVE_COUNTING 0      /**< Without in-wave counting LDS bank conflicts make shaders with low number of bins too expensive */
#endif

#if defined(THREAD_GROUP_W) || defined(THREAD_GROUP_H)
    #error THREAD_GROUP_W and THREAD_GROUP_H shouldn't be explicitly defined unless you know what you do (comment out this error then)
#endif

#ifndef THREAD_GROUP_W
#define THREAD_GROUP_W 32
#endif

#ifndef THREAD_GROUP_H
#define THREAD_GROUP_H (NUM_PIXEL_BINS <= 1024 ? 16 : 32)
#endif

#ifdef WAVE_SIZE
    #error WAVE_SIZE shouldn't be defined
#endif

#if defined(__XBOX_ENABLE_WAVE32)
    #define WAVE_SIZE (32)
#else
    #define WAVE_SIZE (64)
#endif

#include "Common.hlsli"

#if ENABLE_PIXEL_ALLOC_PASS
    #include "PrefixSumIntrinsics.hlsli"
#endif

#if DEBUG_VIS_ENABLE_BIN_DIVERGENCE
    #define RootSig \
        "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
        "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
        "RootConstants(b0, num32bitconstants=9),"                                      \
        "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT)"

    Texture2D<uint>     BinIds : register(t0);
    RWTexture2D<float3> Output : register(u0);
#else
    #define RootSig \
        "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
        "UAV(u0),"                                                                      \
        "UAV(u1),"                                                                      \
        "UAV(u2),"                                                                      \
        "RootConstants(b1, num32bitconstants=1),"                                       \
        "RootConstants(b0, num32bitconstants=10),"                                      \
        "StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT)"

    Texture2D<uint> BinIds : register(t0);
    RWByteAddressBuffer Counters : register(u0);
    RWByteAddressBuffer Prefix512 : register(u1);
    RWByteAddressBuffer Coordinates : register(u2);
#endif

cbuffer ShaderParams
{
    uint resW;
    uint resH;

    uint BinIdBitWidth;                 /**< log2 of the number of bins */

    uint NumMicroTilesPerMacroTileXLog2;/**< Used to determine X coordiante of a macro tile covering current kernel/pixel within image */
    uint NumMicroTilesPerMacroTileYLog2;/**< Used to determine Y coordiante of a macro tile covering current kernel/pixel within image */
    uint NumMacroTilesPerRow;           /**< Used to compute linear tile index within image */
    uint NumMacroTilesPerImage;         /**< Used to compute location of a counter of any BinId from any macro tile within global buffer */

    uint NumBitsPerL1;                  /**< bits [0:3] are used */
    uint NumBitsPerL2;                  /**< bits [0:3] are used */

    uint packingType;                   /**< bits [0:0] are used */
};

#define NUM_THREADS (THREAD_GROUP_W * THREAD_GROUP_H)

#if ((NUM_THREADS & (NUM_THREADS - 1)) != 0) || (NUM_THREADS < WAVE_SIZE)
    #error NUM_THREADS have to be a power of 2 and at least equal to WAVE_SIZE
#endif

#define NUM_WAVES (NUM_THREADS / WAVE_SIZE)

static const uint kLastWaveId = (NUM_WAVES - 1);

static const uint kWaveSizeMinusOne = WAVE_SIZE - 1;

//             8x8  16x16  32x32
// 1024 uints:  16      4      1
// 2048 uints:  32      8      2
#if USE_32BIT_COUNTERS
    #define NUM_UINT_SLOTS (NUM_PIXEL_BINS)
#else
    #define NUM_UINT_SLOTS (NUM_PIXEL_BINS / 2)
#endif

#define NUM_UINT_SLOTS_PER_THREAD ((NUM_UINT_SLOTS + NUM_THREADS - 1) / NUM_THREADS)

#if ((NUM_UINT_SLOTS_PER_THREAD & (NUM_UINT_SLOTS_PER_THREAD - 1)) != 0)
    #error NON_POW_2
#endif

#if USE_32BIT_COUNTERS
    uint GSCounterAddr(uint InCounterSlot)
    {
        return (InCounterSlot / NUM_UINT_SLOTS_PER_THREAD) + (InCounterSlot & (NUM_UINT_SLOTS_PER_THREAD - 1)) * NUM_THREADS;
    }
    uint GSCounterIncMask(uint InCounterSlot, uint InAmount)
    {
        return InAmount;
    }
#else
    uint GSCounterAddr(uint InCounterSlot)
    {
        return InCounterSlot >> 1;
    }
    uint GSCounterIncMask(uint InCounterSlot, uint InAmount)
    {
        /* For counters with odd id, bit location within single "uint" is 16, otherwise it's 0 */
        const uint CounterBitLocation = (InCounterSlot & 0x1) << 4;
        /* For counters with odd id, increment mask within single "uint" is 0x00010000, otherwise it's 0x00000001*/
        return InAmount << CounterBitLocation;
    }
#endif

#define LoadGroupSharedCounter(InGroupSharedCounters, InAddr) \
    InGroupSharedCounters[InAddr]

#define CondIncrementGroupSharedCounter(InCondition, InGroupSharedCounters, InSlotIdx, InAmount) \
    if (InCondition) InterlockedAdd(InGroupSharedCounters[GSCounterAddr(InSlotIdx)], GSCounterIncMask(InSlotIdx, InAmount));

#define CondIncrementGroupSharedCounterRet(InCondition, InGroupSharedCounters, InSlotIdx, InAmount, OutRetValue) \
    if (InCondition) InterlockedAdd(InGroupSharedCounters[GSCounterAddr(InSlotIdx)], GSCounterIncMask(InSlotIdx, InAmount), OutRetValue);

#define CondIncrementGroupSharedCounter4(InCondition, InGroupSharedCounters, InSlotIdx, InAmount) \
    CondIncrementGroupSharedCounter(InCondition.x, InGroupSharedCounters, InSlotIdx.x, InAmount.x) \
    CondIncrementGroupSharedCounter(InCondition.y, InGroupSharedCounters, InSlotIdx.y, InAmount.y) \
    CondIncrementGroupSharedCounter(InCondition.z, InGroupSharedCounters, InSlotIdx.z, InAmount.z) \
    CondIncrementGroupSharedCounter(InCondition.w, InGroupSharedCounters, InSlotIdx.w, InAmount.w)

#define CondIncrementGroupSharedCounterRet4(InCondition, InGroupSharedCounters, InSlotIdx, InAmount, OutRetValue) \
    CondIncrementGroupSharedCounterRet(InCondition.x, InGroupSharedCounters, InSlotIdx.x, InAmount.x, OutRetValue.x) \
    CondIncrementGroupSharedCounterRet(InCondition.y, InGroupSharedCounters, InSlotIdx.y, InAmount.y, OutRetValue.y) \
    CondIncrementGroupSharedCounterRet(InCondition.z, InGroupSharedCounters, InSlotIdx.z, InAmount.z, OutRetValue.z) \
    CondIncrementGroupSharedCounterRet(InCondition.w, InGroupSharedCounters, InSlotIdx.w, InAmount.w, OutRetValue.w)


#define ForUpTo1WorkItems(Tid)\
    WorkItemOp(Tid, 0)

#if NUM_UINT_SLOTS_PER_THREAD > 1
    #define ForUpTo2WorkItems(TId) WorkItemOp(TId, 1)
#else
    #define ForUpTo2WorkItems(TId)
#endif

#if NUM_UINT_SLOTS_PER_THREAD > 2
    #define ForUpTo4WorkItems(TId) WorkItemOp(TId, 2) WorkItemOp(TId, 3)
#else
    #define ForUpTo4WorkItems(TId)
#endif

#if NUM_UINT_SLOTS_PER_THREAD > 4
    #define ForUpTo8WorkItems(TId) WorkItemOp(TId, 4) WorkItemOp(TId, 5) WorkItemOp(TId, 6) WorkItemOp(TId, 7)
#else
    #define ForUpTo8WorkItems(TId)
#endif

#if (NUM_UINT_SLOTS_PER_THREAD > 8) || (NUM_UINT_SLOTS_PER_THREAD & (NUM_UINT_SLOTS_PER_THREAD - 1)) != 0
    #error NUM_UINT_SLOTS_PER_THREAD have to be a power of 2, but <= 8
#endif

#define ForAllWorkItems(TId)\
    ForUpTo1WorkItems(TId)  \
    ForUpTo2WorkItems(TId)  \
    ForUpTo4WorkItems(TId)  \
    ForUpTo8WorkItems(TId)


groupshared uint GSPerBinFragCount[NUM_THREADS > NUM_UINT_SLOTS ? NUM_THREADS : NUM_UINT_SLOTS];

#if ENABLE_PIXEL_ALLOC_PASS
groupshared uint TmpCoordStore[NUM_THREADS * 4];
groupshared uint GSActiveFragSlots;
#endif

SamplerState mySampler : s0;

uint FindFirstBitLo64(uint2 Mask)
{
    return (Mask.x != 0) ? firstbitlow(Mask.x) : firstbitlow(Mask.y) + 32;
}

uint4 PixelPackedCoordFromQuadCoord(uint2 QuadTLCoord)
{
    uint4 PackedCoords;
    PackedCoords.x = PackU32ToU16(QuadTLCoord);
    PackedCoords.y = PackedCoords.x + 0x00000001;
    PackedCoords.z = PackedCoords.x + 0x00010000;
    PackedCoords.w = PackedCoords.x + 0x00010001;
    return PackedCoords;
}

[RootSignature(RootSig)]
[numthreads(THREAD_GROUP_W, THREAD_GROUP_H, 1)]
void main( uint2 ThreadGlobalId : SV_DispatchThreadID, uint2 GroupId : SV_GroupID, uint ThreadId : SV_GroupIndex )
{
    // Make available wave id within threadgroup
    const uint WaveId = __XB_MakeUniform(ThreadId / WAVE_SIZE);

    const uint2 QuadTLCoord = ThreadGlobalId << 1;
#if 0
    uint4 BinId;
    // Keep pixels of the quad in morton order
    BinId.x = BinIds[QuadTLCoord + uint2(0, 0)];
    BinId.y = BinIds[QuadTLCoord + uint2(1, 0)];
    BinId.z = BinIds[QuadTLCoord + uint2(0, 1)];
    BinId.w = BinIds[QuadTLCoord + uint2(1, 1)];
#else
    uint4 BinId = BinIds.GatherRed(mySampler, float2(QuadTLCoord + 0.5) / float2(3840.0, 2160.0)).wzxy;
#endif

    BinId &= (1U << BinIdBitWidth) - 1;

    const bool4 ValidCoords = bool4(
        QuadTLCoord.x < resW,
        QuadTLCoord.y < resH,
        QuadTLCoord.x < resW - 1,
        QuadTLCoord.y < resH - 1
    );

    const bool4 ValidPixels = bool4(
        all(ValidCoords.xy),
        all(ValidCoords.zy),
        all(ValidCoords.xw),
        all(ValidCoords.zw)
    );

    const uint MacroTileCoordX = GroupId.x >> NumMicroTilesPerMacroTileXLog2;
    const uint MacroTileCoordY = GroupId.y >> NumMicroTilesPerMacroTileYLog2;

    const uint ImageRelativeTileOffset = NumMacroTilesPerRow * MacroTileCoordY + MacroTileCoordX;

    #define WorkItemOp(TId, WorkItemIndex) \
        GSPerBinFragCount[TId + NUM_THREADS * WorkItemIndex] = 0;

    ForAllWorkItems(ThreadId)
    #undef WorkItemOp

#if USE_IN_WAVE_COUNTING != 0
    uint2 SubGroupMask0 = __XB_Ballot64(ValidPixels.x);
    uint2 SubGroupMask1 = __XB_Ballot64(ValidPixels.y);
    uint2 SubGroupMask2 = __XB_Ballot64(ValidPixels.z);
    uint2 SubGroupMask3 = __XB_Ballot64(ValidPixels.w);

    #if USE_IN_WAVE_COUNTING == 1
        // Experimental version #1 (SALU bound with high # of ids).
        // PERFORMANCE: Very cheap for low number of ids, but becomes more expensive if # of ids >= 32 (4 loops are likely to have maximal number of iterations)
        // OUTPUT: 'InOutLaneMask' the mask of active lanes with same InElem.
        uint2 LaneGroupMask = 0;
        #define CountLaneGroups(InOutLaneMask, InElem)                                                      \
            LaneGroupMask = 0;                                                                              \
            for ( ;; )                                                                                      \
            {                                                                                               \
                const bool IsSameSubGroup = __XB_MakeUniform(InElem) == InElem;                             \
                LaneGroupMask = __XB_Ballot64(IsSameSubGroup);                                              \
                if (IsSameSubGroup) break;                                                                  \
            }   \
            InOutLaneMask = LaneGroupMask;

    #elif USE_IN_WAVE_COUNTING == 2
        // Experimental version #2 (VALU bound).
        // PERFORMANCE: Stable performance for # of ids up to 1024, then  25% performance drop when nned to support 2048 ids (32 bit counter)
        // OUTPUT: 'InOutLaneMask' the mask of active lanes with same InElem.
        uint i;
        const uint FirstInvalidBitIndex = BinIdBitWidth + 1;
        #define CountLaneGroups(InOutLaneMask, InElem)                                  \
            for (i = 0; i < FirstInvalidBitIndex; ++i)                                  \
            {                                                                           \
                const bool NthBitSet = (InElem & (1U << i)) != 0;                        \
                const uint2 LanesWithNthBitSet = __XB_Ballot64(NthBitSet);              \
                InOutLaneMask &= NthBitSet ? LanesWithNthBitSet : ~LanesWithNthBitSet;  \
            }
    #else
        #error USE_IN_WAVE_COUNTING have to be 0, 1, 2
    #endif

    CountLaneGroups(SubGroupMask0, BinId.x);
    CountLaneGroups(SubGroupMask1, BinId.y);
    CountLaneGroups(SubGroupMask2, BinId.z);
    CountLaneGroups(SubGroupMask3, BinId.w);
    #undef CountLaneGroups

    // Compute subgroup-relative lane index
    const uint4 SubGroupLaneId = uint4(
        __XB_MBCNT64(SubGroupMask0),
        __XB_MBCNT64(SubGroupMask1),
        __XB_MBCNT64(SubGroupMask2),
        __XB_MBCNT64(SubGroupMask3)
    );
    // Only the first lane in each subgroup should update counters in groupshared memory
    const bool4 NeedsUpdatePerBinCount = and(bool4(SubGroupLaneId == 0), ValidPixels);

    const uint4 InWaveCounters = uint4(
        CountBits64(SubGroupMask0),
        CountBits64(SubGroupMask1),
        CountBits64(SubGroupMask2),
        CountBits64(SubGroupMask3)
    );

#else
    // Base version. No in-wave counting, each thread increments LDS counters if pixel coordiantes are valid
    // PERFORMANCE: Moderately expensive for very low # of bins < 32 due to high rate of LDS bank conflicts,
    // but performance gets better for # of bins >= 32 and further decreases with #of bins increased
    const bool4 NeedsUpdatePerBinCount = ValidPixels;

    const uint4 InWaveCounters = 1;
#endif

    // Barrier to make sure GSPerBinFragCount is initialized to ZERO
    GroupMemoryBarrierWithGroupSync();

    // Conditionally increment groupshared counters to determine the number of pixels in every bin
    // NOTE: During the allocation pass, return value holds the index of the pixel within its bin
    #if ENABLE_PIXEL_ALLOC_PASS
        uint4 PixelOffsetWithinBin = ~0u;
        CondIncrementGroupSharedCounterRet4(NeedsUpdatePerBinCount, GSPerBinFragCount, BinId, InWaveCounters, PixelOffsetWithinBin);
        #if !USE_COALESCED_COORD_STORE && USE_IN_WAVE_COUNTING != 0
            const uint SharedLoc0 = WaveId * WAVE_SIZE + FindFirstBitLo64(SubGroupMask0);
            const uint SharedLoc1 = WaveId * WAVE_SIZE + FindFirstBitLo64(SubGroupMask1);
            const uint SharedLoc2 = WaveId * WAVE_SIZE + FindFirstBitLo64(SubGroupMask2);
            const uint SharedLoc3 = WaveId * WAVE_SIZE + FindFirstBitLo64(SubGroupMask3);
            if (NeedsUpdatePerBinCount.x) TmpCoordStore[SharedLoc0 + NUM_THREADS * 0] = PixelOffsetWithinBin.x;
            if (NeedsUpdatePerBinCount.y) TmpCoordStore[SharedLoc1 + NUM_THREADS * 1] = PixelOffsetWithinBin.y;
            if (NeedsUpdatePerBinCount.z) TmpCoordStore[SharedLoc2 + NUM_THREADS * 2] = PixelOffsetWithinBin.z;
            if (NeedsUpdatePerBinCount.w) TmpCoordStore[SharedLoc3 + NUM_THREADS * 3] = PixelOffsetWithinBin.w;
        #endif
    #else
        CondIncrementGroupSharedCounter4(NeedsUpdatePerBinCount, GSPerBinFragCount, BinId, InWaveCounters);
    #endif

    // Barrier to make sure all waves finished updating LDS counters
    GroupMemoryBarrierWithGroupSync();

    #if !USE_COALESCED_COORD_STORE && ENABLE_PIXEL_ALLOC_PASS && USE_IN_WAVE_COUNTING != 0
        if (ValidPixels.x) PixelOffsetWithinBin.x = SubGroupLaneId.x + TmpCoordStore[SharedLoc0 + NUM_THREADS * 0];
        if (ValidPixels.y) PixelOffsetWithinBin.y = SubGroupLaneId.y + TmpCoordStore[SharedLoc1 + NUM_THREADS * 1];
        if (ValidPixels.z) PixelOffsetWithinBin.z = SubGroupLaneId.z + TmpCoordStore[SharedLoc2 + NUM_THREADS * 2];
        if (ValidPixels.w) PixelOffsetWithinBin.w = SubGroupLaneId.w + TmpCoordStore[SharedLoc3 + NUM_THREADS * 3];
        GroupMemoryBarrierWithGroupSync();
    #endif

    // Compute in-thread inclusive prefix sum of counters processed by a thread
    // It's needed only when ENABLE_PIXEL_ALLOC_PASS && USE_COALESCED_COORD_STORE enabled
    uint InThreadBinCntIncPrefix = 0;

    #define WorkItemOp(TId, WorkItemIndex)                                  \
        const uint PerGroupNumPixelsInBin##WorkItemIndex =                  \
            GSPerBinFragCount[TId + NUM_THREADS * WorkItemIndex];           \
        InThreadBinCntIncPrefix += PerGroupNumPixelsInBin##WorkItemIndex;   \

    ForAllWorkItems(ThreadId)
    #undef WorkItemOp

    #if ENABLE_PIXEL_ALLOC_PASS && USE_COALESCED_COORD_STORE

        // Compute in-wave inclusive prefix sum of per-thread sums of counters
        // Each lane will contain how many counters
        const uint InWaveThreadSumIncPrefix = PrefixSum_Inclusive(InThreadBinCntIncPrefix);

        // The last lane in a wave output the sum of counters across the entire wave to the groupshared memory slot
        // corresponding to its wave id. This sum is used later to compute prefix sum within the entire thread group
        const uint InWaveSum = __XB_ReadLane(InWaveThreadSumIncPrefix, kWaveSizeMinusOne);
        if (__XB_GetLaneID() == kWaveSizeMinusOne)
        {
            TmpCoordStore[WaveId] = InWaveSum;
        }
    #endif

#if !DEBUG_VIS_ENABLE_BIN_DIVERGENCE

    uint Base = 0;
    uint Stride = 0;
    if (packingType)
    {
        Base = ((ImageRelativeTileOffset << BinIdBitWidth) + NUM_UINT_SLOTS_PER_THREAD * ThreadId) << 2;
        Stride = 1U << 2U;
    }
    else
    {
        Base = (NUM_UINT_SLOTS_PER_THREAD * ThreadId * NumMacroTilesPerImage + ImageRelativeTileOffset) << 2;
        Stride = (NumMacroTilesPerImage) << 2;
    }

    #if ENABLE_PIXEL_ALLOC_PASS
        #define WorkItemOp(TId, WorkItemIndex)          \
            uint GlobalBinOffset##WorkItemIndex = 0;    \
            if (PerGroupNumPixelsInBin##WorkItemIndex)  \
            {                                           \
                Counters.InterlockedAdd(Base, PerGroupNumPixelsInBin##WorkItemIndex, GlobalBinOffset##WorkItemIndex);        \
                if (NumBitsPerL2 > 0) GlobalBinOffset##WorkItemIndex += Prefix512.Load((Base >> (2 + NumBitsPerL1)) << 2);  \
            }\
            Base += Stride;
    #else
        #define WorkItemOp(TId, WorkItemIndex)          \
            if (PerGroupNumPixelsInBin##WorkItemIndex)  \
            {                                           \
                Counters.InterlockedAdd(Base, PerGroupNumPixelsInBin##WorkItemIndex); \
            }\
            Base += Stride;
    #endif
    ForAllWorkItems(ThreadId)
    #undef WorkItemOp

#if ENABLE_PIXEL_ALLOC_PASS
    #if USE_COALESCED_COORD_STORE

        // wait for all per wave sums
        GroupMemoryBarrierWithGroupSync();

        // Compute in-group exclusive prefix sum of per-wave sums of counters
        uint InGroupWaveSumExcPrefix = 0;

        #if 1 // setting to 0 enables per thread linear accumulation
            const uint LaneId = __XB_GetLaneID();
            const uint Data = LaneId <= WaveId ? TmpCoordStore[LaneId] : 0;
            {
                uint WavePrefix = Data;

            #if NUM_WAVES == 1
                #error Unhandled
            #endif
                uint Tmp = __XB_LaneSwizzle(WavePrefix, 0x001e);

                // Explicitly activate every odd lane
                if (__XB_V_CNDMASK_B32(0xaaaaaaaa, uint2(0, 1))) WavePrefix += Tmp;
            #if NUM_WAVES > 2
                Tmp = __XB_LaneSwizzle(WavePrefix, 0x003c);

                // Explicitly activate every odd group of 2 lanes
                if (__XB_V_CNDMASK_B32(0xcccccccc, uint2(0, 1))) WavePrefix += Tmp;
            #endif

            #if NUM_WAVES > 4
                Tmp = __XB_LaneSwizzle(WavePrefix, 0x0078);

                // Explicitly activate every odd group of 4 lanes
                if (__XB_V_CNDMASK_B32(0xf0f0f0f0, uint2(0, 1))) WavePrefix += Tmp;
            #endif

            #if NUM_WAVES > 8
                Tmp = __XB_LaneSwizzle(WavePrefix, 0x00f0);

                // Explicitly activate every odd group of 8 lanes
                if (__XB_V_CNDMASK_B32(0xff00ff00, uint2(0, 1))) WavePrefix += Tmp;
            #endif

            #if NUM_WAVES > 16
                #error Local prefix sum doesn't support more than 16 waves
            #endif
                InGroupWaveSumExcPrefix = __XB_ReadLane(WavePrefix, WaveId) - InWaveSum;
            }
        #else
            for (uint WaveIndex = 0; WaveIndex < WaveId; ++WaveIndex)
            {
                InGroupWaveSumExcPrefix += TmpCoordStore[WaveIndex];
            }
        #endif
        const uint InGroupThreadSumIncPrefix = InGroupWaveSumExcPrefix + InWaveThreadSumIncPrefix;
        const uint InGroupThreadSumExcPrefix = InGroupThreadSumIncPrefix - InThreadBinCntIncPrefix;

        InThreadBinCntIncPrefix = 0;

        #define WorkItemOp(TId, WorkItemIndex)                               \
            GSPerBinFragCount[TId + NUM_THREADS * WorkItemIndex] =           \
                InGroupThreadSumExcPrefix + InThreadBinCntIncPrefix;         \
            InThreadBinCntIncPrefix += PerGroupNumPixelsInBin##WorkItemIndex;\

        ForAllWorkItems(ThreadId)
        #undef WorkItemOp

        if (WaveId == kLastWaveId && __XB_GetLaneID() == kWaveSizeMinusOne)
        {
            GSActiveFragSlots = InGroupThreadSumIncPrefix;
        }

        // wait for exclusive prefixes
        GroupMemoryBarrierWithGroupSync();

        // At this point GSPerBinFragCount contains an exclusive prefix sum of the number of pixels assigned to each bin
        // For each pixel processed by the current thread, the offset of its bin is retrieved from LDS and stored to VGPR
        uint4 BinOffsetWithinGroup = 0;
        if (ValidPixels.x) BinOffsetWithinGroup.x = GSPerBinFragCount[GSCounterAddr(BinId.x)];
        if (ValidPixels.y) BinOffsetWithinGroup.y = GSPerBinFragCount[GSCounterAddr(BinId.y)];
        if (ValidPixels.z) BinOffsetWithinGroup.z = GSPerBinFragCount[GSCounterAddr(BinId.z)];
        if (ValidPixels.w) BinOffsetWithinGroup.w = GSPerBinFragCount[GSCounterAddr(BinId.w)];

        #if USE_IN_WAVE_COUNTING != 0
            #define PropagateWaveOffset(InElem, InOutWaveOffset)                    \
                for (;;)                                                            \
                {                                                                   \
                                                                                    \
                    const bool IsSameSubGroup = __XB_MakeUniform(InElem) == InElem; \
                    InOutWaveOffset = __XB_MakeUniform(InOutWaveOffset);            \
                    if (IsSameSubGroup) break;                                      \
                }
            PropagateWaveOffset(BinId.x, PixelOffsetWithinBin.x)
            PropagateWaveOffset(BinId.y, PixelOffsetWithinBin.y)
            PropagateWaveOffset(BinId.z, PixelOffsetWithinBin.z)
            PropagateWaveOffset(BinId.w, PixelOffsetWithinBin.w)
            #undef PropagateInWaveGroupOffset

            if (ValidPixels) PixelOffsetWithinBin += SubGroupLaneId;
        #endif

        // Combine offsets of 4 pixels within their bins and offsets of their bins within a threadgroup
        // Resulting offsets determines positions of pixels within a threadgroup
        const uint4 PixelOffsetWithinGroup = PixelOffsetWithinBin + BinOffsetWithinGroup;

        uint4 PackedCoords = PixelPackedCoordFromQuadCoord(QuadTLCoord);

        #define GroupMemoryStore4(InGroupMemory, InSlots, InData)   \
            InGroupMemory[InSlots.x] = InData.x;                    \
            InGroupMemory[InSlots.y] = InData.y;                    \
            InGroupMemory[InSlots.z] = InData.z;                    \
            InGroupMemory[InSlots.w] = InData.w;


        // NOTE: PixelOffsetWithinGroup can be outside of LDS for invalid pixels.
        // Slots which are not written contain ~0u -- which means they aren't touched.
        GroupMemoryStore4(TmpCoordStore, PixelOffsetWithinGroup, PackedCoords);

        // wait for GSPerBinFragCount reads and TmpCoordStore writes
        GroupMemoryBarrierWithGroupSync();

        #define WorkItemOp(TId, WorkItemIndex) \
            GSPerBinFragCount[TId + NUM_THREADS * WorkItemIndex] = GlobalBinOffset##WorkItemIndex;

        ForAllWorkItems(ThreadId)
        #undef WorkItemOp

        // Load coordinates back to VGPRs to free shared memory
        PackedCoords.x = TmpCoordStore[ThreadId];
        PackedCoords.y = TmpCoordStore[ThreadId + NUM_THREADS];
        PackedCoords.z = TmpCoordStore[ThreadId + NUM_THREADS * 2];
        PackedCoords.w = TmpCoordStore[ThreadId + NUM_THREADS * 3];

        // wait for GSPerBinFragCount writes and TmpCoords reads
        GroupMemoryBarrierWithGroupSync();

        uint4 BinGlobalOffset;
        BinGlobalOffset.x = GSPerBinFragCount[GSCounterAddr(BinId.x)];
        BinGlobalOffset.y = GSPerBinFragCount[GSCounterAddr(BinId.y)];
        BinGlobalOffset.z = GSPerBinFragCount[GSCounterAddr(BinId.z)];
        BinGlobalOffset.w = GSPerBinFragCount[GSCounterAddr(BinId.w)];

        uint4 PixelGlobalOffset = PixelOffsetWithinBin + BinGlobalOffset;

        GroupMemoryStore4(TmpCoordStore, PixelOffsetWithinGroup, PixelGlobalOffset);

        // wait for TmpCoordStore writes
        GroupMemoryBarrierWithGroupSync();

        PixelGlobalOffset.x = TmpCoordStore[ThreadId];
        PixelGlobalOffset.y = TmpCoordStore[ThreadId + NUM_THREADS];
        PixelGlobalOffset.z = TmpCoordStore[ThreadId + NUM_THREADS * 2];
        PixelGlobalOffset.w = TmpCoordStore[ThreadId + NUM_THREADS * 3];

        if (ThreadId + NUM_THREADS * 0 < GSActiveFragSlots) Coordinates.Store(PixelGlobalOffset.x << 2, PackedCoords.x);
        if (ThreadId + NUM_THREADS * 1 < GSActiveFragSlots) Coordinates.Store(PixelGlobalOffset.y << 2, PackedCoords.y);
        if (ThreadId + NUM_THREADS * 2 < GSActiveFragSlots) Coordinates.Store(PixelGlobalOffset.z << 2, PackedCoords.z);
        if (ThreadId + NUM_THREADS * 3 < GSActiveFragSlots) Coordinates.Store(PixelGlobalOffset.w << 2, PackedCoords.w);

    #else

        #define WorkItemOp(TId, WorkItemIndex) \
            GSPerBinFragCount[TId + NUM_THREADS * WorkItemIndex] = GlobalBinOffset##WorkItemIndex;

        ForAllWorkItems(ThreadId)
        #undef WorkItemOp

        GroupMemoryBarrierWithGroupSync();

        uint4 BinGlobalOffset;
        BinGlobalOffset.x = GSPerBinFragCount[GSCounterAddr(BinId.x)];
        BinGlobalOffset.y = GSPerBinFragCount[GSCounterAddr(BinId.y)];
        BinGlobalOffset.z = GSPerBinFragCount[GSCounterAddr(BinId.z)];
        BinGlobalOffset.w = GSPerBinFragCount[GSCounterAddr(BinId.w)];

        uint4 PackedCoords = PixelPackedCoordFromQuadCoord(QuadTLCoord);

        const uint4 PixelGlobalOffset = PixelOffsetWithinBin + BinGlobalOffset;
        if (ValidPixels.x)
        {
            Coordinates.Store(PixelGlobalOffset.x << 2, PackedCoords.x);
        }
        if (ValidPixels.y)
        {
            Coordinates.Store(PixelGlobalOffset.y << 2, PackedCoords.y);
        }
        if (ValidPixels.z)
        {
            Coordinates.Store(PixelGlobalOffset.z << 2, PackedCoords.z);
        }
        if (ValidPixels.w)
        {
            Coordinates.Store(PixelGlobalOffset.w << 2, PackedCoords.w);
        }

    #endif

#endif

#else // DEBUG_VIS_ENABLE_BIN_DIVERGENCE

    uint NumNonZeroCountersPerWave = 0;
    #define WorkItemOp(TId, WorkItemIndex) \
        NumNonZeroCountersPerWave += __XB_S_BCNT1_U64(__XB_Ballot64(LoadGroupSharedCounter(GSPerBinFragCount, TId + NUM_THREADS * WorkItemIndex) != 0));
    ForAllWorkItems(ThreadId)
    #undef WorkItemOp

    GroupMemoryBarrierWithGroupSync();

    // DEBUG: Color each micro depending on divergency rate --
    // i.e., the number of total pixels in micro tile divided by the number of non-zero counters
    if ((ThreadId & kWaveSizeMinusOne) == 0)
        GSPerBinFragCount[WaveId] = 0;

    GroupMemoryBarrierWithGroupSync();

    if ((ThreadId & kWaveSizeMinusOne) == 0)
        InterlockedAdd(GSPerBinFragCount[WaveId], NumNonZeroCountersPerWave);

    GroupMemoryBarrierWithGroupSync();

    uint NumNonZeroCounters = 0;
    for (uint iWave = 0; iWave <= kLastWaveId; ++iWave)
    {
        NumNonZeroCounters += GSPerBinFragCount[iWave];
    }

    float DivergencyRate = float(NumNonZeroCounters) / float(min(1U << BinIdBitWidth, NUM_THREADS * NUM_UINT_SLOTS_PER_THREAD));
    float x = DivergencyRate;
    x = clamp (x, 0.0, 1.0);
    float r = -0.121 + 0.893 * x + 0.276 * sin (1.94 - 5.69 * x);
    float g = 0.07 + 0.947 * x;
    float b = 0.107 + (1.5 - 1.22 * x) * x;
    float3 Color = float3(r, g, b) * 0.5;

    Color *= ((MacroTileCoordX + MacroTileCoordY) & 0x1) ? 0.5 : 1.0;

    float3 PrevColor0 = Output[ThreadGlobalId << 1];
    float3 PrevColor1 = Output[(ThreadGlobalId << 1) + uint2(0, 1)];
    float3 PrevColor2 = Output[(ThreadGlobalId << 1) + uint2(1, 1)];
    float3 PrevColor3 = Output[(ThreadGlobalId << 1) + uint2(1, 0)];

    float Alpha = 1.0;

    Output[ThreadGlobalId << 1] = lerp(PrevColor0, Color, Alpha);
    Output[(ThreadGlobalId << 1) + uint2(0, 1)] = lerp(PrevColor1, Color, Alpha);
    Output[(ThreadGlobalId << 1) + uint2(1, 1)] = lerp(PrevColor2, Color, Alpha);
    Output[(ThreadGlobalId << 1) + uint2(1, 0)] = lerp(PrevColor3, Color, Alpha);
#endif
}
