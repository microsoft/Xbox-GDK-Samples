//--------------------------------------------------------------------------------------
// FenceBatch.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FenceBatch.h"

#define VALIDATE_READS

bool FenceBatch::RunSample(const std::wstring& fileName, ID3D12Device* device, uint64_t dataFileSize)
{
    OpenFile(fileName);
    // Create the Event that we can associate with the ID3D12Fence that we can wait on
    m_fenceEvent = CreateEvent(nullptr, false, false, nullptr);
    if (m_fenceEvent == nullptr)
        throw DX::com_exception(static_cast<HRESULT> (GetLastError()));

    // Create an ID3D12Fence object to use with DirectStorage
    DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
    m_fence->SetName(L"SimpleDirectStorage Fence");

    // Helper code that creates a set of random reads and matching destination buffers
    GenerateRandomReadLocationsAndDestinationBuffers(dataFileSize, c_dataBatches * c_readsPerBatch);

    uint32_t curBuffer = 0;
    for (uint32_t curBatch = 0; curBatch < c_dataBatches; ++curBatch)
    {
        for (uint32_t curRead = 0; curRead < c_readsPerBatch; curRead++, ++curBuffer)
        {
            DStorageRequestCrossPlatform request = {};
            request.SetupUncompressedRead(s_files[fileName].Get(), m_destinationBuffers[curBuffer].get(), m_readLocations[curBuffer], m_readSizes[curBuffer]);
            s_queues[0]->EnqueueRequest(&request);
        }

        s_queues[0]->EnqueueSignal(m_fence.Get(), curBatch + 1);
    }
    s_queues[0]->Submit();

    while (m_fence->GetCompletedValue() == 0)
    {
        Sleep(1);
    }
    // at this point we can only say that all batches up to the current fence value are completed

    // NOTE: On the console this function spins on the calling thread. It's recommended you call this from a core/thread you can afford to spin
    // Because of this on the console it's recommended to use DStorageStatusArray entries with EnqueueStatus, they will usually be overall faster than an ID3DFence object
    //     On the console it's only recommended to use EnqueueSignal and a fence if the GPU can use the data immediately after the read with no CPU interaction needed
    m_fence->SetEventOnCompletion(c_dataBatches, m_fenceEvent);

    // NOTE: Since on the console the SetEventOnCompletion function spins until the fence is signaled this function should immediately return WAIT_OBJECT_0
    if(WaitForSingleObject(m_fenceEvent, INFINITE) != WAIT_OBJECT_0)
    {
        throw std::runtime_error("WaitForSingleObject");
    }

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
    return true;
}
