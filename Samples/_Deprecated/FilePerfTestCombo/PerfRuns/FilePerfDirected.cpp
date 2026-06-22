//--------------------------------------------------------------------------------------
// FilePerfDirected.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FileLogger.h"
#include "File.h"
#include "RDTSCPStopWatch.h"
#include "FilePerfTestCombo.h"
#include "PackedFile.h"
#include "LooseFiles.h"
#include "CapturedReadDataXML.h"
#include "FlatPackage.h"
#include "Processor.h"

#define VALIDATE_READS

#if !defined (_GAMING_XBOX)                     // Xbox One Family have a rotational drive, it's pointless to test a super high QD/I
//#define HAMMER_QDI 16768                      // if this define is enabled the system will attempt to use this really high QD/I
                                                // A QD/I of 128 is converted to 8192. A QD/I > 128 will use the value of the define
#endif

#if defined (_GAMING_XBOX_XBOX_ONE)             // Xbox One Family have a rotational drive, so read a less to keep the test from running long
#define MAX_BYTES_READ (1ULL*1024*1024*1024 )
#else
#define MAX_BYTES_READ (2ULL*1024*1024*1024 )
#endif
#define MAX_READS (UINT32_MAX)                  // clamp the number of reads between the number in the dataset and this number
//#define USE_COMPLETION_PORTS
//#define USE_EVENTS

#if defined (USE_COMPLETION_PORTS) && defined (USE_EVENTS)
#undef USE_COMPLETION_PORTS
#undef USE_EVENTS
#endif

using namespace ATG;

namespace
{
    std::atomic<uint32_t> g_currentlyInFlight = 0;
    std::atomic<uint64_t> g_totalDataSizeRead = 0;
    std::atomic<uint64_t> g_totalReadThisIteration = 0;
    uint32_t g_allowedInFlight = 1;

    void AsyncCallbackFunction(HRESULT, FileSystem::File::AsyncRequestId /*requestId*/, uint64_t /*userData*/)
    {
        assert(g_currentlyInFlight >= 1);
        g_currentlyInFlight--;
    }
}

