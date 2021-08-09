//--------------------------------------------------------------------------------------
// PerfRun.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "FileLogger.h"
#include "UserLockable.h"

#pragma warning (push)
#pragma warning (disable: 4324)
#pragma warning (disable: 4061)     // unused enum define in a switch statement
class PerfRun
{
public:
    enum TestType
    {
        PERFORM_SEMAPHORE,
        PERFORM_EVENT,
        PERFORM_MUTEX,
        PERFORM_SRW,
        PERFORM_CRITICAL_SECTION,
        PERFORM_WAIT_ADDRESS,
        PERFORM_CONDITION_CS,
        PERFORM_CONDITION_SRW,
        NumTestTypeDefined,

        FirstTestExecuted = PERFORM_SEMAPHORE,
        LastTestExecuted = PERFORM_CONDITION_SRW,

        FirstTestDefined = PERFORM_SEMAPHORE,
        LastTestDefined = PERFORM_CONDITION_SRW,
    };
    static std::wstring ConvertTestTypeToString(TestType testType)
    {
        switch (testType)
        {
        case PERFORM_SEMAPHORE:
            return L"Semaphore";
        case PERFORM_EVENT:
            return L"Event";
        case PERFORM_SRW:
            return L"SRW";
        case PERFORM_MUTEX:
            return L"Mutex";
        case PERFORM_CRITICAL_SECTION:
            return L"Critical Section";
        case PERFORM_WAIT_ADDRESS:
            return L"WaitOnAddress";
        case PERFORM_CONDITION_CS:
            return L"Condition critical section";
        case PERFORM_CONDITION_SRW:
            return L"Condition SRW";
        default:
            return L"Unknown";
        }
    }

    enum CoreTestType
    {
        SINGLE_CORE,
        SAME_PHYSICAL,
        SAME_CLUSTER,
        CROSS_CLUSTER
    };

    static const uint32_t c_NumTestLoops = 10000;
    static const uint32_t c_SendDelayMS = 1;
    static const uint32_t c_numCoreTestTypes = 4;
    //static const uint32_t s_MaxCore = 7;

private:
    ATG::UserBarrier<false> m_topBarrier, m_middleBarrier, m_bottomBarrier;
    std::thread* m_senderThread;
    std::thread* m_receiverThread;
    std::thread* m_workerThread;
    std::atomic<bool> m_startTestRun;
    std::atomic<bool> m_senderReady;
    std::atomic<bool> m_receiverReady;
    std::atomic<bool> m_senderDone;
    std::atomic<bool> m_receiverDone;
    std::atomic<bool> m_workerShutdownFlag;
    std::atomic<double> m_calcedDelayUS;
    uint64_t m_waitAddress;

    double m_memoryRaceTimingsUS[c_NumTestLoops];
    uint64_t m_memoryRaceTimingsRaw[c_NumTestLoops];
    double m_memoryRaceTimingsCalcedDeltaUS[c_NumTestLoops];
    double m_releaseTimingsUS[c_NumTestLoops];

    double m_combinedMemoryRaceTimingsUS[NumTestTypeDefined][c_numCoreTestTypes][c_NumTestLoops];
    double m_combinedReleaseTimingsUS[NumTestTypeDefined][c_numCoreTestTypes][c_NumTestLoops];

    ATG::FileLogger* m_acquireLogfile;
    ATG::FileLogger* m_releaseLogFile;
    ATG::FileLogger* m_combinedLogfile;

    HANDLE m_semaphore;
    HANDLE m_event;
    HANDLE m_mutex;
    SRWLOCK m_srw;
    CRITICAL_SECTION m_critSection;
    CONDITION_VARIABLE m_conditionVariable;

    typedef std::function<void WINAPI()> KernelFuncObject;
    typedef std::pair<KernelFuncObject, KernelFuncObject> KernelFuncPair;
    KernelFuncPair KernelFuncs[NumTestTypeDefined];

    void WorkerThread(uint32_t core, int32_t priority);
    void SendKernelFunction(TestType whichTest, uint32_t core, int32_t priority);
    void ReceiveKernelFunction(TestType whichTest, uint32_t core, int32_t priority);
    void ReceiveKernelFunctionNoContention(TestType whichTest, uint32_t core, int32_t priority);

    void SendWaitAddressFunction(TestType whichTest, uint32_t core, int32_t priority);
    void ReceiveWaitAddressFunction(TestType whichTest, uint32_t core, int32_t priority);
    void ReceiveWaitAddressFunctionNoContention(TestType whichTest, uint32_t core, int32_t priority);

    void SendCVFunction(TestType whichTest, uint32_t core, int32_t priority);
    void ReceiveCVFunction(TestType whichTest, uint32_t core, int32_t priority);
    void ReceiveCVFunctionNoContention(TestType whichTest, uint32_t core, int32_t priority);

    void OpenLogFiles(bool idleWorkers);

public:
    PerfRun();
    virtual ~PerfRun();

    virtual void RunTests(uint32_t senderCore, uint32_t receiverCore, TestType whichTest, bool idleWorkers, const std::wstring& testName, uint32_t coreTestType);
};
#pragma warning (pop)
