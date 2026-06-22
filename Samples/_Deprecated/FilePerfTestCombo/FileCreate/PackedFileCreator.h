//--------------------------------------------------------------------------------------
// PackedFileCreator.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "BaseFileInterface.h"
#include "File.h"
#include "FlatPackage.h"

#define MEMORY_MAPPED_PACKING

class PackedFileCreator
{
private:
    struct PackedMakerBlock
    {
        uint64_t nameHash;
        std::wstring fileName;
        uint64_t startLocation;
        uint64_t rawSize;
        uint64_t compressedSize;
        void* compressedBuffer;
        PackedMakerBlock(const std::wstring& p1 = L"", uint64_t p2 = 0, uint64_t p3 = 0, uint64_t p4 = 0) { fileName = p1; nameHash = p2; startLocation = p3; rawSize = p4; compressedSize = 0; compressedBuffer = nullptr; }
    };

    struct OpenFileEntry
    {
        uint64_t nameHash;
        uint64_t readPointer;
        BaseFileInterface::PackedFileEntry* entry;
        uint32_t refCount;
        OpenFileEntry(uint64_t p1 = 0, uint64_t p2 = 0, BaseFileInterface::PackedFileEntry* p3 = nullptr) { nameHash = p1; readPointer = p2; entry = p3; refCount = 1; }
        OpenFileEntry(const OpenFileEntry& rhs)
        {
            nameHash = rhs.nameHash;
            readPointer = rhs.readPointer;
            entry = rhs.entry;
            refCount = rhs.refCount;
        }
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

    std::map<uint64_t, BaseFileInterface::PackedFileEntry*> m_directory;
    std::vector<BaseFileInterface::PackedFileEntry*> m_orderedFiles;
    std::unique_ptr <BaseFileInterface::PackedFileEntry[]> m_files;
    std::map<BaseFileInterface::FileHandle, OpenFileEntry> m_openFiles;

    std::vector<DataRequest> m_requests;
    std::vector<PackedMakerBlock> m_packedMakerFiles;

    std::wstring m_packFileName;
    std::wstring m_dataRootPath;

    //FileSystem::File* m_packedFile;

    uintptr_t CreatePackedFileMemoryMappedJob(char* destAddress, const std::wstring& fileName, uint32_t fileSize);
    uintptr_t CreatePackedFileSlowJob(HANDLE destFile, uint64_t destOffset, const std::wstring& fileName, uint32_t fileSize);
    uintptr_t CreatePackedFileZipJob(PackedMakerBlock* params);
    uintptr_t CreatePackedFileZipMergeJob(HANDLE destFile, PackedMakerBlock* params);

    void CreatePackedFileMemoryMapped(uint64_t totalSize, uint64_t directorySize);
    void CreatePackedFileSlow(uint64_t totalSize, uint64_t directorySize);
    void CreatePackedFileZipped(uint64_t totalSize, uint64_t directorySize);
    void UpdatePackedHeader(uint64_t totalSize, uint64_t directorySize);

    uint64_t OpenFiles(const Packages::BaseSubPackage* curChunk, std::vector< PackedMakerBlock>& files);
    uint64_t OpenFiles(const Packages::FlatPackage* curPackage, std::vector< PackedMakerBlock>& files);

public:
    PackedFileCreator();
    virtual ~PackedFileCreator();

    void CreatePackedFile(const std::wstring& packFileName, const Packages::FlatPackage* fileList, const std::wstring& dirRootPath, bool zipped);
};
