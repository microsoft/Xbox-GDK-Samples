//--------------------------------------------------------------------------------------
// InMemoryQueueHandler.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "InMemoryQueueHandler.h"

InMemoryQueueHandler::InMemoryQueueHandler(uint64_t uniqueDataSize, uint64_t totalStreamSize, float initialRandPercent, uint16_t initialAlignment, uint32_t initialChunkSize):
    DSQueueHandler(false, uniqueDataSize, totalStreamSize),
    m_initialRandPercent(initialRandPercent),
    m_initialAlignment(initialAlignment),
    m_initialChunkSize(initialChunkSize)
{
}

void InMemoryQueueHandler::WorkerStartupFunction()
{
    TaskStatus = TaskStatus::GeneratingNewDataSet;
    ActiveOriginData = DataStream::Generate(m_uniqueDataSize, m_initialRandPercent);

    TaskStatus = TaskStatus::CompressingData;

    ActiveCompressedStream = CompressedDataStream::GenerateInMemory(
        ActiveOriginData,
        m_dataSize,
        m_initialChunkSize,
        m_initialAlignment);

    TaskStatus = TaskStatus::Idle;
}
