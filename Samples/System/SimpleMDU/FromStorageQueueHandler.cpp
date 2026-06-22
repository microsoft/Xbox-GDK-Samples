//--------------------------------------------------------------------------------------
// FromStorageQueueHandler.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FromStorageQueueHandler.h"

FromStorageQueueHandler::FromStorageQueueHandler(uint64_t uniqueDataSize, uint64_t totalStreamSize, float initialRandPercent, uint16_t initialAlignment, uint32_t initialChunkSize) :
    DSQueueHandler(true, uniqueDataSize, totalStreamSize),
    m_initialRandPercent(initialRandPercent),
    m_initialAlignment(initialAlignment),
    m_initialChunkSize(initialChunkSize)
{
}

void FromStorageQueueHandler::WorkerStartupFunction()
{
    CompressedDataStream* targetInitialStream = CompressedDataStream::HydrateFromStorage(m_initialRandPercent, m_dataSize, m_initialChunkSize, m_initialAlignment);

    if (!targetInitialStream)
    {
        TaskStatus = TaskStatus::GeneratingNewDataSet;

        // just wait for the InMemory queue handlers to populate this
        targetInitialStream = CompressedDataStream::FetchMemoryBacked(m_initialRandPercent, m_dataSize, m_initialChunkSize, m_initialAlignment);
        while (targetInitialStream == nullptr)
        {
            Sleep(16);
            targetInitialStream = CompressedDataStream::FetchMemoryBacked(m_initialRandPercent, m_dataSize, m_initialChunkSize, m_initialAlignment);
        }

        TaskStatus = TaskStatus::StoringData;
        targetInitialStream->PersistToStorage();
    }

    ActiveCompressedStream = targetInitialStream;
    TaskStatus = TaskStatus::Idle;
}
