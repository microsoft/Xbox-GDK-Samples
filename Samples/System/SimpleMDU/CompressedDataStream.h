//--------------------------------------------------------------------------------------
// CompressedDataStream.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <vector>

class CompressedDataStream;

class DataStream
{
public:

    static DataStream*  Generate(uint64_t size, float percentRandom) noexcept;
    static DataStream*  Fetch(uint64_t size, float percentRandom) noexcept;

    static size_t       GetCount() noexcept { return m_dataStreams.size(); }

    ~DataStream();

    void*               Data            = nullptr;
    uint64_t            Size            = 0;
    float               PercentRandom   = 0;

    void                Lock()              { m_refCount++; }
    void                Release()           { m_refCount--; }

protected:

    DataStream(uint64_t size, float percentRandom);
    static              std::vector<DataStream*> m_dataStreams;
    static std::mutex   m_genMutex;

private:
    std::atomic<int>    m_refCount;

    DataStream() {};

    static constexpr size_t c_allocGranularity  = 0x200000;     // 2 MiB
};






class CompressedDataStream
{
public:

    static CompressedDataStream*    GenerateInMemory(DataStream * data, uint64_t size, uint32_t tileSize, uint16_t alignment);
    static CompressedDataStream*    FetchMemoryBacked(DataStream * data, uint64_t size, uint32_t tileSize, uint16_t alignment);
    static CompressedDataStream*    FetchMemoryBacked(float percentRandom, uint64_t size, uint32_t chunkSize, uint16_t alignment);

    static CompressedDataStream*    HydrateFromStorage(float percentRandom, uint64_t size, uint32_t chunkSize, uint16_t alignment);
    static CompressedDataStream*    FetchStorageBacked(float percentRandom, uint64_t size, uint32_t chunkSize, uint16_t alignment);

    static void                     ResolvePath(wchar_t *path, size_t pathLength, float percentRandom, uint64_t size, uint32_t chunkSize, uint16_t alignment);

    static void                     ReleaseLRU() noexcept;

    static size_t                   GetCount() noexcept { return m_dataStreams.size(); }

    ~CompressedDataStream();

    struct ChunkInfo {
        uint64_t DecopmressedOffset;
        uint64_t CompressedOffset;
        uint32_t CompressedSize;
    };

    void*                   CompressedData          = nullptr; // pointer to the data if it's a in-memory compressed stream
    size_t                  CompressedDataOffset    = 0;       // file offset to the beginning of the data if it's a file on disk

    std::vector<ChunkInfo>  CompressedTiles;
    uint64_t                CompressedSize      = 0;
    uint32_t                ChunkSize           = 0;
    uint16_t                Alignment           = 0;
    float                   DataRandPercent     = 0;
    uint64_t                DataSize            = 0;

    void                    Touch() { m_lastUsed = GetTickCount64(); }
    void                    PersistToStorage();

protected:

    CompressedDataStream(DataStream * data, uint64_t size, uint32_t tileSize, uint16_t alignment);
    CompressedDataStream(float dataRandPercent, uint64_t size, uint64_t compressedSize, uint32_t chunkSize, uint16_t alignment, ChunkInfo* chunks);

    static std::vector<CompressedDataStream*> m_dataStreams;
    static std::mutex       m_genMutex;

private:
    uint64_t                m_lastUsed      = 0;
    DataStream*             m_originalData  = nullptr;
    bool                    m_isInMemory    = false;
    bool                    m_isInStorage   = false;

    CompressedDataStream() {};
};

