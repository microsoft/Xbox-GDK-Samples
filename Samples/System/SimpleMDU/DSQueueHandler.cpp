//--------------------------------------------------------------------------------------
// DSQueueHandler.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DSQueueHandler.h"

struct BatchTime
{
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    uint64_t MduBytesIn;
    uint64_t MduBytesOut;
};

DSQueueHandler::DSQueueHandler(bool fromStorage, uint64_t uniqueDataSize, uint64_t totalStreamSize) :
    TaskStatus(TaskStatus::InitialState),
    QueueStatus(QueueStatus::PreInit),
    MduInMbps(0),
    MduOutMbps(0),
    m_fromStorage(fromStorage),
    m_uniqueDataSize(uniqueDataSize),
    m_dataSize(totalStreamSize)
{
}

void DSQueueHandler::StartWorker()
{
    m_workerThread = new std::thread(&DSQueueHandler::WorkerFunction, this);
}

DSQueueHandler::~DSQueueHandler()
{
    QueueStatus = QueueStatus::Stopped;
    m_workerThread->join();
}

// All the actual Sample code runs in this function which is on it's own thread to avoid interference with rendering
void DSQueueHandler::WorkerFunction()
{
    auto threadHandle = GetCurrentThread();
    SetThreadDescription(threadHandle, L"Worker thread");

    SetThreadAffinityMask(threadHandle, 0x04);

    WorkerStartupFunction();

    // Create the DirectStorage factory
    Microsoft::WRL::ComPtr<IDStorageFactoryX> factory;
    DX::ThrowIfFailed(DStorageGetFactory(__uuidof(IDStorageFactoryX), (void **)(factory.ReleaseAndGetAddressOf())));

    const int c_targetBytesInFlight = 96 * 1024 * 1024;  //96MiB
    const int c_batches = 3;
    const int c_minBytesPerBatch = c_targetBytesInFlight / c_batches;

    // Create a queue that will manage read requests being routed to the hardware
    Microsoft::WRL::ComPtr <IDStorageQueueX> queue;
    DSTORAGE_QUEUE_DESC queueDesc = {};
    queueDesc.SourceType = m_fromStorage ?
        DSTORAGE_REQUEST_SOURCE_FILE :
        DSTORAGE_REQUEST_SOURCE_MEMORY;
    queueDesc.Priority = DSTORAGE_PRIORITY_REALTIME;	
    queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;	// How many outstanding requests can the queue hold
    queueDesc.Name = m_fromStorage ?
        u8"SimpleMDU: From-Storage" :
        u8"SimpleMDU: In-Memory";
    DX::ThrowIfFailed(factory->CreateQueue(&queueDesc, __uuidof(IDStorageQueueX), (void **)(queue.ReleaseAndGetAddressOf())));

    // Create a status array for notification of read completion.  We will group requests into batches, so we
    // need one status entry for each batch, 3 by default.
    Microsoft::WRL::ComPtr <IDStorageStatusArrayX> status;
    DX::ThrowIfFailed(factory->CreateStatusArray(c_batches, u8" Status Array", __uuidof(IDStorageStatusArrayX), (void **)(status.ReleaseAndGetAddressOf())));

    uint64_t nextChunk = 0;
    uint64_t nextBatchSubmit = 0;
    uint64_t nextBatchComplete = 0;

    const uint32_t timings = 160;
    BatchTime batchTimes[timings] = {};

    void* decompressedMem = XMemVirtualAlloc(nullptr, m_dataSize, MEM_RESERVE | MEM_COMMIT | MEM_2MB_PAGES, XMEM_GRAPHICS, PAGE_READWRITE | PAGE_GRAPHICS_READWRITE | PAGE_WRITECOMBINE);
    if (!decompressedMem)
        throw std::runtime_error("Out of memory while initializing queues");

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    CompressedDataStream* backingDataStream[c_batches] = {};            // cache the backing data stream for each batch of requests
    Microsoft::WRL::ComPtr <IDStorageFileX> backingFiles[c_batches];    // 

    QueueStatus = QueueStatus::Running;

    while (true)
    {
        if (QueueStatus::Running == QueueStatus)
        {
            uint64_t bytesInThisBatch = 0;
            uint64_t bytesOutThisBatch = 0;
            uint32_t nextSlot = nextBatchSubmit % c_batches;

            QueryPerformanceCounter(&batchTimes[nextBatchSubmit % timings].start);

            // first fetch the desired data stream we should be using...
            CompressedDataStream* dataStreamForBatch = ActiveCompressedStream;
            dataStreamForBatch->Touch();

            // if that last user-requested stream is different from the current one, we have some work to do in switching
            if (dataStreamForBatch != backingDataStream[nextSlot])
            {
                // if we're storage backed, and we're going back to an existing open file, get that cached file
                bool skipOpen = false;
                if (m_fromStorage)
                {
                    for (int otherSlot = 0; otherSlot < c_batches; otherSlot++)
                    {
                        if (dataStreamForBatch == backingDataStream[otherSlot])
                        {
                            skipOpen = true;
                            backingFiles[nextSlot] = backingFiles[otherSlot];
                            break;
                        }
                    }
                }

                // update our local pointer to the requested data stream, and start at the first chunk of that stream
                backingDataStream[nextSlot] = dataStreamForBatch;
                nextChunk = 0;

                // if we're storage backed, and it wasn't just swapping back to a prior file, open a new file for the requested stream.
                // because we know that the prior requests for this slot were all complete, this action of resetting the old file pointer may
                // also safely close out that old file, if it's the last reference
                if (m_fromStorage & !skipOpen)
                {
                    wchar_t path[MAX_PATH];
                    CompressedDataStream::ResolvePath(
                        path,
                        _countof(path),
                        dataStreamForBatch->DataRandPercent,
                        dataStreamForBatch->DataSize,
                        dataStreamForBatch->ChunkSize,
                        dataStreamForBatch->Alignment);
                    DX::ThrowIfFailed(factory->OpenFile(path, __uuidof(IDStorageFileX), (void **)(backingFiles[nextSlot].ReleaseAndGetAddressOf())));
                }
            }

            // ...then continue as normal, queueing up enough requests to meet our data-in-flight target
            // If this were a real title, these requests might be from a larger pool of files, and the requests would
            // likely originate from a prioritized list of asset load requests.

            while (bytesOutThisBatch < c_minBytesPerBatch)
            {
                assert(dataStreamForBatch != nullptr);
                _Analysis_assume_(dataStreamForBatch != nullptr);  // Tell static analyzer dataStreamForBatch is non-nullptr
                CompressedDataStream::ChunkInfo &nextCompressedChunk = dataStreamForBatch->CompressedTiles[nextChunk];

                // Initialize the read request structure.
                DSTORAGE_REQUEST request = {};
                request.Options.SourceType = m_fromStorage ?
                    DSTORAGE_REQUEST_SOURCE_TYPE::DSTORAGE_REQUEST_SOURCE_FILE :
                    DSTORAGE_REQUEST_SOURCE_TYPE::DSTORAGE_REQUEST_SOURCE_MEMORY;
                request.Options.ZlibDecompress = true;

                if (m_fromStorage)
                {
                    request.File = backingFiles[nextSlot].Get();
                    request.FileOffset = dataStreamForBatch->CompressedDataOffset + nextCompressedChunk.CompressedOffset;
                    assert(request.FileOffset);
                }
                else
                {
                    request.Source = reinterpret_cast<uint8_t*>(dataStreamForBatch->CompressedData) + nextCompressedChunk.CompressedOffset;
                }
                request.SourceSize = nextCompressedChunk.CompressedSize;

                request.Destination = reinterpret_cast<uint8_t*>(decompressedMem) + nextCompressedChunk.DecopmressedOffset;
                request.DestinationSize = dataStreamForBatch->ChunkSize;

                // Enqueue the actual request into the relevant queue
                // The data in the request structure is copied during the enqueue and can safely be recycled immediately after the enqueue call
                queue->EnqueueRequest(&request);

                bytesOutThisBatch += dataStreamForBatch->ChunkSize;
                bytesInThisBatch += nextCompressedChunk.CompressedSize;

                nextChunk = (nextChunk + 1) % dataStreamForBatch->CompressedTiles.size();
            }

            // enqueue the status array along with the slot in the array we want to be signaled when all previous enqueued reads have completed
            // this will immediately flip the slot in the status array to not signaled
            queue->EnqueueStatus(status.Get(), nextSlot);

            // Submit the contents of the queue to the hardware for processing.
            queue->Submit();

            batchTimes[nextBatchSubmit % timings].MduBytesIn = bytesInThisBatch;
            batchTimes[nextBatchSubmit % timings].MduBytesOut = bytesOutThisBatch;
            batchTimes[nextBatchSubmit % timings].end.QuadPart = 0;
            nextBatchSubmit++;

            // We try to keep c_batches in flight at once.  So when starting\restarting streaming, we'll
            // need queue that number of batches before waiting on anything
            if (nextBatchSubmit - nextBatchComplete < c_batches) continue;
        }

        // Spin waiting until the first slot in the status array is marked complete
        // We have enough data queued at any one time that we can 
        while (!status->IsComplete(nextBatchComplete % c_batches))
        {
            //  Topping up the queue to 96MiB decompressed requests every 16ms means a cap of ~5.7GiB/s to memory.
            //  Since this sample includes the ability to use highly compressible streams, that the MDU can
            //  decompressed much faster than that, we're instead going to top up the queue
            //  at a much higher rate, just to demonstrate the hardware throughput characteristics

            Sleep(0);   // This can be adjusted to 16ms to see behavior closer to what a typical title would see if topping up once per frame.
        }

        DX::ThrowIfFailed(status->GetHResult(nextBatchComplete % c_batches));

        QueryPerformanceCounter(&batchTimes[nextBatchComplete % timings].end);
        nextBatchComplete++;


        // compute the average throughput for the valid batch timings that we've collected.
        // just do this occasionally, so the numbers don't flutter too quickly on screen
        if (nextBatchSubmit % (timings / 10) == 0 ||
            nextBatchSubmit == nextBatchComplete )           // this is the case that all submitted batches have completed....  aka we're likely paused
        {
            BatchTime totalTimes = {};
            uint32_t firstBatchTime = nextBatchSubmit % timings;

            for (uint32_t b = 0; b < timings - c_batches; b++)
            {
                // fast forward to the first timing that actually has data
                if (0ll == totalTimes.start.QuadPart)
                {
                    totalTimes.start = batchTimes[(firstBatchTime + b) % timings].end;  // the completion time of this batch will be roughly when the next batch was started
                    continue;
                }
                // and after we've found our first valid timing, break once we run across invalid timings
                if (0 == batchTimes[(firstBatchTime + b) % timings].end.QuadPart)
                {
                    break;
                }
                totalTimes.MduBytesIn += batchTimes[(firstBatchTime + b) % timings].MduBytesIn;
                totalTimes.MduBytesOut += batchTimes[(firstBatchTime + b) % timings].MduBytesOut;
                totalTimes.end = batchTimes[(firstBatchTime + b) % timings].end;
            }
            
            int64_t us = (1000000 * (totalTimes.end.QuadPart - totalTimes.start.QuadPart)) / frequency.QuadPart;
            if (us)
            {
                float s = float(us) / 1000000;

                uint64_t InMB = totalTimes.MduBytesIn / 1000000;
                uint64_t OutMB = totalTimes.MduBytesOut / 1000000;

                MduInMbps = float(InMB) / s;
                MduOutMbps = float(OutMB) / s;
            }
        }

        // on suspend or terminate, we'll reach a point where all requests have completed...
        if (nextBatchSubmit == nextBatchComplete)
        {
            if (QueueStatus::Paused == QueueStatus)
            {
                // clear timings reported throughput values
                MduInMbps = 0.0f;
                MduOutMbps = 0.0f;

                // sleep while paused...
                while (QueueStatus::Paused == QueueStatus) Sleep(16);

                // clear timings, which is signified by an end time of "0"
                for (uint32_t timing = 0; timing < timings; timing++)
                {
                    batchTimes[timing].end.QuadPart = 0;
                }
            }
            else if (QueueStatus::Stopped == QueueStatus)
            {
                break;
            }
        }
    }

    VirtualFree(decompressedMem, 0, MEM_RELEASE);

    for (uint32_t i = 0; i < c_batches; i++)
    {
        if (backingFiles[i])
            backingFiles[i].Reset();
    }
    status.Reset();
    queue.Reset();
    factory.Reset();
}
