//--------------------------------------------------------------------------------------
// ImplementationBase.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include <random>

#include "DirectStorageWin32/DirectStorageCrossPlatform.h"

// Provides repeated functionality for each sample to avoid cluttering the main sample
class ImplementationBase
{
protected:

    static const uint32_t c_queueCapacity = DSTORAGE_MIN_QUEUE_CAPACITY;    // The default number of outstanding requests used by these samples,
                                                                            // make sure to size your queues to hold at least all your potiential simultaneous in-flight requests
    static const uint32_t c_minDataReadSize = 32 * 1024;			        // On the console the recommended read size is 32k or larger
    static const uint32_t c_maxDataReadSize = 2 * 1024 * 1024;
    static const uint32_t c_defaultNumQueues = 5;

    static Microsoft::WRL::ComPtr<DStorageFactoryCrossPlatform> s_factory;
    static std::map<std::wstring, Microsoft::WRL::ComPtr <DStorageFileCrossPlatform>> s_files;
    static Microsoft::WRL::ComPtr <DStorageQueueCrossPlatform> s_queues[c_defaultNumQueues];
#ifdef _GAMING_XBOX_SCARLETT
    static Microsoft::WRL::ComPtr <DStorageQueueCrossPlatform> s_inMemoryQueue;
#endif

    std::vector<std::unique_ptr<char>> m_destinationBuffers;
    std::vector<size_t> m_readLocations;
    std::vector<uint32_t> m_readSizes;

    std::random_device randomDevice;
    std::default_random_engine randomEngine;

    void OpenFile(const std::wstring& fileName);

    void GenerateRandomReadLocationsAndDestinationBuffers(size_t dataFileSize, uint32_t numLocations, uint32_t readSizeOverride = 0);

public:
    ImplementationBase();
    virtual ~ImplementationBase() {}

    static void SetupDirectStorageObjects();
    static void ShutdownDirectStorageObjects();
};
