//--------------------------------------------------------------------------------------
// FilePerfDirectedDStorage.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FileLogger.h"
#include "File.h"
#include "RDTSCPStopWatch.h"
#include "FilePerfTestCombo.h"
#include "PackedFileDStorage.h"
#include "LooseFilesDStorage.h"
#include "CapturedReadDataXML.h"
#include "FlatPackage.h"
#include "Processor.h"

#define VALIDATE_READS                      // Make sure the data read from the disk is the correct data
//#define USE_PHYSICAL_MEMORY_DESTINATION   // Whether to use the physical memory option as the destination
//#define IN_MEMORY_TEST                    // Whether to use memory-to-memory option with DirectStorage

#ifdef _GAMING_XBOX
#include <xmem.h>
#endif

using namespace ATG;

namespace
{
    std::unique_ptr <BaseFileInterfaceDStorage> fileInterface(nullptr);
}

size_t DoDStorageDirectedSyncTestRun(const FullReadSet::ReadSet& curReadSet, uint64_t& totalDataSizeRead, double& time, bool useRealtime)
{
    totalDataSizeRead = 0;
    time = 0;
    ATG::RDTSCPStopWatch timer;

    if (useRealtime)
        fileInterface->ResetQueues(DSTORAGE_MAX_QUEUE_CAPACITY, DSTORAGE_PRIORITY_REALTIME, 1);
    else
        fileInterface->ResetQueues(DSTORAGE_MAX_QUEUE_CAPACITY, DSTORAGE_PRIORITY_NORMAL, 1);

    Microsoft::WRL::ComPtr <IDStorageStatusArrayX> dStorageStatus;
    dStorageStatus = fileInterface->CreateStatusArray(1);
    auto endIter = curReadSet.m_reads.end();
    uint32_t currentReadIndex = 0;
    timer.Start();
    for (auto curRead = curReadSet.m_reads.begin(); curRead != endIter; ++curRead, ++currentReadIndex)
    {
        if (!curRead->dataBuffer)
            DebugBreak();
        HRESULT errorCode = fileInterface->EnqueueRequest(curRead->fileHandle, curRead->dataBuffer, curRead->size, curRead->location, curRead->platformFileHandle);
        if (!SUCCEEDED(errorCode))
            DebugBreak();
        fileInterface->EnqueueStatus(dStorageStatus.Get(), 0);
        fileInterface->Submit(0);
        while (!dStorageStatus->IsComplete(0))
        {
            _mm_pause();
        }
        totalDataSizeRead += curRead->size;
    }
    timer.Stop();
    time = timer.GetTotalSeconds();
    return currentReadIndex;
}

