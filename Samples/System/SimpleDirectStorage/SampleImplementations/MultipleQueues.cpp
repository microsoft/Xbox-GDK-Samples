//--------------------------------------------------------------------------------------
// MultipleQueues.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MultipleQueues.h"

#define VALIDATE_READS

const uint16_t MultipleQueues::c_readsPerQueue[c_numberOfQueues] = { 25,15,3 };

bool MultipleQueues::RunSample(const std::wstring& fileName, uint64_t dataFileSize)
{
    // Shared code to initialize DirectStorage, open the data file, and create a queue
    // The queue size has enough space for all reads and a status slot for each batch
    SetupDirectStorage(fileName, 1);

    uint32_t totalNumberOfReads = 0;
    for (uint32_t curQueue = 0; curQueue < c_numberOfQueues; ++curQueue)
    {
        totalNumberOfReads += c_readsPerQueue[curQueue];

        // create at least a queue at each priority level, the rest of the queues are all set to normal
        DSTORAGE_QUEUE_DESC queueDesc = {};
        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        switch (curQueue)
        {
        case 0:
            queueDesc.Priority = DSTORAGE_PRIORITY_HIGH;
            queueDesc.Name = u8"SimpleDirectStorage: Multiple Queues at High Priority";
            break;
        case 1:
            queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
            queueDesc.Name = u8"SimpleDirectStorage: Multiple Queues at Normal Priority";
            break;
        case 2:
            queueDesc.Priority = DSTORAGE_PRIORITY_LOW;
            queueDesc.Name = u8"SimpleDirectStorage: Multiple Queues at Low Priority";
            break;
        default:
            queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
            queueDesc.Name = u8"SimpleDirectStorage: Multiple Queues at Normal Priority";
        }

        // ensure enough space in the queue for all requests and a slot for the status notification
        queueDesc.Capacity = std::max<uint16_t>(c_readsPerQueue[curQueue] + 1U, DSTORAGE_MIN_QUEUE_CAPACITY);
        DX::ThrowIfFailed(m_factory->CreateQueue(&queueDesc, __uuidof(IDStorageQueueX), (void **)(m_priorityQueues[curQueue].ReleaseAndGetAddressOf())));

        // A separate status array is created for each queue
        // Notification happens in FIFO order, but that is per queue
        DX::ThrowIfFailed(m_factory->CreateStatusArray(1, u8"MultipleQueues Status Array", __uuidof(IDStorageStatusArrayX), (void **)(m_statusEntries[curQueue].ReleaseAndGetAddressOf())));
    }

    // Helper code that creates a set of random reads and match destination buffers
    GenerateRandomReadLocationsAndDestinationBuffers(dataFileSize, totalNumberOfReads);

    // enqueue all the reads with each batch going into a different queue
    uint32_t curBuffer = 0;
    for (uint32_t curQueue = 0; curQueue < c_numberOfQueues; ++curQueue)
    {
        for (uint32_t curRead = 0; curRead < c_readsPerQueue[curQueue]; ++curRead, ++curBuffer)
        {
            DSTORAGE_REQUEST request = {};
            request.File = m_file.Get();
            request.Destination = m_destinationBuffers[curBuffer].get();
            request.DestinationSize = m_readSizes[curBuffer];
            request.FileOffset = m_readLocations[curBuffer];
            request.SourceSize = m_readSizes[curBuffer];
            m_priorityQueues[curQueue]->EnqueueRequest(&request);
        }

        // enqueue the notification status, but use a unique status array per queue
        m_priorityQueues[curQueue]->EnqueueStatus(m_statusEntries[curQueue].Get(), 0);

        // Submit just the requests for this particular queue
        m_priorityQueues[curQueue]->Submit();
    }

    while (!m_statusEntries[0]->IsComplete(0))
    {
        Sleep(1);
    }

    // at this point all of the high priority requests have completed
    // Nothing can be said about other requests since they were in a different queue and complete in their own FIFO order

    // Have to wait for completion for each queue individually since each queue is using its own status array object
    for (uint32_t curQueue = 1; curQueue < c_numberOfQueues; ++curQueue)
    {
        while (!m_statusEntries[curQueue]->IsComplete(0))
        {
            Sleep(1);
        }
    }

    // check the contents of the data reads from the file to make sure the correct contents were loaded
#ifdef VALIDATE_READS
    curBuffer = 0;
    for (uint32_t curQueue = 0; curQueue < c_numberOfQueues; ++curQueue)
    {
        for (uint32_t curRead = 0; curRead < c_readsPerQueue[curQueue]; ++curRead, ++curBuffer)
        {
            uint32_t startValue = static_cast<uint32_t> (m_readLocations[curBuffer] / sizeof(uint32_t));
            uint32_t* temp = (uint32_t*)m_destinationBuffers[curBuffer].get();
            uint32_t numberEntries = m_readSizes[curBuffer] / sizeof(uint32_t);
            for (uint32_t location = 0; location < numberEntries; location++)
            {
                if (temp[location] != startValue)
                    DebugBreak();
                assert(temp[location] == startValue);
                startValue++;
            }
        }
    }
#endif

    // Cleanup all related objects created
    for (uint32_t curQueue = 0; curQueue < c_numberOfQueues; ++curQueue)
    {
        m_priorityQueues[curQueue].Reset();
    }
    ShutdownDirectStorage();
    return true;
}
