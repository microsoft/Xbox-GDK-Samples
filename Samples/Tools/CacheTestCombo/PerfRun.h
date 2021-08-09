//--------------------------------------------------------------------------------------
// PerfRun.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "FileLogger.h"
#include "UserLockable.h"

class PerfRun
{
public:
    enum TestType
    {
        PERFORM_RAW_READ,
        PERFORM_ATOMIC_READ,
        PERFORM_RAW_WRITE,
        PERFORM_ATOMIC_WRITE,
        PERFORM_ATOMIC_CAS,

        FirstTestExecuted = PERFORM_RAW_READ,
        LastTestExecuted = PERFORM_ATOMIC_CAS,

        FirstTestDefined = PERFORM_RAW_READ,
        LastTestDefined = PERFORM_ATOMIC_CAS,
        NumTestsDefined = PERFORM_ATOMIC_CAS + 1,
    };

    static const uint32_t c_NumCoreIterations = 100;
    static const uint32_t c_NumSingleOperations = 40000;            // how many core operations to do for each core iteration

    static const uint32_t c_CacheLineSize = 64;                             // 64 bytes

    static const uint32_t c_maxCoreMaskIndexes = 6;
#if defined(_GAMING_XBOX_SCARLETT)
    static const uint32_t c_maxNumWorkerThreads = 14;
#elif defined (_GAMING_XBOX)
    static const uint32_t c_maxNumWorkerThreads = 7;
#else
    static const uint32_t c_maxNumWorkerThreads = 16;
#endif

private:
    struct ThreadParams
    {
        uint64_t threadId;      // which thread this is, used for which log line to save timing data
        uint64_t coreNum;       // which core this thread should run on
        uint64_t iterations;    // How many iterations to perform over the data
        uint64_t baseIndex;     // Which worker data value to use
        bool forceStart;
    };

#pragma warning (disable:4324)  // WorkerData is padded due to alignment
    struct alignas(c_CacheLineSize)WorkerData
    {
        uint64_t m_data;
    };

    WorkerData* m_workerData;

    std::map<std::wstring, ATG::FileLogger*> m_openLogs;
    std::vector<std::wstring> m_coreTestNames;
    uint64_t* m_singleTimePerThread;
    uint64_t m_combinedTimePerThread[c_maxCoreMaskIndexes][NumTestsDefined][2][c_maxNumWorkerThreads][c_NumCoreIterations];		// second is shared memory or not
    uint64_t** m_totalTimePerThread;// [c_NumWorkerThreads][c_NumCoreIterations];
    std::thread** m_threadBuffer;
    ATG::UserBarrier<false> m_startBarrier, m_finishBarrier, m_resourceCreationBarrier;

    ATG::FileLogger* GetOpenLog(const std::wstring& fileName);
    void DumpLogFile(TestType whichTest, uint64_t coreMask, uint64_t numIterations, bool sharedMemory);

    uint64_t WorkerThreadRead(ThreadParams params);
    uint64_t WorkerThreadAtomicRead(ThreadParams params);
    uint64_t WorkerThreadWrite(ThreadParams params);
    uint64_t WorkerThreadAtomicWrite(ThreadParams params);
    uint64_t WorkerThreadAtomicCAS(ThreadParams params);

public:
    PerfRun(const std::vector<std::wstring>& testNames);
    ~PerfRun();

    void RunTests(TestType whichTest, uint64_t coreMask, uint64_t numIterations, uint32_t coreMaskIndex);
};
