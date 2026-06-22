//--------------------------------------------------------------------------------------
// LooseFiles.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "BaseFileInterface.h"

class LooseFiles : public BaseFileInterface
{
private:
    struct OpenFileEntry
    {
        uint64_t nameHash;
        FileSystem::File *file;
        uint32_t refCount;
        OpenFileEntry(uint64_t p1 = 0, FileSystem::File *p2 = nullptr) { nameHash = p1; file = p2; refCount = 1; }
    };

    std::map<FileHandle, OpenFileEntry> m_openFiles;
    std::wstring m_directoryOffset;

public:
    LooseFiles();
    virtual ~LooseFiles();

    virtual FileHandle OpenFile(const std::wstring& fileName, uint32_t creationDisposition, uint32_t flags, uintptr_t& platformHandle);
    virtual void CloseFile(FileHandle file);

    virtual void SetDirectoryOffset(const std::wstring& dirName) { m_directoryOffset = dirName; }

    virtual size_t SetReadPointer(FileHandle file, uint64_t newLocation);
    virtual HRESULT ReadFile(FileHandle file, void *data, uint32_t size);
    virtual HRESULT AsyncRead(FileHandle file, void *buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t userData, FileSystem::File::AsyncRequestId& requestId, FileSystem::File::AsyncCallback callback, bool useEvent);
};
