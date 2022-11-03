//--------------------------------------------------------------------------------------
// PerfRun.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "pch.h"

#include "PerfRun.h"
#include "Processor.h"
#include "RDTSCPStopWatch.h"
#include "Random.h"
#include <algorithm>

PerfRun::PerfRun() :
    m_senderThread(nullptr)
    , m_receiverThread(nullptr)
    , m_startTestRun(false)
    , m_senderReady(false)
    , m_receiverReady(false)
    , m_senderDone(false)
    , m_receiverDone(false)
    , m_acquireLogfile(nullptr)
    , m_releaseLogFile(nullptr)
    , m_combinedLogfile(nullptr)
{
    m_semaphore = CreateSemaphoreEx(nullptr, 1, 1, nullptr, 0, SEMAPHORE_ALL_ACCESS);
    m_event = CreateEvent(nullptr, false, true, nullptr);
    m_mutex = CreateMutex(nullptr, false, nullptr);
    InitializeSRWLock(&m_srw);
    InitializeCriticalSection(&m_critSection);
    InitializeConditionVariable(&m_conditionVariable);
    m_waitAddress = 0;

    KernelFuncs[PERFORM_SEMAPHORE] = KernelFuncPair(std::bind(ReleaseSemaphore, m_semaphore, 1, nullptr), std::bind(WaitForSingleObject, m_semaphore, INFINITE));
    KernelFuncs[PERFORM_EVENT] = KernelFuncPair(std::bind(SetEvent, m_event), std::bind(WaitForSingleObject, m_event, INFINITE));
    KernelFuncs[PERFORM_MUTEX] = KernelFuncPair(std::bind(ReleaseMutex, m_mutex), std::bind(WaitForSingleObject, m_mutex, INFINITE));
    KernelFuncs[PERFORM_SRW] = KernelFuncPair(std::bind(ReleaseSRWLockExclusive, &m_srw), std::bind(AcquireSRWLockExclusive, &m_srw));
    KernelFuncs[PERFORM_CRITICAL_SECTION] = KernelFuncPair(std::bind(LeaveCriticalSection, &m_critSection), std::bind(EnterCriticalSection, &m_critSection));
    KernelFuncs[PERFORM_WAIT_ADDRESS] = KernelFuncPair(nullptr, nullptr);
    KernelFuncs[PERFORM_CONDITION_SRW] = KernelFuncPair(std::bind(ReleaseSRWLockExclusive, &m_srw), std::bind(AcquireSRWLockExclusive, &m_srw));
    KernelFuncs[PERFORM_CONDITION_CS] = KernelFuncPair(std::bind(LeaveCriticalSection, &m_critSection), std::bind(EnterCriticalSection, &m_critSection));

    for (uint32_t i = FirstTestDefined; i <= LastTestDefined; i++)
    {
        for (uint32_t k = 0; k < c_numCoreTestTypes; k++)
        {
            for (uint32_t j = 0; j < c_NumTestLoops; j++)
            {
                m_combinedMemoryRaceTimingsUS[i][k][j] = -1;
                m_combinedReleaseTimingsUS[i][k][j] = -1;
            }
        }
    }
}

PerfRun::~PerfRun()
{
    if (m_senderThread)
        m_senderThread->join();
    if (m_receiverThread)
        m_receiverThread->join();
    delete m_senderThread;
    delete m_receiverThread;
    CloseHandle(m_semaphore);
    CloseHandle(m_event);
    CloseHandle(m_mutex);
    DeleteCriticalSection(&m_critSection);
    delete m_acquireLogfile;
    delete m_releaseLogFile;
    delete m_combinedLogfile;
}

void PerfRun::OpenLogFiles(bool idleWorkers)
{
    if (m_acquireLogfile == nullptr)
    {
        std::wstring logName;
        logName = L"Primitive_Acquire_";
        if (idleWorkers)
            logName += L"idle_";
        logName += ATG::GetProcessorName();
        logName += L".csv";
        m_acquireLogfile = new ATG::FileLogger(logName, false);
        {
            std::wstring textBuffer;
            textBuffer = L"Run,Pattern,Median,Timings";
            m_acquireLogfile->Log(textBuffer);
        }
    }
    if (m_releaseLogFile == nullptr)
    {
        std::wstring releaseName;
        releaseName = L"Primitive_Release_";
        if (idleWorkers)
            releaseName += L"idle_";
        releaseName += ATG::GetProcessorName();
        releaseName += L".csv";
        m_releaseLogFile = new ATG::FileLogger(releaseName, false);
        {
            std::wstring textBuffer;
            textBuffer = L"Run,Pattern,Median,Timings";
            m_releaseLogFile->Log(textBuffer);
        }
    }
}

