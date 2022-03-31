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

#if defined(_GAMING_XBOX)
#include <xsystem.h>
#endif

#include <set>
#include <cassert>
#include <algorithm>
#include <map>

#ifdef SUPPORT_LOGGING
#include "FileLogger.h"
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061 4062)

namespace
{
    std::wstring g_processorName;											// The general processor name. This can be changed by the user to allow easier usage in their code
    std::wstring g_trueProcessorName;										// The actual name of the processor as returned by the OS
    uint64_t g_processMask = 0;												// In general all the cores that exist on the processor, when >64 this will be bound to the group the process is assigned to
    std::vector<uint32_t> g_numLogicalCores;								// How many logical cores exist, if not hyperthreaded == number of physical cores
    std::vector<uint64_t> g_availableCoresMask;								// Which cores are actually available to the caller, for example ERA does not allow the 8th core to be used, index is groupID

#if !defined(_XBOX_ONE) 													// we don't use this on ERA because GetLogicalProcessorInfoEx and GetLogicalProcessorInformation is not supported
    struct OSProcessorInfo
    {
        size_t bufferSize;
        union
        {
#ifdef _XBOX_ONE
            // Xbox XDK development uses raw data because GetLogicalProcessorInformation and GetLogicalProcessorInformationEx
#elif defined(_GAMING_XBOX)
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION* procInfo;
#else
            SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* procInfo;
#endif
            char* rawData;
        };
    };
    OSProcessorInfo g_osProcessorInfo;
#endif

    SYSTEM_INFO										g_systemInfo;				// Cached copy of data from GetSystemInfo
    std::vector<ATG::UniqueProcessorMask>			g_topLevelCacheMask;		// For each top level cache the mask of cores that share id. This tends to be processors that have several internal clusters, for example Durango has two clusters
    std::map<ATG::ProcessorGroupID, std::vector<std::pair<bool, uint64_t>>>			g_physicalCores;			// For each physical core a mask of the logical cores that map to that physical core and a flag for whether it's hyperthreaded
    std::vector <ATG::CacheInformation>				g_caches;					// A list of each cache present on the processor, L1, L2, L3, Instruction, Data, and Trace (haven't seen this on any processor yet)
    double											g_rdtscpFrequencySeconds;	// The frequency in seconds of the __rdtscp, measured automatically at startup

    std::vector<uint64_t>							g_representativeCoreTests;			// A list of core masks that represent a good cross section of tests based on processor layout. This can be either cache based or physical/logical/die based
    std::vector<std::wstring>						g_representativeCoreTestNames;		// The name describing each core test from g_representativeCoreTests

    // Extra support for special situations, handling >64 processors and NUMA nodes. If you don't need this functionality it's safe to ignore this block

    std::map<ATG::ProcessorNumaNodeID, ATG::UniqueProcessorMask>	g_numaNodeMask;				// For each NUMA node the mask of cores present.
    std::map<ATG::ProcessorGroupID, ATG::UniqueProcessorMask>		g_groupMask;				// For each group the mask of cores present.
    std::map<uint32_t, ATG::UniqueProcessorMask>	g_dieMask;					// For each die the mask of cores present. Most cases this is equal to the number of Numa nodes or the number of sockets, however certain cores like ThreadRipper it will be different
    std::vector<ATG::UniqueProcessorMask>			g_representativeGroupCoreTests;		// A list of core masks that represent a good cross section of tests based on processor layout. This can be either cache based or physical/logical/die based
    std::vector<std::wstring>						g_representativeGroupCoreTestNames;	// The name describing each core test from g_representativeCoreTests

    void InternalGenerateRepresentiveQueueTests(ATG::ProcessorGroupID groupID, std::vector<uint64_t>& coreTests, std::vector<std::wstring>& testNames);
    void InternalGenerateRepresentiveLayoutTests(ATG::ProcessorGroupID groupID, std::vector<uint64_t>& coreTests, std::vector<std::wstring>& testNames);
    void CalcRDTSCPFrequency();
    void ParseOSProcessorInfo();
}

