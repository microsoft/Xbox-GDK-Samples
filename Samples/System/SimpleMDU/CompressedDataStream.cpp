// CompressedDataStream.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CompressedDataStream.h"


#include <zlib.h>

std::vector<DataStream*> DataStream::m_dataStreams;
std::mutex DataStream::m_genMutex;
std::vector<CompressedDataStream*> CompressedDataStream::m_dataStreams;
std::mutex CompressedDataStream::m_genMutex;

DataStream* DataStream::Generate(uint64_t size, float percentRandom) noexcept
{
    std::lock_guard<std::mutex> guard(m_genMutex);

    DataStream* stream = nullptr;
    for (uint32_t i = 0; i < m_dataStreams.size(); i++)
    {
        if (m_dataStreams[i]->Size == size && m_dataStreams[i]->PercentRandom == percentRandom)
        {
            stream = m_dataStreams[i];
            break;
        }
    }
    if (!stream)
    {
        stream = new DataStream(size, percentRandom);
        m_dataStreams.push_back(stream);
    }
    return stream;
}

DataStream* DataStream::Fetch(uint64_t size, float percentRandom) noexcept
{
    DataStream* stream = nullptr;
    for (uint32_t i = 0; i < m_dataStreams.size(); i++)
    {
        if (m_dataStreams[i]->Size == size && m_dataStreams[i]->PercentRandom == percentRandom)
        {
            stream = m_dataStreams[i];
            break;
        }
    }
    return stream;
}

DataStream::DataStream(uint64_t size, float percentRandom) :
    Data(nullptr),
    Size(size),
    PercentRandom(percentRandom),
    m_refCount(0)
{
    // A static seed is used since we may want to discard whole data streams based on requested compressibility
    // settings.  So we want to be able to re-create the exact data stream over the entire execution duration.
    static uint32_t seed = GetTickCount();

    const size_t randGranularity = 1024;
    size_t allocationSize = ((size + (c_allocGranularity - 1)) & ~(c_allocGranularity - 1));

    std::default_random_engine generator(seed);
    std::uniform_int_distribution<uint64_t> distribution(0, UINT64_MAX);

    // For this sample, we don't actually use the GPU mapped memory, but since that is the primary scenario,
    // decompressing into GPU accessible memory, that is what is illustrated here.

    Data = XMemVirtualAlloc(nullptr, allocationSize, MEM_RESERVE | MEM_COMMIT | MEM_2MB_PAGES, XMEM_GRAPHICS, PAGE_READWRITE | PAGE_GRAPHICS_READWRITE | PAGE_WRITECOMBINE);
    assert(Data);

    // This fills the buffer with a selectable amount of random data, so as to be able to dial the compression ratio easily for each block.
    // Ideally, block sizes of 64Kib or larger are used, but for purposes of illustrating compression throughput of the MDU.  

    size_t subBlocks = allocationSize / 1024;
    uint32_t randThreshold = uint32_t(randGranularity * PercentRandom);

    if (Data)
    {
        for (size_t b = 0; b < subBlocks; b++)
        {
            uint8_t* blockMem = reinterpret_cast<uint8_t*>(Data) + (b * randGranularity);

            for (uint32_t offset = 0; offset < randGranularity; offset += sizeof(uint64_t))
            {
                *reinterpret_cast<uint64_t*>(blockMem + offset) = (offset < randThreshold) ? distribution(generator) : 0;
            }
        }
    }
}

DataStream::~DataStream()
{
    assert(0 == m_refCount);
    if (Data)
    {
        VirtualFree(Data, 0, MEM_RELEASE);
    }
}


CompressedDataStream* CompressedDataStream::GenerateInMemory(DataStream * data, uint64_t size, uint32_t chunkSize, uint16_t alignment)
{
    std::lock_guard<std::mutex> guard(m_genMutex);

    CompressedDataStream* stream = nullptr;
    for (uint32_t i = 0; i < m_dataStreams.size(); i++)
    {
        if (m_dataStreams[i]->m_originalData == data &&
            m_dataStreams[i]->DataSize == size &&
            m_dataStreams[i]->ChunkSize == chunkSize &&
            m_dataStreams[i]->Alignment == alignment)
        {
            stream = m_dataStreams[i];
            break;
        }
    }
    if (!stream)
    {
        stream = new CompressedDataStream(data, size, chunkSize, alignment);
        m_dataStreams.push_back(stream);
    }
    return stream;
}

