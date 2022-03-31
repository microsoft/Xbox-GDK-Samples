//--------------------------------------------------------------------------------------
// RecommendedPattern.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "RecommendedPattern.h"

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"
#endif

#define VALIDATE_READS

// This shows the recommended pattern when using DirectStorage.
// It is functionally almost identical to the StatusBatch sample with one important difference
// The size of the batches and the number of batches being issues before submission
// In this case it's 20 batches with 50 read requests per batch for a total of 1,000 read requests
// This helps to avoid adding any bubbles in the pipeline waiting for the title to submit new requests
bool RecommendedPattern::RunSample(const std::wstring& fileName, uint64_t dataFileSize)
{
    // Shared code to initialize DirectStorage, open the data file, and create a queue
    // The queue size has enough space for all reads and a status slot for each batch
    SetupDirectStorage(fileName, (c_dataBatches * c_readsPerBatch) + c_dataBatches);

    // Helper code that creates a set of random reads and match destination buffers
    GenerateRandomReadLocationsAndDestinationBuffers(dataFileSize, c_dataBatches * c_readsPerBatch);

    // Create the status array with a single slot for each batch that is being enqueued
    DX::ThrowIfFailed(m_factory->CreateStatusArray(c_dataBatches, u8"RecommendedPattern Status Array", __uuidof(IDStorageStatusArrayX), (void **)(m_statusEntries.ReleaseAndGetAddressOf())));

    // enqueue all the reads using multiple batches
    uint32_t curBuffer = 0;
    uint64_t totalReadSize = 0;
    for (uint32_t curBatch = 0; curBatch < c_dataBatches; ++curBatch)
    {
        for (uint32_t curRead = 0; curRead < c_readsPerBatch; curRead++, ++curBuffer)
        {
            DSTORAGE_REQUEST request = {};
            request.File = m_file.Get();
            request.Destination = m_destinationBuffers[curBuffer].get();
            request.DestinationSize = m_readSizes[curBuffer];
            request.FileOffset = m_readLocations[curBuffer];
            request.SourceSize = m_readSizes[curBuffer];
            totalReadSize += m_readSizes[curBuffer];
            m_queue->EnqueueRequest(&request);
        }

        // Enqueue one status notification per batch as opposed to per read requeust
        m_queue->EnqueueStatus(m_statusEntries.Get(), curBatch);
    }

    // Submit the entire queue to the hardware
    m_queue->Submit();

    // Wait for the first batch to complete, signaled through slot 0 in the status array
    while (!m_statusEntries->IsComplete(0))
    {
        Sleep(1);
    }
    // At this point we can only say all reads in the first batch have completed

    // wait for the last batch to complete
    // signaled through the last slot in the status array
    while (!m_statusEntries->IsComplete(c_dataBatches - 1))
    {
        Sleep(1);
    }
    // At this point it's guaranteed all the batches have finished
    // Notification is always in FIFO order

    // check the contents of the data reads from the file to make sure the correct contents were loaded
#ifdef VALIDATE_READS
    curBuffer = 0;
    for (uint32_t curBatch = 0; curBatch < c_dataBatches; ++curBatch)
    {
        for (uint32_t curRead = 0; curRead < c_readsPerBatch; curRead++, ++curBuffer)
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
    ShutdownDirectStorage();

    return true;
}
