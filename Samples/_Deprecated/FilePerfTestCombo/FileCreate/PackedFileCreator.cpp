//--------------------------------------------------------------------------------------
// PackedFileCreator.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PackedFileCreator.h"
#include <codecvt>
#include "JobSystem.h"
#include "Processor.h"
#include "zlib/include/zlib.h"

using namespace Packages;

PackedFileCreator::PackedFileCreator()
{
}

PackedFileCreator::~PackedFileCreator()
{
}

uint64_t PackedFileCreator::OpenFiles(const BaseSubPackage* curChunk, std::vector< PackedMakerBlock>& files)
{
    uint64_t toret = 0;
    files.reserve(files.size() + curChunk->NumOrderFiles());

    auto fileIter = curChunk->beginFileOrder();
    auto endFileIter = curChunk->endFileOrder();
    for (; fileIter != endFileIter; ++fileIter)
    {
        files.emplace_back(PackedMakerBlock((*fileIter)->fullName, (*fileIter)->nameHash, 0, (*fileIter)->fileSize));
        toret += (*fileIter)->fileSize;
    }
    return toret;
}

uint64_t PackedFileCreator::OpenFiles(const FlatPackage* curPackage, std::vector< PackedMakerBlock>& files)
{
    uint64_t toret = 0;
    auto chunkIter = curPackage->beginSubPackage();
    auto endChunkIter = curPackage->endSubPackage();
    for (; chunkIter != endChunkIter; ++chunkIter)
    {
        toret += OpenFiles((*chunkIter), files);
    }
    return toret;
}

