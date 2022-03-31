//--------------------------------------------------------------------------------------
// MultipleQueues.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MultipleQueues.h"

#define VALIDATE_READS

const uint16_t MultipleQueues::c_readsPerQueue[c_defaultNumQueues] = { 25,15,3,5,20 };

bool MultipleQueues::RunSample(const std::wstring& fileName, uint64_t dataFileSize)
{
    OpenFile(fileName);

    uint32_t totalNumberOfReads = 0;
    // Note: It's recommended that a title create their main queues during title startup
    //    They are a circular list that represents a pipeline where new requests can be added as space becomes available
    //    There is no need to constantly create and destroy them, this only leads to lower performance due to unnecessary work
    for (uint32_t curQueue = 0; curQueue < c_defaultNumQueues; ++curQueue)
    {
        totalNumberOfReads += c_readsPerQueue[curQueue];

        // A separate status array is created for each queue
        // Notification happens in FIFO order, but that is per queue
        DX::ThrowIfFailed(s_factory->CreateStatusArray(1, u8"MultipleQueues Status Array", __uuidof(DStorageStatusArrayCrossPlatform), (void**)(m_statusEntries[curQueue].ReleaseAndGetAddressOf())));
    }

    // Helper code that creates a set of random reads and matching destination buffers
    GenerateRandomReadLocationsAndDestinationBuffers(dataFileSize, totalNumberOfReads);

    // enqueue all the reads with each batch going into a different queue
    uint32_t curBuffer = 0;
    for (uint32_t curQueue = 0; curQueue < c_defaultNumQueues; ++curQueue)
    {
        for (uint32_t curRead = 0; curRead < c_readsPerQueue[curQueue]; ++curRead, ++curBuffer)
        {
            DStorageRequestCrossPlatform request = {};
            request.SetupUncompressedRead(s_files[fileName].Get(), m_destinationBuffers[curBuffer].get(), m_readLocations[curBuffer], m_readSizes[curBuffer]);
            s_queues[curQueue]->EnqueueRequest(&request);
        }

        // enqueue the notification status, but use a unique status array per queue
        s_queues[curQueue]->EnqueueStatus(m_statusEntries[curQueue].Get(), 0);

        // Submit just the requests for this particular queue
        s_queues[curQueue]->Submit();
    }

    // Wait for just the entries in the first queue are complete, the high priority queue
    while (!m_statusEntries[1]->IsComplete(0))
    {
        Sleep(1);
    }

    // at this point all of the high priority requests have completed, the requests in the second queue
    // Nothing can be said about other requests since they were in a different queue and complete in their own FIFO order

    // Have to wait for completion for each queue individually since each queue is using its own status array object
    // Note: It's safe to query the requests in the second queue again, the status entry is not reset until it's been Enqueued again
    for (uint32_t curQueue = 0; curQueue < c_defaultNumQueues; ++curQueue)
    {
        while (!m_statusEntries[curQueue]->IsComplete(0))
        {
            Sleep(1);
        }
    }

    // check the contents of the data reads from the file to make sure the correct contents were loaded
#ifdef VALIDATE_READS
    curBuffer = 0;
    for (uint32_t curQueue = 0; curQueue < c_defaultNumQueues; ++curQueue)
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

    return true;
}