CompressedDataStream* CompressedDataStream::FetchMemoryBacked(DataStream * data, uint64_t size, uint32_t chunkSize, uint16_t alignment)
{
    std::lock_guard<std::mutex> guard(m_genMutex);
    CompressedDataStream* stream = nullptr;
    for (uint32_t i = 0; i < m_dataStreams.size(); i++)
    {
        if (m_dataStreams[i]->m_originalData == data &&
            m_dataStreams[i]->DataSize == size &&
            m_dataStreams[i]->ChunkSize == chunkSize &&
            m_dataStreams[i]->Alignment == alignment)
        {
            stream = m_dataStreams[i];
            break;
        }
    }
    return stream;
}

CompressedDataStream* CompressedDataStream::FetchMemoryBacked(float percentRandom, uint64_t size, uint32_t chunkSize, uint16_t alignment)
{
    std::lock_guard<std::mutex> guard(m_genMutex);
    CompressedDataStream* stream = nullptr;
    for (uint32_t i = 0; i < m_dataStreams.size(); i++)
    {
        if (m_dataStreams[i]->m_isInMemory &&
            m_dataStreams[i]->m_originalData->PercentRandom == percentRandom &&
            m_dataStreams[i]->DataSize == size &&
            m_dataStreams[i]->ChunkSize == chunkSize &&
            m_dataStreams[i]->Alignment == alignment)
        {
            stream = m_dataStreams[i];
            break;
        }
    }
    return stream;
}

CompressedDataStream* CompressedDataStream::FetchStorageBacked(float percentRandom, uint64_t size, uint32_t chunkSize, uint16_t alignment)
{
    std::lock_guard<std::mutex> guard(m_genMutex);
    CompressedDataStream* stream = nullptr;
    for (uint32_t i = 0; i < m_dataStreams.size(); i++)
    {
        if (m_dataStreams[i]->m_isInStorage &&
            m_dataStreams[i]->DataSize == size &&
            m_dataStreams[i]->DataRandPercent == percentRandom &&
            m_dataStreams[i]->ChunkSize == chunkSize &&
            m_dataStreams[i]->Alignment == alignment)
        {
            stream = m_dataStreams[i];
            break;
        }
    }
    return stream;
}

void CompressedDataStream::ReleaseLRU() noexcept
{
    std::lock_guard<std::mutex> guard(m_genMutex);

    uint32_t oldest = 0;
    for (uint32_t i = 1; i < m_dataStreams.size(); i++)
    {
        if (m_dataStreams[i]->m_lastUsed < m_dataStreams[oldest]->m_lastUsed)
        {
            oldest = i;
        }
    }
    if (m_dataStreams.size() > 2)
    {
        delete m_dataStreams[oldest];
        m_dataStreams.erase(m_dataStreams.begin() + oldest);
    }
}

// Resolves the location for persisting data streams for the from-storage data streaming.
// Note that actual drive throughput may vary by backing store.  R/W locations such as below will be slightly slower
// than read-only locations like a paackaged game.  Development drive may also not match retail drives exactly,
// but generating data on the fly in this manner allows quick comparisons in the sample, rather than generating large
// amounts of data on the PC and then pushing it across to the console.
void CompressedDataStream::ResolvePath(wchar_t *path, size_t pathLength, float percentRandom, uint64_t size, uint32_t chunkSize, uint16_t alignment)
{
    swprintf(path, pathLength, L"D:\\SimpleMDU_%I64dMiB_%1.1f_0x%x_0x%x.data", size / (1024*1024), percentRandom, chunkSize, alignment);
}