std::wstring ATG::GetProcessorMaskString(uint64_t mask)
{
    wchar_t buf[24];
    DWORD highBitIndex;
    _BitScanForward64(&highBitIndex, mask);

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

std::wstring ATG::GetCoreTestName(uint64_t mask)
{
    uint32_t currentMask(0);
    for (auto& iter : g_representativeCoreTests)
    {
        if (iter == mask)
            return g_representativeCoreTestNames[currentMask];
        currentMask++;
    }
    return L"Undefined test name: " + GetProcessorMaskString(mask);
}

const std::vector<uint64_t>& ATG::GetRepresentativeCoreTests()
{
    return g_representativeCoreTests;
}
const std::vector<std::wstring>& ATG::GetRepresentativeCoreTestNames()
{
    return g_representativeCoreTestNames;
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

uint32_t ATG::GetNumPhysicalCores(ProcessorGroupID groupID)
{
    return static_cast<uint32_t>(g_physicalCores[groupID].size());
}

uint32_t ATG::GetNumLogicalCores(ProcessorGroupID groupID)
{
    return g_numLogicalCores[groupID];
}
uint32_t ATG::GetTotalNumCores(ProcessorGroupID groupID)
{
    return g_numLogicalCores[groupID];
}
uint64_t ATG::GetAvailableCoresMask(ProcessorGroupID groupID)
{
    return g_availableCoresMask[groupID];
}

uint64_t ATG::GetLogicalProcessorMask(uint64_t coreMask)
{
    return GetLogicalProcessorMask(UniqueProcessorMask(coreMask, 0));
}

uint64_t ATG::GetLogicalProcessorMask(const UniqueProcessorMask& coreMask)
{
    uint64_t toret = coreMask.coreMask;
    for (const auto& groupIter : g_physicalCores)
    {
        if (groupIter.first != coreMask.groupID)
            continue;
        for (const auto& iter : groupIter.second)
        {
            if ((iter.second & coreMask.coreMask) != 0)
                toret |= iter.second;
        }
    }
    return toret;
}

uint32_t ATG::GetNumberNumaNodes()
{
    return static_cast<uint32_t>(g_numaNodeMask.size());
}

ATG::UniqueProcessorMask ATG::GetNumNumaNodeCores(ProcessorNumaNodeID numaNode)
{
    for (const auto& iter : g_numaNodeMask)
    {
        if (iter.first == numaNode)
            return iter.second;
    }
    return UniqueProcessorMask();
}

uint64_t ATG::GetTotalNumberCoresIncludingNuma()
{
    uint64_t toret = 0;
    for (const auto& iter : g_numaNodeMask)
    {
        toret += __popcnt64(iter.second.coreMask);
    }
    return toret;
}

uint16_t ATG::GetNumberGroups()
{
    return static_cast<uint16_t>(g_groupMask.size());
}
uint64_t ATG::GetNumGroupCores(ProcessorGroupID groupID);
ATG::UniqueProcessorMask ATG::GetGroupCoreMask(ProcessorGroupID groupID);

uint64_t ATG::GetNumGroupCores(ProcessorGroupID groupID)
{
    for (const auto& iter : g_groupMask)
    {
        if (iter.first == groupID)
            return __popcnt64(iter.second.coreMask);
    }
    return 0;
}

uint32_t ATG::GetNumberDies()
{
    return static_cast<uint32_t>(g_dieMask.size());
}

void ATG::GetDieMask(UniqueProcessorMask coreMask, std::vector<uint32_t>& information)
{
    for (const auto& iter : g_dieMask)
    {
        if ((coreMask.coreMask & iter.second.coreMask) != 0)
            information.push_back(iter.first);
    }
}

ATG::UniqueProcessorMask ATG::GetDieCoreMask(uint32_t die)
{
    for (const auto& iter : g_dieMask)
    {
        if (iter.first == die)
            return iter.second;
    }
    return 0;
}

uint32_t ATG::GetNumberTopLevelCacheSets(ProcessorGroupID groupID)
{
    uint32_t toret(0);
    for (const auto& iter : g_topLevelCacheMask)
    {
        if (iter.groupID == groupID)
            toret++;
    }
    return toret;
}

void ATG::GetTopLevelCacheMask(UniqueProcessorMask queryMask, UniqueProcessorMask& resultMask)
{
    resultMask = UniqueProcessorMask(0, queryMask.groupID);
    for (const auto& iter : g_topLevelCacheMask)
    {
        if (iter.groupID != queryMask.groupID)
            continue;
        if ((queryMask.coreMask & iter.coreMask) != 0)
            resultMask.coreMask |= iter.coreMask;
    }
}

uint64_t ATG::GetTopLevelCacheCoreMask(uint32_t index, ProcessorGroupID groupID)
{
    for (const auto& iter : g_topLevelCacheMask)
    {
        if (iter.groupID == groupID)
        {
            if (index == 0)
                return iter.coreMask;
            else
                --index;
        }
    }
    return 0;
}

void ATG::GetCacheInformation(uint64_t coreMask, std::vector<CacheInformation>& information)
{
    GetCacheInformation(UniqueProcessorMask(coreMask), information);
}

void ATG::GetCacheInformation(const UniqueProcessorMask& coreMask, std::vector<CacheInformation>& information)
{
    for (const auto& iter : g_caches)
    {
        if (iter.coreMask.groupID != coreMask.groupID)
            continue;
        if ((iter.coreMask.coreMask & coreMask.coreMask) != 0)
        {
            information.push_back(iter);
        }
    }
}

bool ATG::SetupProcessorData()
{
#if !defined(_XBOX_ONE) && !defined(_GAMING_XBOX)			// Xbox doesn't have this registry entry
    g_osProcessorInfo.bufferSize = 0;
    g_osProcessorInfo.rawData = nullptr;
    wchar_t processorName[256] = {};
    DWORD dataSize = sizeof(wchar_t) * 256;
    RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString", RRF_RT_REG_SZ, nullptr, processorName, &dataSize);
    g_trueProcessorName = processorName;
#elif defined(_GAMING_XBOX)
    XSystemDeviceType info;
    info = XSystemGetDeviceType();
    switch (info)
    {
    case XSystemDeviceType::XboxOne:                g_trueProcessorName = L"Microsoft Xbox One CPU"; break;
    case XSystemDeviceType::XboxOneS:               g_trueProcessorName = L"Microsoft Xbox One S CPU"; break;
    case XSystemDeviceType::XboxOneX:               g_trueProcessorName = L"Microsoft Xbox One X CPU"; break;
    case XSystemDeviceType::XboxOneXDevkit:         g_trueProcessorName = L"Microsoft Xbox One X DevKit CPU"; break;
    case XSystemDeviceType::XboxScarlettLockhart:   g_trueProcessorName = L"Microsoft Xbox Series S CPU"; break;
    case XSystemDeviceType::XboxScarlettAnaconda:   g_trueProcessorName = L"Microsoft Xbox Series X CPU"; break;
    case XSystemDeviceType::XboxScarlettDevkit:     g_trueProcessorName = L"Microsoft Xbox Series X DevKit CPU"; break;
    case XSystemDeviceType::Pc: assert(false);      // fallthrough, this case should never happen because this block of code is only on console
    case XSystemDeviceType::Unknown:                g_trueProcessorName = L"Microsoft Undefined Xbox CPU"; break;
    }
#else       // Xbox XDK path
    {
        CONSOLE_TYPE consoleType;
        consoleType = GetConsoleType();
        switch (consoleType)
        {
        case CONSOLE_TYPE_UNKNOWN:
            g_trueProcessorName = L"Microsoft Unknown Xbox One CPU";
            break;
        case CONSOLE_TYPE_XBOX_ONE:
            g_trueProcessorName = L"Microsoft Xbox One CPU";
            break;
        case CONSOLE_TYPE_XBOX_ONE_S:
            g_trueProcessorName = L"Microsoft Xbox One S CPU";
            break;
        case CONSOLE_TYPE_XBOX_ONE_X:
            g_trueProcessorName = L"Microsoft Xbox One X CPU";
            break;
        case CONSOLE_TYPE_XBOX_ONE_X_DEVKIT:
            g_trueProcessorName = L"Microsoft Xbox One X DevKit CPU";
            break;
        default:
            g_trueProcessorName = L"Microsoft Undefined Xbox CPU";
            break;
        }
    }
#endif
    std::for_each(g_trueProcessorName.begin(), g_trueProcessorName.end(), [](wchar_t& letter) {if (letter == L'/') letter = L'_'; });
    std::for_each(g_trueProcessorName.begin(), g_trueProcessorName.end(), [](wchar_t& letter) {if (letter == L'\\') letter = L'_'; });
    std::for_each(g_trueProcessorName.begin(), g_trueProcessorName.end(), [](wchar_t& letter) {if (letter == L':') letter = L'_'; });

    // remove tail spaces
    if (g_trueProcessorName[g_trueProcessorName.size() - 1] == L' ')
    {
        auto index = g_trueProcessorName.size() - 1;
        while (g_trueProcessorName[index--] == L' ');
        if (index != (g_trueProcessorName.size() - 1))
            g_trueProcessorName.resize(index);
    }
    // convert rest of spaces to _
    std::for_each(g_trueProcessorName.begin(), g_trueProcessorName.end(), [](wchar_t& letter) {if (letter == L' ') letter = L'_'; });
    g_processorName = g_trueProcessorName;

    uint64_t systemAffinityMask, processAffinityMask;
    ::GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
    g_processMask = processAffinityMask;

    GetSystemInfo(&g_systemInfo);

#if defined(_XBOX_ONE)	// ERA doesn't have GetLogicalProcessorInformationEx or GetLogicalProcessorInformation, will handle it manually in ParseOSProcessorInfo
#elif defined (_GAMING_XBOX)
    char* data;
    DWORD bufferSize = 0;
    GetLogicalProcessorInformation(nullptr, &bufferSize);
    data = new char[bufferSize];
    GetLogicalProcessorInformation(reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*>(data), &bufferSize);
    g_osProcessorInfo.rawData = data;
    g_osProcessorInfo.bufferSize = bufferSize;
#else
    char* data;
    DWORD bufferSize = 0;
    GetLogicalProcessorInformationEx(RelationAll, nullptr, &bufferSize);
    data = new char[bufferSize];
    GetLogicalProcessorInformationEx(RelationAll, reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(data), &bufferSize);
    g_osProcessorInfo.rawData = data;
    g_osProcessorInfo.bufferSize = bufferSize;
#endif
    CalcRDTSCPFrequency();
    ParseOSProcessorInfo();

#if defined(_GAMING_XBOX)
    if (ATG::IsHyperThreaded())
        g_trueProcessorName += L"_SMT";
    g_processorName = g_trueProcessorName;
#endif
    return true;
}

uint64_t ATG::GetProcessAffinityMask()
{
    return g_processMask;
}

#if !defined(_GAMING_XBOX) && !defined(_XBOX_ONE)
bool ATG::GetProcessGroupAffinity(std::vector<uint16_t>& results)
{
    std::unique_ptr<uint16_t[]> groupAffinities;
    uint16_t groupCount = 0;
    if (!::GetProcessGroupAffinity(GetCurrentProcess(), &groupCount, groupAffinities.get()))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            return false;
    }
    groupAffinities.reset(new uint16_t[groupCount]);
    if (!::GetProcessGroupAffinity(GetCurrentProcess(), &groupCount, groupAffinities.get()))
    {
        return false;
    }

    for (uint32_t i = 0; i < groupCount; i++)
    {
        results.push_back(groupAffinities[i]);
    }
    return true;
}
#endif

void ATG::SetThreadPriority(void* thread, int32_t priority)
{
    ::SetThreadPriority(thread, priority);
}

void ATG::SetThreadAffinityMask(void* thread, uint64_t mask)
{
    ::SetThreadAffinityMask(thread, mask);
}

void ATG::SetThreadAffinityMask(void* thread, uint64_t mask, ProcessorGroupID groupID)
{
    SetThreadAffinityMask(thread, ATG::UniqueProcessorMask(mask, groupID));
}

void ATG::SetThreadAffinityMask(void* thread, const UniqueProcessorMask& mask)
{
    GROUP_AFFINITY setAffinity, getAffinity;
    memset(&setAffinity, 0, sizeof(setAffinity));
    memset(&getAffinity, 0, sizeof(getAffinity));
    setAffinity.Group = mask.groupID;
    setAffinity.Mask = mask.coreMask;
    ::SetThreadGroupAffinity(thread, &setAffinity, &getAffinity);
}

void ATG::SetProcessAffinityMask(uint64_t coreMask)
{
    g_processMask = coreMask;
    ::SetProcessAffinityMask(GetCurrentProcess(), coreMask);
}

void ATG::SetThreadPriorityBoost(void* thread, bool disablePriorityBoost)
{
    ::SetThreadPriorityBoost(thread, disablePriorityBoost);
}

#ifdef SUPPORT_LOGGING
#ifdef _XBOX_ONE
#error Logging is not supported when building with the XDK
#endif

void ATG::LogProcessorInfo(const std::wstring& logName)
{
    std::wstring fileName;
    if (logName.size() != 0)
    {
        fileName = logName;
    }
    else
    {
        wchar_t  buffer[256];
        swprintf_s(buffer, 256, L"%s_procInfo", g_trueProcessorName.c_str());
        fileName = buffer;
    }
    ATG::FileLogger procLog(fileName, false);

#if !defined(_GAMING_XBOX)
    wchar_t buffer[256];
    assert(g_osProcessorInfo.bufferSize);
    char* data = g_osProcessorInfo.rawData;

    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* procInfo = g_osProcessorInfo.procInfo;
    uint32_t entries = 0;
    size_t dataLeft = g_osProcessorInfo.bufferSize;
    while (dataLeft)
    {
        procLog.Log(L"");
        procInfo = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*> (data);
        switch (procInfo->Relationship)
        {
        case RelationProcessorPackage:
            procLog.Log(L"Processor Package");
            if (procInfo->Processor.EfficiencyClass != 0)
                procLog.Log(L"  Core efficiency class is set");
            for (uint32_t i = 0; i < procInfo->Processor.GroupCount; i++)
            {
                swprintf(buffer, 256, L"  Group Number - %d", procInfo->Processor.GroupMask[i].Group);
                procLog.Log(buffer);
                swprintf(buffer, 256, L"  GroupMask - %016I64x", procInfo->Processor.GroupMask[i].Mask);
                procLog.Log(buffer);
            }
            break;
        case RelationProcessorCore:
            procLog.Log(L"Processor Core");
            if (procInfo->Processor.Flags == LTP_PC_SMT)
                procLog.Log(L"  Core is hyper threaded");
            if (procInfo->Processor.EfficiencyClass != 0)
                procLog.Log(L"  Core efficiency class is set");
            for (uint32_t i = 0; i < procInfo->Processor.GroupCount; i++)
            {
                swprintf(buffer, 256, L"  Group Number - %d", procInfo->Processor.GroupMask[i].Group);
                procLog.Log(buffer);
                swprintf(buffer, 256, L"    GroupMask - %016I64x", procInfo->Processor.GroupMask[i].Mask);
                procLog.Log(buffer);
            }
            break;
        case RelationNumaNode:
            procLog.Log(L"Numa Node");
            swprintf(buffer, 256, L"  Numa Number - %d", procInfo->NumaNode.NodeNumber);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  Group Number - %d", procInfo->NumaNode.GroupMask.Group);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  GroupMask - %016I64x", procInfo->NumaNode.GroupMask.Mask);
            procLog.Log(buffer);
            break;
        case RelationCache:
            procLog.Log(L"Cache");
            switch (procInfo->Cache.Type)
            {
            case CacheUnified:
                procLog.Log(L"  Unified");
                break;
            case CacheInstruction:
                procLog.Log(L"  Instruction");
                break;
            case CacheData:
                procLog.Log(L"  Data");
                break;
            case CacheTrace:
                procLog.Log(L"  Trace");
                break;
            }
            swprintf(buffer, 256, L"  Level - %hhd", procInfo->Cache.Level);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  Associativity - %hhd", procInfo->Cache.Associativity);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  Line Size - %hd", procInfo->Cache.LineSize);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  Cache Size - %d", procInfo->Cache.CacheSize);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  Group Number - %d", procInfo->Cache.GroupMask.Group);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  GroupMask - %016I64x", procInfo->Cache.GroupMask.Mask);
            procLog.Log(buffer);

            break;
        case RelationGroup:
            procLog.Log(L"Group");
            swprintf(buffer, 256, L"  Maximum Groups - %d", procInfo->Group.MaximumGroupCount);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  Active Groups - %d", procInfo->Group.ActiveGroupCount);
            procLog.Log(buffer);
            for (uint32_t i = 0; i < procInfo->Group.ActiveGroupCount; i++)
            {
                swprintf(buffer, 256, L"  Group Number - %d", i);
                procLog.Log(buffer);
                swprintf(buffer, 256, L"    Maximum Processor Count - %d", procInfo->Group.GroupInfo[i].MaximumProcessorCount);
                procLog.Log(buffer);
                swprintf(buffer, 256, L"    Active Processor Count - %d", procInfo->Group.GroupInfo[i].ActiveProcessorCount);
                procLog.Log(buffer);
                swprintf(buffer, 256, L"    Active Processor Mask - %016I64x", procInfo->Group.GroupInfo[i].ActiveProcessorMask);
                procLog.Log(buffer);
            }
            break;
        default:
            break;
        }
        dataLeft -= procInfo->Size;
        data = data + procInfo->Size;
        entries++;
    }
