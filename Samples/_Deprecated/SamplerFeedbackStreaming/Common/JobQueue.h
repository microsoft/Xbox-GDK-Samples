//--------------------------------------------------------------------------------------
// JobQueue.h
//
// A simple multithreaded job queue for scheduling tasks to be executed asynchronously.
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <windows.h>
#include <deque>

typedef UINT32 (*JobWorkerFunction)( void* pContext, void* pData0, void* pData1, void* pThreadData );
typedef void* (*JobThreadDataInitFunction)( UINT32 ThreadIndex );
typedef void (*JobThreadDataFreeFunction)( UINT32 ThreadIndex, void* pThreadData );

struct JobQueueDesc
{
    UINT32 ThreadCount;
    JobThreadDataInitFunction pJobThreadDataInitFunction;
    JobThreadDataFreeFunction pJobThreadDataFreeFunction;
};

#define JOB_QUEUE_MAX_THREADS 16

class JobQueue
{
private:
    struct JobEntry
    {
        JobWorkerFunction pFunction;
        void* pContext;
        void* pData[2];
    };

    CRITICAL_SECTION m_QueueCritSec;
    std::deque<JobEntry> m_Jobs;

    HANDLE m_hWorkerThreads[JOB_QUEUE_MAX_THREADS];
    bool m_Terminate;

    HANDLE m_hJobReadyEvent;

    struct ThreadInitParams
    {
        JobQueue* pQueue;
        UINT32 ThreadIndex;
    };
    ThreadInitParams m_InitParams[JOB_QUEUE_MAX_THREADS];

    JobQueueDesc m_Desc;

public:
    JobQueue(void);
    ~JobQueue(void);

    void Initialize( const JobQueueDesc* pDesc );

    void AddJob( JobWorkerFunction pFunction, void* pContext, void* pData0, void* pData1 );
    void ClearJobs();

private:
    DWORD WorkerLoop( const ThreadInitParams* pParams );
    static DWORD WINAPI WorkerThreadEntry( const ThreadInitParams* pInitParams );
    bool GetNextJob( JobEntry* pEntry );
};

extern JobQueue g_JobQueue;