void CompressedDataStream::PersistToStorage()
{
    std::lock_guard<std::mutex> guard(m_genMutex);

    wchar_t path[MAX_PATH] = {};
    std::vector<uint8_t> headerBuffer;

    ResolvePath(path, _countof(path), DataRandPercent, DataSize, ChunkSize, Alignment);

    // compute all the expected info:
    size_t headerDataSize =
        sizeof(uint32_t) +          // 'S' 'M' 'D' 'U'
        sizeof(uint64_t) +          // compressedStreamSize  (not counting header)
        sizeof(uint64_t) +          // originalStreamSize
        CompressedTiles.size() * sizeof(ChunkInfo);

    size_t headerAlignedSize = (headerDataSize + (Alignment - 1)) & ~(Alignment - 1);

    HANDLE file = CreateFile2(path, GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr);

    if (INVALID_HANDLE_VALUE == file)
        return;

    headerBuffer.resize(headerAlignedSize);
    headerBuffer[0] = 'S';
    headerBuffer[1] = 'M';
    headerBuffer[2] = 'D';
    headerBuffer[3] = 'U';


    uint64_t* compressedSize = reinterpret_cast<uint64_t*>(headerBuffer.data() + sizeof(uint32_t));
    uint64_t* originalSize = reinterpret_cast<uint64_t*>(headerBuffer.data() + sizeof(uint32_t) + sizeof(uint64_t));
    ChunkInfo* chunks = reinterpret_cast<ChunkInfo*>(headerBuffer.data() + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t));

    *compressedSize = CompressedSize;
    *originalSize = DataSize;
    memcpy(chunks, CompressedTiles.data(), CompressedTiles.size() * sizeof(ChunkInfo));

    DWORD bytesOut;
    WriteFile(file, headerBuffer.data(), DWORD(headerAlignedSize), &bytesOut, nullptr);
    assert(bytesOut == DWORD(headerAlignedSize));

    assert(CompressedSize <= DWORD_MAX);      // if changing to larger files, write a portion at a time
    WriteFile(file, CompressedData, DWORD(CompressedSize), &bytesOut, nullptr);
    assert(bytesOut == DWORD(CompressedSize));

    CloseHandle(file);

    CompressedDataOffset = headerAlignedSize;
    m_isInStorage = true;
}

// Loads compressed chunk header from the file, but not the rest of the file.
// In a game scenario, the headers for multiple compressed streams might be combined, and loaded as part of initial asset load
// and then individual data sections might be loaded on priority order on demand.
CompressedDataStream* CompressedDataStream::HydrateFromStorage(float percentRandom, uint64_t size, uint32_t chunkSize, uint16_t alignment)
{
    std::lock_guard<std::mutex> guard(m_genMutex);

    CompressedDataStream* stream = nullptr;
    for (uint32_t i = 0; i < m_dataStreams.size(); i++)
    {
        if (m_dataStreams[i]->m_isInStorage &&
            m_dataStreams[i]->DataSize == size &&
            m_dataStreams[i]->DataRandPercent == percentRandom &&
            m_dataStreams[i]->ChunkSize == chunkSize &&
            m_dataStreams[i]->Alignment == alignment)
        {
            stream = m_dataStreams[i];
            break;
        }
    }
    if (!stream)
    {
        wchar_t path[MAX_PATH] = {};
        std::vector<uint8_t> headerBuffer;

        ResolvePath(path, _countof(path), percentRandom, size, chunkSize, alignment);

        // compute all the expected info:
        uint32_t chunkCount = uint32_t(size / chunkSize);

        uint32_t headerDataSize =
            sizeof(uint32_t) +          // 'S' 'M' 'D' 'U'
            sizeof(uint64_t) +          // compressedStreamSize  (not counting header)
            sizeof(uint64_t) +          // originalStreamSize
            chunkCount * sizeof(ChunkInfo);

        uint32_t headerAlignedSize = (headerDataSize + (alignment - 1)) & ~(alignment - 1);

        HANDLE file = CreateFile2(path, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr);

        if (INVALID_HANDLE_VALUE == file)
            return nullptr;

        headerBuffer.resize(headerDataSize);
        DWORD bytesRead = 0;
        if (ReadFile(file, headerBuffer.data(), headerDataSize, &bytesRead, nullptr))
        {
            LARGE_INTEGER fileSize;
            GetFileSizeEx(file, &fileSize);

            if (headerBuffer[0] == 'S' &&
                headerBuffer[1] == 'M' &&
                headerBuffer[2] == 'D' &&
                headerBuffer[3] == 'U')
            {
                uint64_t* compressedSize = reinterpret_cast<uint64_t*>(headerBuffer.data() + sizeof(uint32_t));
                uint64_t* originalSize = reinterpret_cast<uint64_t*>(headerBuffer.data() + sizeof(uint32_t) + sizeof(uint64_t));
                ChunkInfo* chunks = reinterpret_cast<ChunkInfo*>(headerBuffer.data() + sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint64_t));

                if (fileSize.QuadPart == int64_t(headerAlignedSize + *compressedSize) &&
                    size == *originalSize)
                {
                    stream = new CompressedDataStream(percentRandom, size, *compressedSize, chunkSize, alignment, chunks);
                    m_dataStreams.push_back(stream);
                }
            }
            if (nullptr == stream)
            {
                OutputDebugString(L"Corrupt or mismatched file found, will over-write\n");
            }
        }
        CloseHandle(file);
    }
    return stream;
}



