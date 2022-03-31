//--------------------------------------------------------------------------------------
// MultipleQueues.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "ImplementationBase.h"

// DirectStorage supports MultipleQueues which allow various priorities of read requests
// This sample shows how to create multiple queues at different priorities
class MultipleQueues : public ImplementationBase
{
private:
    static constexpr uint32_t c_numberOfQueues = 3;									// only support one per priority
    Microsoft::WRL::ComPtr <IDStorageQueueX> m_priorityQueues[c_numberOfQueues];	// one for each priority
    static const uint16_t c_readsPerQueue[c_numberOfQueues];					// one for each priority

    // There is no requirement to bind an IDStorageStatusX instance to only one queue, they are bound to the factory
    // it is only done here for simplicity.
    Microsoft::WRL::ComPtr <IDStorageStatusArrayX> m_statusEntries[c_numberOfQueues];

public:
    MultipleQueues() {}
    ~MultipleQueues() {}

    bool RunSample(const std::wstring& fileName, uint64_t dataFileSize);
};
