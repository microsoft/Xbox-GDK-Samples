//--------------------------------------------------------------------------------------
// LooseFilesDStorage.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LooseFilesDStorage.h"

LooseFilesDStorage::LooseFilesDStorage(uint16_t capacity, DSTORAGE_PRIORITY priority, uint32_t numQueues) : BaseFileInterfaceDStorage(capacity, priority, numQueues)
{
    ResetQueues(capacity, priority, numQueues, false);
}

LooseFilesDStorage::~LooseFilesDStorage()
{
    for (auto& iter : m_queue)
    {
        iter.Reset();
    }
    m_queue.clear();
    for (auto& iter : m_openFiles)
    {
        iter.second.dStorageFile->Close();
        iter.second.dStorageFile.Reset();
    }
    m_openFiles.clear();
    m_factory.Reset();
}

void LooseFilesDStorage::ResetQueues(uint16_t capacity, DSTORAGE_PRIORITY priority, uint32_t numQueues, bool inMemory)
{
    static uint32_t curQueueIndex = 0;
    DX::ThrowIfFailed(DStorageGetFactory(__uuidof(IDStorageFactoryX), (void**)(m_factory.ReleaseAndGetAddressOf())));
#ifdef _DEBUG
    m_factory->SetDebugFlags(DSTORAGE_DEBUG_SHOW_ERRORS + DSTORAGE_DEBUG_BREAK_ON_ERROR);
#endif
    for (auto& iter : m_queue)
    {
        iter.Reset();
    }
    m_queue.clear();
    for (uint32_t i = 0; i < numQueues; i++)
    {
        DSTORAGE_QUEUE_DESC queueDesc = {};
        queueDesc.Priority = priority;
        if (capacity < DSTORAGE_MIN_QUEUE_CAPACITY)
            capacity = DSTORAGE_MIN_QUEUE_CAPACITY;
        if (capacity > DSTORAGE_MAX_QUEUE_CAPACITY)
            capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
        queueDesc.Capacity = capacity;
        char buffer[64];
        sprintf_s(buffer, 64, "LooseFilesDStorageQueue_%d", curQueueIndex++);
        queueDesc.Name = buffer;
        if (inMemory)
        {
            queueDesc.Priority = DSTORAGE_PRIORITY_REALTIME;
            queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
        }
        IDStorageQueueX* newQueue;
        DX::ThrowIfFailed(m_factory->CreateQueue(&queueDesc, __uuidof(IDStorageQueueX), (void**)(&newQueue)));
        m_queue.push_back(newQueue);
        newQueue->Release();
    }
}

void LooseFilesDStorage::Submit(uint32_t queueIndex)
{
    m_queue[queueIndex]->Submit();
}

LooseFilesDStorage::FileHandle LooseFilesDStorage::OpenFile(const std::wstring& fileName, uint32_t /*creationDisposition*/, uint32_t /*flags*/, uintptr_t& platformHandle)
{
    std::wstring fullName;

    if (m_directoryOffset.size())
    {
        fullName = m_directoryOffset;
        fullName += L"\\";
        fullName += fileName;
    }
    else
    {
        fullName = fileName;
    }

    uint64_t nameHash = GenerateFilenameHash(fullName);
    {
        auto iter = m_openFiles.find(nameHash);
        if (iter != m_openFiles.end())
        {
            platformHandle = reinterpret_cast<uintptr_t> (iter->second.dStorageFile.Get());
            iter->second.refCount++;
            return nameHash;
        }
    }

    Microsoft::WRL::ComPtr <IDStorageFileX> file;
    DX::ThrowIfFailed(m_factory->OpenFile(fullName.c_str(), __uuidof(IDStorageFileX), (void**)(file.ReleaseAndGetAddressOf())));

    m_openFiles[nameHash] = OpenFileEntry(nameHash, file.Get());
    platformHandle = reinterpret_cast<uintptr_t> (file.Get());

    return nameHash;
}

void LooseFilesDStorage::CloseFile(FileHandle file)
{
    auto iter = m_openFiles.find(file);
    if (iter != m_openFiles.end())
    {
        iter->second.refCount--;
        if (iter->second.refCount == 0)
        {
            iter->second.dStorageFile->Close();
            m_openFiles.erase(iter);
        }
    }
}