size_t DoDStorageDirectedAsyncTestRun(const FullReadSet::ReadSet& curReadSet, uint64_t& totalDataSizeRead, double& time, bool useRealtime)
{
    std::vector<Microsoft::WRL::ComPtr <IDStorageStatusArrayX>> dStorageStatus;
    std::vector<uint32_t> numBatchesPerQueue;
    std::vector<uint32_t> numReadsPerQueue;
    std::vector<uint32_t> curBatchIndex;
    ATG::RDTSCPStopWatch timer, delayTimer;
    static const uint32_t batchSize = 2048;
    totalDataSizeRead = 0;
    time = 0;
    ATG::RDTSCPStopWatch submitTimer;

    std::random_device m_randomDevice;
    std::default_random_engine m_randomEngine(m_randomDevice());
    std::uniform_int_distribution<uint32_t> timeDist(1, 100);

    uint32_t totalNumReads = static_cast<uint32_t> (curReadSet.m_reads.size());
    uint32_t totalNumQueueEntries = totalNumReads + (totalNumReads / batchSize) + 64;	// extra space for final status entry
    uint32_t numQueues = (totalNumQueueEntries / DSTORAGE_MAX_QUEUE_CAPACITY) + 1;
    if (numQueues > 60)
    {
        totalDataSizeRead = 1;
        time = 1;
        return 1;
    }
    numQueues = std::max<uint32_t>(std::min<uint32_t>(60, numQueues), 1);
    if (useRealtime)
        fileInterface->ResetQueues(DSTORAGE_MAX_QUEUE_CAPACITY, DSTORAGE_PRIORITY_REALTIME, numQueues);
    else
        fileInterface->ResetQueues(DSTORAGE_MAX_QUEUE_CAPACITY, DSTORAGE_PRIORITY_NORMAL, numQueues);

    for (uint32_t i = 0; i < numQueues; i++)
    {
        numReadsPerQueue.push_back(totalNumReads / numQueues);
        numBatchesPerQueue.push_back((totalNumReads / numQueues) / batchSize);
        curBatchIndex.push_back(1);
        dStorageStatus.push_back(fileInterface->CreateStatusArray(((totalNumReads / numQueues) / batchSize) + 5));		// add some buffer for rounding
    }

    auto curRead = curReadSet.m_reads.begin();
    uint32_t curQueueIndex = 0;
    timer.Start();
    for (uint32_t i = 0; i < totalNumReads; i++)
    {
#ifdef USE_PHYSICAL_MEMORY_DESTINATION
        HRESULT errorCode = fileInterface->EnqueueRequest(curRead->fileHandle, curRead->physicalPageArray, curRead->physicalOffset, curRead->size, curRead->location, curRead->platformFileHandle, curQueueIndex);
#else
        HRESULT errorCode = fileInterface->EnqueueRequest(curRead->fileHandle, curRead->dataBuffer, curRead->size, curRead->location, curRead->platformFileHandle, curQueueIndex);
#endif

        if (!SUCCEEDED(errorCode))
            DebugBreak();

        if ((i % batchSize) == (batchSize - 1))
        {
            fileInterface->EnqueueStatus(dStorageStatus[curQueueIndex].Get(), curBatchIndex[curQueueIndex], curQueueIndex);
            curBatchIndex[curQueueIndex]++;
            fileInterface->Submit(curQueueIndex);
            curQueueIndex++;
            curQueueIndex %= numQueues;
        }

        totalDataSizeRead += curRead->size;
        ++curRead;
    }
    submitTimer.Start();
    for (uint32_t i = 0; i < numQueues; i++)
    {
        fileInterface->EnqueueStatus(dStorageStatus[i].Get(), 0, i);
        fileInterface->Submit(i);
    }
    submitTimer.Stop();

    std::vector<uint32_t> lastBatchIndex;
    lastBatchIndex.resize(curBatchIndex.size(), 1);
    bool notDone = true;
    while (notDone)
    {
        _mm_pause();
        notDone = false;
        for (uint32_t i = 0; i < numQueues; i++)
        {
            if (dStorageStatus[i]->IsComplete(lastBatchIndex[i]))
            {
                wchar_t blah[128];
                HRESULT err = dStorageStatus[i]->GetHResult(lastBatchIndex[i]);
                if (err != S_OK)
                {
                    swprintf(blah, 128, L"Error on Status: %d:%d - %x\n", i, lastBatchIndex[i], err);
                    OutputDebugStringW(blah);
                }
                //swprintf(blah, 128, L"Finished one status: %d:%d\n", i, lastBatchIndex[i]);
                //OutputDebugStringW(blah);
                lastBatchIndex[i]++;
            }

            if (!dStorageStatus[i]->IsComplete(0))
            {
                notDone = true;
            }
        }
    }
    timer.Stop();
#ifdef IN_MEMORY_TEST
    {
        wchar_t buffer[128];
        swprintf(buffer, 128, L"File: Num Reads: %d\n", totalNumReads);
        OutputDebugStringW(buffer);
        swprintf(buffer, 128, L"File: Submit Time: %5.2f\n", submitTimer.GetTotalMicroseconds());
        OutputDebugStringW(buffer);
        swprintf(buffer, 128, L"File: Total Time:  %5.2f\n", timer.GetTotalMicroseconds());
        OutputDebugStringW(buffer);
    }
#endif
    time = timer.GetTotalSeconds();
    return totalNumReads;
}

