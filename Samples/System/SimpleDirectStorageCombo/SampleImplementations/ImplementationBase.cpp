//--------------------------------------------------------------------------------------
// ImplementationBase.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ImplementationBase.h"

Microsoft::WRL::ComPtr<DStorageFactoryCrossPlatform> ImplementationBase::s_factory;
std::map<std::wstring, Microsoft::WRL::ComPtr <DStorageFileCrossPlatform>> ImplementationBase::s_files;
Microsoft::WRL::ComPtr <DStorageQueueCrossPlatform> ImplementationBase::s_queues[c_defaultNumQueues];
#ifdef _GAMING_XBOX_SCARLETT
Microsoft::WRL::ComPtr <DStorageQueueCrossPlatform> ImplementationBase::s_inMemoryQueue;
#endif

ImplementationBase::ImplementationBase() :randomEngine(randomDevice())
{
    if (!s_factory)
        SetupDirectStorageObjects();
}

void ImplementationBase::SetupDirectStorageObjects()
{
    // Create the DirectStorage factory
    // Note: There is only one factory object even if there are multiple file devices
    //     DirectStorage will route correctly based on the path of the file used in the request
    if (!s_factory)
    {
        DX::ThrowIfFailed(DStorageGetFactoryCrossPlatform((void**)(s_factory.ReleaseAndGetAddressOf())));
        s_factory->SetDebugFlags(DSTORAGE_DEBUG_SHOW_ERRORS + DSTORAGE_DEBUG_BREAK_ON_ERROR + DSTORAGE_DEBUG_RECORD_OBJECT_NAMES);
    }

    // Note: It's recommended that a title create their main queues during title startup
    //    They are each a circular list that represents a pipeline where new requests can be added as space becomes available
    //    There is no need to constantly create and destroy them, this only leads to lower performance due to unnecessary work
    for (uint32_t curQueue = 0; curQueue < c_defaultNumQueues; ++curQueue)
    {
        // create at least one queue at each priority level, the rest of the queues are all set to normal
        DSTORAGE_QUEUE_DESC queueDesc = {};
        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        switch (curQueue)
        {
        case 0:
            queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
            queueDesc.Name = u8"SimpleDirectStorage: Initial Normal Priority Queue";
            break;
        case 1:
            queueDesc.Priority = DSTORAGE_PRIORITY_HIGH;
            queueDesc.Name = u8"SimpleDirectStorage: Initial High Priority Queue";
            break;
        case 2:
            queueDesc.Priority = DSTORAGE_PRIORITY_LOW;
            queueDesc.Name = u8"SimpleDirectStorage: Initial Low Priority Queue";
            break;
        case 3:
            queueDesc.Priority = DSTORAGE_PRIORITY_REALTIME;
            queueDesc.Name = u8"SimpleDirectStorage: Initial Real Time Priority Queue";
            break;
        default:
            queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
            queueDesc.Name = u8"SimpleDirectStorage: Extra Queues at Normal Priority";
        }

        // Ensure enough space in the queue for all requests and a slot for the status notification
        // Make sure to size this to hold at least all your potiential simultaneous in-flight requests
        queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
        DX::ThrowIfFailed(s_factory->CreateQueue(&queueDesc, __uuidof(DStorageQueueCrossPlatform), (void**)(s_queues[curQueue].ReleaseAndGetAddressOf())));
    }

    // An in-memory queue needs to be created as a real time queue with it's source type set to memory
#ifdef _GAMING_XBOX_SCARLETT
    {
        DSTORAGE_QUEUE_DESC queueDesc = {};
        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
        queueDesc.Priority = DSTORAGE_PRIORITY_REALTIME;	// Four possible priorities, high, normal, low, and realtime. Decompression of assets already in memory on the console requires realtime priority
        queueDesc.Capacity = DSTORAGE_MIN_QUEUE_CAPACITY;	// How many outstanding requests can the queue hold, make sure to size this to hold at least all your potiential simultaneous in-flight requests
        queueDesc.Name = u8"SimpleDirectStorage: In Memory Queue";
        DX::ThrowIfFailed(s_factory->CreateQueue(&queueDesc, __uuidof(DStorageQueueCrossPlatform), (void**)(s_inMemoryQueue.ReleaseAndGetAddressOf())));
}
#endif
}

void ImplementationBase::ShutdownDirectStorageObjects()
{
    for (auto& iter : s_files)
        iter.second.Reset();
    s_files.clear();
    for (auto& iter : s_queues)
        iter.Reset();
    s_factory.Reset();
}

void ImplementationBase::OpenFile(const std::wstring& fileName)
{
    if (s_files.find(fileName) == s_files.end())
    {
        DStorageFileCrossPlatform *newFile;
        DX::ThrowIfFailed(s_factory->OpenFile(fileName.c_str(), __uuidof(DStorageFileCrossPlatform), (void**)(&newFile)));
        s_files[fileName] = newFile;
    }
}

// Helper function that creates a set of random reads and matching destination buffers
void ImplementationBase::GenerateRandomReadLocationsAndDestinationBuffers(size_t dataFileSize, uint32_t numLocations, uint32_t readSizeOverride)
{
    m_destinationBuffers.clear();
    m_readSizes.clear();
    std::uniform_int_distribution<size_t> locationRandomValue(0, dataFileSize - c_maxDataReadSize);
    std::uniform_int_distribution<uint32_t> sizeRandomValue(c_minDataReadSize, c_maxDataReadSize);

    for (uint32_t i = 0; i < numLocations; ++i)
    {
        size_t location = locationRandomValue(randomEngine);
        uint32_t size;
        if (readSizeOverride > 0)
            size = readSizeOverride;
        else
            size = sizeRandomValue(randomEngine);
        // round to uint64_t because that's how our data file is setup, this way we can easily validate the results we read
        location &= ~0x3f;
        size &= ~0x3f;

        m_readLocations.push_back(location);
        m_readSizes.push_back(size);
        m_destinationBuffers.push_back(std::unique_ptr<char>(new char[size]));
    }
}
