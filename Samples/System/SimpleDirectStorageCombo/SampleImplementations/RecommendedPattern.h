//--------------------------------------------------------------------------------------
// RecommendedPattern.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "ImplementationBase.h"

// This shows the recommended pattern when using DirectStorage.
// It is functionally almost identical to the StatusBatch sample with one important difference
// The size of the batches and the number of batches being issues before submission
// In this case it's 20 batches with 50 read requests per batch for a total of 1,000 read requests
// It's recommended to submit all known requests, do not try and buffer the number of requests being submitted to DirectStorage
// This helps to avoid adding any bubbles in the pipeline waiting for the title to submit new requests
class RecommendedPattern : public ImplementationBase
{
private:
    static const uint32_t c_dataBatches = 20;
    static const uint32_t c_readsPerBatch = 50;

    Microsoft::WRL::ComPtr <DStorageStatusArrayCrossPlatform> m_statusEntries;

public:
    RecommendedPattern() {}
    ~RecommendedPattern() {}

    bool RunSample(const std::wstring& fileName, uint64_t dataFileSize);
};
