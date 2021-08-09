//--------------------------------------------------------------------------------------
// PerfRun.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"

#include "PerfRun.h"
#include "Processor.h"

#pragma warning (push)
#pragma warning (disable: 4061)     // unused enum define in a switch statement
#pragma warning (disable: 4062)     // unused enum define in a switch statement

PerfRun::PerfRun(const std::vector<std::wstring>& testNames) :
    m_coreTestNames(testNames)
    , m_singleTimePerThread(nullptr)
    , m_totalTimePerThread(nullptr)
    , m_threadBuffer(nullptr)
{
    memset(m_combinedTimePerThread, 0, sizeof(m_combinedTimePerThread));
}

PerfRun::~PerfRun()
{
    for (auto& iter : m_openLogs)
    {
        delete iter.second;
    }
}

ATG::FileLogger* PerfRun::GetOpenLog(const std::wstring& fileName)
{
    auto iter = m_openLogs.find(fileName);
    if (iter != m_openLogs.end())
        return iter->second;
    ATG::FileLogger* newLogFile;
    newLogFile = new ATG::FileLogger(fileName, false);

    std::wstring headerInfo = L"Test Type,Median";
    for (uint32_t i = 0; i < c_NumCoreIterations; i++)
    {
        wchar_t temp[64];
        swprintf_s(temp, 64, L",Run %d", i);
        headerInfo += temp;
    }
    newLogFile->Log(headerInfo);

    m_openLogs[fileName] = newLogFile;
    return newLogFile;
}

void PerfRun::DumpLogFile(TestType whichTest, uint64_t coreMask, uint64_t numIterations, bool sharedMemory)
{
    wchar_t tempStr[128];
    swprintf(tempStr, 128, L"%s_%s.csv", L"AllInstructions", ATG::GetProcessorName().c_str());
    ATG::FileLogger* mainLogFile = GetOpenLog(tempStr);

    uint64_t numCores = __popcnt64(coreMask);

    std::wstring header;
    std::wstring fileName;
    switch (whichTest)
    {
    case PERFORM_RAW_READ:
        header += L"raw read ";
        fileName = L"Reads";
        break;
    case PERFORM_ATOMIC_READ:
        header += L"atomic read ";
        fileName = L"AtomicRead";
        break;
    case PERFORM_RAW_WRITE:
        header += L"raw write ";
        fileName = L"Writes";
        break;
    case PERFORM_ATOMIC_WRITE:
        header += L"atomic write ";
        fileName = L"AtomicWrite";
        break;
    case PERFORM_ATOMIC_CAS:
        header += L"atomic CAS ";
        fileName = L"AtomicCAS";
        break;
    default:
        assert(false);
        break;
    }
    fileName += L"_";
    fileName += ATG::GetProcessorName();
    fileName += L".csv";
    ATG::FileLogger* testLogFile = GetOpenLog(fileName);

    swprintf_s(tempStr, 128, L"(%s) Shared(%s) ", ATG::GetProcessorMaskString(coreMask).c_str(), sharedMemory ? L"true" : L"false");
    header += tempStr;

    swprintf_s(tempStr, 128, L"%s_%s_%s.csv", L"Instructions", ATG::GetProcessorMaskString(coreMask).c_str(), ATG::GetProcessorName().c_str());
    ATG::FileLogger* threadLogFile = GetOpenLog(tempStr);

    uint64_t tempCoreMask = coreMask;
    for (uint32_t k = 0; k < numCores; k++)
    {
        uint32_t currentCore;
        _BitScanForward64(reinterpret_cast<DWORD*> (&currentCore), tempCoreMask);
        tempCoreMask &= ~(1ULL << currentCore);
        std::wstring textBuffer(header);
        swprintf_s(tempStr, 128, L"Core(%d)", currentCore);
        textBuffer += tempStr;
        {
            std::vector<double> medianCalc;
            medianCalc.resize(numIterations);
            for (uint64_t i = 0; i < numIterations; i++)
            {
                medianCalc[i] = m_totalTimePerThread[k][i] / ATG::GetRDTSCPFrequencyMicroseconds();
            }
            std::sort(medianCalc.begin(), medianCalc.end());
            swprintf_s(tempStr, 128, L",%f", medianCalc[medianCalc.size() / 2]);
            textBuffer += tempStr;
        }
        for (uint64_t i = 0; i < numIterations; i++)
        {
            double value = m_totalTimePerThread[k][i] / ATG::GetRDTSCPFrequencyMicroseconds();
            swprintf_s(tempStr, 128, L",%f", value);
            textBuffer += tempStr;
        }
        mainLogFile->Log(textBuffer);
        testLogFile->Log(textBuffer);
        threadLogFile->Log(textBuffer);
    }

    {
        wchar_t tempBuffer[128];
        std::vector<uint64_t> times;
        std::wstring logName;
        logName = ATG::GetProcessorName();
        logName += L"_combined.csv";
        ATG::FileLogger* combinedLog = new ATG::FileLogger(logName, false);
        combinedLog->Log(L"Cores,Raw Read,Atomic Load, Raw Write,Atomic Store, CAS");
        for (uint32_t maskIndex = 0; (maskIndex < c_maxCoreMaskIndexes) && (maskIndex < m_coreTestNames.size()); maskIndex++)
        {
            for (uint32_t shared = 0; shared < 2; shared++)
            {
                std::wstring logLine;
                logLine = m_coreTestNames[maskIndex];
                if (shared)
                    logLine += L" Unique";
                else
                    logLine += L" Shared";

                for (uint32_t curTest = FirstTestDefined; curTest <= LastTestDefined; curTest++)
                {
                    times.clear();
                    for (uint32_t iteration = 0; iteration < c_NumCoreIterations; iteration++)
                    {
                        for (uint32_t curThread = 0; curThread < c_maxNumWorkerThreads; curThread++)
                        {
                            if (m_combinedTimePerThread[maskIndex][curTest][shared][curThread][iteration] != 0)
                                times.push_back(m_combinedTimePerThread[maskIndex][curTest][shared][curThread][iteration]);
                        }
                    }
                    double median(0);
                    if (times.size() != 0)
                    {
                        std::sort(times.begin(), times.end());
                        median = times[times.size() / 2] / ATG::GetRDTSCPFrequencyMicroseconds();
                    }
                    swprintf(tempBuffer, 128, L",%5.2f", median);
                    logLine += tempBuffer;
                }
                combinedLog->Log(logLine);
            }
        }
        delete combinedLog;
    }
}