HRESULT LooseFilesDStorage::EnqueueRequest(void* srcBuffer, void* destBuffer, uint32_t bytesToRead, uint32_t queueIndex)
{
    DSTORAGE_REQUEST request = {};
    request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
    request.Source = srcBuffer;
    request.SourceSize = bytesToRead;
    request.Destination = destBuffer;
    request.DestinationSize = bytesToRead;
    m_queue[queueIndex]->EnqueueRequest(&request);
    return S_OK;
}

HRESULT LooseFilesDStorage::EnqueueRequest(uintptr_t* srcPhysicalPageArray, uint16_t srcPhysicalOffset, uintptr_t* destPhysicalPageArray, uint16_t destPhysicalOffset, uint32_t bytesToRead, uint32_t queueIndex)
{
    DSTORAGE_REQUEST request = {};
    request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
    request.Options.DestinationIsPhysicalPages = 1;
    request.Options.SourceIsPhysicalPages = 1;
    request.DestinationPageArray = destPhysicalPageArray;
    request.DestinationPageOffset = destPhysicalOffset;
    request.SourcePageArray = srcPhysicalPageArray;
    request.SourcePageOffset = srcPhysicalOffset;

    request.DestinationSize = bytesToRead;
    request.SourceSize = bytesToRead;

    m_queue[queueIndex]->EnqueueRequest(&request);
    return S_OK;
}

HRESULT LooseFilesDStorage::EnqueueRequest(FileHandle nameHash, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t platformFileHandle, uint32_t queueIndex)
{
    if (platformFileHandle == 0)
    {
        auto iter = m_openFiles.find(nameHash);
        if (iter == m_openFiles.end())
            return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_INVALID_HANDLE);
        platformFileHandle = reinterpret_cast<uintptr_t> (iter->second.dStorageFile.Get());
    }

    DSTORAGE_REQUEST request = {};
    request.File = reinterpret_cast<IDStorageFileX*> (platformFileHandle);
    request.Destination = buffer;
    request.DestinationSize = bytesToRead;
    request.FileOffset = readLocation;
    request.SourceSize = bytesToRead;
    m_queue[queueIndex]->EnqueueRequest(&request);

    return S_OK;
}

HRESULT LooseFilesDStorage::EnqueueRequest(FileHandle nameHash, uintptr_t* physicalPageArray, uint16_t physicalOffset, uint32_t bytesToRead, uint64_t readLocation, uintptr_t platformFileHandle, uint32_t queueIndex)
{
    if (platformFileHandle == 0)
    {
        auto iter = m_openFiles.find(nameHash);
        if (iter == m_openFiles.end())
            return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_INVALID_HANDLE);
        platformFileHandle = reinterpret_cast<uintptr_t> (iter->second.dStorageFile.Get());
    }

    DSTORAGE_REQUEST request = {};
    request.File = reinterpret_cast<IDStorageFileX*> (platformFileHandle);
    request.Options.DestinationIsPhysicalPages = 1;
    request.DestinationPageArray = physicalPageArray;
    request.DestinationPageOffset = physicalOffset;
    request.DestinationSize = bytesToRead;
    request.FileOffset = readLocation;
    request.SourceSize = bytesToRead;
    m_queue[queueIndex]->EnqueueRequest(&request);

    return S_OK;
}

Microsoft::WRL::ComPtr <IDStorageStatusArrayX> LooseFilesDStorage::EnqueueStatus(uint32_t capacity, uint32_t index, uint32_t queueIndex)
{
    Microsoft::WRL::ComPtr <IDStorageStatusArrayX> status = CreateStatusArray(capacity);
    m_queue[queueIndex]->EnqueueStatus(status.Get(), index);
    return status;
}

HRESULT LooseFilesDStorage::EnqueueStatus(IDStorageStatusArrayX* status, uint32_t index, uint32_t queueIndex)
{
    m_queue[queueIndex]->EnqueueStatus(status, index);
    return S_OK;
}
