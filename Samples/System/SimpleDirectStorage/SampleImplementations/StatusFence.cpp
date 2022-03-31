//--------------------------------------------------------------------------------------
// StatusFence.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "StatusFence.h"

#define VALIDATE_READS

bool StatusFence::RunSample(const std::wstring& fileName, ID3D12Device* device, uint64_t dataFileSize)
{
    // Create the Event that we can associate with the ID3D12Fence that we can wait on
    m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
    if (m_fenceEvent == nullptr)
        throw DX::com_exception(static_cast<HRESULT> (GetLastError()));

    // Create an ID3D12Fence object to use with DirectStorage
    DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
    m_fence->SetName(L"SimpleDirectStorage Fence");

    // Shared code to initialize DirectStorage, open the data file, and create a queue
    // The queue size has enough space for all reads and a status slot for each batch
    SetupDirectStorage(fileName, (c_dataBatches * c_readsPerBatch) + c_dataBatches);

    // Helper code that creates a set of random reads and match destination buffers
    GenerateRandomReadLocationsAndDestinationBuffers(dataFileSize, c_dataBatches * c_readsPerBatch);

    uint32_t curBuffer = 0;
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
            m_queue->EnqueueRequest(&request);
        }

        m_queue->EnqueueSignal(m_fence.Get(), curBatch + 1);
    }
    m_queue->Submit();

    while (m_fence->GetCompletedValue() == 0)
    {
        Sleep(1);
    }
    // at this point we can only say that all batches up to the current fence value are completed

    // NOTE: This function spins on the calling thread. It's recommended you call this from a core/thread you can afford to spin
    m_fence->SetEventOnCompletion(c_dataBatches, m_fenceEvent);

    // NOTE: Since the SetEventOnCompletion function spins until the fence is signaled this function should immediately return WAIT_OBJECT_0
    DX::ThrowIfFailed(WaitForSingleObject(m_fenceEvent, INFINITE) != WAIT_OBJECT_0);
    // at this point it's guaranteed all of the batches have finished since notification is always in queue order

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

    ShutdownDirectStorage();
    return true;
}
