//--------------------------------------------------------------------------------------
// DesktopCPUDecompression.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DesktopCPUDecompression.h"
#include "zlib/include/zlib.h"

#define VALIDATE_READS

#ifndef _GAMING_XBOX
bool DesktopCPUDecompression::RunSample(const std::wstring& fileName, uint32_t zipFileUncompressedSize)
{
    uint32_t compressedFileSize = 0;
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(fileName.c_str(), GetFileExInfoStandard, &info))
        return false;

    compressedFileSize = info.nFileSizeLow;

    // Create the DirectStorage factory
    // Note: There is only one factory object even if there are multiple file devices
    //     DirectStorage will route correctly based on the path of the file used in the request
    Microsoft::WRL::ComPtr<DStorageFactoryCrossPlatform> factory;
    DX::ThrowIfFailed(DStorageGetFactory(__uuidof(DStorageFactoryCrossPlatform), (void**)(factory.ReleaseAndGetAddressOf())));

    // DirectStorage on Desktop has a single decompression queue which contains all the requests that need to be decompressed by the title
    // The decompression queue is a singleton and the way to get access to it is through the DirectStorage factory using QueryInterface
    Microsoft::WRL::ComPtr< IDStorageCustomDecompressionQueue> decompressionQueue;
    factory->QueryInterface(__uuidof(IDStorageCustomDecompressionQueue), (void**)(decompressionQueue.ReleaseAndGetAddressOf()));

    // Open the data file
    // Files are opened with FILE_SHARED_READ access. Titles can also open the file using Win32 as long as the sharing permissions are respected
    Microsoft::WRL::ComPtr <DStorageFileCrossPlatform> file;
    DX::ThrowIfFailed(factory->OpenFile(fileName.c_str(), __uuidof(DStorageFileCrossPlatform), (void**)(file.ReleaseAndGetAddressOf())));

    // Create a queue that will manage read requests being routed to the hardware
    Microsoft::WRL::ComPtr <DStorageQueueCrossPlatform> queue;
    DSTORAGE_QUEUE_DESC queueDesc = {};
    queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
    queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;	    // Four possible priorities, high, normal, low, and realtime.
    queueDesc.Capacity = DSTORAGE_MIN_QUEUE_CAPACITY;	// How many outstanding requests can the queue hold, make sure to size this to hold at least all your potiential simultaneous in-flight requests
    queueDesc.Name = u8"SimpleDirectStorage: DesktopCPUDecompression";
    DX::ThrowIfFailed(factory->CreateQueue(&queueDesc, __uuidof(DStorageQueueCrossPlatform), (void**)(queue.ReleaseAndGetAddressOf())));

    // create a status array for notification of read completion. In this case we only need one slot in the array
    // It's not valid to check a slot for completion until after that slot has been enqueued, the default is signaled as complete
    Microsoft::WRL::ComPtr < DStorageStatusArrayCrossPlatform> status;
    DX::ThrowIfFailed(factory->CreateStatusArray(1, u8"DesktopCPUDecompression Status Array", __uuidof(DStorageStatusArrayCrossPlatform), (void**)(status.ReleaseAndGetAddressOf())));

    // create the destination buffer for the data loaded from disk
    std::unique_ptr<char> destBuffer(new char[zipFileUncompressedSize]);

    // Initialize the read request structure and tell DirectStorage it's going to be using a custom decompression codec
    DSTORAGE_REQUEST request;
    request.UncompressedSize = zipFileUncompressedSize;
    request.Destination.Memory.Buffer = destBuffer.get();
    request.Destination.Memory.Size = zipFileUncompressedSize;
    request.CancellationTag = 0;
    request.Source.File.Source = file.Get();
    request.Source.File.Size = compressedFileSize;
    request.Source.File.Offset = 0;
    request.Options.CompressionFormat = DSTORAGE_CUSTOM_COMPRESSION_0;
    request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MEMORY;
    request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;

    // Enqueue the actual request into the relevant queue
    // The data in the request structure is copied during the enqueue and can safely be recycled immediately after the enqueue call
    queue->EnqueueRequest(&request);

    // enqueue the status array along with the slot in the array we want to be signaled when all previous enqueued reads have completed
    // this will immediately flip the slot in the status array to not signaled
    queue->EnqueueStatus(status.Get(), 0);

    // Submit the contents of the queue to the hardware for processing.
    queue->Submit();

    // With DirectStorage on Desktop there is an Event that is signaled when there are requests ready to be processed
    // When the Event is signalled the title can get a list of requests to be decompressed, perform the decompression, then submit the results back to DirectStorage
    uint32_t totalRequestsProcessed(0);
    while (totalRequestsProcessed != 1)
    {
        // wait for the Event bound to the decompression queue to signal to say there is work to do
        WaitForSingleObject(decompressionQueue->GetEvent(), INFINITE);
        uint32_t numRequests(0);
        DSTORAGE_CUSTOM_DECOMPRESSION_REQUEST decompressRequest;
        // Get the lists of requests ready to be decompressed by the title.
        // Note: In this sample we're just submitting one request so we know there can be at most one request to be decompressed
        // The title can query the pending list of requests in batches of any size and GetRequests will return up to that number of pending requests
        decompressionQueue->GetRequests(1, &decompressRequest, &numRequests);
        if (numRequests != 0)
        {
            LZInflate(decompressRequest.DstBuffer, decompressRequest.DstSize, decompressRequest.SrcBuffer, decompressRequest.SrcSize);
            DSTORAGE_CUSTOM_DECOMPRESSION_RESULT result;
            result.Id = decompressRequest.Id;
            result.Result = S_OK;
            // Once the decompression has completed the result needs to be submitted back to DirectStorage so its processing can be completed
            // It's not safe to use the results of this decompression until the matching Status/Fence has been signaled by DirectStorage
            decompressionQueue->SetRequestResults(numRequests, &result);
            totalRequestsProcessed += numRequests;
        }
    }

    // Spin waiting until the first slot in the status array is marked complete
    while (!status->IsComplete(0))
    {
        Sleep(1);
    }

    // check the contents of the data read from the file to make sure the correct contents were loaded
#ifdef VALIDATE_READS
    uint32_t startValue = 0;
    uint32_t* temp = (uint32_t*)destBuffer.get();
    uint32_t numberEntries = zipFileUncompressedSize / sizeof(uint32_t);
    for (uint32_t location = 0; location < numberEntries; location++)
    {
        if (temp[location] != startValue)
            DebugBreak();
        assert(temp[location] == startValue);
        startValue++;
    }
#endif

    // cleanup all buffers and release DirectStorage objects
    destBuffer.reset();
    status.Reset();
    queue.Reset();
    file.Reset();
    factory.Reset();
    return true;
}

size_t DesktopCPUDecompression::LZInflate(void* output, size_t outputSize, const void* input, size_t length)
{
    // Initialize inflate
    z_stream strm = {};
    strm.data_type = Z_BINARY;
    int err = inflateInit(&strm);

    strm.total_in = strm.avail_in = (uInt)length;
    strm.next_in = (Bytef*)input;
    strm.avail_out = (uInt)outputSize;
    strm.next_out = (Bytef*)output;

    // Inflate the input buffer in one step
    err = inflate(&strm, Z_FINISH);
    if (err != Z_STREAM_END)
    {
        throw std::exception("Internal ZLIB error");
    }

    inflateEnd(&strm);

    return strm.total_out;
}
#endif