void PerfRun::RunTests(uint32_t senderCore, uint32_t receiverCore, TestType whichTest, bool idleWorkers, const std::wstring& testName, uint32_t coreTestType)
{
    if ((whichTest < FirstTestExecuted) || (whichTest > LastTestExecuted))
        return;
    if (coreTestType >= c_numCoreTestTypes)
        return;

    bool noContention = (senderCore == receiverCore);
    if (noContention && idleWorkers)
        return;

    OpenLogFiles(idleWorkers);
    m_startTestRun = false;
    m_receiverReady = false;
    m_senderReady = false;
    m_receiverDone = false;
    m_senderDone = false;
    m_senderThread = nullptr;
    m_receiverThread = nullptr;
    m_workerThread = nullptr;
    ATG::SetThreadAffinityMask(GetCurrentThread(), ATG::GetProcessAffinityMask() & ~(1ULL << senderCore) & ~(1ULL << receiverCore));

    m_workerShutdownFlag = false;
    if (idleWorkers)
        m_workerThread = new std::thread(&PerfRun::WorkerThread, this, receiverCore, THREAD_PRIORITY_IDLE);

    if (noContention)
    {
        static std::function<void(PerfRun*, TestType whichTest, uint32_t, uint32_t)> NoContentionFunc[] =
        {
            &PerfRun::ReceiveKernelFunctionNoContention,
            &PerfRun::ReceiveKernelFunctionNoContention,
            &PerfRun::ReceiveKernelFunctionNoContention,
            &PerfRun::ReceiveKernelFunctionNoContention,
            &PerfRun::ReceiveKernelFunctionNoContention,
            &PerfRun::ReceiveWaitAddressFunctionNoContention,
            &PerfRun::ReceiveCVFunctionNoContention,
            &PerfRun::ReceiveCVFunctionNoContention,
        };

        m_topBarrier.reset(1);
        m_middleBarrier.reset(1);
        m_bottomBarrier.reset(1);
        m_receiverThread = new std::thread(NoContentionFunc[whichTest], this, whichTest, receiverCore, THREAD_PRIORITY_TIME_CRITICAL);
    }
    else
    {
        typedef std::pair<std::function<void(PerfRun*, TestType whichTest, uint32_t, uint32_t)>, std::function<void(PerfRun*, TestType whichTest, uint32_t, uint32_t)>> ContentionPair;
        static ContentionPair ContentionFunc[] =
        {
            ContentionPair(&PerfRun::SendKernelFunction,&PerfRun::ReceiveKernelFunction),
            ContentionPair(&PerfRun::SendKernelFunction,&PerfRun::ReceiveKernelFunction),
            ContentionPair(&PerfRun::SendKernelFunction,&PerfRun::ReceiveKernelFunction),
            ContentionPair(&PerfRun::SendKernelFunction,&PerfRun::ReceiveKernelFunction),
            ContentionPair(&PerfRun::SendKernelFunction,&PerfRun::ReceiveKernelFunction),
            ContentionPair(&PerfRun::SendWaitAddressFunction,&PerfRun::ReceiveWaitAddressFunction),
            ContentionPair(&PerfRun::SendCVFunction,&PerfRun::ReceiveCVFunction),
            ContentionPair(&PerfRun::SendCVFunction,&PerfRun::ReceiveCVFunction),
        };

        m_topBarrier.reset(2);
        m_middleBarrier.reset(2);
        m_bottomBarrier.reset(2);
        m_senderThread = new std::thread(ContentionFunc[whichTest].first, this, whichTest, senderCore, THREAD_PRIORITY_TIME_CRITICAL);
        m_receiverThread = new std::thread(ContentionFunc[whichTest].second, this, whichTest, receiverCore, THREAD_PRIORITY_TIME_CRITICAL);
    }

    for (uint32_t i = 0; i < c_NumTestLoops; i++)
    {
        m_memoryRaceTimingsUS[i] = 0;
        m_releaseTimingsUS[i] = 0;
    }

    m_startTestRun = true;
    if (!noContention)
        m_senderThread->join();
    m_receiverThread->join();
    delete m_senderThread;
    delete m_receiverThread;
    m_senderThread = nullptr;
    m_receiverThread = nullptr;
    m_workerShutdownFlag = true;
    if (m_workerThread)
    {
        m_workerThread->join();
        delete m_workerThread;
        m_workerThread = nullptr;
    }

    for (uint32_t i = 0; i < c_NumTestLoops; i++)
    {
        m_combinedMemoryRaceTimingsUS[whichTest][coreTestType][i] = m_memoryRaceTimingsUS[i];
        m_combinedReleaseTimingsUS[whichTest][coreTestType][i] = m_releaseTimingsUS[i];
    }

    {
        std::wstring acquireTextBuffer;
        std::wstring releaseTextBuffer;
        wchar_t tempStr[128];
        if (noContention)
            swprintf_s(tempStr, 128, L"%s %d,%s_noContention,", ConvertTestTypeToString(whichTest).c_str(), senderCore, testName.c_str());
        else
            swprintf_s(tempStr, 128, L"%s %d-%d,%s_contention,", ConvertTestTypeToString(whichTest).c_str(), senderCore, receiverCore, testName.c_str());
        acquireTextBuffer = tempStr;
        releaseTextBuffer = tempStr;
        for (uint32_t k = 1; k < c_NumTestLoops; k++)
        {
            double value = m_memoryRaceTimingsUS[k];
            swprintf_s(tempStr, 64, L",%5.2f", value);
            acquireTextBuffer += tempStr;

            value = m_releaseTimingsUS[k];
            swprintf_s(tempStr, 64, L",%5.2f", value);
            releaseTextBuffer += tempStr;
        }
        if (m_acquireLogfile)
            m_acquireLogfile->Log(acquireTextBuffer);
        if (m_releaseLogFile)
            m_releaseLogFile->Log(releaseTextBuffer);

        std::vector<double> acquireTotal;
        std::vector<double> releaseTotal;
        acquireTotal.resize(c_NumTestLoops, 0);
        releaseTotal.resize(c_NumTestLoops, 0);

        {
            std::wstring releaseName;
            releaseName = L"Primitive_Combined_";
            if (idleWorkers)
                releaseName += L"idle_";
            releaseName += ATG::GetProcessorName();
            releaseName += L".csv";
            m_combinedLogfile = new ATG::FileLogger(releaseName, false);
            {
                std::wstring textBuffer;
                if (ATG::IsSMTSupported())
                    textBuffer = L"Test,Single core,Same Physical,Same Cluster,Cross Cluster";
                else
                    textBuffer = L"Test,Single core,Same Cluster,Cross Cluster";
                m_combinedLogfile->Log(textBuffer);
            }

            for (uint32_t i = FirstTestExecuted; i <= LastTestExecuted; i++)
            {
                swprintf_s(tempStr, 128, L"%s ", ConvertTestTypeToString(static_cast<PerfRun::TestType> (i)).c_str());
                acquireTextBuffer = tempStr;
                swprintf_s(tempStr, 128, L"   Release ");
                releaseTextBuffer = tempStr;

                for (uint32_t k = 0; k < c_numCoreTestTypes; k++)
                {
                    for (uint32_t j = 1; j < c_NumTestLoops; j++)
                    {
                        acquireTotal[j] = m_combinedMemoryRaceTimingsUS[i][k][j];
                        releaseTotal[j] = m_combinedReleaseTimingsUS[i][k][j];
                    }
                    std::sort(acquireTotal.begin(), acquireTotal.end());
                    std::sort(releaseTotal.begin(), releaseTotal.end());
                    double median;
                    median = acquireTotal[c_NumTestLoops / 2] + acquireTotal[(c_NumTestLoops / 2)];
                    median /= 2.0;
                    swprintf_s(tempStr, 128, L",%5.2f", median);
                    acquireTextBuffer += tempStr;

                    median = releaseTotal[c_NumTestLoops / 2] + releaseTotal[(c_NumTestLoops / 2)];
                    median /= 2.0;
                    swprintf_s(tempStr, 128, L",%5.2f", median);
                    releaseTextBuffer += tempStr;
                }
                m_combinedLogfile->Log(acquireTextBuffer);
                m_combinedLogfile->Log(releaseTextBuffer);
            }
            delete m_combinedLogfile;
            m_combinedLogfile = nullptr;
        }
    }
}