CompressedDataStream::CompressedDataStream(float dataRandPercent, uint64_t size, uint64_t compressedSize, uint32_t chunkSize, uint16_t alignment, ChunkInfo* chunks) :
    CompressedData(nullptr),
    CompressedDataOffset(0),
    CompressedSize(compressedSize),
    ChunkSize(chunkSize),
    Alignment(alignment),
    DataRandPercent(dataRandPercent),
    DataSize(size),
    m_originalData(nullptr),
    m_isInMemory(false),
    m_isInStorage(true)
{
    uint32_t chunkCount = uint32_t(size / chunkSize);
    CompressedTiles.resize(chunkCount);
    memcpy(CompressedTiles.data(), chunks, chunkCount * sizeof(ChunkInfo));

    size_t headerDataSize =
        sizeof(uint32_t) +          // 'S' 'M' 'D' 'U'
        sizeof(uint64_t) +          // compressedStreamSize  (not counting header)
        sizeof(uint64_t) +          // originalStreamSize
        chunkCount * sizeof(ChunkInfo);

    CompressedDataOffset = (headerDataSize + (alignment - 1)) & ~(alignment - 1);

    m_lastUsed = GetTickCount64();
}


// Because it takes a while to generate and compress random data, we're taking a shortcut here, and allowing a smaller chunk of unique data to be
// used over and over to fill a larger contiguous compressed stream. This allows much faster switching between streams when we need to regenerate,
// and means we don't need to allocate the whole original data size in memory.

