//--------------------------------------------------------------------------------------
// ImplementationBase.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ImplementationBase.h"

void ImplementationBase::SetupDirectStorage(const std::wstring& fileName, uint16_t queueCapacity)
{
    if (queueCapacity < DSTORAGE_MIN_QUEUE_CAPACITY)
        queueCapacity = DSTORAGE_MIN_QUEUE_CAPACITY;
    if (queueCapacity > DSTORAGE_MAX_QUEUE_CAPACITY)
        queueCapacity = DSTORAGE_MAX_QUEUE_CAPACITY;

    DX::ThrowIfFailed(DStorageGetFactory(__uuidof(IDStorageFactoryX), (void **)(m_factory.ReleaseAndGetAddressOf())));

    DX::ThrowIfFailed(m_factory->OpenFile(fileName.c_str(), __uuidof(IDStorageFileX), (void **)(m_file.ReleaseAndGetAddressOf())));

    DSTORAGE_QUEUE_DESC queueDesc = {};
    queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
    queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
    queueDesc.Capacity = queueCapacity;
    queueDesc.Name = u8"SimpleDirectStorage queue";
    DX::ThrowIfFailed(m_factory->CreateQueue(&queueDesc, __uuidof(IDStorageQueueX), (void **)(m_queue.ReleaseAndGetAddressOf())));
}

void ImplementationBase::ShutdownDirectStorage()
{
    m_destinationBuffers.clear();
    m_readSizes.clear();
    m_queue.Reset();
    m_file.Reset();
    m_factory.Reset();
}

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
