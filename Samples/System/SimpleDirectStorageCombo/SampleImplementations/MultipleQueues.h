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

// DirectStorage supports multiple queues which can each have their own priority for read requests
// This sample shows how to create multiple queues at different priorities
// Note: It's recommended that a title create their main queues during title startup
//    They are a circular list that represents a pipeline where new requests can be added as space becomes available
class MultipleQueues : public ImplementationBase
{
private:
    static const uint16_t c_readsPerQueue[c_defaultNumQueues];

    // There is no requirement to bind an IDStorageStatusX instance to only one queue, they are bound to the factory
    // it is only done here for simplicity.
    Microsoft::WRL::ComPtr <DStorageStatusArrayCrossPlatform> m_statusEntries[c_defaultNumQueues];

public:
    MultipleQueues() {}
    ~MultipleQueues() {}

    bool RunSample(const std::wstring& fileName, uint64_t dataFileSize);
};