void PerfRun::RunTests(TestType whichTest, uint64_t coreMask, uint64_t numIterations, uint32_t coreMaskIndex)
{
    if (whichTest < FirstTestExecuted)
        return;
    if (whichTest > LastTestExecuted)
        return;

    std::function<uint64_t(ThreadParams)> testFunction;
    switch (whichTest)
    {
    case PERFORM_RAW_READ:
        testFunction = std::bind(&PerfRun::WorkerThreadRead, this, std::placeholders::_1);
        break;
    case PERFORM_ATOMIC_READ:
        testFunction = std::bind(&PerfRun::WorkerThreadAtomicRead, this, std::placeholders::_1);
        break;
    case PERFORM_RAW_WRITE:
        testFunction = std::bind(&PerfRun::WorkerThreadWrite, this, std::placeholders::_1);
        break;
    case PERFORM_ATOMIC_WRITE:
        testFunction = std::bind(&PerfRun::WorkerThreadAtomicWrite, this, std::placeholders::_1);
        break;
    case PERFORM_ATOMIC_CAS:
        testFunction = std::bind(&PerfRun::WorkerThreadAtomicCAS, this, std::placeholders::_1);
        break;
    default:
        break;
    }
    uint64_t numCores = __popcnt64(coreMask);
    if (numCores == 0)
        return;

    m_threadBuffer = new std::thread * [numCores];

    m_singleTimePerThread = new uint64_t[numCores];
    memset(m_singleTimePerThread, 0, sizeof(uint64_t) * numCores);
    m_totalTimePerThread = new uint64_t * [numCores];
    for (uint32_t i = 0; i < numCores; i++)
    {
        m_totalTimePerThread[i] = new uint64_t[c_NumCoreIterations];
        memset(m_totalTimePerThread[i], 0, sizeof(uint64_t) * c_NumCoreIterations);
    }

    if (numCores == 1)
    {
        uint32_t coreId;
        _BitScanForward64(reinterpret_cast<DWORD*> (&coreId), coreMask);
        for (uint32_t i = 0; i < numIterations; i++)
        {
            m_workerData = static_cast<WorkerData*>(_aligned_malloc(sizeof(WorkerData) * numCores, c_CacheLineSize));
            SetThreadAffinityMask(GetCurrentThread(), 1ULL << coreId);
            ThreadParams temp;
            temp.baseIndex = 0;
            temp.iterations = c_NumSingleOperations;
            temp.threadId = 0;
            temp.coreNum = coreId;
            temp.forceStart = true;
            m_singleTimePerThread[0] = 0;
            testFunction(temp);
            m_totalTimePerThread[0][i] = m_singleTimePerThread[0];
            m_combinedTimePerThread[coreMaskIndex][whichTest][0][coreId][i] = m_singleTimePerThread[0];
            _aligned_free(m_workerData);
        }
        DumpLogFile(whichTest, coreMask, numIterations, false);
    }
    else
    {
        uint64_t systemAffinityMask, processAffinityMask;
        ::GetProcessAffinityMask(GetCurrentProcess(), &processAffinityMask, &systemAffinityMask);
        processAffinityMask &= ~coreMask;
        SetThreadAffinityMask(GetCurrentThread(), processAffinityMask);

        for (uint32_t offset = 0; offset < 2; offset++)
        {
            for (uint32_t i = 0; i < numIterations; i++)
            {
                m_startBarrier.reset(numCores + 1);
                m_finishBarrier.reset(numCores + 1);
                m_workerData = static_cast<WorkerData*>(_aligned_malloc(sizeof(WorkerData) * numCores, c_CacheLineSize));
                uint64_t tempCoreMask = coreMask;
                for (uint32_t j = 0; j < numCores; j++)
                {
                    uint32_t currentCore;
                    _BitScanForward64(reinterpret_cast<DWORD*> (&currentCore), tempCoreMask);
                    tempCoreMask &= ~(1ULL << currentCore);
                    ThreadParams params;
                    params.threadId = j;
                    params.coreNum = currentCore;
                    params.iterations = c_NumSingleOperations;
                    params.baseIndex = j * offset;
                    params.forceStart = false;

                    m_singleTimePerThread[j] = 0;
                    m_threadBuffer[j] = new std::thread(testFunction, params);
                }

                m_startBarrier.enterBarrier(true);
                m_finishBarrier.enterBarrier(true);

                for (uint32_t j = 0; j < numCores; j++)
                {
                    m_threadBuffer[j]->join();
                    m_totalTimePerThread[j][i] = m_singleTimePerThread[j];
                    m_combinedTimePerThread[coreMaskIndex][whichTest][offset][j][i] = m_singleTimePerThread[j];

                    delete m_threadBuffer[j];
                }
                _aligned_free(m_workerData);
            }
            DumpLogFile(whichTest, coreMask, numIterations, offset == 0);
        }
    }
    delete[] m_singleTimePerThread;

    for (uint32_t i = 0; i < numCores; i++)
    {
        delete[] m_totalTimePerThread[i];
    }
    delete[] m_totalTimePerThread;

    delete[] m_threadBuffer;
}

