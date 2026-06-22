//--------------------------------------------------------------------------------------
// PackedFile.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PackedFile.h"
#include "JobSystem.h"
#include "LooseFiles.h"

using namespace Packages;

PackedFile::PackedFile()
{
}

PackedFile::~PackedFile()
{
    ClosePackFile();
}

void PackedFile::OpenPackFile(const std::wstring& fileName, uint32_t creationFlags, uintptr_t& platformHandle, BaseFileInterface* fileInterface)
{
    m_openFiles.clear();

    ReadDirectory(fileName);

    std::wstring fullName(fileName);
    fullName += L".packed";
    if (fileInterface == nullptr)
        m_packedFileInterface = new LooseFiles();
    else
        m_packedFileInterface = fileInterface;
    m_packedFile = m_packedFileInterface->OpenFile(fullName, OPEN_EXISTING, creationFlags, platformHandle);
}

void PackedFile::ClosePackFile()
{
    m_openFiles.clear();
    if (m_packedFile != BaseFileInterface::s_invalidFileHandle)
        m_packedFileInterface->CloseFile(m_packedFile);
    delete m_packedFileInterface;
    m_packedFileInterface = nullptr;
    m_packedFile = BaseFileInterface::s_invalidFileHandle;
}

PackedFile::FileHandle PackedFile::OpenFile(const std::wstring& fileName, uint32_t /*creationDisposition*/, uint32_t /*flags*/, uintptr_t& platformHandle)
{
    uint64_t nameHash = GenerateFilenameHash(fileName);
    {
        auto iter = m_openFiles.find(nameHash);
        if (iter != m_openFiles.end())
        {
            iter->second.refCount++;
            return nameHash;
        }
    }

    auto iter = m_directory.find(nameHash);
    if (iter == m_directory.end())
        return s_invalidFileHandle;

    m_openFiles[nameHash] = OpenFileEntry(nameHash, 0, iter->second);
    platformHandle = 0;
    return nameHash;
}

void PackedFile::CloseFile(FileHandle file)
{
    auto iter = m_openFiles.find(file);
    if (iter != m_openFiles.end())
    {
        iter->second.refCount--;
        if (iter->second.refCount == 0)
            m_openFiles.erase(iter);
    }
}

uint64_t PackedFile::LookupFileByLocation(const std::wstring& /*rootFile*/, uint64_t offset, uint64_t size)
{
    uint64_t rootStart = offset;
    uint64_t rootEnd = offset + size;
    for (auto& iter : m_directory)
    {
        uint64_t fileStart, fileEnd;
        fileStart = iter.second->startLocation;
        fileEnd = fileStart + iter.second->sizeOnDisk;
        fileStart = std::max(fileStart, rootStart);
        fileEnd = std::min(fileEnd, rootEnd);
        if (fileStart < fileEnd)
            return iter.second->nameHash;
    }
    return s_invalidFileHandle;
}

void PackedFile::ReadDirectory(const std::wstring& packedFileName)
{
    FileSystem::File* packedFile;
    std::wstring fullName(packedFileName);
    fullName += L".packed";
    HRESULT errorCode;
    packedFile = FileSystem::File::OpenFile(fullName, errorCode, OPEN_EXISTING, 0);

    uint32_t block[4096 / sizeof(uint32_t)];
    uint32_t numEntries;
    packedFile->Read(block, 4096);
    numEntries = block[0];

    uint32_t dirSize = sizeof(PackedFileEntry) * numEntries;
    dirSize += 4096 - 1;
    dirSize &= ~(4096 - 1);						// round up to 4k boundary, so data files start on 4k boundary
    char* tempBuffer = new char[dirSize];
    packedFile->Read(tempBuffer, dirSize);

    m_files = std::make_unique<PackedFileEntry[]>(numEntries);
    memcpy(m_files.get(), tempBuffer, sizeof(PackedFileEntry) * numEntries);
    for (uint32_t i = 0; i < numEntries; i++)
    {
        auto iter = m_directory.find(m_files[i].nameHash);
        if (iter != m_directory.end())
            DebugBreak();
        m_directory[m_files[i].nameHash] = &(m_files[i]);
    }
    delete[] tempBuffer;
    packedFile->CloseFile(packedFile);
}

uint64_t PackedFile::GetFileMemorySize(uint64_t fileNameHash)
{
    auto iter = m_directory.find(fileNameHash);
    if (iter == m_directory.end())
        return UINT64_MAX;

    return iter->second->sizeInMemory;
}

uint64_t PackedFile::GetFileDiskSize(uint64_t fileNameHash)
{
    auto iter = m_directory.find(fileNameHash);
    if (iter == m_directory.end())
        return UINT64_MAX;

    return iter->second->sizeOnDisk;
}

