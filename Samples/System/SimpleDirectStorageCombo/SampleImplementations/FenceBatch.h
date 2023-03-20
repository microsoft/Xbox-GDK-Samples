//--------------------------------------------------------------------------------------
// FenceBatch.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "ImplementationBase.h"

// DirectStorage supports two main methods of notification, status block and ID3D12Fence
// DirectStorage supports three main methods of notification, status block, ID3D12Fence, and Windows Events
// Note: On the console it's recommended to use DStorageStatusArray entries with EnqueueStatus, they will usually be overall faster than an ID3DFence object
//     On the console it's only recommended to use EnqueueSignal and an ID3D12Fence if the GPU can use the data immediately after the read with no CPU interaction needed
class FenceBatch : public ImplementationBase
{
private:
    static const uint32_t c_dataBatches = 10;
    static const uint32_t c_readsPerBatch = 7;

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;		// only one fence is needed since batches complete inorder, each batch gets a constantly incrementing number
    HANDLE m_fenceEvent;

public:
    FenceBatch() : m_fence(nullptr) {}
    ~FenceBatch() = default;

    bool RunSample(const std::wstring& fileName, ID3D12Device* device, uint64_t dataFileSize);
};
