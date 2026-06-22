//--------------------------------------------------------------------------------------
// PackedFileDStorage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "BaseFileInterfaceDStorage.h"
#include "File.h"
#include "BasePackage.h"

class PackedFile;
class PackedFileDStorage : public BaseFileInterfaceDStorage
{
private:
    uint64_t m_packedFileNameHash;

    Microsoft::WRL::ComPtr <IDStorageFileX> m_directPackedFile;

    PackedFile* m_packedFile;

public:
    PackedFileDStorage(uint16_t capacity, DSTORAGE_PRIORITY priority, uint32_t numQueues);
    virtual ~PackedFileDStorage();

    virtual void ResetQueues(uint16_t capacity, DSTORAGE_PRIORITY priority, uint32_t numQueues, bool inMemory);
    virtual void Submit(uint32_t queueIndex = 0);

    virtual void OpenPackFile(const std::wstring& fileName, uint32_t /*creationFlags*/, uintptr_t& platformHandle, BaseFileInterface* fileInterface);
    virtual void ClosePackFile();

    virtual FileHandle OpenFile(const std::wstring& fileName, uint32_t creationDisposition, uint32_t flags, uintptr_t& platformHandle);
    virtual void CloseFile(FileHandle file);

    virtual HRESULT EnqueueRequest(void* /*srcBuffer*/, void* /*destBuffer*/, uint32_t /*bytesToRead*/, uint32_t /*queueIndex*/ = 0) { return S_OK; }
    virtual HRESULT EnqueueRequest(uintptr_t* /*srcPhysicalPageArray*/, uint16_t /*srcPhysicalOffset*/, uintptr_t* /*destPhysicalPageArray*/, uint16_t /*destPhysicalOffset*/, uint32_t /*bytesToRead*/, uint32_t /*queueIndex*/ = 0) { return S_OK; }
    virtual HRESULT EnqueueRequest(FileHandle file, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t platformFileHandle, uint32_t queueIndex = 0);
    virtual HRESULT EnqueueRequest(FileHandle file, uintptr_t* physicalPageArray, uint16_t physicalOffset, uint32_t bytesToRead, uint64_t readLocation, uintptr_t platformFileHandle, uint32_t queueIndex = 0);
    virtual HRESULT EnqueueStatus(IDStorageStatusArrayX* status, uint32_t index, uint32_t queueIndex = 0);
    virtual Microsoft::WRL::ComPtr <IDStorageStatusArrayX> EnqueueStatus(uint32_t capacity, uint32_t index, uint32_t queueIndex = 0);
    virtual HRESULT EnqueueFence(uint32_t queueIndex = 0) { ((void)queueIndex);  return S_OK; }

    uint64_t LookupFileByName(const std::wstring& rootFile, uint64_t offset, uint64_t size);
    uint64_t LookupFileByHash(uint64_t rootFile, uint64_t offset, uint64_t size);
    uint64_t GetFileEnd(const std::wstring& fileName);
    uint64_t GetFileStart(const std::wstring& fileName);
    uint64_t GetFileEnd(uint64_t fileNameHash);
    uint64_t GetFileStart(uint64_t fileNameHash);
};