CompressedDataStream::CompressedDataStream(DataStream * data, uint64_t size, uint32_t chunkSize, uint16_t alignment) :
    CompressedData(nullptr),
    CompressedDataOffset(0),
    ChunkSize(chunkSize),
    Alignment(alignment),
    DataRandPercent(data->PercentRandom),
    DataSize(size),
    m_originalData(data),
    m_isInMemory(true),
    m_isInStorage(false)
{
    assert(1 == __popcnt(chunkSize));                                       // must be a power of two
    assert(chunkSize >= 4096 && chunkSize <= DSTORAGE_MAX_DESTINATION_SIZE);  // must be acceptable limits
    assert(nullptr != data);
    assert(m_originalData->Size % chunkSize == 0);                            // data buffer must be a multiple of the chunk size
    assert(size % m_originalData->Size == 0);                                 // final size must be a multiple of the unique data size

    std::vector<void*> compressedMemChunks;
    m_originalData->Lock();

    ChunkInfo cInfo = {};
    uint32_t chunks = uint32_t(size / chunkSize);
    uint32_t uniqueChunks = uint32_t(m_originalData->Size / chunkSize);

    z_stream strm = {};
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    int err = deflateInit(&strm, Z_BEST_SPEED);
    if (err != Z_OK)
        throw std::runtime_error("Error in deflate on while zipping chunks");

    for (size_t chunk = 0; chunk < uniqueChunks; chunk++)
    {
        cInfo.DecopmressedOffset = chunk * uint64_t(ChunkSize);
        uint8_t* chunkSrcMem = reinterpret_cast<uint8_t*>(m_originalData->Data) + cInfo.DecopmressedOffset;

        if (chunk > 0)
        {
            if (deflateReset(&strm) != Z_OK)
                throw std::runtime_error("Error in deflate on while zipping chunks");
        }

        uint64_t zipBufferSize = deflateBound(&strm, static_cast<uint32_t> (ChunkSize));
        void *destBuffer = XMemVirtualAlloc(nullptr, zipBufferSize, MEM_COMMIT, XMEM_CPU, PAGE_READWRITE);
        if (!destBuffer)
            throw std::runtime_error("Out of memory while compressing chunks");

        strm.avail_in = uint32_t(ChunkSize);
        strm.next_in = chunkSrcMem;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        strm.avail_out = static_cast<uInt> (zipBufferSize);
        strm.next_out = static_cast<Bytef *> (destBuffer);
        err = deflate(&strm, Z_FINISH);    /* no bad return value */
        if (err == Z_STREAM_ERROR)
            throw std::runtime_error("Error in deflate on while zipping chunks");
        assert(err != Z_STREAM_ERROR);  /* state not clobbered */
        if (strm.avail_in != 0)
            throw std::runtime_error("Failed to zip the entire chunk");

        cInfo.CompressedSize = strm.total_out;

        compressedMemChunks.push_back(destBuffer);
        CompressedTiles.push_back(cInfo);
    }
    (void)deflateEnd(&strm);

    for (size_t chunk = uniqueChunks; chunk < chunks; chunk++)
    {
        cInfo.DecopmressedOffset = chunk * uint64_t(ChunkSize);
        cInfo.CompressedSize = CompressedTiles[chunk % uniqueChunks].CompressedSize;
        CompressedTiles.push_back(cInfo);
    }

    // compute the total memory needed, when placing all the copmressd chunks into a contiguous buffer
    uint64_t totalSize = 0;
    for (uint32_t i = 0; i < CompressedTiles.size(); i++)
    {
        // round up to the next alignment 
        totalSize += (Alignment - 1);
        totalSize &= ~(Alignment - 1);

        CompressedTiles[i].CompressedOffset = totalSize;
        totalSize += CompressedTiles[i].CompressedSize;
    }

    CompressedSize = totalSize;
    CompressedData = XMemVirtualAlloc(nullptr, totalSize, MEM_RESERVE | MEM_COMMIT | MEM_2MB_PAGES, XMEM_GRAPHICS, PAGE_READWRITE | PAGE_GRAPHICS_READWRITE | PAGE_WRITECOMBINE);
    if (!CompressedData)
        throw std::runtime_error("Out of memory while compressing chunks");
    memset(CompressedData, 0, totalSize);

    // ...then copy all those individual buffers into the correctly aligned offsets per the initial request
    for (uint32_t i = 0; i < CompressedTiles.size(); i++)
    {
        memcpy(reinterpret_cast<uint8_t*>(CompressedData) + CompressedTiles[i].CompressedOffset, compressedMemChunks[i % uniqueChunks], CompressedTiles[i].CompressedSize);
    }

    m_lastUsed = GetTickCount64();

    // free all the individual compressed blocks;
    for (uint32_t i = 0; i < uniqueChunks; i++)
    {
        VirtualFree(compressedMemChunks[i], 0, MEM_RELEASE);
    }
}

CompressedDataStream::~CompressedDataStream()
{
    if (m_isInMemory)
    {
        m_originalData->Release();
    }
    if (CompressedData)
    {
        VirtualFree(CompressedData, 0, MEM_RELEASE);
    }
}