#else
    wchar_t buffer[256];
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION* procInfo = g_osProcessorInfo.procInfo;
    uint32_t entries = 0;
    char* data = g_osProcessorInfo.rawData;
    size_t dataLeft = g_osProcessorInfo.bufferSize;
    while (dataLeft)
    {
        procLog.Log(L"");
        procInfo = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*> (data);

        switch (procInfo->Relationship)
        {
        case RelationCache:
            procLog.Log(L"Processor Cache");
            swprintf(buffer, 256, L"    CoreMask - %016I64x", procInfo->ProcessorMask);
            procLog.Log(L"Cache");
            switch (procInfo->Cache.Type)
            {
            case CacheUnified:
                procLog.Log(L"  Unified");
                break;
            case CacheInstruction:
                procLog.Log(L"  Instruction");
                break;
            case CacheData:
                procLog.Log(L"  Data");
                break;
            case CacheTrace:
                procLog.Log(L"  Trace");
                break;
            }
            swprintf(buffer, 256, L"  Level - %hhd", procInfo->Cache.Level);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  Associativity - %hhd", procInfo->Cache.Associativity);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  Line Size - %hd", procInfo->Cache.LineSize);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  Cache Size - %d", procInfo->Cache.Size);
            procLog.Log(buffer);
            swprintf(buffer, 256, L"  CoreMask - %016I64x", procInfo->ProcessorMask);
            procLog.Log(buffer);
            break;
        case RelationNumaNode:
            procLog.Log(L"Numa Node");
            swprintf(buffer, 256, L"  Numa Number - %d", procInfo->NumaNode.NodeNumber);
            procLog.Log(buffer);
            break;
        case RelationProcessorCore:
            procLog.Log(L"Processor Core");
            if (procInfo->ProcessorCore.Flags == 1)
                procLog.Log(L"  Core is hyper threaded");
            swprintf(buffer, 256, L"    CoreMask - %016I64x", procInfo->ProcessorMask);
            procLog.Log(buffer);
            break;
        case RelationProcessorPackage:
            procLog.Log(L"Processor Package");
            swprintf(buffer, 256, L"    CoreMask - %016I64x", procInfo->ProcessorMask);
            procLog.Log(buffer);
            break;
        default:
            break;
        }

        dataLeft -= sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        data = data + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        entries++;
    }