uint64_t PackedFile::GetFileEnd(uint64_t fileNameHash)
{
    auto iter = m_directory.find(fileNameHash);
    if (iter == m_directory.end())
        return UINT64_MAX;

    return iter->second->startLocation + iter->second->sizeOnDisk;
}

uint64_t PackedFile::GetFileStart(uint64_t fileNameHash)
{
    auto iter = m_directory.find(fileNameHash);
    if (iter == m_directory.end())
        return UINT64_MAX;

    return iter->second->startLocation;
}

uint64_t PackedFile::GetFileEnd(const std::wstring& fileName)
{
    uint64_t nameHash = GenerateFilenameHash(fileName);

    auto iter = m_directory.find(nameHash);
    if (iter == m_directory.end())
        return UINT64_MAX;

    return iter->second->startLocation + iter->second->sizeOnDisk;
}

uint64_t PackedFile::GetFileStart(const std::wstring& fileName)
{
    uint64_t nameHash = GenerateFilenameHash(fileName);

    auto iter = m_directory.find(nameHash);
    if (iter == m_directory.end())
        return UINT64_MAX;

    return iter->second->startLocation;
}

size_t PackedFile::SetReadPointer(FileHandle nameHash, uint64_t newLocation)
{
    auto iter = m_openFiles.find(nameHash);
    if (iter == m_openFiles.end())
        return SIZE_MAX;
    if ((iter->second.entry->sizeInMemory != iter->second.entry->sizeOnDisk) && (newLocation != 0))
        return SIZE_MAX;
    if (newLocation > iter->second.entry->sizeOnDisk)
        return SIZE_MAX;
    iter->second.readPointer = newLocation;
    return iter->second.readPointer;
}

HRESULT PackedFile::ReadFile(FileHandle nameHash, void* data, uint32_t size)
{
    auto iter = m_openFiles.find(nameHash);
    if (iter == m_openFiles.end())
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_INVALID_HANDLE);
    if ((iter->second.entry->sizeInMemory != iter->second.entry->sizeOnDisk) && (size != iter->second.entry->sizeInMemory))
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_NOT_ENOUGH_MEMORY);
    if (size > (iter->second.entry->sizeOnDisk + iter->second.readPointer))
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_NOT_ENOUGH_MEMORY);

    m_packedFileInterface->SetReadPointer(m_packedFile, iter->second.entry->startLocation + iter->second.readPointer);
    HRESULT err = m_packedFileInterface->ReadFile(m_packedFile, data, size);
    if (err != S_OK)
        return err;
    iter->second.readPointer += size;
    return S_OK;
}

bool PackedFile::ValidFile(uint64_t fileNameHash) const
{
    auto iter = m_openFiles.find(fileNameHash);
    if (iter == m_openFiles.end())
        return false;
    return true;
}

bool PackedFile::ValidReadLocation(uint64_t fileNameHash, uint64_t readLocation, uint64_t bytesToRead)
{
    auto iter = m_openFiles.find(fileNameHash);
    if (iter == m_openFiles.end())
        return false;
    if (bytesToRead > (iter->second.entry->sizeInMemory - readLocation))
        return false;
    return true;
}

HRESULT PackedFile::AsyncRead(FileHandle nameHash, void* buffer, uint32_t bytesToRead, uint64_t readLocation, uintptr_t userData, FileSystem::File::AsyncRequestId& requestId, FileSystem::File::AsyncCallback callback, bool useEvent)
{
    auto iter = m_openFiles.find(nameHash);
    if (iter == m_openFiles.end())
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_INVALID_HANDLE);
    if (bytesToRead > (iter->second.entry->sizeInMemory - readLocation))
        return MAKE_HRESULT(SEVERITY_ERROR, 0, ERROR_NOT_ENOUGH_MEMORY);
    HRESULT err = m_packedFileInterface->AsyncRead(m_packedFile, buffer, bytesToRead, iter->second.entry->startLocation + readLocation, userData, requestId, callback, useEvent);
    if (err != S_OK)
        return err;
    return S_OK;
}

bool PackedFile::NeedUnaligned(const Packages::DirectoryEntry* curDir)
{
    auto fileIter = curDir->beginFile();
    auto endFileIter = curDir->endFile();
    for (; fileIter != endFileIter; ++fileIter)
    {
        if ((*fileIter)->fileSize % 4096)
            return true;
    }

    auto dirIter = curDir->beginDirectory();
    auto endDirIter = curDir->endDirectory();
    for (; dirIter != endDirIter; ++dirIter)
    {
        if (NeedUnaligned(dirIter->get()))
            return true;
    }
    return false;
}

bool PackedFile::NeedUnaligned(const BasePackage* curPackage)
{
    auto chunkIter = curPackage->beginSubPackage();
    auto endChunkIter = curPackage->endSubPackage();
    for (; chunkIter != endChunkIter; ++chunkIter)
    {
        if (NeedUnaligned(&(*chunkIter)->GetRootDirectory()))
            return true;
    }

    return false;
}
