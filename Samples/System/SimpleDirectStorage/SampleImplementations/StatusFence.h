//--------------------------------------------------------------------------------------
// StatusFence.h
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
// This sample shows how to use an ID3D12Fence for notification
class StatusFence : public ImplementationBase
{
private:
    static constexpr uint32_t c_dataBatches = 10;
    static constexpr uint32_t c_readsPerBatch = 7;

    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;		// only one fence is needed since batches complete inorder, each batch gets a constantly incrementing number
    HANDLE m_fenceEvent;

public:
    StatusFence() {}
    ~StatusFence() {}

    bool RunSample(const std::wstring& fileName, ID3D12Device* device, uint64_t dataFileSize);
};
