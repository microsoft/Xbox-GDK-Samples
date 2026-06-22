//--------------------------------------------------------------------------------------
// JobQueue.cpp
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "precomp.hpp"
#include "JobQueue.h"

JobQueue g_JobQueue;

JobQueue::JobQueue(void)
{
    ZeroMemory( m_hWorkerThreads, sizeof(m_hWorkerThreads) );
}

JobQueue::~JobQueue(void)
{
}

void JobQueue::Initialize( const JobQueueDesc* pDesc )
{
    m_Desc = *pDesc;

    InitializeCriticalSection( &m_QueueCritSec );
    m_Terminate = false;

    m_Desc.ThreadCount = std::min( m_Desc.ThreadCount, (UINT32)ARRAYSIZE(m_hWorkerThreads) );

    m_hJobReadyEvent = CreateEvent(nullptr, FALSE, FALSE, L"Job Queue Job Ready Event");

    for (UINT32 i = 0; i < m_Desc.ThreadCount; ++i)
    {
        m_InitParams[i].pQueue = this;
        m_InitParams[i].ThreadIndex = i;
        m_hWorkerThreads[i] = CreateThread( nullptr, 0, (LPTHREAD_START_ROUTINE)WorkerThreadEntry, &m_InitParams[i], 0, nullptr );
    }
}

DWORD JobQueue::WorkerLoop( const ThreadInitParams* pParams )
{
    assert( pParams != nullptr );
    const UINT32 ThreadIndex = pParams->ThreadIndex;

    void* pThreadData = nullptr;
    if (m_Desc.pJobThreadDataInitFunction != nullptr)
    {
        pThreadData = m_Desc.pJobThreadDataInitFunction( ThreadIndex );
    }

    JobEntry CurrentJob = {};
    while (!m_Terminate)
    {
        bool JobReady = GetNextJob( &CurrentJob );
        if (JobReady)
        {
            CurrentJob.pFunction( CurrentJob.pContext, CurrentJob.pData[0], CurrentJob.pData[1], pThreadData );
        }
        else if (!m_Terminate)
        {
            WaitForSingleObject(m_hJobReadyEvent, INFINITE);
            //SwitchToThread();
        }
    }

    if (m_Desc.pJobThreadDataFreeFunction != nullptr)
    {
        m_Desc.pJobThreadDataFreeFunction( ThreadIndex, pThreadData );
    }
    pThreadData = nullptr;

    return 0;
}

void JobQueue::AddJob( JobWorkerFunction pFunction, void* pContext, void* pData0, void* pData1 )
{
    assert( pFunction != nullptr );
    JobEntry Entry;
    Entry.pFunction = pFunction;
    Entry.pContext = pContext;
    Entry.pData[0] = pData0;
    Entry.pData[1] = pData1;

    EnterCriticalSection( &m_QueueCritSec );
    m_Jobs.push_back( Entry );
    LeaveCriticalSection( &m_QueueCritSec );

    SetEvent(m_hJobReadyEvent);
}

bool JobQueue::GetNextJob( JobEntry* pEntry )
{
    bool Result = false;
    EnterCriticalSection( &m_QueueCritSec );
    if (!m_Jobs.empty())
    {
        *pEntry = m_Jobs.front();
        m_Jobs.pop_front();
        Result = true;
    }
    LeaveCriticalSection( &m_QueueCritSec );
    return Result;
}

DWORD WINAPI JobQueue::WorkerThreadEntry( const ThreadInitParams* pInitParams )
{
    JobQueue* pQueue = pInitParams->pQueue;
    return pQueue->WorkerLoop( pInitParams );
}
