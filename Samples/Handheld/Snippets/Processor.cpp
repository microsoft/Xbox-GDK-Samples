//--------------------------------------------------------------------------------------
// Processor.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "Processor.h"
#include <winapifamily.h>

#include <windows.h>
#include <sysinfoapi.h>
#undef max
#undef min
#include "intrin.h"

#ifdef __clang__
#ifndef _M_ARM64
#include <cpuid.h>
#include <x86intrin.h>
#endif
#endif

#include <set>
#include <cassert>
#include <algorithm>
#include <map>

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061 4062)

namespace
{
    std::wstring g_processorName;											// The general processor name. This can be changed by the user to allow easier usage in their code
    std::wstring g_trueProcessorName;										// The actual name of the processor as returned by the OS
    ATG::UniqueProcessorMask g_processMask = 0;												// In general all the cores that exist on the processor, when >64 this will be bound to the group the process is assigned to
    uint32_t g_numLogicalCores = 0;								// How many logical cores exist, if SMT is not supported == number of physical cores
    uint64_t g_availableCoresMask = 0;                           // Mask of all cores available on the machine

    struct OSProcessorInfo
    {
        size_t bufferSize;
        union
        {
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* procInfo;
            char* rawData;
        };
    };
    OSProcessorInfo g_osProcessorInfo;

    SYSTEM_INFO										g_systemInfo;				// Cached copy of data from GetSystemInfo
    std::vector<ATG::UniqueProcessorMask>			g_topLevelCacheMask;		// For each top level cache the mask of cores that share id. This tends to be processors that have several internal clusters, for example AMD Zen cores have multiple clusters
    std::vector<std::pair<bool, ATG::UniqueProcessorMask>>			g_physicalCores;			// For each physical core a mask of the logical cores that map to that physical core and a flag for whether it has support for SMT
    std::vector <ATG::CacheInformation>				g_caches;					// A list of each cache present on the processor, L1, L2, L3, Instruction, Data, and Trace (haven't seen this on any processor yet)
    double											g_rdtscpFrequencySeconds;	// The frequency in seconds of the __rdtscp, measured automatically at startup
    std::vector<ATG::UniqueProcessorMask>			g_efficiencyMask;		// For each efficiency class the mask of cores that share that class

    double CalcRDTSCPFrequency();
    void ParseOSProcessorInfo();
#ifdef _M_ARM64
    uint64_t COUNT_BITS(uint64_t v)
    {
        v = v - ((v >> 1ULL) & 0x55555555);                    // reuse input as temporary
        v = (v & 0x33333333) + ((v >> 2ULL) & 0x33333333);     // temp
        return ((v + (v >> 4ULL) & 0xF0F0F0F) * 0x1010101) >> 24ULL; // count
    }
#else
#define COUNT_BITS(value) __popcnt64(value)
#endif
}

std::wstring ATG::GetProcessorMaskString(uint64_t mask, uint32_t minHighBit)
{
    wchar_t buf[24];
    DWORD highBitIndex;
    _BitScanForward64(&highBitIndex, mask);
    if (highBitIndex < minHighBit)
        highBitIndex = minHighBit;

    if (highBitIndex < 8)
        swprintf(buf, 24, L"%02I64x", mask);
    else if (highBitIndex < 16)
        swprintf(buf, 24, L"%04I64x", mask);
    else if (highBitIndex < 32)
        swprintf(buf, 24, L"%08I64x", mask);
    else
        swprintf(buf, 24, L"%016I64x", mask);
    return std::wstring(buf);
}

std::wstring ATG::GetMemorySizeString(uint64_t memorySize)
{
    wchar_t buf[128];
    if (memorySize < 1024)
        swprintf(buf, 128, L"%llu Bytes", memorySize);
    else if (memorySize < (1024 * 1024))
        swprintf(buf, 128, L"%llu KB", memorySize / (1024));
    else if (memorySize < (1024 * 1024 * 1024))
        swprintf(buf, 128, L"%llu MB", memorySize / (1024 * 1024));
    else if (memorySize < (1024ULL * 1024ULL * 1024ULL * 1024ULL))
        swprintf(buf, 128, L"%llu GB", memorySize / (1024 * 1024 * 1024));
    return std::wstring(buf);
}

double ATG::GetRDTSCPFrequencySeconds()
{
    return g_rdtscpFrequencySeconds;
}