void PerfRun::WorkerThread(uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriorityBoost(curThread, true);
    ATG::SetThreadPriority(curThread, priority);
    while (!m_workerShutdownFlag)
    {
        _mm_pause();
    }
}

void PerfRun::SendKernelFunction(TestType whichTest, uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();
    while (!m_startTestRun);
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriorityBoost(curThread, true);
    ATG::SetThreadPriority(curThread, priority);
    ATG::RDTSCPStopWatch timer;

    KernelFuncs[whichTest].second();
    for (uint32_t i = 0; i < c_NumTestLoops; i++)
    {
        m_senderReady = true;
        while (!m_receiverReady);

        timer.Start();
        while ((timer.GetCurrentMilliseconds()) < c_SendDelayMS) {} // need to give time for the receive thread to actually suspend waiting on the object.
        timer.Stop();
        m_calcedDelayUS = timer.GetTotalMicroseconds();

        timer.Start();
        KernelFuncs[whichTest].first();
        timer.Stop();
        m_releaseTimingsUS[i] = timer.GetTotalMicroseconds();

        while (!m_receiverDone) { /*SwitchToThread();*/ }
        m_senderReady = false;
        m_senderDone = true;

        KernelFuncs[whichTest].second();
        m_senderDone = false;
    }
    KernelFuncs[whichTest].first();
}

