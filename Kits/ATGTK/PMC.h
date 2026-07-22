//--------------------------------------------------------------------------------------
// PMC.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <string>

namespace ATG_PMC
{
    enum  PMCValues
    {
        //static constexpr uint32_t 0   Reserved
        RetiredSseAvx = 1,                  // The number of SSE / AVX operations retired
        DCacheAccesses = 2,                 // The number of accesses to the L1 data cache for load and store references
        DCacheMisses = 3,                   // The number of L1 data cache references which miss the data cache
        DCacheRefills = 4,                  // The number of L1 data cache refills satisfied from the L2 cache (and/or the northbridge)
        L1DTLBMissL2DTLBHit = 5,            // The number of L1 data cache accesses that miss in the L1 DTLB and hit in the L2 DTLB
        DtlbMiss = 6,                       // The number of L1 data cache accesses that miss in both the L1 and L2 DTLBs
        L1DtlbHit = 7,                      // The number of L1 data cache accesses that hit in the L1 DTLB
        L1ITLBMissL2ITLBHit = 8,            // The number of instruction fetches that miss in the L1 ITLB but hit in the L2 ITLB
        ItlbMiss = 9,                       // The number of instruction fetches that miss in the 4K ITLB and 2M ITLB
        ItlbInstructionFetchHits = 10,      // The number of instruction fetches that hit in the 4K ITLB and 2M ITLB
        MisalignedAccess = 11,              // The number of L1 data cache accesses that are misaligned. Misaligned accesses incur at least an extra cache access and an extra cycle of latency on reads
        IneffectiveSWPrefetches = 12,       // The number of software prefetches that do not cause an actual L1 data cache refill
        CpuClkNotHalted = 13,               // The number of clocks that the CPU is not in a halted state
        RetiredInstructions = 14,           // The number of instructions retired (execution completed and architectural state updated). This count includes exceptions and interrupts.
        BranchInstructions = 15,            // The number of branch instructions retired. This includes all types of architectural control flow changes, including exceptions and interrupts.
        MispredictedBranch = 16,            // The number of branch instructions retired, of any type, that were not correctly predicted in either target or direction. This includes those for which prediction is not attempted (far control transfers, exceptions and interrupts), and excludes resyncs. ,
        RetiredTakenBranch = 17,            // The number of taken branches that were retired. This includes all types of architectural control flow changes, including exceptions and interrupts, and excludes resyncs.
        RetiredFarSyscall = 18,             // The number of far syscalls retired
        RetiredNearReturns = 19,            // The number of near return instructions retired
        RetiredReturnsMispredicted = 20,    // A near return instruction was retired that mispredicted in either target or direction
        MispredictedTakenBranch = 21,       // A taken branch instruction was retired that mispredicted in target address (but not in direction)
        //static constexpr uint32_t (Deprecated) MmxFPInstructions = 22              A floating point (x87, MMX, or SSE) instruction was retired
        //static constexpr uint32_t (Deprecated - Use RetiredMmxInstructions instead) RetiredFPInstructions = 23          The number of SSE/AVX operations retired
        RetiredMmxInstructions = 24,        // The number of MMX operations retired
        //static constexpr uint32_t (Deprecated) RetiredSseInstructions = 25         The number of SSE operations retired
        InstructionFetchStalls = 26,        // The number cycles that the instruction fetch engine is stalled
        DataCachePrefetches = 27,           // L1 data cache prefetches
        DataCacheReadSize = 28,             // The number of L1 data cache reads
        DataCacheWriteSize = 29,            // The number of L1 data cache writes
        ITLBReloadStalls = 30,              // The number of cycles when the fetch engine is stalled for an ITLB reload
    };

    enum NBPMCValues
    {
        //  0   Reserved
        NBGarlicReads = 1,                  // The number of garlic bus reads
        NBGarlicWrites = 2,                 // The number of garlic bus writes
        NBGarlicReadsWrites = 3,            // The number of garlic bus reads and writes
        NBOnionReads = 4,                   // The number of onion bus reads
        NBOnionWrites = 5,                  // The number of onion bus writes
        NBOnionReadsWrites = 6,             // The number of onion bus reads and write
        NBAllReadWrites = 7,                // The number of all NB reads and writes
        NBCPUToMemory = 8,                  // The number of CPU transactions to memory
        NBIOToMemory = 9,                   // The number of IO transactions to memory
    };
    enum L2PMCValues
    {
        //  0   Reserved
        L2MissICFill = 1,                   // The number of IC requests that miss the L2
        L2MissDCFill = 2,                   // The number of DC requests that miss the L2
        L2MissPrefetchFill = 3,             // The number of prefetch requests that miss the L2
        L2MissAll = 4,                      // The total number of L2 Misses
        L2Writeback = 5,                    // The number of L2 clean and dirty writebacks
        L2CleanWriteback = 6,               // The number of L2 clean writebacks
        L2Fill = 7,                         // The number of L2 Fills
        L2FillWriteback = 8,                // The total number of L2 fills and writebacks
    };

    constexpr uint32_t c_totalPMCBufferSize = 14u;        // used for all PMC internal buffers. If this number changes code must be changed, all for loops have been unrolled, index 4,5 are not used
    extern std::wstring c_PMCNames[];
    extern std::wstring c_NBPMCNames[];
    extern std::wstring c_L2PMCNames[];

    extern const uint32_t c_PMCGetOffset[c_totalPMCBufferSize];
    extern std::wstring   *s_startPMCName[c_totalPMCBufferSize];
#if defined(_XBOX_ONE) || defined(_GAMING_XBOX)
#ifdef __clang__
#define READ_PMC_VALUES(pmcBuffer) \
    pmcBuffer[0] = __rdpmc(0);        \
    pmcBuffer[1] = __rdpmc(1);        \
    pmcBuffer[2] = __rdpmc(2);        \
    pmcBuffer[3] = __rdpmc(3);        \
    pmcBuffer[6] = __rdpmc(6);        \
    pmcBuffer[7] = __rdpmc(7);        \
    pmcBuffer[8] = __rdpmc(8);        \
    pmcBuffer[9] = __rdpmc(9);        \
    pmcBuffer[10] = __rdpmc(10);      \
    pmcBuffer[11] = __rdpmc(11);      \
    pmcBuffer[12] = __rdpmc(12);      \
    pmcBuffer[13] = __rdpmc(13);
#else
#define READ_PMC_VALUES(pmcBuffer) \
    pmcBuffer[0] = __readpmc(0);        \
    pmcBuffer[1] = __readpmc(1);        \
    pmcBuffer[2] = __readpmc(2);        \
    pmcBuffer[3] = __readpmc(3);        \
    pmcBuffer[6] = __readpmc(6);        \
    pmcBuffer[7] = __readpmc(7);        \
    pmcBuffer[8] = __readpmc(8);        \
    pmcBuffer[9] = __readpmc(9);        \
    pmcBuffer[10] = __readpmc(10);      \
    pmcBuffer[11] = __readpmc(11);      \
    pmcBuffer[12] = __readpmc(12);      \
    pmcBuffer[13] = __readpmc(13);
#endif
#else
#define READ_PMC_VALUES(pmcBuffer)
#endif
};
