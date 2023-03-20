//--------------------------------------------------------------------------------------
// CompletionEvent.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CompletionEvent.h"

#define VALIDATE_READS

#if ((_GRDK_VER >= 0x585D073C)&&(defined _GAMING_XBOX_SCARLETT)) || defined(_GAMING_DESKTOP) /* GDK Edition 221000 */
// DirectStorage supports three main methods of notification, status block, ID3D12Fence, and Windows Events
// This sample shows how to use a Windows Event for notification
// Using a Windows Event for notification is only supported on Xbox using the October 2022 GXDK or DirectStorage on PC

bool CompletionEvent::RunSample(const std::wstring& fileName, uint64_t dataFileSize)
{
    OpenFile(fileName);

    // Helper code that creates a set of random reads and matching destination buffers
    GenerateRandomReadLocationsAndDestinationBuffers(dataFileSize, c_dataBatches * c_readsPerBatch);

    // enqueue all the reads using multiple batches
    uint32_t curBuffer = 0;
    for (uint32_t curBatch = 0; curBatch < c_dataBatches; ++curBatch)
    {
        // Create an Event for each batch that is being enqueued, Events should be reused when they are no longer waiting on a batch to complete
        m_completionEvent[curBatch] = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        for (uint32_t curRead = 0; curRead < c_readsPerBatch; curRead++, ++curBuffer)
        {
            DStorageRequestCrossPlatform request = {};
            request.SetupUncompressedRead(s_files[fileName].Get(), m_destinationBuffers[curBuffer].get(), m_readLocations[curBuffer], m_readSizes[curBuffer]);
            s_queues[0]->EnqueueRequest(&request);
        }

        // Enqueue one status notification per batch as opposed to per read requeust
        s_queues[0]->EnqueueSetEvent(m_completionEvent[curBatch]);
    }

    // Submit the entire queue to the hardware
    s_queues[0]->Submit();

    // Wait for the first batch to complete, signaled through slot 0 in the status array
    WaitForSingleObject(m_completionEvent[0], INFINITE);
    // At this point we can only say all reads in the first batch have completed

    // wait for the last batch to complete
    // signaled through the last slot in the status array
    WaitForSingleObject(m_completionEvent[c_dataBatches - 1], INFINITE);
    // At this point it's guaranteed all the batches have finished
    // Notification is always in FIFO order

    // check the contents of the data reads from the file to make sure the correct contents were loaded
#ifdef VALIDATE_READS
    curBuffer = 0;
    for (uint32_t curBatch = 0; curBatch < c_dataBatches; ++curBatch, ++curBuffer)
    {
        for (uint32_t curRead = 0; curRead < c_readsPerBatch; curRead++)
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

    for (auto& iter : m_completionEvent)
    {
        CloseHandle(iter);
    }
    return true;
}
#endif