void PerfRun::ReceiveKernelFunction(TestType whichTest, uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();

    while (!m_startTestRun);
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriorityBoost(curThread, true);
    ATG::SetThreadPriority(curThread, priority);
    ATG::RDTSCPStopWatch timer;

    for (uint32_t loopCount = 0; loopCount < c_NumTestLoops; loopCount++)
    {
        m_receiverReady = true;
        while (!m_senderReady);

        timer.Start();
        KernelFuncs[whichTest].second();
        timer.Stop();

        m_receiverReady = false;
        m_receiverDone = true;
        while (!m_senderDone) { /*SwitchToThread();*/ }

        m_memoryRaceTimingsUS[loopCount] = timer.GetTotalMicroseconds() < m_calcedDelayUS ? 0 : timer.GetTotalMicroseconds() - m_calcedDelayUS;
        m_memoryRaceTimingsRaw[loopCount] = timer.GetTotalRaw();
        m_memoryRaceTimingsCalcedDeltaUS[loopCount] = m_calcedDelayUS;
        m_receiverDone = false;
        KernelFuncs[whichTest].first();
    }
}

void PerfRun::SendWaitAddressFunction(TestType /*whichTest*/, uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();
    while (!m_startTestRun);
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriority(curThread, priority);
    ATG::SetThreadPriorityBoost(curThread, true);
    ATG::RDTSCPStopWatch timer;

    for (uint32_t i = 0; i < c_NumTestLoops; i++)
    {
        m_senderReady = true;
        while (!m_receiverReady);
        m_senderDone = false;

        timer.Start();
        while ((timer.GetCurrentMilliseconds()) < c_SendDelayMS) {} // need to give time for the receive thread to actually suspend waiting on the object.
        timer.Stop();
        m_calcedDelayUS = timer.GetTotalMicroseconds();

        m_waitAddress++;
        timer.Start();
        WakeByAddressSingle(&m_waitAddress);
        timer.Stop();
        m_releaseTimingsUS[i] = timer.GetTotalMicroseconds();

        while (!m_receiverDone) { /*SwitchToThread();*/ }
        m_senderReady = false;
        m_senderDone = true;
    }
}

void PerfRun::ReceiveWaitAddressFunction(TestType /*whichTest*/, uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();

    while (!m_startTestRun);
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriority(curThread, priority);
    ATG::SetThreadPriorityBoost(curThread, true);
    ATG::RDTSCPStopWatch timer;

    for (uint32_t loopCount = 0; loopCount < c_NumTestLoops; loopCount++)
    {
        m_receiverReady = true;
        while (!m_senderReady);
        uint64_t currentAddressValue = m_waitAddress;

        timer.Start();
        WaitOnAddress(&m_waitAddress, &currentAddressValue, 8, INFINITE);
        timer.Stop();

        m_receiverReady = false;
        m_receiverDone = true;
        while (!m_senderDone) { /*SwitchToThread();*/ }

        m_memoryRaceTimingsUS[loopCount] = timer.GetTotalMicroseconds() < m_calcedDelayUS ? 0 : timer.GetTotalMicroseconds() - m_calcedDelayUS;
        m_memoryRaceTimingsRaw[loopCount] = timer.GetTotalRaw();
        m_memoryRaceTimingsCalcedDeltaUS[loopCount] = m_calcedDelayUS;

        m_receiverDone = false;
    }
}