#ifdef IN_MEMORY_TEST
#ifdef USE_PHYSICAL_MEMORY_DESTINATION
size_t DoDStorageDirectedAsyncTestRunInMemory(const FullReadSet::ReadSet& curReadSet, uint64_t& totalDataSizeRead, double& time, std::vector < uintptr_t*> destPhysicalPageArray)
#else
size_t DoDStorageDirectedAsyncTestRunInMemory(const FullReadSet::ReadSet& curReadSet, uint64_t& totalDataSizeRead, double& time, std::vector<char*>& destBlocks)
#endif
{
    std::vector<Microsoft::WRL::ComPtr <IDStorageStatusArrayX>> dStorageStatus;
    std::vector<uint32_t> numBatchesPerQueue;
    std::vector<uint32_t> numReadsPerQueue;
    std::vector<uint32_t> curBatchIndex;
    ATG::RDTSCPStopWatch timer, delayTimer;
    static const uint32_t batchSize = 2048;
    totalDataSizeRead = 0;
    time = 0;
    ATG::RDTSCPStopWatch submitTimer;

    std::random_device m_randomDevice;
    std::default_random_engine m_randomEngine(m_randomDevice());
    std::uniform_int_distribution<uint32_t> timeDist(1, 100);

    uint32_t totalNumReads = static_cast<uint32_t> (curReadSet.m_reads.size());
    uint32_t totalNumQueueEntries = totalNumReads + (totalNumReads / batchSize) + 64;	// extra space for final status entry
    uint32_t numQueues = (totalNumQueueEntries / DSTORAGE_MAX_QUEUE_CAPACITY) + 1;
    numQueues *= 4;
    if (numQueues > 60)
    {
        totalDataSizeRead = 1;
        time = 1;
        return 1;
    }
    numQueues = std::max<uint32_t>(std::min<uint32_t>(60, numQueues), 1);
    fileInterface->ResetQueues(DSTORAGE_MAX_QUEUE_CAPACITY, DSTORAGE_PRIORITY_REALTIME, numQueues, true);

    for (uint32_t i = 0; i < numQueues; i++)
    {
        numReadsPerQueue.push_back(totalNumReads / numQueues);
        numBatchesPerQueue.push_back((totalNumReads / numQueues) / batchSize);
        curBatchIndex.push_back(1);
        dStorageStatus.push_back(fileInterface->CreateStatusArray(((totalNumReads / numQueues) / batchSize) + 5));		// add some buffer for rounding
    }

    auto curRead = curReadSet.m_reads.begin();
    uint32_t curQueueIndex = 0;
    timer.Start();
    for (uint32_t i = 0; i < totalNumReads; i++)
    {
#ifdef USE_PHYSICAL_MEMORY_DESTINATION
        HRESULT errorCode = fileInterface->EnqueueRequest(curRead->physicalPageArray, curRead->physicalOffset, destPhysicalPageArray[i], 0, curRead->size, curQueueIndex);
#else
        HRESULT errorCode = fileInterface->EnqueueRequest(curRead->dataBuffer, destBlocks[i], curRead->size, curQueueIndex);
#endif

        if (!SUCCEEDED(errorCode))
            DebugBreak();

        if ((i % batchSize) == (batchSize - 1))
        {
            //fileInterface->EnqueueStatus(dStorageStatus[curQueueIndex], curBatchIndex[curQueueIndex], curQueueIndex);
            //curBatchIndex[curQueueIndex]++;
            //fileInterface->Submit(curQueueIndex);
            curQueueIndex++;
            curQueueIndex %= numQueues;
        }

        totalDataSizeRead += curRead->size;
        ++curRead;
    }
    submitTimer.Start();
    for (uint32_t i = 0; i < numQueues; i++)
    {
        fileInterface->EnqueueStatus(dStorageStatus[i].Get(), 0, i);
        fileInterface->Submit(i);
    }
    submitTimer.Stop();
    std::vector<uint32_t> lastBatchIndex;
    lastBatchIndex.resize(curBatchIndex.size(), 1);
    bool notDone = true;
    while (notDone)
    {
        _mm_pause();
        notDone = false;
        for (uint32_t i = 0; i < numQueues; i++)
        {
            if (dStorageStatus[i]->IsComplete(lastBatchIndex[i]))
            {
                wchar_t blah[128];
                HRESULT err = dStorageStatus[i]->GetHResult(lastBatchIndex[i]);
                if (err != S_OK)
                {
                    swprintf(blah, 128, L"Error on Status: %d:%d - %x\n", i, lastBatchIndex[i], err);
                    OutputDebugStringW(blah);
                }
                //swprintf(blah, 128, L"Finished one status: %d:%d\n", i, lastBatchIndex[i]);
                //OutputDebugStringW(blah);
                lastBatchIndex[i]++;
            }

            if (!dStorageStatus[i]->IsComplete(0))
            {
                notDone = true;
            }
        }
    }

    timer.Stop();
#if 0
    {
        wchar_t buffer[128];
        swprintf(buffer, 128, L"Memory: Num Reads: %d\n", totalNumReads);
        OutputDebugStringW(buffer);
        swprintf(buffer, 128, L"Memory: Submit Time: %5.2f\n", submitTimer.GetTotalMicroseconds());
        OutputDebugStringW(buffer);
        swprintf(buffer, 128, L"Memory: Total Time:  %5.2f\n", timer.GetTotalMicroseconds());
        OutputDebugStringW(buffer);
    }
#endif
    time = timer.GetTotalSeconds();
    return totalNumReads;
}
#endif

