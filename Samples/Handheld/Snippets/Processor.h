//--------------------------------------------------------------------------------------
// Processor.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ATG
{
    // NOTE: Even though the system supports more than one group and thus more than 64 cores there is not a requirement to use it
    // By default all of the functions will apply to group 0, which will always be the only group with <= 64 cores.
    // Note the system can only support 2 logical cores per physical cores

    __forceinline uint64_t GetCPUTimeStamp()
    {
#ifndef _M_ARM64
        uint32_t tempAux;
        return __rdtscp(&tempAux);
#else
#ifdef __clang__
        uint64_t value;
        asm volatile("mrs %0, cntvct_el0" : "=r" (value));
        return value;
#else
        __isb(_ARM64_BARRIER_SY);
        return _ReadStatusReg(ARM64_SYSREG(3, 3, 14, 0, 2));    //ARM64_CNTVCT_EL0
#endif
#endif
    }

    using UniqueProcessorMask = uint64_t;

    enum class CacheType {
        CacheUnified,
        CacheInstruction,
        CacheData,
        CacheTrace
    };

    struct CacheInformation
    {
        CacheType cacheType;
        uint32_t level;
        uint32_t lineSize;
        uint32_t cacheSize;
        UniqueProcessorMask coreMask;

        CacheInformation() : cacheType(CacheType::CacheUnified), level(0), lineSize(0), cacheSize(0) {}
        CacheInformation(CacheType p1, uint32_t p2, uint32_t p3, uint32_t p4, const UniqueProcessorMask& p5)
            : cacheType(p1)
            , level(p2)
            , lineSize(p3)
            , cacheSize(p4)
            , coreMask(p5)
        {}
    };

    bool SetupProcessorData();

    std::wstring GetProcessorMaskString(uint64_t mask, uint32_t minHighBit);
    std::wstring GetMemorySizeString(uint64_t memorySize);

    uint32_t GetNumberPhysicalCores();
    uint32_t GetNumberLogicalCores();
    uint32_t GetTotalNumCores();
    uint64_t GetAvailableCoresMask();

    inline bool IsSMTSupported() { return GetNumberPhysicalCores() < GetNumberLogicalCores(); }
    double GetRDTSCPFrequencySeconds();
    double GetRDTSCPFrequencyMilliseconds();
    double GetRDTSCPFrequencyMicroseconds();

    uint32_t GetProcessorBaseFrequencyMHz(UniqueProcessorMask coreMask);

    const std::wstring& GetProcessorName();										// will be the override value which is initially the true name
    const std::wstring& GetTrueProcessorName();
    void SetProcessorName(const std::wstring& overrideName);

    uint64_t GetLogicalProcessorMask(UniqueProcessorMask coreMask);						// adjusts the mask to cover all logical processors

    uint32_t GetNumberEfficiencyClasses();
    UniqueProcessorMask GetEfficiencyClassMask(uint32_t efficiencyClass);	// get the mask for all the cores in this efficiency class

    uint32_t GetNumberTopLevelCacheSets();										// The number of high level cache sets. In general each entry matches a cluster or an efficency class
    void GetTopLevelCacheMask(UniqueProcessorMask queryMask, UniqueProcessorMask& resultMask);	// get a list of the high level cache sets that match this core mask
    uint64_t GetTopLevelCacheCoreMask(uint32_t highLevelCacheID);				// mask of cores that exist within this high level cache set
    uint32_t GetTopLevelCacheIndex();

    uint64_t GetProcessAffinityMask();

    uint64_t GetLogicalProcessorMask(UniqueProcessorMask coreMask);			// adjusts the mask to cover all logical processors

    void GetCacheInformation(UniqueProcessorMask coreMask, std::vector<CacheInformation>& information);
}