uint64_t PerfRun::WorkerThreadRead(ThreadParams params)
{
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint32_t tscTime = 0;
    uint64_t deltaTime = 0;

    SetThreadAffinityMask(GetCurrentThread(), 1ULL << params.coreNum);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    if (!params.forceStart)
        m_startBarrier.enterBarrier();

    volatile uint64_t* buffer = &(m_workerData[params.baseIndex].m_data);
    uint64_t fred(0);

    startTime = __rdtscp(&tscTime);
    for (uint64_t j = 0; j < params.iterations; j++)
    {
        fred += *buffer + j;
    }
    endTime = __rdtscp(&tscTime);

    if (!params.forceStart)
        m_finishBarrier.enterBarrier();
    deltaTime = endTime - startTime;
    m_singleTimePerThread[params.threadId] = deltaTime;
    return fred;    // This value is ignored, but the compiler doesn't know that, therefore it can't optimize out the entire loop
}

uint64_t PerfRun::WorkerThreadWrite(ThreadParams params)
{
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint32_t tscTime = 0;
    uint64_t deltaTime = 0;

    SetThreadAffinityMask(GetCurrentThread(), 1ULL << params.coreNum);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    if (!params.forceStart)
        m_startBarrier.enterBarrier();

    volatile uint64_t* buffer = &(m_workerData[params.baseIndex].m_data);
    uint64_t fred(0);

    startTime = __rdtscp(&tscTime);
    for (uint64_t j = 0; j < params.iterations; j++)
    {
        *buffer = *buffer + 1;
    }
    endTime = __rdtscp(&tscTime);

    if (!params.forceStart)
        m_finishBarrier.enterBarrier();
    deltaTime = endTime - startTime;
    m_singleTimePerThread[params.threadId] = deltaTime;
    return fred;    // This value is ignored, but the compiler doesn't know that, therefore it can't optimize out the entire loop
}

