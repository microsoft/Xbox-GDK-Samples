//--------------------------------------------------------------------------------------
// PackedFile.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "BaseFileInterface.h"
#include "File.h"
#include "BasePackage.h"

#define MEMORY_MAPPED_PACKING

class PackedFile : public BaseFileInterface
{
private:
    struct OpenFileEntry
    {
        uint64_t nameHash;
        uint64_t readPointer;
        PackedFileEntry* entry;
        uint32_t refCount;
        OpenFileEntry(uint64_t p1 = 0, uint64_t p2 = 0, PackedFileEntry* p3 = nullptr) { nameHash = p1; readPointer = p2; entry = p3; refCount = 1; }
    };

    struct DataRequest
    {
        union
        {
            char* blockAddress;		// used for memory mapped
            size_t destFileOffset;		// used for slow mode
        };

        uint32_t fileSize;
        std::wstring fileName;

        DataRequest() : blockAddress(nullptr), fileSize(0), fileName(L"") {}
        DataRequest(char* p1, uint32_t p2, const std::wstring& p3) { blockAddress = p1; fileSize = p2; fileName = p3; }
    };

    std::map<uint64_t, PackedFileEntry*> m_directory;
    std::vector<PackedFileEntry*> m_orderedFiles;
    std::unique_ptr <PackedFileEntry[]> m_files;
    std::map<FileHandle, OpenFileEntry> m_openFiles;

    std::vector<DataRequest> m_requests;

    std::wstring m_packFileName;
    std::wstring m_dataRootPath;

    BaseFileInterface* m_packedFileInterface = nullptr;
    FileHandle m_packedFile = {};

    void ReadDirectory(const std::wstring& fileName);
    bool NeedUnaligned(const Packages::DirectoryEntry* curDir);
    bool NeedUnaligned(const Packages::BasePackage* curPackage);

public:
    PackedFile();
    virtual ~PackedFile();

    virtual void OpenPackFile(const std::wstring& fileName, uint32_t creationFlags, uintptr_t& platformHandle, BaseFileInterface* fileInterface = nullptr);
    virtual void ClosePackFile();

    FileHandle OpenFile(const std::wstring& fileName, uint32_t creationDisposition, uint32_t flags, uintptr_t& platformHandle);
    void CloseFile(FileHandle file);

    virtual bool ValidFile(uint64_t fileNameHash) const;
    virtual bool ValidReadLocation(uint64_t fileNameHash, uint64_t readLocation, uint64_t bytesToRead);
    virtual uint64_t LookupFileByLocation(const std::wstring& rootFile, uint64_t offset, uint64_t size);

    uint64_t GetFileEnd(const std::wstring& fileName);
    uint64_t GetFileStart(const std::wstring& fileName);
    uint64_t GetFileEnd(uint64_t fileNameHash);
    uint64_t GetFileStart(uint64_t fileNameHash);
    uint64_t GetFileMemorySize(uint64_t fileNameHash);
    uint64_t GetFileDiskSize(uint64_t fileNameHash);
    size_t SetReadPointer(FileHandle file, uint64_t newLocation);
    HRESULT ReadFile(FileHandle file, void* data, uint32_t size);
    HRESULT AsyncRead(FileHandle file, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t userData, FileSystem::File::AsyncRequestId& requestId, FileSystem::File::AsyncCallback callback, bool useEvent);
};
