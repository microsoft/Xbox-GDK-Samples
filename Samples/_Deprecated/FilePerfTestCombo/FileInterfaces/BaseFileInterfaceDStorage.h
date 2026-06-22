//--------------------------------------------------------------------------------------
// BaseFileInterfaceDStorage.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "BaseFileInterface.h"

#ifdef _GAMING_XBOX_SCARLETT
#include <dstorage_xs.h>
#else
#include "DirectStorageWin32\dstorage_win32.h"
#endif

class BaseFileInterfaceDStorage : public BaseFileInterface
{
protected:
    std::vector<Microsoft::WRL::ComPtr<IDStorageQueueX>> m_queue;
    Microsoft::WRL::ComPtr<IDStorageFactoryX> m_factory;

public:
    BaseFileInterfaceDStorage() = delete;
    BaseFileInterfaceDStorage(uint16_t /*capacity*/, DSTORAGE_PRIORITY /*priority*/, uint32_t numQueues = 1) { ((void)numQueues); }
    virtual ~BaseFileInterfaceDStorage() {  }

    virtual void ResetQueues(uint16_t capacity, DSTORAGE_PRIORITY priority, uint32_t numQueues, bool inMemory = false) = 0;
    virtual void Submit(uint32_t queueIndex = 0) = 0;

    virtual void GetDStorageReadParams(uint64_t fileHash, uint64_t offset, uint64_t& realFileNameHash, uint64_t& realOffset) { realFileNameHash = fileHash; realOffset = offset; }
    virtual size_t SetReadPointer(FileHandle /*file*/, uint64_t /*newLocation*/) { assert(false); return UINT64_MAX; }
    virtual HRESULT ReadFile(FileHandle /*file*/, void* /*data*/, uint32_t /*size*/) { assert(false); return E_NOTIMPL; }
    virtual HRESULT AsyncRead(FileHandle /*file*/, void* /*buffer*/, uint32_t /*bytesToRead*/, uint64_t /*readLocation*/, uintptr_t /*userData*/, FileSystem::File::AsyncRequestId& /*requestId*/, FileSystem::File::AsyncCallback /*callback*/, bool /*userEvent*/) { assert(false); return E_NOTIMPL; }
    virtual HRESULT EnqueueRequest(void* /*srcBuffer*/, void* /*destBuffer*/, uint32_t /*bytesToRead*/, uint32_t /*queueIndex*/ = 0) = 0;
    virtual HRESULT EnqueueRequest(uintptr_t* /*srcPhysicalPageArray*/, uint16_t /*srcPhysicalOffset*/, uintptr_t* /*destPhysicalPageArray*/, uint16_t /*destPhysicalOffset*/, uint32_t /*bytesToRead*/, uint32_t /*queueIndex*/ = 0) = 0;

    virtual HRESULT EnqueueRequest(FileHandle file, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t platformFileHandle, uint32_t queueIndex = 0) = 0;
    virtual HRESULT EnqueueRequest(FileHandle file, uintptr_t* physicalPageArray, uint16_t physicalOffset, uint32_t bytesToRead, uint64_t readLocation, uintptr_t platformFileHandle, uint32_t queueIndex = 0) = 0;
    virtual HRESULT EnqueueStatus(IDStorageStatusArrayX* status, uint32_t index, uint32_t queueIndex = 0) = 0;
    virtual Microsoft::WRL::ComPtr<IDStorageStatusArrayX> EnqueueStatus(uint32_t capacity, uint32_t index, uint32_t queueIndex = 0) = 0;
    virtual HRESULT EnqueueFence(uint32_t queueIndex = 0) = 0;
    virtual Microsoft::WRL::ComPtr <IDStorageStatusArrayX> CreateStatusArray(uint32_t capacity, PCSTR name = nullptr)
    {
        Microsoft::WRL::ComPtr <IDStorageStatusArrayX> status;
        DX::ThrowIfFailed(m_factory->CreateStatusArray(capacity, name, __uuidof(IDStorageStatusArrayX), (void**)(status.ReleaseAndGetAddressOf())));
        return status;
    }
};
