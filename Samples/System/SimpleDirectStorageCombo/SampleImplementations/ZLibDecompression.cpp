//--------------------------------------------------------------------------------------
// Decompression.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ZLibDecompression.h"

#define VALIDATE_READS

bool ZLibDecompression::RunSample(const std::wstring& fileName, uint32_t zipFileUncompressedSize)
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
    queueDesc.Name = u8"SimpleDirectStorage: Decompression";
    DX::ThrowIfFailed(factory->CreateQueue(&queueDesc, __uuidof(DStorageQueueCrossPlatform), (void**)(queue.ReleaseAndGetAddressOf())));

    // create a status array for notification of read completion. In this case we only need one slot in the array
    // It's not valid to check a slot for completion until after that slot has been enqueued, the default is signaled as complete
    Microsoft::WRL::ComPtr < DStorageStatusArrayCrossPlatform> status;
    DX::ThrowIfFailed(factory->CreateStatusArray(1, u8"ZLibDecompression Status Array", __uuidof(DStorageStatusArrayCrossPlatform), (void**)(status.ReleaseAndGetAddressOf())));

    // create the destination buffer for the data loaded from disk
    std::unique_ptr<char> destBuffer(new char[zipFileUncompressedSize]);

    // Initialize the read request structure.
    // Note: On the console the compressed size is required to be less than the uncompressed size
    DStorageRequestCrossPlatform request = {};
    request.SetupXboxHardwareZLibRead(file.Get(), destBuffer.get(), 0, compressedFileSize, zipFileUncompressedSize);

    // Enqueue the actual request into the relevant queue
    // The data in the request structure is copied during the enqueue and can safely be recycled immediately after the enqueue call
    queue->EnqueueRequest(&request);

    // enqueue the status array along with the slot in the array we want to be signaled when all previous enqueued reads have completed
    // this will immediately flip the slot in the status array to not signaled
    queue->EnqueueStatus(status.Get(), 0);

    // Submit the contents of the queue to the hardware for processing.
    queue->Submit();

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
