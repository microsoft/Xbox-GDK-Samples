//--------------------------------------------------------------------------------------
// Cancellation.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "ImplementationBase.h"

// DirectStorage supports the cancellation of pending read requests
// This sample shows how cancel pending requests and how to handle failed to cancel requests
class Cancellation : public ImplementationBase
{
private:

    // flag for what type of data is being read in each request
    // It's the index of the bit in tag for the request
    enum ReadType
    {
        TEXTURE,
        NORMAL_MAP,
        MATERIAL,
        MESH,
        ANIMATION,
        AI,
        AUDIO,
        TERRAIN,
        FIGURE,
        BUILDING,
        SPECULATIVE_TEXTURE,
        SPECULATIVE_NORMAL_MAP,
        SPECULATIVE_MATERIAL,
        SPECULATIVE_MESH,
        SPECULATIVE_ANIMATION,
        SPECULATIVE_AI,
        SPECULATIVE_AUDIO,
        SPECULATIVE_TERRAIN,
        SPECULATIVE_FIGURE,
        SPECULATIVE_BUILDING,
        NUM_READ_TYPES = SPECULATIVE_BUILDING,
    };

    static constexpr uint32_t c_totalReads = 500;

    Microsoft::WRL::ComPtr <IDStorageStatusArrayX> m_status;

    static constexpr uint64_t c_numSpeculativeGroups = 32;		// Each group represents a set of related reads, each group is a unique bit in the 64-bit cancellation mask on the request

    static constexpr uint32_t c_numSpeculativeFlags = 3;					// A read can be part of multiple ReadTypes in this sample, multiple flags can be set

    static constexpr uint64_t c_speculativeGroupToCancel = 1ULL << SPECULATIVE_TERRAIN;	// which group to cancel, in this case all terrain loads

public:
    Cancellation() {}
    ~Cancellation() {}

    bool RunSample(const std::wstring& fileName, uint64_t dataFileSize);
};