uint64_t PerfRun::WorkerThreadAtomicWrite(ThreadParams params)
{
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint32_t tscTime = 0;
    uint64_t deltaTime = 0;

    uint64_t* buffer = &(m_workerData[params.baseIndex].m_data);

    SetThreadAffinityMask(GetCurrentThread(), 1ULL << params.coreNum);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    if (!params.forceStart)
        m_startBarrier.enterBarrier();

    startTime = __rdtscp(&tscTime);
    for (uint64_t j = 0; j < params.iterations; j++)
    {
        ((std::atomic<uint64_t>*) buffer)->store(((std::atomic<uint64_t>*) buffer)->load() + 1);
    }
    endTime = __rdtscp(&tscTime);

    if (!params.forceStart)
        m_finishBarrier.enterBarrier();
    deltaTime = endTime - startTime;
    m_singleTimePerThread[params.threadId] = deltaTime;
    return 0;
}

uint64_t PerfRun::WorkerThreadAtomicRead(ThreadParams params)
{
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint32_t tscTime = 0;
    uint64_t deltaTime = 0;
    uint64_t fred = 0;

    uint64_t* buffer = &(m_workerData[params.baseIndex].m_data);
    SetThreadAffinityMask(GetCurrentThread(), 1ULL << params.coreNum);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    if (!params.forceStart)
        m_startBarrier.enterBarrier();

    startTime = __rdtscp(&tscTime);
    for (uint64_t j = 0; j < params.iterations; j++)
    {
        fred += ((std::atomic<uint64_t>*) buffer)->load() + j;
    }
    endTime = __rdtscp(&tscTime);

    if (!params.forceStart)
        m_finishBarrier.enterBarrier();
    deltaTime = endTime - startTime;
    m_singleTimePerThread[params.threadId] = deltaTime;
    return fred;    // This value is ignored, but the compiler doesn't know that, therefore it can't optimize out the entire loop
}

uint64_t PerfRun::WorkerThreadAtomicCAS(ThreadParams params)
{
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    uint32_t tscTime = 0;
    uint64_t deltaTime = 0;

    std::atomic<uint64_t>* buffer = (std::atomic<uint64_t>*) (&m_workerData[params.baseIndex]);
    SetThreadAffinityMask(GetCurrentThread(), 1ULL << params.coreNum);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

    if (!params.forceStart)
        m_startBarrier.enterBarrier();

    startTime = __rdtscp(&tscTime);
    for (uint64_t j = 0; j < params.iterations; j++)
    {
        uint64_t temp = buffer->load(std::memory_order_relaxed);
        buffer->compare_exchange_strong(temp, temp + 1);
    }
    endTime = __rdtscp(&tscTime);

    if (!params.forceStart)
        m_finishBarrier.enterBarrier();
    deltaTime = endTime - startTime;
    m_singleTimePerThread[params.threadId] = deltaTime;
    return 0;
}
#pragma warning (pop)
