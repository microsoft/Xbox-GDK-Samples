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

// define SUPPORT_LOGGING if you want the logging functions enabled. This requires including FileLogger.cpp.h
//#define SUPPORT_LOGGING

namespace ATG
{
    // NOTE: Even though the system supports more than one group and thus more than 64 cores there is not a requirement to use it
    // By default all of the functions will apply to group 0, which will always be the only group with <= 64 cores.
    // Note the system can only support 2 logical cores per physical cores, there are rumors for upcoming processors that may have 4 logical cores per physical core

    using ProcessorGroupID = uint16_t;
    using ProcessorNumaNodeID = uint32_t;
    constexpr uint32_t JAGUAR_PROCESSOR_FAMILY = 0x16;
    constexpr uint32_t ZEN2_PROCESSOR_FAMILY = 0x17;

    struct UniqueProcessorMask
    {
        uint64_t coreMask;
        ProcessorGroupID groupID;
        UniqueProcessorMask() : coreMask(0), groupID(0) {}
        UniqueProcessorMask(uint64_t newMask, ProcessorGroupID newGroup = 0) : coreMask(newMask), groupID(newGroup) {}
        bool operator<(const UniqueProcessorMask& rhs)
        {
            if (groupID == rhs.groupID)
                return coreMask < rhs.coreMask;
            return groupID < rhs.groupID;
        }
    };

    inline bool operator<(const UniqueProcessorMask& lhs, const UniqueProcessorMask& rhs)
    {
        if (lhs.groupID == rhs.groupID)
            return lhs.coreMask < rhs.coreMask;
        return lhs.groupID < rhs.groupID;
    }

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
    void SetThreadPriority(void* thread, int32_t priority);
    void SetThreadAffinityMask(void* thread, uint64_t mask);
    void SetThreadPriorityBoost(void* thread, bool disablePriorityBoost);

#ifdef SUPPORT_LOGGING
    void LogProcessorInfo(const std::wstring& logName = L"");
    void LogCoreTests(const std::wstring& suffix, const std::wstring& logName = L"");
#endif

    std::wstring GetProcessorMaskString(uint64_t mask);
    std::wstring GetCoreTestName(uint64_t mask);

    uint32_t GetNumPhysicalCores(ProcessorGroupID groupID = 0);
    uint32_t GetNumLogicalCores(ProcessorGroupID groupID = 0);
    uint32_t GetTotalNumCores(ProcessorGroupID groupID = 0);
    uint64_t GetAvailableCoresMask(ProcessorGroupID groupID = 0);

    inline bool IsSMTSupported(ProcessorGroupID groupID = 0) { return GetNumPhysicalCores(groupID) < GetNumLogicalCores(groupID); }
    double GetRDTSCPFrequencySeconds();
    double GetRDTSCPFrequencyMilliseconds();
    double GetRDTSCPFrequencyMicroseconds();

    const std::wstring& GetProcessorName();										// will be the override value which is initially the true name
    const std::wstring& GetTrueProcessorName();
    void SetProcessorName(const std::wstring& overrideName);
    uint32_t GetProcessorFamily();

    uint64_t GetLogicalProcessorMask(uint64_t coreMask);						// adjusts the mask to cover all logical processors

    uint32_t GetNumberTopLevelCacheSets(ProcessorGroupID groupID = 0);										// The number of high level cache sets. In general each entry matches a cluster, for example Durango has 2
    void GetTopLevelCacheMask(UniqueProcessorMask queryMask, UniqueProcessorMask& resultMask);	// get a list of the high level cache sets that match this core mask
    uint64_t GetTopLevelCacheCoreMask(uint32_t highLevelCacheID, ProcessorGroupID groupID = 0);				// mask of cores that exist within this high level cache set

    void GetCacheInformation(uint64_t coreMask, std::vector<CacheInformation>& information);

    void GenerateRepresentiveCoreTests(bool cacheBased);
    const std::vector<uint64_t>& GetRepresentativeCoreTests();
    const std::vector<std::wstring>& GetRepresentativeCoreTestNames();

    uint64_t GetProcessAffinityMask();
    void SetProcessAffinityMask(uint64_t coreMask);

    // Extra support for special situations, handling >64 processors and NUMA nodes. If you don't need this functionality it's safe to ignore this block

    uint32_t GetNumberDies();													// The number of dies in the machine. In most cases this is 1, however cores like a ThreadRipper is will be greater than 1
                                                                                // This is not necessarily the numbers of sockets which are present
    void GetDieMask(UniqueProcessorMask coreMask, std::vector<uint32_t>& information);		// get a list of the dies that match this core mask
    UniqueProcessorMask GetDieCoreMask(uint32_t die);

    uint32_t GetNumberNumaNodes();												// The number of NUMA nodes in the system. This really is only used for memory performance, in all other cases use cache data or group data
    UniqueProcessorMask GetNumNumaNodeCores(ProcessorNumaNodeID numaNode);		// The processor cores that are present in this NUMA node
    uint64_t GetTotalNumberCoresIncludingNuma();

    void SetThreadAffinityMask(void* thread, uint64_t mask, ProcessorGroupID groupID);
    void SetThreadAffinityMask(void* thread, const UniqueProcessorMask& mask);
#if !defined(_GAMING_XBOX) && !defined(_XBOX_ONE)
    bool GetProcessGroupAffinity(std::vector<uint16_t>& results);
#endif

    uint16_t GetNumberGroups();													// This adds support for >64 cores
    uint64_t GetNumGroupCores(ProcessorGroupID groupID);
    UniqueProcessorMask GetGroupCoreMask(ProcessorGroupID groupID);

    uint64_t GetLogicalProcessorMask(const UniqueProcessorMask& coreMask);			// adjusts the mask to cover all logical processors
    void GetDieCoreMask(uint32_t die, std::vector<UniqueProcessorMask>& information);

    void GetCacheInformation(const UniqueProcessorMask& coreMask, std::vector<CacheInformation>& information);

    const std::vector<UniqueProcessorMask>& GetRepresentativeGroupCoreTests();
    const std::vector<std::wstring>& GetRepresentativeGroupCoreTestNames();
}