bool RunDirectedTest(const std::wstring& layoutFileName, const std::wstring& readSetFileName, uint32_t numIterations, const std::wstring& packFileName, const std::wstring& rootDirName, const std::wstring& dataOrder, uint32_t dataSize, bool forceAsync, uint32_t qdiOverrideLow, uint32_t qdiOverrideHigh)
{
    static uint32_t runIteration[2] = { 0,0 };		    // one for sync, one for async
    ATG::RDTSCPStopWatch timer;
    volatile double totalTimeSecs = 0;

    std::unique_ptr<Packages::FlatPackage> layoutSet;
    layoutSet.reset(new Packages::FlatPackage());
    if (!layoutSet->ReadDataFile(layoutFileName, L""))
    {
        assert(false);
        return false;
    }
    std::unique_ptr<CapturedReadData> capturedReadSet;
    capturedReadSet.reset(new CapturedReadDataXML());
    if (!capturedReadSet->ReadSingleDataSetFile(readSetFileName, L""))
    {
        assert(false);
        return false;
    }
    FullReadSet& fullReadSet = capturedReadSet->GetCapturedSet("");

    if ((qdiOverrideLow == UINT32_MAX) && (qdiOverrideHigh == UINT32_MAX))
    {
        qdiOverrideLow = qdiOverrideHigh = fullReadSet.GetReadSetMaxQDI();
    }
    else if (qdiOverrideLow == UINT32_MAX)
    {
        qdiOverrideLow = c_MinQDIDepth;
    }
    else if (qdiOverrideHigh == UINT32_MAX)
    {
        qdiOverrideHigh = c_MaxQDIDepth;
    }
    bool doingSyncTestRun = (qdiOverrideLow == 0) && (qdiOverrideHigh == 0) && !forceAsync;
#ifdef USE_COMPLETION_PORTS
    FileSystem::File::InitFileSystem(true, 0);
#elif defined (USE_EVENTS)
    FileSystem::File::InitFileSystem(false, ConvertOverlapDepthToValue(qdiOverrideHigh));
#else
    FileSystem::File::InitFileSystem(false, 0);
#endif

    std::unique_ptr <BaseFileInterface> fileInterface(nullptr);
    if (packFileName.size())
    {
        fileInterface = std::make_unique<PackedFile>();
        uint64_t platformHandle;
        fileInterface->OpenPackFile(packFileName, doingSyncTestRun ? 0 : FileSystem::File::e_openFlag_EnableAsync, platformHandle, nullptr);
    }
    else
    {
        fileInterface = std::make_unique<LooseFiles>();
        fileInterface->SetDirectoryOffset(rootDirName);
    }

    uint32_t blockReadSize = 0;

    std::wstring allUpTimingLogName;
    allUpTimingLogName = doingSyncTestRun ? L"Sync_" : L"ASync_";

    allUpTimingLogName += ATG::GetProcessorName();

#ifdef USE_COMPLETION_PORTS
    allUpTimingLogName += L"_ports";
#elif defined (USE_EVENTS)
    allUpTimingLogName += L"_events";
#endif

    allUpTimingLogName += L".csv";

    FileLogger allUptimingLog(allUpTimingLogName, runIteration[doingSyncTestRun ? 0 : 1] != 0, false, false, false, true);
    if (runIteration[doingSyncTestRun ? 0 : 1] == 0)
    {
        if (!doingSyncTestRun)
            allUptimingLog.Log(L"Name,Order,Size,QD/I,MB/s,Times");
        else
            allUptimingLog.Log(L"Name,Order,Size,MB/s,Times");
    }
    for (uint32_t curQDIDepth = qdiOverrideLow; curQDIDepth <= qdiOverrideHigh; curQDIDepth++)
    {
        g_totalDataSizeRead = 0;
        totalTimeSecs = 0;
        Sample::GetInstance()->SetWorkingOverlapDepth(curQDIDepth);

        std::wstring allUpTimingsHeader(fullReadSet.GetName());
        if (!doingSyncTestRun)
        {
            wchar_t buf[128];
            swprintf_s(buf, 128, L"_d%d", ConvertOverlapDepthToValue(curQDIDepth));
            allUpTimingsHeader += buf;
            swprintf_s(buf, 128, L",%s,%d,%d", dataOrder.c_str(), dataSize, ConvertOverlapDepthToValue(curQDIDepth));
            allUpTimingsHeader += buf;
        }
        else
        {
            wchar_t buf[128];
            swprintf_s(buf, 128, L",%s,%d", dataOrder.c_str(), dataSize);
            allUpTimingsHeader += buf;
        }
        std::wstring allUpTimingsTimes;
        {
            for (uint32_t curIteration = 0; curIteration < numIterations; ++curIteration)
            {
                size_t curReadSetIndex = std::rand() % fullReadSet.GetNumCapturedSets();
                FullReadSet::ReadSet& curReadSet = fullReadSet.GetReadSet(curReadSetIndex);

                g_allowedInFlight = ConvertOverlapDepthToValue(curQDIDepth);
#ifdef HAMMER_QDI
                if (g_allowedInFlight == 128)
                    g_allowedInFlight = 8192;
                else if (g_allowedInFlight > 128)
                    g_allowedInFlight = HAMMER_QDI;
#endif

                capturedReadSet->OpenFiles(curReadSet.m_name, "", fileInterface.get(), doingSyncTestRun ? 0 : FileSystem::File::e_openFlag_EnableAsync);

                g_totalReadThisIteration = 0;

                auto endIter = curReadSet.m_reads.end();
                for (auto curRead = curReadSet.m_reads.begin(); curRead != endIter; ++curRead)
                {
                    delete[] curRead->dataBuffer;
                    curRead->dataBuffer = (char*)VirtualAlloc(nullptr, curRead->size, MEM_COMMIT, PAGE_READWRITE);
                    memset(curRead->dataBuffer, 0x17, curRead->size);
                }

                g_currentlyInFlight = 0;
                uint32_t currentReadIndex = 0;
                timer.Start();
                for (auto curRead = curReadSet.m_reads.begin(); curRead != endIter; )
                {
                    if (!doingSyncTestRun)
                    {
                        while (g_currentlyInFlight >= g_allowedInFlight)
                        {
#ifdef USE_COMPLETION_PORTS
                            FileSystem::File::WaitCompletionPortRead(true);
#elif defined (USE_EVENTS)
                            FileSystem::File::WaitAnyAsyncRead(true);
#else
                            FileSystem::File::CheckAnyAsyncRead();
#endif
                        }
                    }
                    if (!curRead->dataBuffer)
                        continue;
                    blockReadSize = curRead->size;
                    if (doingSyncTestRun)
                    {
                        fileInterface->SetReadPointer(curRead->fileHandle, curRead->location);
                        HRESULT errorCode = fileInterface->ReadFile(curRead->fileHandle, curRead->dataBuffer, curRead->size);
                        if (!SUCCEEDED(errorCode))
                            DebugBreak();
                        g_totalDataSizeRead += curRead->size;
                        g_totalReadThisIteration += curRead->size;
                        currentReadIndex++;
                        if (g_totalReadThisIteration >= MAX_BYTES_READ)
                            curRead = endIter;
                        else
                            ++curRead;
                        if (currentReadIndex >= MAX_READS)
                            curRead = endIter;
                    }
                    else if (g_currentlyInFlight < g_allowedInFlight)
                    {
                        {
                            uint64_t requestID;
#ifdef USE_EVENTS
                            HRESULT errorCode = fileInterface->AsyncRead(curRead->fileHandle, curRead->dataBuffer, curRead->size, curRead->location, currentReadIndex, requestID, AsyncCallbackFunction, true);
#else
                            HRESULT errorCode = fileInterface->AsyncRead(curRead->fileHandle, curRead->dataBuffer, curRead->size, curRead->location, currentReadIndex, requestID, AsyncCallbackFunction, false);
#endif
                            if (!SUCCEEDED(errorCode))
                                DebugBreak();
                            if (requestID == UINT64_MAX)
                            {
                                DebugBreak();
                                g_allowedInFlight = g_currentlyInFlight < 101 ? 25 : g_currentlyInFlight - 100;
                            }
                            else
                            {
                                g_totalDataSizeRead += curRead->size;
                                g_totalReadThisIteration += curRead->size;
                                currentReadIndex++;
                                g_currentlyInFlight++;
                                if (g_totalReadThisIteration >= MAX_BYTES_READ)
                                    curRead = endIter;
                                else
                                    ++curRead;
                                if (currentReadIndex >= MAX_READS)
                                    curRead = endIter;
                            }
                        }
                    }
                }

                if (!doingSyncTestRun)
                {
                    while (g_currentlyInFlight != 0)
                    {
#ifdef USE_COMPLETION_PORTS
                        FileSystem::File::WaitCompletionPortRead(true);
#elif defined (USE_EVENTS)
                        FileSystem::File::WaitAnyAsyncRead(true);
#else
                        FileSystem::File::CheckAnyAsyncRead();
#endif
                    }
                }

                timer.Stop();

#ifdef VALIDATE_READS
                for (auto& curRead : curReadSet.m_reads)
                {
                    uint64_t startValue = curRead.location / sizeof(uint64_t);
                    uint64_t* temp = (uint64_t*)curRead.dataBuffer;
                    uint32_t numberEntries = curRead.size / sizeof(uint64_t);
                    for (uint32_t location = 0; location < numberEntries; location++)
                    {
                        if (temp[location] != startValue)
                            DebugBreak();
                        assert(temp[location] == startValue);
                        startValue++;
                    }
                }
#endif

                for (auto curRead = curReadSet.m_reads.begin(); curRead != curReadSet.m_reads.end(); ++curRead)
                {
                    VirtualFree(curRead->dataBuffer, 0, MEM_RELEASE);
                    curRead->dataBuffer = nullptr;
                }

                {
                    totalTimeSecs += timer.GetTotalSeconds();
                    wchar_t tempStr[32];
                    swprintf_s(tempStr, 32, L",%5.3f", timer.GetTotalMilliseconds());
                    allUpTimingsTimes += tempStr;
                }
                capturedReadSet->CloseFiles(fileInterface.get());
            }

            {
                wchar_t tempStr[128];

                double mbs = ((g_totalDataSizeRead) / (1000 * 1000)) / (totalTimeSecs);
                swprintf_s(tempStr, 32, L",%5.2f", mbs);

                std::wstring tempAllUp(allUpTimingsHeader);
                tempAllUp += tempStr;
                tempAllUp += allUpTimingsTimes;
                OutputDebugStringW(L"\n");
                OutputDebugStringW(tempAllUp.c_str());
                OutputDebugStringW(L"\n");
                allUptimingLog.Log(tempAllUp);
            }
        }
    }

    fileInterface->ClosePackFile();
    ++runIteration[doingSyncTestRun ? 0 : 1];

    FileSystem::File::ShutdownFileSystem();
    return true;
}
