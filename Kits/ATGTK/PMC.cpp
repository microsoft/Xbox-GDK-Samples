//--------------------------------------------------------------------------------------
// PMC.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PMC.h"

namespace ATG_PMC
{
    std::wstring c_PMCNames[] = {
        L"Reserved",
        L"RetiredSseAvx",
        L"DCacheAccesses",
        L"DCacheMisses",
        L"DCacheRefills",
        L"L1DTLBMissL2DTLBHit",
        L"DtlbMiss",
        L"L1DtlbHit",
        L"L1ITLBMissL2ITLBHit",
        L"ItlbMiss",
        L"ItlbInstructionFetchHits",
        L"MisalignedAccess",
        L"IneffectiveSWPrefetches",
        L"CpuClkNotHalted",
        L"RetiredInstructions",
        L"BranchInstructions",
        L"MispredictedBranch",
        L"RetiredTakenBranch",
        L"RetiredFarSyscall",
        L"RetiredNearReturns",
        L"RetiredReturnsMispredicted",
        L"MispredictedTakenBranch",
        L"(Deprecated) MmxFPInstructions",
        L"(Deprecated - Use RetiredMmxInstructions instead) RetiredFPInstructions",
        L"RetiredMmxInstructions",
        L"(Deprecated) RetiredSseInstructions",
        L"InstructionFetchStalls",
        L"DataCachePrefetches",
        L"DataCacheReadSize",
        L"DataCacheWriteSize",
        L"ITLBReloadStalls"
    };

    std::wstring c_NBPMCNames[] = {
        L"Reserved",
        L"NBGarlicReads",
        L"NBGarlicWrites",
        L"NBGarlicReadsWrites",
        L"NBOnionReads",
        L"NBOnionWrites",
        L"NBOnionReadsWrites",
        L"NBAllReadWrites",
        L"NBCPUToMemory",
        L"NBIOToMemory",
    };

    std::wstring c_L2PMCNames[] = {
        L"Reserved",
        L"L2MissICFill",
        L"L2MissDCFill",
        L"L2MissPrefetchFill",
        L"L2MissAll",
        L"L2Writeback",
        L"L2CleanWriteback",
        L"L2Fill",
        L"L2FillWriteback",
    };

    const uint32_t c_PMCGetOffset[c_totalPMCBufferSize] = {
        0,0,0,0,
        0,0,
        6,6,6,6,
        10,10,10,10
    };

    std::wstring *s_startPMCName[c_totalPMCBufferSize] = {
        &(c_PMCNames[0]),
        &(c_PMCNames[0]),
        &(c_PMCNames[0]),
        &(c_PMCNames[0]),
        nullptr,
        nullptr,
        &(c_NBPMCNames[0]),
        &(c_NBPMCNames[0]),
        &(c_NBPMCNames[0]),
        &(c_NBPMCNames[0]),
        &(c_L2PMCNames[0]),
        &(c_L2PMCNames[0]),
        &(c_L2PMCNames[0]),
        &(c_L2PMCNames[0]),
    };
}