uintptr_t PackedFileCreator::CreatePackedFileMemoryMappedJob(char* destAddress, const std::wstring& fileName, uint32_t fileSize)
{
    CREATEFILE2_EXTENDED_PARAMETERS params;
    HANDLE file;
    memset(&params, 0, sizeof(params));

    std::wstring realFileName(m_dataRootPath);
    realFileName += L"\\";
    realFileName += fileName;
    params.dwSize = sizeof(params);
    params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    file = CreateFileW(realFileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        DebugBreak();
    DWORD actuallyRead = 0;
    ::ReadFile(file, destAddress, fileSize, &actuallyRead, nullptr);
    CloseHandle(file);
    return 0;
}

void PackedFileCreator::CreatePackedFileMemoryMapped(uint64_t totalSize, uint64_t /*directorySize*/)
{
    HANDLE fileMap;
    HANDLE file;
    char* baseAddress;

    file = CreateFile(m_packFileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    fileMap = CreateFileMapping(file, nullptr, PAGE_READWRITE, totalSize >> 32ULL, totalSize & 0xffffffff, nullptr);

    baseAddress = static_cast<char*>(MapViewOfFile(fileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0));

    char* baseDirectoryAddress = baseAddress;
    *(reinterpret_cast<uint32_t*> (baseDirectoryAddress)) = static_cast<uint32_t> (m_directory.size());
    baseDirectoryAddress += sizeof(uint32_t);
    *(reinterpret_cast<uint32_t*> (baseDirectoryAddress)) = 0;      // the need unaligned flag static_cast<uint32_t> (needUnaligned);
    baseDirectoryAddress += 4096 - (sizeof(uint32_t));
    BaseFileInterface::PackedFileEntry* baseFileEntryAddress = reinterpret_cast<BaseFileInterface::PackedFileEntry*> (baseDirectoryAddress);
    for (const auto& iter : m_directory)
    {
        *baseFileEntryAddress = *(iter.second);
        baseFileEntryAddress++;
    }

    uint32_t numJobThreads = std::min<uint32_t>(std::thread::hardware_concurrency() * 2, 64);
    ATG::JobSystem::CreateJobQueues(numJobThreads, numJobThreads);

    uint32_t queueIndex = 0;
    for (const auto& iter : m_packedMakerFiles)
    {
        char* curAddress = baseAddress + iter.startLocation;
        ATG::JobSystem::PushNewJob(std::bind(&PackedFileCreator::CreatePackedFileMemoryMappedJob, this, curAddress, iter.fileName, static_cast<uint32_t> (iter.rawSize)), queueIndex++);
        queueIndex %= numJobThreads;
    }

    while (ATG::JobSystem::g_waitingJobs)
    {
        uintptr_t temp;
        if (!ATG::JobSystem::InvokeSingleJob(temp))
            Sleep(1);
    }

    ATG::JobSystem::CleanupJobQueues();

    UnmapViewOfFile(baseAddress);
    CloseHandle(fileMap);
    CloseHandle(file);
}

uintptr_t PackedFileCreator::CreatePackedFileSlowJob(HANDLE destFile, uint64_t destOffset, const std::wstring& fileName, uint32_t fileSize)
{
    void* dataBlock = VirtualAlloc(nullptr, fileSize, MEM_COMMIT, PAGE_READWRITE);
    if (dataBlock == nullptr)
        DebugBreak();
    HANDLE file;

    std::wstring realFileName(m_dataRootPath);
    realFileName += L"\\";
    realFileName += fileName;
    file = CreateFileW(realFileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        DebugBreak();
    DWORD actuallyRead = 0;
    ::ReadFile(file, dataBlock, fileSize, &actuallyRead, nullptr);
    CloseHandle(file);

    DWORD dontCareButWindows;
    OVERLAPPED overlap = {};
    overlap.OffsetHigh = (destOffset >> 32ULL);
    overlap.Offset = (destOffset & MAXUINT32);
    if (!WriteFile(destFile, dataBlock, fileSize, &dontCareButWindows, &overlap))
        DebugBreak();
    if (dontCareButWindows != fileSize)
        DebugBreak();

    VirtualFree(dataBlock, 0, MEM_RELEASE);

    return 0;
}

void PackedFileCreator::CreatePackedFileSlow(uint64_t /*totalSize*/, uint64_t /*directorySize*/)
{
    HANDLE destFile;

    destFile = CreateFile(m_packFileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    m_requests.reserve(m_packedMakerFiles.size());

    uint32_t numJobThreads = std::min<uint32_t>(std::thread::hardware_concurrency() * 2, 64);
    ATG::JobSystem::CreateJobQueues(numJobThreads, numJobThreads);
    uint32_t queueIndex = 0;
    for (const auto& iter : m_packedMakerFiles)
    {
        ATG::JobSystem::PushNewJob(std::bind(&PackedFileCreator::CreatePackedFileSlowJob, this, destFile, iter.startLocation, iter.fileName, static_cast<uint32_t> (iter.rawSize)), queueIndex++);
        queueIndex %= numJobThreads;
    }

    while (ATG::JobSystem::g_waitingJobs)
    {
        uintptr_t temp;
        if (!ATG::JobSystem::InvokeSingleJob(temp))
            Sleep(1);
    }

    ATG::JobSystem::CleanupJobQueues();

    CloseHandle(destFile);
}

uintptr_t PackedFileCreator::CreatePackedFileZipJob(PackedMakerBlock* params)
{
    void* dataBlock = VirtualAlloc(nullptr, params->rawSize, MEM_COMMIT, PAGE_READWRITE);

    z_stream strm = {};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    int err = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
    if (err != Z_OK)
        return 0;

    uint64_t zipBufferSize = deflateBound(&strm, static_cast<uint32_t> (params->rawSize));

    void* zippedBlock = VirtualAlloc(nullptr, zipBufferSize, MEM_COMMIT, PAGE_READWRITE);
    if (dataBlock == nullptr)
        DebugBreak();
    if (zippedBlock == nullptr)
        DebugBreak();
    HANDLE file;

    std::wstring realFileName(m_dataRootPath);
    realFileName += L"\\";
    realFileName += params->fileName;
    file = CreateFileW(realFileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        DebugBreak();
    DWORD actuallyRead = 0;
    if (!::ReadFile(file, dataBlock, static_cast<uint32_t> (params->rawSize), &actuallyRead, nullptr))
        DebugBreak();
    CloseHandle(file);

    strm.avail_in = static_cast<uint32_t> (params->rawSize);
    strm.next_in = reinterpret_cast<Bytef*> (dataBlock);

    strm.avail_out = static_cast<uint32_t> (zipBufferSize);
    strm.next_out = reinterpret_cast<Bytef*> (zippedBlock);
    err = deflate(&strm, Z_FINISH);    /* no bad return value */
    if (err == Z_STREAM_ERROR)
        DebugBreak();
    assert(err != Z_STREAM_ERROR);  /* state not clobbered */
    if (strm.avail_in != 0)
        DebugBreak();
    assert(strm.avail_in == 0);     /* all input will be used */

    params->compressedSize = strm.total_out;
    params->compressedBuffer = zippedBlock;
    std::ignore = deflateEnd(&strm);

    VirtualFree(dataBlock, 0, MEM_RELEASE);

    return 0;
}

uintptr_t PackedFileCreator::CreatePackedFileZipMergeJob(HANDLE destFile, PackedMakerBlock* params)
{
    DWORD dontCareButWindows;
    OVERLAPPED overlap = {};
    if (params->startLocation & 0x0f)
        DebugBreak();
    overlap.OffsetHigh = (params->startLocation >> 32ULL);
    overlap.Offset = (params->startLocation & MAXUINT32);
    if (!WriteFile(destFile, params->compressedBuffer, static_cast<uint32_t> (params->compressedSize), &dontCareButWindows, &overlap))
        DebugBreak();
    if (dontCareButWindows != params->compressedSize)
        DebugBreak();

    VirtualFree(params->compressedBuffer, 0, MEM_RELEASE);
    params->compressedBuffer = nullptr;

    return 0;
}

void PackedFileCreator::CreatePackedFileZipped(uint64_t /*totalSize*/, uint64_t directorySize)
{
    HANDLE destFile;

    destFile = CreateFile(m_packFileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    m_requests.reserve(m_packedMakerFiles.size());

    uint64_t numJobThreads = std::min<uint32_t>(std::thread::hardware_concurrency() - 2, 64);
    if (ATG::GetNumberNumaNodes() > 1)
        numJobThreads = ATG::GetTotalNumberCoresIncludingNuma() - 2;

    ATG::JobSystem::CreateJobQueues(numJobThreads, numJobThreads);
    uint64_t queueIndex = 0;
    for (auto& iter : m_packedMakerFiles)
    {
        ATG::JobSystem::PushNewJob(std::bind(&PackedFileCreator::CreatePackedFileZipJob, this, &iter), queueIndex++);
        queueIndex %= numJobThreads;
    }

    while (ATG::JobSystem::g_waitingJobs)
    {
        uintptr_t temp;
        if (!ATG::JobSystem::InvokeSingleJob(temp))
            Sleep(1);
    }

    uint64_t curOffset = directorySize;
    for (auto& iter : m_packedMakerFiles)
    {
        curOffset = (curOffset + 15) & ~0x0f;	// round up to 16 byte alignment
        iter.startLocation = curOffset;	// round up to 16 byte alignment
        curOffset += iter.compressedSize;
        ATG::JobSystem::PushNewJob(std::bind(&PackedFileCreator::CreatePackedFileZipMergeJob, this, destFile, &iter), queueIndex++);
        queueIndex %= numJobThreads;
    }

    while (ATG::JobSystem::g_waitingJobs)
    {
        uintptr_t temp;
        if (!ATG::JobSystem::InvokeSingleJob(temp))
            Sleep(1);
    }

    ATG::JobSystem::CleanupJobQueues();

    CloseHandle(destFile);

    for (auto& iter : m_packedMakerFiles)
    {
        m_directory[iter.nameHash]->sizeOnDisk = iter.compressedSize;
        if (iter.startLocation & 0x0f)
            DebugBreak();
        m_directory[iter.nameHash]->startLocation = iter.startLocation;
    }
}

void PackedFileCreator::UpdatePackedHeader(uint64_t /*totalSize*/, uint64_t directorySize)
{
    HANDLE destFile;

    destFile = CreateFile(m_packFileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    //Create a small file map here just because it's easier to write out the directory structure this way and preallocate the file
    {
        HANDLE fileMap;
        char* baseAddress;
        fileMap = CreateFileMapping(destFile, nullptr, PAGE_READWRITE, directorySize >> 32ULL, directorySize & 0xffffffff, nullptr);

        baseAddress = static_cast<char*>(MapViewOfFile(fileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0));

        char* baseDirectoryAddress = baseAddress;
        *(reinterpret_cast<uint32_t*> (baseDirectoryAddress)) = static_cast<uint32_t> (m_directory.size());
        baseDirectoryAddress += sizeof(uint32_t);
        *(reinterpret_cast<uint32_t*> (baseDirectoryAddress)) = 0;  // The need unaligned flag static_cast<uint32_t> (needUnaligned);
        baseDirectoryAddress += 4096 - (sizeof(uint32_t));
        BaseFileInterface::PackedFileEntry* baseFileEntryAddress = reinterpret_cast<BaseFileInterface::PackedFileEntry*> (baseDirectoryAddress);
        for (const auto& iter : m_directory)
        {
            *baseFileEntryAddress = *(iter.second);
            baseFileEntryAddress++;
        }
        UnmapViewOfFile(baseAddress);
        CloseHandle(fileMap);
    }
    CloseHandle(destFile);
}

void PackedFileCreator::CreatePackedFile(const std::wstring& packFileName, const FlatPackage* fileList, const std::wstring& dirRootPath, bool zipped)
{
    m_dataRootPath = dirRootPath;
    m_packFileName = packFileName;

    uint64_t totalSize = 0;

    std::wstring workingDirectory;

    {
        m_packedMakerFiles.clear();

        char currentWorkingDirectory[2048];
        GetCurrentDirectoryA(2048, currentWorkingDirectory);
        SetCurrentDirectoryW(dirRootPath.c_str());
        totalSize = OpenFiles(fileList, m_packedMakerFiles);
        assert(totalSize != 0);
        SetCurrentDirectoryA(currentWorkingDirectory);
    }

    m_files = std::make_unique<BaseFileInterface::PackedFileEntry[]>(m_packedMakerFiles.size());

    uint64_t directorySize = m_packedMakerFiles.size() * sizeof(BaseFileInterface::PackedFileEntry);
    directorySize += 4096;								// first 4k block only has header tags, real directory starts on second 4k block
    directorySize += 4096 - 1;                          // number of entries space, need to align to 4k boundary
    directorySize &= ~(4096 - 1);						// round up to 4k boundary, so data files start on 4k boundary
    totalSize += directorySize;

    uint64_t currentStartLocation = directorySize;
    uint32_t fileIndex = 0;
    for (auto& iter : m_packedMakerFiles)
    {
        iter.startLocation = currentStartLocation;
        m_files[fileIndex] = BaseFileInterface::PackedFileEntry(iter.nameHash, zipped ? UINT64_MAX : currentStartLocation, iter.rawSize);
        m_directory[iter.nameHash] = &(m_files[fileIndex]);
        currentStartLocation += iter.rawSize;
        fileIndex++;
    }

    // zero out the file by creating a blank new one
    {
        HANDLE destFile;
        destFile = CreateFile(m_packFileName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        CloseHandle(destFile);
    }

    UpdatePackedHeader(totalSize, directorySize);
    bool useMemoryMappedFile = false;
#ifdef MEMORY_MAPPED_PACKING
    {
        MEMORYSTATUSEX status;
        memset(&status, 0, sizeof(status));
        status.dwLength = sizeof(status);
        GlobalMemoryStatusEx(&status);
        if (totalSize < status.ullAvailPhys)
            useMemoryMappedFile = true;
    }
#endif
    if (zipped)
        CreatePackedFileZipped(totalSize, directorySize);
    else
        if (useMemoryMappedFile)
            CreatePackedFileMemoryMapped(totalSize, directorySize);
        else
            CreatePackedFileSlow(totalSize, directorySize);

    UpdatePackedHeader(totalSize, directorySize);
}