bool RunDirectedDStorageTest(const std::wstring& layoutFileName, const std::wstring& readSetFileName, uint32_t numIterations, const std::wstring& packFileName, const std::wstring& rootDirName, const std::wstring& dataOrder, uint32_t dataSize, bool forceSync, bool zipFileCompressed, bool useRealtime)
{
    static uint32_t runIteration[2][2] = { };		// one for sync, one for async
    double totalTimeSecs = 0;
    uint64_t totalDataSizeRead = 0;

#ifdef USE_PHYSICAL_MEMORY_DESTINATION
    if (dataSize & 65535)
        return false;
    if (forceSync)
        return false;
#endif

    std::unique_ptr<Packages::FlatPackage> layoutSet;
    layoutSet.reset(new Packages::FlatPackage());
    if (!layoutSet->ReadDataFile(layoutFileName, L""))
    {
        assert(zipFileCompressed);
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

    if (packFileName.size())
    {
        fileInterface = std::make_unique<PackedFileDStorage>(static_cast<uint16_t> (DSTORAGE_MAX_QUEUE_CAPACITY), DSTORAGE_PRIORITY_NORMAL, 1);
        uint64_t platformHandle;
        fileInterface->OpenPackFile(packFileName, 0, platformHandle, nullptr);
    }
    else
    {
        fileInterface = std::make_unique<LooseFilesDStorage>(static_cast<uint16_t> (DSTORAGE_MAX_QUEUE_CAPACITY), DSTORAGE_PRIORITY_NORMAL, 1);
        fileInterface->SetDirectoryOffset(rootDirName);
    }

    std::wstring allUpTimingLogName;
    allUpTimingLogName = forceSync ? (zipFileCompressed ? L"Sync_ZIP_DStorage_" : L"Sync_DStorage_") : (zipFileCompressed ? L"ASync_ZIP_DStorage_" : L"ASync_DStorage_");
    allUpTimingLogName += ATG::GetProcessorName();
    if (useRealtime)
        allUpTimingLogName += L"_realtime";
    allUpTimingLogName += L".csv";

    FileLogger allUptimingLog(allUpTimingLogName, runIteration[forceSync ? 0 : 1][zipFileCompressed ? 0 : 1] != 0, false, false, false, true);
    if (runIteration[forceSync ? 0 : 1][zipFileCompressed ? 0 : 1] == 0)
    {
        allUptimingLog.Log(L"Name,Order,Size,MB/s,Times");
    }
    {
        Sample::GetInstance()->SetWorkingOverlapDepth(0);
        std::wstring allUpTimingsHeader(fullReadSet.GetName());
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
                double totalTimeThisReadSet = 0;
                uint64_t totalDataSizeReadThisReadSet = 0;
                FullReadSet::ReadSet& curReadSet = fullReadSet.GetReadSet(curReadSetIndex);

                capturedReadSet->OpenFiles(curReadSet.m_name, "", fileInterface.get(), 0);

                auto endIter = curReadSet.m_reads.end();
                for (auto curRead = curReadSet.m_reads.begin(); curRead != endIter; ++curRead)
                {
#ifdef USE_PHYSICAL_MEMORY_DESTINATION
                    curRead->sizePhysicalPageArray = (dataSize / 65536);
                    curRead->physicalPageArray = new uintptr_t[curRead->sizePhysicalPageArray];
                    XMemAllocatePhysicalPages(unsigned(dataSize < (2 * 1024 * 1024) ? MEM_64K_PAGES : MEM_2MB_PAGES), &curRead->sizePhysicalPageArray, curRead->physicalPageArray);
#elif defined (_GAMING_XBOX)
                    if (curRead->dataBuffer)
                        VirtualFree(curRead->dataBuffer, 0, MEM_RELEASE);
                    curRead->dataBuffer = (char*)XMemVirtualAlloc(nullptr, curRead->size, MEM_COMMIT | static_cast<uint64_t> (curRead->size < (2 * 1024 * 1024) ? MEM_64K_PAGES : MEM_2MB_PAGES), XMEM_GRAPHICS, PAGE_READWRITE | PAGE_GRAPHICS_READWRITE | PAGE_GRAPHICS_COHERENT);
                    memset(curRead->dataBuffer, 0x37, curRead->size);
#else
                    if (curRead->dataBuffer)
                        VirtualFree(curRead->dataBuffer, 0, MEM_RELEASE);
                    curRead->dataBuffer = (char*)VirtualAlloc(nullptr, curRead->size, MEM_COMMIT, PAGE_READWRITE);
                    memset(curRead->dataBuffer, 0x37, curRead->size);
#endif
                }

                size_t totalNumReads = 0;
                if (forceSync)
                {
                    totalNumReads = DoDStorageDirectedSyncTestRun(curReadSet, totalDataSizeReadThisReadSet, totalTimeThisReadSet, useRealtime);
                }
                else
                {
                    totalNumReads = DoDStorageDirectedAsyncTestRun(curReadSet, totalDataSizeReadThisReadSet, totalTimeThisReadSet, useRealtime);
                }

#ifdef USE_PHYSICAL_MEMORY_DESTINATION
                for (auto& curRead : curReadSet.m_reads)
                {
                    // allocate a virtual address range for each physical page array
                    auto baseAddress = XMemVirtualAlloc(nullptr, curRead.size, unsigned(MEM_RESERVE | (curRead.size < 65536 ? MEM_64K_PAGES : MEM_2MB_PAGES)), XMEM_MAPPABLE | XMEM_GRAPHICS, PAGE_READWRITE | PAGE_GRAPHICS_READWRITE | PAGE_GRAPHICS_COHERENT);
                    curRead.dataBuffer = (char*)XMemMapPhysicalPages(baseAddress, curRead.sizePhysicalPageArray, curRead.physicalPageArray);
                }
#endif
#ifdef VALIDATE_READS
                {
                    size_t testReadIndex = 0;
                    for (auto& curRead : curReadSet.m_reads)
                    {
                        uint64_t* temp = (uint64_t*)(curRead.dataBuffer);
                        uint64_t startValue = curRead.location / sizeof(uint64_t);
                        uint32_t numberEntries = curRead.size / sizeof(uint64_t);
                        for (uint32_t location = 0; location < numberEntries; location++)
                        {
                            if (temp[location] != startValue)
                                DebugBreak();
                            assert(temp[location] == startValue);
                            startValue++;
                        }
                        ++testReadIndex;
                        if (testReadIndex >= totalNumReads)
                            break;
                    }
                }
#endif
#ifdef IN_MEMORY_TEST
                {
#ifdef USE_PHYSICAL_MEMORY_DESTINATION
                    std::vector<uintptr_t*> destBlocks;
                    std::vector<size_t> physicalPageBlocks;
                    destBlocks.resize(curReadSet.m_reads.size());
                    physicalPageBlocks.resize(curReadSet.m_reads.size());
                    size_t numPagesAlloced(0);
                    uint32_t index = 0;
                    for (auto curRead = curReadSet.m_reads.begin(); curRead != endIter; ++curRead)
                    {
                        destBlocks[index] = new uintptr_t[dataSize / 65536];
                        numPagesAlloced = dataSize / 65536;
                        XMemAllocatePhysicalPages(unsigned(dataSize < (2 * 1024 * 1024) ? MEM_64K_PAGES : MEM_2MB_PAGES), &numPagesAlloced, destBlocks[index]);
                        physicalPageBlocks[index++] = numPagesAlloced;
                    }
                    totalTimeThisReadSet = 0;
                    totalDataSizeReadThisReadSet = 0;
                    totalNumReads = 0;
                    totalNumReads = DoDStorageDirectedAsyncTestRunInMemory(curReadSet, totalDataSizeReadThisReadSet, totalTimeThisReadSet, destBlocks);
                    for (auto iter : destBlocks)
                    {
                        uintptr_t* temp = (uintptr_t*)iter;
                        XMemFreePhysicalPages(numPagesAlloced, temp);
                        delete[] temp;
                    }
#else
                    std::vector<char*> destBlocks;
                    destBlocks.resize(curReadSet.m_reads.size());
                    uint32_t index = 0;
                    for (auto curRead = curReadSet.m_reads.begin(); curRead != endIter; ++curRead)
                    {
                        destBlocks[index++] = (char*)XMemVirtualAlloc(nullptr, dataSize, MEM_RESERVE | MEM_COMMIT | static_cast<uint64_t> (dataSize < (2 * 1024 * 1024) ? MEM_64K_PAGES : MEM_2MB_PAGES), XMEM_GRAPHICS, PAGE_READWRITE | PAGE_GRAPHICS_READWRITE | PAGE_GRAPHICS_COHERENT);
                        memset(destBlocks[index - 1], 0x37, dataSize);
                    }
                    totalTimeThisReadSet = 0;
                    totalDataSizeReadThisReadSet = 0;
                    totalNumReads = 0;
                    totalNumReads = DoDStorageDirectedAsyncTestRunInMemory(curReadSet, totalDataSizeReadThisReadSet, totalTimeThisReadSet, destBlocks);
#ifdef VALIDATE_READS
                    {
                        size_t testReadIndex = 0;
                        for (auto& curRead : curReadSet.m_reads)
                        {
                            uint64_t* temp = (uint64_t*)(destBlocks[testReadIndex]);
                            uint64_t startValue = curRead.location / sizeof(uint64_t);
                            uint32_t numberEntries = curRead.size / sizeof(uint64_t);
                            for (uint32_t location = 0; location < numberEntries; location++)
                            {
                                if (temp[location] != startValue)
                                    DebugBreak();
                                assert(temp[location] == startValue);
                                startValue++;
                            }
                            ++testReadIndex;
                            if (testReadIndex >= totalNumReads)
                                break;
                        }
                    }
#endif
                    for (auto iter : destBlocks)
                    {
                        VirtualFree(iter, 0, MEM_RELEASE);
                    }
#endif
                }

#endif

                for (auto curRead = curReadSet.m_reads.begin(); curRead != curReadSet.m_reads.end(); ++curRead)
                {
#ifdef USE_PHYSICAL_MEMORY_DESTINATION
                    VirtualFree(curRead->dataBuffer, 0, MEM_RELEASE);
                    curRead->dataBuffer = nullptr;
                    XMemFreePhysicalPages(curRead->sizePhysicalPageArray, curRead->physicalPageArray);
                    delete[] curRead->physicalPageArray;
                    curRead->physicalPageArray = nullptr;
                    curRead->sizePhysicalPageArray = 0;

#else
                    VirtualFree(curRead->dataBuffer, 0, MEM_RELEASE);
                    //delete[] curRead->dataBuffer;
                    curRead->dataBuffer = nullptr;
#endif
                }

                {
                    totalTimeSecs += totalTimeThisReadSet;
                    totalDataSizeRead += totalDataSizeReadThisReadSet;
                    wchar_t tempStr[32];
                    swprintf_s(tempStr, 32, L",%5.3f", totalTimeThisReadSet * 1000.0);
                    allUpTimingsTimes += tempStr;
                }
                OutputDebugStringW(L"Finished one iteration\n");
                capturedReadSet->CloseFiles(fileInterface.get());
            }

            {
                double mbs = ((totalDataSizeRead) / (1000 * 1000)) / (totalTimeSecs);
                wchar_t tempStr[32];
                swprintf_s(tempStr, 32, L",%5.2f", mbs);
                std::wstring tempAllUp(allUpTimingsHeader);
                tempAllUp += tempStr;
                tempAllUp += allUpTimingsTimes;
                OutputDebugStringW(tempAllUp.c_str());
                OutputDebugStringW(L"\n");
                allUptimingLog.Log(tempAllUp);
            }
        }
    }

    fileInterface->ClosePackFile();
    ++runIteration[forceSync ? 0 : 1][zipFileCompressed ? 0 : 1];
    fileInterface.reset();
    return true;
}