void PerfRun::SendCVFunction(TestType whichTest, uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();
    while (!m_startTestRun);
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriority(curThread, priority);
    ATG::SetThreadPriorityBoost(curThread, true);
    ATG::RDTSCPStopWatch timer;

    KernelFuncs[whichTest].second();
    for (uint32_t i = 0; i < c_NumTestLoops; i++)
    {
        m_senderReady = true;
        while (!m_receiverReady);
        KernelFuncs[whichTest].first();
        timer.Start();
        while ((timer.GetCurrentMilliseconds()) < c_SendDelayMS) {} // need to give time for the receive thread to actually suspend waiting on the object.
        timer.Stop();
        m_calcedDelayUS = timer.GetTotalMicroseconds();

        timer.Start();
        WakeConditionVariable(&m_conditionVariable);
        timer.Stop();
        m_releaseTimingsUS[i] = timer.GetTotalMicroseconds();

        while (!m_receiverDone) { /*SwitchToThread();*/ }
        m_senderReady = false;
        m_senderDone = true;

        KernelFuncs[whichTest].second();
        m_senderDone = false;
    }
    KernelFuncs[whichTest].first();
}

void PerfRun::ReceiveCVFunction(TestType whichTest, uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();

    while (!m_startTestRun);
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriority(curThread, priority);
    ATG::SetThreadPriorityBoost(curThread, true);
    ATG::RDTSCPStopWatch timer;

    for (uint32_t loopCount = 0; loopCount < c_NumTestLoops; loopCount++)
    {
        m_receiverReady = true;
        while (!m_senderReady);

        timer.Start();
        KernelFuncs[whichTest].second();
        if (whichTest == PERFORM_CONDITION_CS)
            SleepConditionVariableCS(&m_conditionVariable, &m_critSection, INFINITE);
        else
            SleepConditionVariableSRW(&m_conditionVariable, &m_srw, INFINITE, !CONDITION_VARIABLE_LOCKMODE_SHARED);
        timer.Stop();

        m_receiverReady = false;
        m_receiverDone = true;
        while (!m_senderDone) { /*SwitchToThread();*/ }

        m_memoryRaceTimingsUS[loopCount] = timer.GetTotalMicroseconds() < m_calcedDelayUS ? 0 : timer.GetTotalMicroseconds() - m_calcedDelayUS;
        m_memoryRaceTimingsRaw[loopCount] = timer.GetTotalRaw();
        m_memoryRaceTimingsCalcedDeltaUS[loopCount] = m_calcedDelayUS;

        m_receiverDone = false;
        KernelFuncs[whichTest].first();
    }
}

//////////////////////////////////////////////////////////////////////////
//
//
//
// The no contention functions
//
//
//////////////////////////////////////////////////////////////////////////

void PerfRun::ReceiveKernelFunctionNoContention(TestType whichTest, uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();

    while (!m_startTestRun);
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriorityBoost(curThread, true);
    ATG::SetThreadPriority(curThread, priority);
    ATG::RDTSCPStopWatch timer;

    for (uint32_t loopCount = 0; loopCount < c_NumTestLoops; loopCount++)
    {
        timer.Start();
        KernelFuncs[whichTest].second();
        timer.Stop();

        m_memoryRaceTimingsUS[loopCount] = timer.GetTotalMicroseconds();

        timer.Start();
        KernelFuncs[whichTest].first();
        timer.Stop();

        m_releaseTimingsUS[loopCount] = timer.GetTotalMicroseconds();
    }
}

void PerfRun::ReceiveWaitAddressFunctionNoContention(TestType /*whichTest*/, uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();

    while (!m_startTestRun);
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriority(curThread, priority);
    ATG::SetThreadPriorityBoost(curThread, true);
    ATG::RDTSCPStopWatch timer;

    for (uint32_t loopCount = 0; loopCount < c_NumTestLoops; loopCount++)
    {
        uint64_t currentAddressValue = m_waitAddress + 1;

        timer.Start();
        WaitOnAddress(&m_waitAddress, &currentAddressValue, 8, INFINITE);
        timer.Stop();

        m_memoryRaceTimingsUS[loopCount] = timer.GetTotalMicroseconds();

        timer.Start();
        WakeByAddressSingle(&m_waitAddress);
        timer.Stop();
        m_releaseTimingsUS[loopCount] = timer.GetTotalMicroseconds();
    }
}

void PerfRun::ReceiveCVFunctionNoContention(TestType /*whichTest*/, uint32_t core, int32_t priority)
{
    HANDLE curThread = GetCurrentThread();

    while (!m_startTestRun);
    ATG::SetThreadAffinityMask(curThread, 1ULL << core);
    ATG::SetThreadPriority(curThread, priority);
    ATG::SetThreadPriorityBoost(curThread, true);

    for (uint32_t loopCount = 0; loopCount < c_NumTestLoops; loopCount++)
    {
        m_releaseTimingsUS[loopCount] = 0;
    }
}
