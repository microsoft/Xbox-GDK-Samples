//--------------------------------------------------------------------------------------
// LooseFilesDStorage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "BaseFileInterfaceDStorage.h"

class LooseFilesDStorage : public BaseFileInterfaceDStorage
{
private:
    struct OpenFileEntry
    {
        uint64_t nameHash;
        uint32_t refCount;
        Microsoft::WRL::ComPtr <IDStorageFileX> dStorageFile;
        OpenFileEntry(uint64_t initialNameHash = s_invalidFileHandle, IDStorageFileX* initialDirectStorageFile = nullptr) { nameHash = initialNameHash; refCount = 1; dStorageFile = initialDirectStorageFile; }
    };

    std::map<FileHandle, OpenFileEntry> m_openFiles;
    std::wstring m_directoryOffset;

public:
    LooseFilesDStorage(uint16_t capacity, DSTORAGE_PRIORITY priority, uint32_t numQueues);
    virtual ~LooseFilesDStorage();

    virtual void Submit(uint32_t queueIndex = 0);
    virtual void ResetQueues(uint16_t capacity, DSTORAGE_PRIORITY priority, uint32_t numQueues, bool inMemory);

    virtual FileHandle OpenFile(const std::wstring& fileName, uint32_t creationDisposition, uint32_t flags, uintptr_t& platformHandle);
    virtual void CloseFile(FileHandle file);

    virtual void SetDirectoryOffset(const std::wstring& dirName) { m_directoryOffset = dirName; }

    virtual HRESULT EnqueueRequest(void* /*srcBuffer*/, void* /*destBuffer*/, uint32_t /*bytesToRead*/, uint32_t /*queueIndex*/ = 0);
    virtual HRESULT EnqueueRequest(uintptr_t* srcPhysicalPageArray, uint16_t srcPhysicalOffset, uintptr_t* destPhysicalPageArray, uint16_t destPhysicalOffset, uint32_t bytesToRead, uint32_t queueIndex = 0);
    virtual HRESULT EnqueueRequest(FileHandle file, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t platformFileHandle, uint32_t queueIndex = 0);
    virtual HRESULT EnqueueRequest(FileHandle file, uintptr_t* physicalPageArray, uint16_t physicalOffset, uint32_t bytesToRead, uint64_t readLocation, uintptr_t platformFileHandle, uint32_t queueIndex = 0);
    virtual HRESULT EnqueueStatus(IDStorageStatusArrayX* status, uint32_t index, uint32_t queueIndex = 0);
    virtual Microsoft::WRL::ComPtr <IDStorageStatusArrayX> EnqueueStatus(uint32_t capacity, uint32_t index, uint32_t queueIndex = 0);
    virtual HRESULT EnqueueFence(uint32_t queueIndex = 0) { ((void)queueIndex);  return S_OK; }
};
