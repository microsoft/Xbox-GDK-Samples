//--------------------------------------------------------------------------------------
// DSQueueHandler.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CompressedDataStream.h"

enum QueueStatus
{
    PreInit,
    Running,
    Paused,
    Stopped
};

enum TaskStatus
{
    InitialState,
    Idle,
    GeneratingNewDataSet,
    CompressingData,
    StoringData,
};

class DSQueueHandler
{
public:

    void StartWorker();

    // communication properties
    std::atomic<TaskStatus>             TaskStatus;
    std::atomic<QueueStatus>            QueueStatus;
    std::atomic<float>                  MduInMbps;
    std::atomic<float>                  MduOutMbps;
    std::atomic<DataStream*>            ActiveOriginData;
    std::atomic<CompressedDataStream*>  ActiveCompressedStream;

protected:
    DSQueueHandler(bool fromStorage, uint64_t uniqueDataSize, uint64_t totalStreamSize);
    virtual ~DSQueueHandler();

    virtual void WorkerStartupFunction() = 0;
    void WorkerFunction();

    std::thread*                        m_workerThread = nullptr;
    bool                                m_fromStorage;
    uint64_t                            m_uniqueDataSize;
    uint64_t                            m_dataSize;
};