double ATG::GetRDTSCPFrequencyMilliseconds()
{
    return g_rdtscpFrequencySeconds / 1000.0;
}

double ATG::GetRDTSCPFrequencyMicroseconds()
{
    return g_rdtscpFrequencySeconds / 1000000.0;
}

const std::wstring& ATG::GetProcessorName()
{
    return g_processorName;
}

const std::wstring& ATG::GetTrueProcessorName()
{
    return g_trueProcessorName;
}

void ATG::SetProcessorName(const std::wstring& overrideName)
{
    g_processorName = overrideName;
}

uint32_t ATG::GetNumberPhysicalCores()
{
    return static_cast<uint32_t>(g_physicalCores.size());
}

uint32_t ATG::GetNumberLogicalCores()
{
    return g_numLogicalCores;
}
uint32_t ATG::GetTotalNumCores()
{
    return g_numLogicalCores;
}

uint64_t ATG::GetAvailableCoresMask()
{
    return g_availableCoresMask;
}

uint64_t ATG::GetLogicalProcessorMask(UniqueProcessorMask coreMask)
{
    uint64_t toret = coreMask;
    for (const auto& iter : g_physicalCores)
    {
        if ((iter.second & coreMask) != 0)
            toret |= iter.second;
    }
    return toret;
}

uint32_t ATG::GetNumberEfficiencyClasses()
{
    return static_cast<uint32_t> (g_efficiencyMask.size());
}

ATG::UniqueProcessorMask ATG::GetEfficiencyClassMask(uint32_t efficiencyClass)
{
    if (efficiencyClass >= g_efficiencyMask.size())
        return 0;
    return g_efficiencyMask[efficiencyClass];
}

uint32_t ATG::GetTopLevelCacheIndex()
{
    uint32_t topLevelCacheIndex = 0;
    for (auto& iter : g_caches)
    {
        if (iter.level > topLevelCacheIndex)
            topLevelCacheIndex = iter.level;
    }
    return topLevelCacheIndex;
}

uint32_t ATG::GetNumberTopLevelCacheSets()
{
    return static_cast<uint32_t> (g_topLevelCacheMask.size());
}

void ATG::GetTopLevelCacheMask(UniqueProcessorMask queryMask, UniqueProcessorMask& resultMask)
{
    resultMask = UniqueProcessorMask(0);
    for (const auto& iter : g_topLevelCacheMask)
    {
        if ((queryMask & iter) != 0)
            resultMask |= iter;
    }
}

uint64_t ATG::GetTopLevelCacheCoreMask(uint32_t highLevelCacheID)
{
    if (highLevelCacheID >= g_topLevelCacheMask.size())
        return 0;
    return g_topLevelCacheMask[highLevelCacheID];
}

void ATG::GetCacheInformation(UniqueProcessorMask coreMask, std::vector<CacheInformation>& information)
{
    for (const auto& iter : g_caches)
    {
        if ((iter.coreMask & coreMask) != 0)
        {
            information.push_back(iter);
        }
    }
}

uint32_t ATG::GetProcessorBaseFrequencyMHz(UniqueProcessorMask coreMask)
{
    uint64_t singleCoreIndex = COUNT_BITS(coreMask);
    HKEY regKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor", 0, KEY_READ, &regKey) != ERROR_SUCCESS)
        return 0;
    DWORD frequency(0);
    DWORD bufferSize = sizeof(DWORD);
    wchar_t buf[16];
    swprintf(buf, 16, L"%llu", singleCoreIndex - 1);
    if (RegGetValueW(regKey, buf, L"~MHz", RRF_RT_REG_DWORD, nullptr, &frequency, &bufferSize) != ERROR_SUCCESS)
        return 0;
    return frequency;
}

