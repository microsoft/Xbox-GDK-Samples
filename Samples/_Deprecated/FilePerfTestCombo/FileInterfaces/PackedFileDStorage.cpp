//--------------------------------------------------------------------------------------
// PackedFileDStorage.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PackedFileDStorage.h"
#include "PackedFile.h"
#include "LooseFilesDStorage.h"

using namespace Packages;

PackedFileDStorage::PackedFileDStorage(uint16_t capacity, DSTORAGE_PRIORITY priority, uint32_t numQueues) : BaseFileInterfaceDStorage(capacity, priority, numQueues)
{
    ResetQueues(capacity, priority, numQueues, false);

    m_packedFileNameHash = s_invalidFileHandle;
}

PackedFileDStorage::~PackedFileDStorage()
{
    if (m_packedFile)
        m_packedFile->ClosePackFile();
    m_queue.clear();
    m_factory.Reset();
}

void PackedFileDStorage::ResetQueues(uint16_t capacity, DSTORAGE_PRIORITY priority, uint32_t numQueues, bool /*inMemory*/)
{
    if (numQueues > 60)
        DebugBreak();

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
        sprintf_s(buffer, 64, "PackedFilesDStorageQueue_%d", curQueueIndex++);
        queueDesc.Name = buffer;
        IDStorageQueueX* newQueue;
        DX::ThrowIfFailed(m_factory->CreateQueue(&queueDesc, __uuidof(IDStorageQueueX), (void**)(&newQueue)));
        m_queue.push_back(newQueue);
        newQueue->Release();
    }
}

void PackedFileDStorage::Submit(uint32_t queueIndex)
{
    m_queue[queueIndex]->Submit();
}

uint64_t PackedFileDStorage::LookupFileByName(const std::wstring& rootFile, uint64_t offset, uint64_t size)
{
    return m_packedFile->LookupFileByName(rootFile, offset, size);
}
uint64_t PackedFileDStorage::LookupFileByHash(uint64_t rootFile, uint64_t offset, uint64_t size)
{
    return m_packedFile->LookupFileByHash(rootFile, offset, size);
}

void PackedFileDStorage::OpenPackFile(const std::wstring& fileName, uint32_t /*creationFlags*/, uintptr_t& fileHandle, BaseFileInterface* /*fileInterface*/)
{
    m_packedFile = new PackedFile();
    std::wstring fullName(fileName);
    fullName += L".packed";
    m_packedFileNameHash = BaseFileInterface::GenerateFilenameHash(fullName);
    DX::ThrowIfFailed(m_factory->OpenFile(fullName.c_str(), __uuidof(IDStorageFileX), (void**)(m_directPackedFile.ReleaseAndGetAddressOf())));
    LooseFilesDStorage* packedInterface = new LooseFilesDStorage(10, DSTORAGE_PRIORITY_NORMAL, 1);
    m_packedFile->OpenPackFile(fileName, 0, fileHandle, packedInterface);
}

void PackedFileDStorage::ClosePackFile()
{
    if (m_packedFile)
    {
        m_packedFile->ClosePackFile();
        delete m_packedFile;
        m_packedFile = nullptr;
    }

    m_packedFileNameHash = s_invalidFileHandle;
}

PackedFileDStorage::FileHandle PackedFileDStorage::OpenFile(const std::wstring& fileName, uint32_t creationDisposition, uint32_t flags, uintptr_t& platformHandle)
{
    return m_packedFile->OpenFile(fileName, creationDisposition, flags, platformHandle);
}

void PackedFileDStorage::CloseFile(FileHandle file)
{
    m_packedFile->CloseFile(file);
}

uint64_t PackedFileDStorage::GetFileEnd(const std::wstring& fileName)
{
    return m_packedFile->GetFileEnd(fileName);
}

uint64_t PackedFileDStorage::GetFileStart(const std::wstring& fileName)
{
    return m_packedFile->GetFileStart(fileName);
}

uint64_t PackedFileDStorage::GetFileEnd(uint64_t fileNameHash)
{
    return m_packedFile->GetFileEnd(fileNameHash);
}

uint64_t PackedFileDStorage::GetFileStart(uint64_t fileNameHash)
{
    return m_packedFile->GetFileStart(fileNameHash);
}