#endif
}

void ATG::LogCoreTests(const std::wstring& suffix, const std::wstring& logName)
{
    wchar_t buffer[256];
    std::wstring fileName;
    if (logName.size() != 0)
    {
        fileName = logName;
    }
    else
    {
        swprintf_s(buffer, 256, L"%s_coreTests_%s", g_trueProcessorName.c_str(), suffix.c_str());
        fileName = buffer;
    }
    ATG::FileLogger testLog(fileName, false);
    size_t numTests = g_representativeCoreTests.size();
    for (size_t i = 0; i < numTests; i++)
    {
        swprintf(buffer, 256, L"%016I64x: %s", g_representativeCoreTests[i], g_representativeCoreTestNames[i].c_str());
        testLog.Log(buffer);
    }
}
#endif

void ATG::GenerateRepresentiveCoreTests(bool cacheBased)
{
    g_representativeCoreTests.clear();
    g_representativeCoreTestNames.clear();

    if (cacheBased)
    {
        InternalGenerateRepresentiveQueueTests(0, g_representativeCoreTests, g_representativeCoreTestNames);
    }
    else
    {
        InternalGenerateRepresentiveLayoutTests(0, g_representativeCoreTests, g_representativeCoreTestNames);
    }
}

namespace
{
    void InternalGenerateRepresentiveQueueTests(ATG::ProcessorGroupID groupID, std::vector<uint64_t>& coreTests, std::vector<std::wstring>& testNames)
    {
        coreTests.clear();
        testNames.clear();
        std::set<uint64_t> configurationDone;

        std::set<uint64_t> cpuSets;
        std::set<uint64_t> cpuHighCacheSets;	// share the highest cache
        std::set<uint64_t> cpuLowCacheSets;		// share the lowest cache
        for (const auto& iter : g_caches)
        {
            if (iter.coreMask.groupID != groupID)
                continue;
            if ((iter.cacheType != ATG::CacheType::CacheUnified) && (iter.cacheType != ATG::CacheType::CacheData))
                continue;
            if (cpuSets.find(iter.coreMask.coreMask) != cpuSets.end())
                continue;
            cpuSets.insert(iter.coreMask.coreMask);
        }

        uint64_t maxHighCacheCores = 0;
        uint64_t minLowCacheCores = UINT64_MAX;
        uint64_t allCoresMask(0);
        for (const auto& iter : cpuSets)
        {
            const uint64_t curCoreCount = __popcnt64(iter);
            if (curCoreCount > maxHighCacheCores)
                maxHighCacheCores = curCoreCount;
            if (curCoreCount < minLowCacheCores)
                minLowCacheCores = curCoreCount;
            allCoresMask |= iter;
        }
        if (maxHighCacheCores == minLowCacheCores)	// There is no shared cache between physical cores, just assume one large top level cache
        {
            maxHighCacheCores = ATG::GetNumLogicalCores(groupID);
            cpuHighCacheSets.insert(allCoresMask);
            cpuSets.insert(allCoresMask);
        }
        else
        {
            for (const auto& iter : cpuSets)
            {
                const uint64_t curCoreCount = __popcnt64(iter);
                if (curCoreCount == minLowCacheCores)
                    cpuLowCacheSets.insert(iter);
                else if (curCoreCount == maxHighCacheCores)
                    cpuHighCacheSets.insert(iter);
            }
        }

        wchar_t tempBuffer[256];

        // all cores
        configurationDone.insert(allCoresMask);
        coreTests.push_back(allCoresMask);		// do everything
        testNames.push_back(L"all_cores");

        // one logical per physical
        if (ATG::IsHyperThreaded())
        {
            uint64_t oneLogicalMask = 0;
            for (const auto& iter : cpuSets)
            {
                const uint64_t curCoreCount = __popcnt64(iter);
                if (curCoreCount == 2)
                {
                    DWORD lowBitIndex;
                    _BitScanForward64(&lowBitIndex, iter);
                    oneLogicalMask |= (1ULL << lowBitIndex);
                }
            }
            configurationDone.insert(oneLogicalMask);
            coreTests.push_back(oneLogicalMask);
            testNames.push_back(L"one_logical_per_physical");
        }

        // single logical core, one per cluster
        {
            for (const auto& iter : cpuSets)
            {
                const uint64_t curCoreCount = __popcnt64(iter);
                if (curCoreCount > minLowCacheCores)				// this must be a cluster
                {
                    DWORD lowBitIndex;
                    _BitScanForward64(&lowBitIndex, iter);
                    if (configurationDone.find(1ULL << lowBitIndex) != configurationDone.end())		// kind of slow, but for this init code who really cares
                        continue;
                    configurationDone.insert(1ULL << lowBitIndex);
                    coreTests.push_back(1ULL << lowBitIndex);
                    swprintf_s(tempBuffer, 256, L"logical_%x", lowBitIndex);
                    testNames.push_back(tempBuffer);
                }
            }
        }

        // single physical core, one per cluster
        if (minLowCacheCores > 1)		// this means hyperthreaded
        {
            for (const auto& cluster : cpuSets)
            {
                const uint64_t curCoreCount = __popcnt64(cluster);
                if (curCoreCount > minLowCacheCores)		// this must be a cluster, more than one physical core
                {
                    for (const auto& group : cpuSets)
                    {
                        if ((cluster & group) != 0)
                        {
                            if (configurationDone.find(group) != configurationDone.end())
                                continue;
                            configurationDone.insert(group);
                            coreTests.push_back(group);
                            swprintf_s(tempBuffer, 256, L"physical_%llx", group);
                            testNames.push_back(tempBuffer);
                            break;
                        }
                    }
                }
            }

            // all cores in a single cluster, one logical in each physical
            for (const auto& cluster : cpuSets)
            {
                const uint64_t curCoreCount = __popcnt64(cluster);
                if (curCoreCount > minLowCacheCores)		// this must be a cluster, more than one physical core
                {
                    uint64_t coreMask(0);
                    for (const auto& group : cpuSets)
                    {
                        const uint64_t innerCoreCount = __popcnt64(group);
                        if (innerCoreCount > minLowCacheCores)		// this must be a cluster, more than one physical core
                        {
                            if ((cluster & group) != 0)
                            {
                                DWORD lowBitIndex;
                                _BitScanForward64(&lowBitIndex, group);
                                coreMask |= 1ULL << lowBitIndex;
                            }
                        }
                    }
                    if (configurationDone.find(coreMask) != configurationDone.end())
                        continue;
                    configurationDone.insert(coreMask);
                    coreTests.push_back(coreMask);
                    swprintf_s(tempBuffer, 256, L"cluster_%llx_one_physical", cluster);
                    testNames.push_back(tempBuffer);
                }
            }
        }

        // all cores in a single cluster
        {
            for (const auto& outerCluster : cpuSets)
            {
                const uint64_t outerCoreCount = __popcnt64(outerCluster);
                if (outerCoreCount > minLowCacheCores)		// this must be a cluster, more than one physical core
                {
                    if (configurationDone.find(outerCluster) != configurationDone.end())		// kind of slow, but for this init code who really cares
                        continue;
                    configurationDone.insert(outerCluster);
                    coreTests.push_back(outerCluster);
                    swprintf_s(tempBuffer, 256, L"cluster_%llx", outerCluster);
                    testNames.push_back(tempBuffer);
                }
            }
        }

        // two cores in a single cluster
        {
            for (const auto& outerCluster : cpuSets)
            {
                const uint64_t outerCoreCount = __popcnt64(outerCluster);
                if (outerCoreCount > minLowCacheCores)		// this must be a cluster, more than one physical core
                {
                    DWORD lowBitIndex, highBitIndex;
                    _BitScanForward64(&lowBitIndex, outerCluster);
                    _BitScanReverse64(&highBitIndex, outerCluster);
                    if (lowBitIndex == highBitIndex)
                        continue;
                    const uint64_t coreMask = (1ULL << lowBitIndex) + (1ULL << highBitIndex);

                    if (configurationDone.find(coreMask) != configurationDone.end())		// kind of slow, but for this init code who really cares
                        continue;
                    configurationDone.insert(coreMask);
                    coreTests.push_back(coreMask);
                    swprintf_s(tempBuffer, 256, L"single_cluster_%llx", coreMask);
                    testNames.push_back(tempBuffer);
                }
            }
        }

        // cross cluster tests, pick all cores, we do them all since on some processors the costs are not equal between clusters
        {
            for (const auto& outerCluster : cpuSets)
            {
                const uint64_t outerCoreCount = __popcnt64(outerCluster);
                if (outerCoreCount > minLowCacheCores)		// this must be a cluster, more than one physical core
                {
                    for (const auto& innerCluster : cpuSets)
                    {
                        const uint64_t innerCoreCount = __popcnt64(innerCluster);
                        if (innerCoreCount > minLowCacheCores)		// this must be a cluster, more than one physical core
                        {
                            if (innerCluster & outerCluster)
                                continue;
                            const uint64_t mask = outerCluster | innerCluster;
                            if (configurationDone.find(mask) != configurationDone.end())		// kind of slow, but for this init code who really cares
                                continue;
                            configurationDone.insert(mask);
                            coreTests.push_back(mask);
                            swprintf_s(tempBuffer, 256, L"cross_cluster_%llx", outerCluster | innerCluster);
                            testNames.push_back(tempBuffer);
                        }
                    }
                }
            }
        }

        // cross cluster tests, pick one physical core even if hyperthreaded
        {
            for (const auto& outerCluster : cpuSets)
            {
                const uint64_t outerCoreCount = __popcnt64(outerCluster);
                if (outerCoreCount > minLowCacheCores)		// this must be a cluster, more than one physical core
                {
                    for (const auto& innerCluster : cpuSets)
                    {
                        const uint64_t innerCoreCount = __popcnt64(innerCluster);
                        if (innerCoreCount > minLowCacheCores)		// this must be a cluster, more than one physical core
                        {
                            if (innerCluster & outerCluster)
                                continue;
                            uint64_t outerGroup(UINT64_MAX), innerGroup(UINT64_MAX);
                            for (const auto& group : cpuSets)
                            {
                                if (((outerCluster & group) != 0) && (outerGroup == UINT64_MAX))
                                    outerGroup = group;
                                if (((innerCluster & group) != 0) && (innerGroup == UINT64_MAX))
                                    innerGroup = group;
                            }
                            if (configurationDone.find(innerGroup | outerGroup) != configurationDone.end())		// kind of slow, but for this init code who really cares
                                continue;
                            configurationDone.insert(innerGroup | outerGroup);
                            coreTests.push_back(innerGroup | outerGroup);
                            swprintf_s(tempBuffer, 256, L"cross_physical_%llx", outerGroup | innerGroup);
                            testNames.push_back(tempBuffer);
                        }
                    }
                }
            }
        }
    }