bool ATG::SetupProcessorData()
{
    if (g_trueProcessorName.size())
        return true;

    g_osProcessorInfo.bufferSize = 0;
    g_osProcessorInfo.rawData = nullptr;
    {
        wchar_t processorName[256];
        DWORD dataSize = sizeof(wchar_t) * 256;
        RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString", RRF_RT_REG_SZ, nullptr, processorName, &dataSize);
        g_trueProcessorName = processorName;
    }

    std::for_each(g_trueProcessorName.begin(), g_trueProcessorName.end(), [](wchar_t& letter) {if (letter == L'/') letter = L'_'; });
    std::for_each(g_trueProcessorName.begin(), g_trueProcessorName.end(), [](wchar_t& letter) {if (letter == L'\\') letter = L'_'; });
    std::for_each(g_trueProcessorName.begin(), g_trueProcessorName.end(), [](wchar_t& letter) {if (letter == L':') letter = L'_'; });

    // remove tail spaces
    size_t index = g_trueProcessorName.find_last_not_of(L' ');
    if (index != (g_trueProcessorName.size() - 1))
        g_trueProcessorName = g_trueProcessorName.substr(0, index + 1);

    // convert rest of spaces to _
    //std::for_each(g_trueProcessorName.begin(), g_trueProcessorName.end(), [](wchar_t& letter) {if (letter == L' ') letter = L'_'; });
    g_processorName = g_trueProcessorName;

    uint64_t systemAffinityMask, processAffinityMask;
    ::GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
    g_processMask = processAffinityMask;

    GetSystemInfo(&g_systemInfo);

    char* data;
    DWORD bufferSize = 0;
    GetLogicalProcessorInformationEx(RelationAll, nullptr, &bufferSize);
    data = new char[bufferSize];
    GetLogicalProcessorInformationEx(RelationAll, reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(data), &bufferSize);
    g_osProcessorInfo.rawData = data;
    g_osProcessorInfo.bufferSize = bufferSize;

    CalcRDTSCPFrequency();
    ParseOSProcessorInfo();

    return true;
}

uint64_t ATG::GetProcessAffinityMask()
{
    return g_processMask;
}

namespace
{
    double CalcRDTSCPFrequency()
    {
        ATG::SetupProcessorData();

        constexpr uint32_t msDuration = 1000u;

        LARGE_INTEGER qpcTempRate;
        QueryPerformanceFrequency(&qpcTempRate);
        const double qpcRate = (double)qpcTempRate.QuadPart;
        double rdtscFrequencyIdle;
        double rdtscFrequencyLoad;

        const int32_t currentPriority = GetThreadPriority(GetCurrentThread());
        ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        // don't use the first core, don't really care if this fails the code still works, this is just here as an extra forcing value just in case we're on a platform that forces interrupts to core 0
        const uint64_t oldAffinityMask = ::SetThreadAffinityMask(GetCurrentThread(), 0x04);
        // Measure __rdtscp() when the machine is mostly idle
        {
            const uint64_t rdtscStart = ATG::GetCPUTimeStamp();
            QueryPerformanceCounter(&qpcTempRate);
            const int64_t qpcStart = qpcTempRate.QuadPart;
            Sleep(msDuration);
            const uint64_t rdtscElapsed = ATG::GetCPUTimeStamp() - rdtscStart;
            QueryPerformanceCounter(&qpcTempRate);
            const int64_t qpcElapsed = qpcTempRate.QuadPart - qpcStart;
            rdtscFrequencyIdle = rdtscElapsed / (qpcElapsed / qpcRate);
        }

        // Measure __rdtscp() when the machine is busy
        {
            const uint64_t rdtscStart = ATG::GetCPUTimeStamp();
            QueryPerformanceCounter(&qpcTempRate);
            const int64_t qpcStart = qpcTempRate.QuadPart;
            const uint64_t startTick = GetTickCount64();
            for (;;)
            {
                const uint64_t tickDuration = GetTickCount64() - startTick;
                if (tickDuration >= msDuration)
                    break;
            }
            const uint64_t rdtscElapsed = ATG::GetCPUTimeStamp() - rdtscStart;
            QueryPerformanceCounter(&qpcTempRate);
            const int64_t qpcElapsed = qpcTempRate.QuadPart - qpcStart;
            rdtscFrequencyLoad = rdtscElapsed / (qpcElapsed / qpcRate);
        }
        if (oldAffinityMask != 0)
            ::SetThreadAffinityMask(GetCurrentThread(), oldAffinityMask);

        ::SetThreadPriority(GetCurrentThread(), currentPriority);
        g_rdtscpFrequencySeconds = (rdtscFrequencyIdle + rdtscFrequencyLoad) / 2.0;		// the numbers should be close to identical so just average
        return g_rdtscpFrequencySeconds;
    }