HRESULT PackedFileDStorage::EnqueueRequest(FileHandle file, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t /*platformFileHandle*/, uint32_t queueIndex)
{
    if (!m_packedFile->ValidFile(file))
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_INVALID_HANDLE);
    if (!m_packedFile->ValidReadLocation(file, readLocation, bytesToRead))
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_NOT_ENOUGH_MEMORY);
    uint64_t packedReadLocation = m_packedFile->GetFileStart(file) + readLocation;

    uint32_t memorySize = static_cast<uint32_t> (m_packedFile->GetFileMemorySize(file));
    uint32_t diskSize = static_cast<uint32_t> (m_packedFile->GetFileDiskSize(file));
    if (memorySize != diskSize)
        assert(memorySize == bytesToRead);

    DSTORAGE_REQUEST request = {};
    request.File = m_directPackedFile.Get();
    request.Destination = buffer;
    if (memorySize != diskSize)
    {
        if (readLocation != 0)
            return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_NOT_ENOUGH_MEMORY);
        if (bytesToRead != memorySize)
            return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_NOT_ENOUGH_MEMORY);
        assert((packedReadLocation & 0x0f) == 0);
        assert((diskSize - ((diskSize / 4096) * 4096)) > 15);
        assert(diskSize > 15);
        request.DestinationSize = memorySize;
        request.FileOffset = packedReadLocation;
        request.SourceSize = diskSize;
        request.Options.ZlibDecompress = true;
    }
    else
    {
        request.DestinationSize = bytesToRead;
        request.FileOffset = packedReadLocation;
        request.SourceSize = bytesToRead;
    }

    m_queue[queueIndex]->EnqueueRequest(&request);

    return S_OK;
}

HRESULT PackedFileDStorage::EnqueueRequest(FileHandle file, uintptr_t* physicalPageArray, uint16_t physicalOffset, uint32_t bytesToRead, uint64_t readLocation, uintptr_t /*platformFileHandle*/, uint32_t queueIndex)
{
    if (!m_packedFile->ValidFile(file))
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_INVALID_HANDLE);
    if (!m_packedFile->ValidReadLocation(file, readLocation, bytesToRead))
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_NOT_ENOUGH_MEMORY);
    uint64_t packedReadLocation = m_packedFile->GetFileStart(file) + readLocation;

    uint32_t memorySize = static_cast<uint32_t> (m_packedFile->GetFileMemorySize(file));
    uint32_t diskSize = static_cast<uint32_t> (m_packedFile->GetFileDiskSize(file));
    if (memorySize != diskSize)
        assert(memorySize == bytesToRead);

    DSTORAGE_REQUEST request = {};
    request.File = m_directPackedFile.Get();
    request.Options.DestinationIsPhysicalPages = 1;
    request.DestinationPageArray = physicalPageArray;
    request.DestinationPageOffset = physicalOffset;
    if (memorySize != diskSize)
    {
        if (readLocation != 0)
            return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_NOT_ENOUGH_MEMORY);
        if (bytesToRead != memorySize)
            return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_NOT_ENOUGH_MEMORY);
        assert((packedReadLocation & 0x0f) == 0);
        assert((diskSize - ((diskSize / 4096) * 4096)) > 15);
        assert(diskSize > 15);
        request.DestinationSize = memorySize;
        request.FileOffset = packedReadLocation;
        request.SourceSize = diskSize;
        request.Options.ZlibDecompress = true;
    }
    else
    {
        request.DestinationSize = bytesToRead;
        request.FileOffset = packedReadLocation;
        request.SourceSize = bytesToRead;
    }

    m_queue[queueIndex]->EnqueueRequest(&request);

    return S_OK;
}

HRESULT PackedFileDStorage::EnqueueStatus(IDStorageStatusArrayX* status, uint32_t index, uint32_t queueIndex)
{
    m_queue[queueIndex]->EnqueueStatus(status, index);
    return S_OK;
}

Microsoft::WRL::ComPtr <IDStorageStatusArrayX> PackedFileDStorage::EnqueueStatus(uint32_t capacity, uint32_t index, uint32_t queueIndex)
{
    Microsoft::WRL::ComPtr <IDStorageStatusArrayX> status = CreateStatusArray(capacity);
    m_queue[queueIndex]->EnqueueStatus(status.Get(), index);
    return status;
}
