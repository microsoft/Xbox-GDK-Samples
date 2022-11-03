//--------------------------------------------------------------------------------------
// XBoxInMemoryZLibDecompression.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "XBoxInMemoryZLibDecompression.h"
#include "RDTSCPStopWatch.h"
#include "zlib/include/zlib.h"

#define VALIDATE_READS

#ifdef _GAMING_XBOX_SCARLETT

// DirectStorage on the console is the way to access the hardware decompression engine
// It supports the zlib format, https://www.ietf.org/rfc/rfc1950.txt, as well as the Microsoft BCPack format
// This sample is for the console only and shows how to use decompression for data already in memory
bool XBoxInMemoryZLibDecompression::RunSample()
{
    CreateDataBlocks();

    // create a status array for notification of read completion. In this case we only need one slot in the array
    // It's not valid to check a slot for completion until after that slot has been enqueued, the default is signaled as complete
    DX::ThrowIfFailed(s_factory->CreateStatusArray(1, u8"XBoxInMemoryZLibDecompression Status Array", __uuidof(DStorageStatusArrayCrossPlatform), (void**)(m_status.ReleaseAndGetAddressOf())));

    std::unique_ptr<char[]> destBuffer[c_numBlocks];
    for (uint32_t i = 0; i < c_numBlocks; i++)
    {
        uint32_t uncompressedSize = m_dataBlockSizes[i].first;
        uint32_t compressedSize = m_dataBlockSizes[i].second;

        destBuffer[i].reset(new char[uncompressedSize]);
        if (compressedSize != uncompressedSize)             // check if the block was actually compressed since hardware decompression on the console can only handle compressed size < uncompressed size
        {
            // Initialize the read request structure.
            DStorageRequestCrossPlatform request = {};
            request.SetupXboxHardwareInMemoryZLib(m_dataBlocks[i].get(), destBuffer[i].get(), compressedSize, uncompressedSize);

            // Enqueue the actual request into the relevant queue
            // The data in the request structure is copied during the enqueue and can safely be recycled immediately after the enqueue call
            s_inMemoryQueue->EnqueueRequest(&request);
        }
        else     // data was not actually compressed
        {
            memcpy(destBuffer[i].get(), m_dataBlocks[i].get(), uncompressedSize);
        }
    }
    // enqueue the status array along with the slot in the array we want to be signaled when all previous enqueued reads have completed
    // this will immediately flip the slot in the status array to not signaled
    s_inMemoryQueue->EnqueueStatus(m_status.Get(), 0);

    // Submit the contents of the queue to the hardware for processing.
    s_inMemoryQueue->Submit();

    // Spin waiting until the first slot in the status is marked complete
    while (!m_status->IsComplete(0))
    {
        _mm_pause();
    }

    // check the contents of the data read from the file to make sure the correct contents were loaded
#ifdef VALIDATE_READS
    for (uint32_t i = 0; i < c_numBlocks; i++)
    {
        uint64_t startValue = 0;
        uint64_t* temp = (uint64_t*)destBuffer[i].get();
        uint32_t numberEntries = m_dataBlockSizes[i].first / sizeof(uint64_t);
        for (uint32_t location = 0; location < numberEntries; location++)
        {
            if (temp[location] != startValue)
                DebugBreak();
            assert(temp[location] == startValue);
            startValue++;
        }
    }
#endif
    return true;
}

void XBoxInMemoryZLibDecompression::CreateDataBlocks()
{
    static uint32_t c_blockSize[c_numBlocks] = {
      16,                       // smallest block size for hardware decompression
      32,
      128,
      256,
      1 * s_1KB,
      8 * s_1KB,
      16 * s_1KB,
      64 * s_1KB,
      512 * s_1KB,
      1 * s_1MB,
      4 * s_1MB,
      8 * s_1MB,
      16 * s_1MB,
      32 * s_1MB                // largest block size for hardware decompression on the console
    };
    m_dataBlocks.resize(c_numBlocks);

    for (uint32_t curBlock = 0; curBlock < c_numBlocks; curBlock++)
    {
        std::unique_ptr<uint64_t[]> buffer(new uint64_t[c_blockSize[curBlock]]);
        for (uint32_t j = 0; j < c_blockSize[curBlock] / sizeof(uint64_t); j++)
        {
            buffer[j] = j;
        }

        uint32_t compressedSize = 0;
        z_stream strm = {};
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        int err = deflateInit(&strm, Z_DEFAULT_COMPRESSION);
        if (err != Z_OK)
            return;

        uint32_t zipBufferSize = deflateBound(&strm, static_cast<uint32_t> (c_blockSize[curBlock]));
        std::unique_ptr<char[]> destBuffer(new char[zipBufferSize]);

        strm.avail_in = c_blockSize[curBlock];
        strm.next_in = reinterpret_cast<Bytef*> (buffer.get());

        /* run deflate() on input until output buffer not full, finish compression if all of source has been read in */
        strm.avail_out = static_cast<uInt> (zipBufferSize);
        strm.next_out = static_cast<Bytef*> (static_cast<void*> (destBuffer.get()));
        err = deflate(&strm, Z_FINISH);    /* no bad return value */
        if (err == Z_STREAM_ERROR)
            throw std::runtime_error("Error in deflate on zipped file creation");
        assert(err != Z_STREAM_ERROR);  /* state not clobbered */
        if (strm.avail_in != 0)
            throw std::runtime_error("Failed to zip the entire stream in file creation");

        compressedSize = strm.total_out;
        (void)deflateEnd(&strm);
        if (compressedSize >= c_blockSize[curBlock])
        {
            compressedSize = c_blockSize[curBlock];
            memcpy(destBuffer.get(), buffer.get(), c_blockSize[curBlock]);
        }
        m_dataBlockSizes.push_back(std::make_pair(c_blockSize[curBlock], compressedSize));
        m_dataBlocks[curBlock].reset(destBuffer.release());
    }
}
#endif