    void ParseOSProcessorInfo()
    {
        uint32_t topLevelCache = 0;
        uint16_t groupCount = 0;
        g_numLogicalCores = 0;
        g_availableCoresMask = 0;

        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* procInfo = g_osProcessorInfo.procInfo;
        char* data = g_osProcessorInfo.rawData;

        size_t dataLeft = g_osProcessorInfo.bufferSize;

        data = g_osProcessorInfo.rawData;
        dataLeft = g_osProcessorInfo.bufferSize;
        while (dataLeft)
        {
            procInfo = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*> (data);
            switch (procInfo->Relationship)
            {
            case RelationProcessorPackage:
                for (uint32_t i = 0; i < procInfo->Processor.GroupCount; i++)
                {
                    //TODO: Check how we merge dies and groups
                    g_availableCoresMask |= procInfo->Processor.GroupMask[0].Mask;
                }
                break;
            case RelationProcessorCore:
                g_physicalCores.push_back(std::pair<bool, uint64_t>(procInfo->Processor.Flags == LTP_PC_SMT, procInfo->Processor.GroupMask[0].Mask));
                g_numLogicalCores += static_cast<uint32_t>(COUNT_BITS(procInfo->Processor.GroupMask[0].Mask));
                if (g_efficiencyMask.size() <= procInfo->Processor.EfficiencyClass)
                    g_efficiencyMask.resize(procInfo->Processor.EfficiencyClass + 1u, 0);
                g_efficiencyMask[procInfo->Processor.EfficiencyClass] |= procInfo->Processor.GroupMask[0].Mask;
                break;
            case RelationCache:
            {
                ATG::CacheType cacheType(ATG::CacheType::CacheUnified);
                switch (procInfo->Cache.Type)
                {
                case CacheUnified:
                    cacheType = ATG::CacheType::CacheUnified;
                    break;
                case CacheInstruction:
                    cacheType = ATG::CacheType::CacheInstruction;
                    break;
                case CacheData:
                    cacheType = ATG::CacheType::CacheData;
                    break;
                case CacheTrace:
                    cacheType = ATG::CacheType::CacheTrace;
                    break;
                }
                if (procInfo->Cache.Level > topLevelCache)
                    topLevelCache = procInfo->Cache.Level;
                if (procInfo->Cache.GroupMask.Group == 0)  // only merge cores for the first group
                    g_caches.push_back(ATG::CacheInformation(cacheType, procInfo->Cache.Level, procInfo->Cache.LineSize, procInfo->Cache.CacheSize, ATG::UniqueProcessorMask(procInfo->Cache.GroupMask.Mask)));
                //g_caches.push_back(ATG::CacheInformation(cacheType, procInfo->Cache.Level, procInfo->Cache.LineSize, procInfo->Cache.CacheSize, ATG::UniqueProcessorMask(procInfo->Cache.GroupMask.Mask)));
            }
            break;
            case RelationGroup:	// already handled above
                break;
            case RelationAll:
                DebugBreak();	// just checking
                break;
            default:
                break;
            }
            dataLeft -= procInfo->Size;
            data = data + procInfo->Size;
        }

        // build up top level cache list based on top level cache
        {
            for (auto iter = g_caches.begin(); iter != g_caches.end(); ++iter)
            {
                if (iter->level == topLevelCache)
                {
                    g_topLevelCacheMask.push_back(ATG::UniqueProcessorMask(iter->coreMask));
                }
            }
        }

        // get rid of the data that doesn't match to one of the available cores in the system
        // If there are more than 1 group the process is on their own to handle that case
        if (groupCount == 1)
        {
            g_availableCoresMask &= g_processMask;
            g_numLogicalCores = 0;

            {
                for (auto iter = g_topLevelCacheMask.begin(); iter != g_topLevelCacheMask.end(); )
                {
                    *iter &= g_availableCoresMask;
                    if (*iter == 0)
                        iter = g_topLevelCacheMask.erase(iter);
                    else
                        ++iter;
                }
            }
            {
                for (auto iter = g_physicalCores.begin(); iter != g_physicalCores.end(); )
                {
                    iter->second &= g_availableCoresMask;
                    if (iter->second == 0)
                        iter = g_physicalCores.erase(iter);
                    else
                        ++iter;
                }
            }
            {
                for (auto iter = g_caches.begin(); iter != g_caches.end(); )
                {
                    iter->coreMask &= g_availableCoresMask;
                    if (iter->coreMask == 0)
                        iter = g_caches.erase(iter);
                    else
                        ++iter;
                }
            }
        }
    }
}