    void InternalGenerateRepresentiveLayoutTests(ATG::ProcessorGroupID groupID, std::vector<uint64_t>& coreTests, std::vector<std::wstring>& testNames)
    {
        coreTests.clear();
        testNames.clear();

        constexpr uint32_t s_maxSingleCores = 4u;
        bool hyperThreaded = false;
        uint64_t allCoresMask(0);
        wchar_t tempBuffer[256];
        for (const auto& iter : g_physicalCores[groupID])
        {
            allCoresMask |= iter.second;
            if (iter.first)
                hyperThreaded = true;
        }
        coreTests.push_back(allCoresMask);		// do everything
        testNames.push_back(L"all_cores");
        // physical cores
        {
            const uint32_t numSingle = std::min(ATG::GetNumPhysicalCores(groupID), s_maxSingleCores);
            const uint32_t coreDelta = std::max<uint32_t>(hyperThreaded ? 2U : 1U, static_cast<uint32_t> ((((float)ATG::GetTotalNumCores(groupID)) / numSingle) + 0.5f));	// assume hyperthreaded are only 2 cores per physical
            uint64_t curMask = 0;
            for (uint32_t i = 0; i < numSingle; i++, curMask += coreDelta)
            {
                coreTests.push_back(1ULL << curMask);
                swprintf_s(tempBuffer, 256, L"physical_0x%llx", 1ULL << curMask);
                testNames.push_back(tempBuffer);
            }
        }
        if (hyperThreaded)
        {
            const uint32_t numSingle = std::min(ATG::GetNumPhysicalCores(groupID), s_maxSingleCores);
            const uint32_t coreDelta = std::max<uint32_t>(2, ATG::GetTotalNumCores(groupID) / numSingle);	// assume hyperthreaded are only 2 cores per physical. Code below will set the mask correctly
            uint64_t curMask = 0;
            for (uint32_t i = 0; i < numSingle; i++, curMask += coreDelta)
            {
                const uint64_t logicalMask = ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << curMask, groupID));
                coreTests.push_back(logicalMask);
                swprintf_s(tempBuffer, 256, L"logical_0x%llx", logicalMask);
                testNames.push_back(tempBuffer);
            }
        }
        if (g_dieMask.size() > 1)		// numa nodes will show up here
        {
            // all cores in a cluster
            uint32_t curCluster = 0;
            for (const auto& iter : g_dieMask)
            {
                if (iter.second.groupID != groupID)
                    continue;
                coreTests.push_back(iter.second.coreMask);
                swprintf_s(tempBuffer, 256, L"full_cluster_0x%llx", iter.second.coreMask);
                testNames.push_back(tempBuffer);
                curCluster++;
            }

            // two cores in a cluster
            curCluster = 0;
            for (const auto& iter : g_dieMask)
            {
                if (iter.second.groupID != groupID)
                    continue;
                DWORD lowBitIndex, highBitIndex;
                _BitScanForward64(&lowBitIndex, iter.second.coreMask);
                _BitScanReverse64(&highBitIndex, iter.second.coreMask);

                coreTests.push_back((1ULL << lowBitIndex) | (1ULL << highBitIndex));
                swprintf_s(tempBuffer, 256, L"within_cluster_physical_0x%llx", (1ULL << lowBitIndex) | (1ULL << highBitIndex));
                testNames.push_back(tempBuffer);

                if (hyperThreaded)
                {
                    coreTests.push_back(ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << lowBitIndex, groupID)) | ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << highBitIndex, groupID)));
                    swprintf_s(tempBuffer, 256, L"within_cluster_hyper_0x%llx", ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << lowBitIndex, groupID)) | ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << highBitIndex, groupID)));
                    testNames.push_back(tempBuffer);
                }

                curCluster++;
            }

            // one physical on all combinations of 2 clusters
            std::map<uint32_t, ATG::UniqueProcessorMask>::const_iterator endIter = g_dieMask.end();
            for (std::map<uint32_t, ATG::UniqueProcessorMask>::const_iterator outer = g_dieMask.begin(); outer != endIter; ++outer)
            {
                if (outer->second.groupID != groupID)
                    continue;
                std::map<uint32_t, ATG::UniqueProcessorMask>::const_iterator startInnerIter = outer;
                startInnerIter++;
                for (std::map<uint32_t, ATG::UniqueProcessorMask>::const_iterator inner = startInnerIter; inner != endIter; ++inner)
                {
                    if (inner->second.groupID != groupID)
                        continue;
                    if (outer->first == inner->first)
                        continue;

                    swprintf_s(tempBuffer, 256, L"cross_cluster_0x%llx", outer->second.coreMask | inner->second.coreMask);
                    testNames.push_back(tempBuffer);
                    coreTests.push_back(outer->second.coreMask | inner->second.coreMask);

                    DWORD innerBitIndex, outerBitIndex;
                    _BitScanForward64(&outerBitIndex, outer->second.coreMask);
                    _BitScanForward64(&innerBitIndex, inner->second.coreMask);

                    if (hyperThreaded)
                    {
                        coreTests.push_back(ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << outerBitIndex, groupID)) | ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << innerBitIndex, groupID)));
                        swprintf_s(tempBuffer, 256, L"cross_cluster_logical_0x%llx", ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << outerBitIndex, groupID)) | ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << innerBitIndex, groupID)));
                        testNames.push_back(tempBuffer);
                    }

                    coreTests.push_back((1ULL << outerBitIndex) | (1ULL << innerBitIndex));
                    swprintf_s(tempBuffer, 256, L"cross_cluster_physical_0x%llx", (1ULL << outerBitIndex) | (1ULL << innerBitIndex));
                    testNames.push_back(tempBuffer);
                }
            }
        }
        else
        {
            // half the cores in a single
            const uint32_t halfLogical = ATG::GetNumLogicalCores(groupID) / 2;
            uint64_t lowMask(0), highMask(0);
            for (uint32_t i = 0; i < halfLogical; i += hyperThreaded ? 2 : 1)
            {
                lowMask |= ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << i, groupID));
                highMask |= ATG::GetLogicalProcessorMask(ATG::UniqueProcessorMask(1ULL << (halfLogical + i), groupID));
            }
            coreTests.push_back(lowMask);
            swprintf_s(tempBuffer, 256, L"lowCores_0x%llx", lowMask);
            testNames.push_back(tempBuffer);

            coreTests.push_back(highMask);
            swprintf_s(tempBuffer, 256, L"highCores_0x%llx", highMask);
            testNames.push_back(tempBuffer);

            // two physical cores
            if (hyperThreaded && (ATG::GetNumPhysicalCores(groupID) >= 4))	// just assume 2 cores in a hyperthread and at least 4 physical cores
            {
                coreTests.push_back(0x05);
                swprintf_s(tempBuffer, 256, L"first two physical 0x05");
                testNames.push_back(tempBuffer);

                coreTests.push_back(0xA0);
                swprintf_s(tempBuffer, 256, L"second two physical 0xA0");
                testNames.push_back(tempBuffer);
            }
            // handle 2 physical hyperthreaded cores
            if (hyperThreaded && (ATG::GetNumPhysicalCores(groupID) == 2))
            {
                coreTests.push_back(0x05);
                swprintf_s(tempBuffer, 256, L"Two physical_0x05");
                testNames.push_back(tempBuffer);
            }
        }
    }

    void CalcRDTSCPFrequency()
    {
        constexpr uint32_t msDuration = 1000u;

        LARGE_INTEGER qpcTempRate;
        QueryPerformanceFrequency(&qpcTempRate);
        const double qpcRate = (double)qpcTempRate.QuadPart;
        double rdtscFrequencyIdle;
        double rdtscFrequencyLoad;
        uint32_t tempAux;

        const int32_t currentPriority = GetThreadPriority(GetCurrentThread());
        ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        const uint64_t oldAffinityMask = ::SetThreadAffinityMask(GetCurrentThread(), 0x04);		// don't use the first core, don't really care if this fails the code still works, this is just here as an extra forcing value just in case

        // Measure __rdtscp() when the machine is mostly idle
        {
            const uint64_t rdtscStart = __rdtscp(&tempAux);
            QueryPerformanceCounter(&qpcTempRate);
            const int64_t qpcStart = qpcTempRate.QuadPart;
            Sleep(msDuration);
            const uint64_t rdtscElapsed = __rdtscp(&tempAux) - rdtscStart;
            QueryPerformanceCounter(&qpcTempRate);
            const int64_t qpcElapsed = qpcTempRate.QuadPart - qpcStart;
            rdtscFrequencyIdle = rdtscElapsed / (qpcElapsed / qpcRate);
        }

        // Measure __rdtscp() when the machine is busy
        {
            const uint64_t rdtscStart = __rdtscp(&tempAux);
            QueryPerformanceCounter(&qpcTempRate);
            const int64_t qpcStart = qpcTempRate.QuadPart;
            const uint64_t startTick = GetTickCount64();
            for (;;)
            {
                const uint64_t tickDuration = GetTickCount64() - startTick;
                if (tickDuration >= msDuration)
                    break;
            }
            const uint64_t rdtscElapsed = __rdtscp(&tempAux) - rdtscStart;
            QueryPerformanceCounter(&qpcTempRate);
            const int64_t qpcElapsed = qpcTempRate.QuadPart - qpcStart;
            rdtscFrequencyLoad = rdtscElapsed / (qpcElapsed / qpcRate);
        }
        if (oldAffinityMask != 0)
            ::SetThreadAffinityMask(GetCurrentThread(), oldAffinityMask);
        ::SetThreadPriority(GetCurrentThread(), currentPriority);
        g_rdtscpFrequencySeconds = (rdtscFrequencyIdle + rdtscFrequencyLoad) / 2.0;		// the numbers should be close to identical so just average
    }

    void ParseOSProcessorInfo()
    {
        uint32_t topLevelCache = 0;
        uint16_t groupCount = 0;

#if defined(_XBOX_ONE)
        // this is the actual hardware. The available core mask takes care of masking out invalid cores for testing
        groupCount = 1;
        g_numLogicalCores.resize(1, 0);
        g_availableCoresMask.resize(1, 0);

        g_numLogicalCores[0] = 8;
        g_availableCoresMask[0] = 0x7f;
        g_groupMask[0] = ATG::UniqueProcessorMask(0xff, 0);

        //g_packageMask.push_back(0xff);

        //g_numaNodeMask.push_back(std::pair<uint32_t, uint64_t>(0, 0xff));
        //g_dieMask.push_back(std::pair<uint32_t, uint32_t>(0, 0xff));

        //g_clusterMask.push_back(std::pair<uint32_t, uint32_t>(0, 0x0f));
        //g_clusterMask.push_back(std::pair<uint32_t, uint32_t>(1, 0xf0));
        g_dieMask[0] = ATG::UniqueProcessorMask(0xff, 0);

        g_physicalCores[0].push_back(std::pair<bool, uint64_t>(false, 0x01));
        g_physicalCores[0].push_back(std::pair<bool, uint64_t>(false, 0x02));
        g_physicalCores[0].push_back(std::pair<bool, uint64_t>(false, 0x04));
        g_physicalCores[0].push_back(std::pair<bool, uint64_t>(false, 0x08));
        g_physicalCores[0].push_back(std::pair<bool, uint64_t>(false, 0x10));
        g_physicalCores[0].push_back(std::pair<bool, uint64_t>(false, 0x20));
        g_physicalCores[0].push_back(std::pair<bool, uint64_t>(false, 0x40));
        g_physicalCores[0].push_back(std::pair<bool, uint64_t>(false, 0x80));

        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheUnified, 2, 64, 2097152, ATG::UniqueProcessorMask(0x0f, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheUnified, 2, 64, 2097152, ATG::UniqueProcessorMask(0xf0, 0)));

        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheData, 1, 64, 32768, ATG::UniqueProcessorMask(0x01, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheData, 1, 64, 32768, ATG::UniqueProcessorMask(0x02, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheData, 1, 64, 32768, ATG::UniqueProcessorMask(0x04, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheData, 1, 64, 32768, ATG::UniqueProcessorMask(0x08, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheData, 1, 64, 32768, ATG::UniqueProcessorMask(0x10, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheData, 1, 64, 32768, ATG::UniqueProcessorMask(0x20, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheData, 1, 64, 32768, ATG::UniqueProcessorMask(0x40, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheData, 1, 64, 32768, ATG::UniqueProcessorMask(0x80, 0)));

        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheInstruction, 1, 64, 32768, ATG::UniqueProcessorMask(0x01, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheInstruction, 1, 64, 32768, ATG::UniqueProcessorMask(0x02, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheInstruction, 1, 64, 32768, ATG::UniqueProcessorMask(0x04, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheInstruction, 1, 64, 32768, ATG::UniqueProcessorMask(0x08, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheInstruction, 1, 64, 32768, ATG::UniqueProcessorMask(0x10, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheInstruction, 1, 64, 32768, ATG::UniqueProcessorMask(0x20, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheInstruction, 1, 64, 32768, ATG::UniqueProcessorMask(0x40, 0)));
        g_caches.push_back(ATG::CacheInformation(ATG::CacheType::CacheInstruction, 1, 64, 32768, ATG::UniqueProcessorMask(0x80, 0)));
        topLevelCache = 2;
#elif defined (_GAMING_XBOX)
        groupCount = 1;
        g_numLogicalCores.resize(1, 0);
        g_availableCoresMask.resize(1, 0);

        SYSTEM_LOGICAL_PROCESSOR_INFORMATION* procInfo;
        char* data = g_osProcessorInfo.rawData;
        size_t dataLeft = g_osProcessorInfo.bufferSize;

        while (dataLeft)
        {
            procInfo = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION*> (data);

            switch (procInfo->Relationship)
            {
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
                g_caches.push_back(ATG::CacheInformation(cacheType, procInfo->Cache.Level, procInfo->Cache.LineSize, procInfo->Cache.Size, ATG::UniqueProcessorMask(procInfo->ProcessorMask, 0)));
            }
            break;
            case RelationNumaNode:
                g_numaNodeMask[procInfo->NumaNode.NodeNumber] = ATG::UniqueProcessorMask(procInfo->ProcessorMask, 0);
                break;
            case RelationProcessorCore:
                g_physicalCores[0].push_back(std::pair<bool, uint64_t>(procInfo->ProcessorCore.Flags == 1, procInfo->ProcessorMask));
                g_numLogicalCores[0] += static_cast<uint32_t>(__popcnt64(procInfo->ProcessorMask));
                break;
            case RelationProcessorPackage:
                g_dieMask[0].coreMask |= procInfo->ProcessorMask;
                g_availableCoresMask[0] |= procInfo->ProcessorMask;
                break;
            default:
                break;
            }

            dataLeft -= sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            data = data + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        }
#else       // not _GAMING_XBOX
        SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX* procInfo = g_osProcessorInfo.procInfo;
        char* data = g_osProcessorInfo.rawData;

        size_t dataLeft = g_osProcessorInfo.bufferSize;
        while (dataLeft)
        {
            procInfo = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*> (data);
            if (procInfo->Relationship == RelationGroup)
            {
                groupCount = std::max(procInfo->Group.MaximumGroupCount, groupCount);
                for (ATG::ProcessorGroupID groupID = 0; groupID < procInfo->Group.MaximumGroupCount; groupID++)
                {
                    g_groupMask[groupID] = ATG::UniqueProcessorMask(procInfo->Group.GroupInfo[groupID].ActiveProcessorMask, groupID);
                }
            }

            dataLeft -= procInfo->Size;
            data = data + procInfo->Size;
        }

        g_numLogicalCores.resize(groupCount, 0);
        g_availableCoresMask.resize(groupCount, 0);

        uint32_t entries = 0;
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
                    g_dieMask[procInfo->Processor.GroupMask[i].Group].coreMask |= procInfo->Processor.GroupMask[i].Mask;
                    g_availableCoresMask[procInfo->Processor.GroupMask[i].Group] |= procInfo->Processor.GroupMask[i].Mask;
                }
                break;
            case RelationProcessorCore:
                for (uint32_t i = 0; i < procInfo->Processor.GroupCount; i++)
                {
                    g_physicalCores[procInfo->Processor.GroupMask[i].Group].push_back(std::pair<bool, uint64_t>(procInfo->Processor.Flags == LTP_PC_SMT, procInfo->Processor.GroupMask[i].Mask));
                    g_numLogicalCores[procInfo->Processor.GroupMask[i].Group] += static_cast<uint32_t>(__popcnt64(procInfo->Processor.GroupMask[i].Mask));
                }
                break;
            case RelationNumaNode:
                g_numaNodeMask[procInfo->NumaNode.NodeNumber] = ATG::UniqueProcessorMask(procInfo->NumaNode.GroupMask.Mask, procInfo->NumaNode.GroupMask.Group);
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
                g_caches.push_back(ATG::CacheInformation(cacheType, procInfo->Cache.Level, procInfo->Cache.LineSize, procInfo->Cache.CacheSize, ATG::UniqueProcessorMask(procInfo->Cache.GroupMask.Mask, procInfo->Cache.GroupMask.Group)));
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
            entries++;
        }
#endif

        // build up top level cache list based on top level cache
        {
            for (auto iter = g_caches.begin(); iter != g_caches.end(); ++iter)
            {
                if (iter->level == topLevelCache)
                {
                    g_topLevelCacheMask.push_back(ATG::UniqueProcessorMask(iter->coreMask.coreMask, iter->coreMask.groupID));
                }
            }
        }

        // get rid of the data that doesn't match to one of the available cores in the system
        // If there are more than 1 group the process is on their own to handle that case
        if (groupCount == 1)
        {
            g_availableCoresMask[0] &= g_processMask;
            g_numLogicalCores[0] = 0;

            {
                for (auto iter = g_dieMask.begin(); iter != g_dieMask.end(); )
                {
                    iter->second.coreMask &= g_availableCoresMask[0];
                    if (iter->second.coreMask == 0)
                        iter = g_dieMask.erase(iter);
                    else
                        ++iter;
                }
            }

            {
                for (const auto& iter : g_dieMask)
                {
                    g_numLogicalCores[0] += static_cast<uint32_t> (__popcnt64(iter.second.coreMask));
                }
            }

            {
                for (auto iter = g_topLevelCacheMask.begin(); iter != g_topLevelCacheMask.end(); )
                {
                    iter->coreMask &= g_availableCoresMask[0];
                    if (iter->coreMask == 0)
                        iter = g_topLevelCacheMask.erase(iter);
                    else
                        ++iter;
                }
            }
            {
                for (auto iter = g_physicalCores[0].begin(); iter != g_physicalCores[0].end(); )
                {
                    iter->second &= g_availableCoresMask[0];
                    if (iter->second == 0)
                        iter = g_physicalCores[0].erase(iter);
                    else
                        ++iter;
                }
            }
            {
                for (auto iter = g_caches.begin(); iter != g_caches.end(); )
                {
                    iter->coreMask.coreMask &= g_availableCoresMask[0];
                    if (iter->coreMask.coreMask == 0)
                        iter = g_caches.erase(iter);
                    else
                        ++iter;
                }
            }
        }
    }
}
