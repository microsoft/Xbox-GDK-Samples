//--------------------------------------------------------------------------------------
// DesktopGPUDecompression.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DesktopGPUDecompression.h"
#include "zlib/include/zlib.h"

#ifndef _GAMING_XBOX
bool DesktopGPUDecompression::RunSample(const std::wstring& fileName, ID3D12Device* device)
{
    struct ChunkMetaData
    {
        uint64_t chunkOffset;
        uint64_t chunkSize;
    };
    // custom header format for this sample has all uint64_t entries
    // numChunks, chunkUncompressedSize, array<chunkOffset, chunkCompressedSize>
    uint64_t numChunks;
    uint64_t chunkUncompressedSize;
    std::unique_ptr<ChunkMetaData[]> headerData;
    Microsoft::WRL::ComPtr<ID3D12Resource> bufferResource;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;

    ScopedHandle fenceEvent;

    // Use Win32 to read in the metadata at the head of the file, could be done using DirectStorage as well.
    {
        CREATEFILE2_EXTENDED_PARAMETERS params = {};

        params.dwSize = sizeof(params);
        params.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        params.dwFileFlags = 0;
        ScopedHandle dataFile(safe_handle(CreateFile2(fileName.c_str(), GENERIC_READ, 0, OPEN_EXISTING, &params)));
        if (!dataFile)
            return false;

        DWORD actualRead;
        ReadFile(dataFile.get(), &numChunks, 8, &actualRead, nullptr);
        if (actualRead != 8)
            return false;
        ReadFile(dataFile.get(), &chunkUncompressedSize, 8, &actualRead, nullptr);
        if (actualRead != 8)
            return false;
        headerData.reset(new ChunkMetaData[numChunks]);
        ReadFile(dataFile.get(), headerData.get(), static_cast<uint32_t>(numChunks * sizeof(ChunkMetaData)), &actualRead, nullptr);
        if (actualRead != numChunks * sizeof(ChunkMetaData))
            return false;
    }

    {
        // Create the ID3D12Resource buffer which will be populated with the file's contents
        D3D12_HEAP_PROPERTIES bufferHeapProps = {};
        bufferHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = numChunks * chunkUncompressedSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(device->CreateCommittedResource(&bufferHeapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_GRAPHICS_PPV_ARGS(bufferResource.ReleaseAndGetAddressOf())));

        // Configure a fence to be signaled when the request is completed
        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.ReleaseAndGetAddressOf())));
    }

    // Create the DirectStorage factory
    // Note: There is only one factory object even if there are multiple file devices
    //     DirectStorage will route correctly based on the path of the file used in the request
    Microsoft::WRL::ComPtr<DStorageFactoryCrossPlatform> factory;
    DX::ThrowIfFailed(DStorageGetFactory(__uuidof(DStorageFactoryCrossPlatform), (void**)(factory.ReleaseAndGetAddressOf())));

    // Open the data file
    // Files are opened with FILE_SHARED_READ access. Titles can also open the file using Win32 as long as the sharing permissions are respected
    Microsoft::WRL::ComPtr <DStorageFileCrossPlatform> file;
    DX::ThrowIfFailed(factory->OpenFile(fileName.c_str(), __uuidof(DStorageFileCrossPlatform), (void**)(file.ReleaseAndGetAddressOf())));

    fenceEvent.reset(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    uint64_t fenceValue = 1;

    DX::ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent.get()));

    // Create a queue that will manage read requests being routed to the hardware
    Microsoft::WRL::ComPtr <DStorageQueueCrossPlatform> queue;
    DSTORAGE_QUEUE_DESC queueDesc = {};
    queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
    queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;	    // Four possible priorities, high, normal, low, and realtime.
    queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;	// How many outstanding requests can the queue hold, make sure to size this to hold at least all your potiential simultaneous in-flight requests
    queueDesc.Device = device;
    queueDesc.Name = u8"SimpleDirectStorage: DesktopGPUDecompression";
    DX::ThrowIfFailed(factory->CreateQueue(&queueDesc, __uuidof(DStorageQueueCrossPlatform), (void**)(queue.ReleaseAndGetAddressOf())));

    for (uint32_t i = 0; i < numChunks; i++)
    {
        // Initialize the read request structure and tell DirectStorage it's going to be using a custom decompression codec
        DSTORAGE_REQUEST request;
        request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_BUFFER;
        request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_GDEFLATE;
        request.Source.File.Source = file.Get();
        request.Source.File.Offset = static_cast<uint32_t> (headerData[i].chunkOffset);
        request.Source.File.Size = static_cast<uint32_t> (headerData[i].chunkSize);
        request.UncompressedSize = static_cast<uint32_t> (chunkUncompressedSize);
        request.Destination.Buffer.Resource = bufferResource.Get();
        request.Destination.Buffer.Offset = i * chunkUncompressedSize;
        request.Destination.Buffer.Size = static_cast<uint32_t> (chunkUncompressedSize);

        // Enqueue the actual request into the relevant queue
        // The data in the request structure is copied during the enqueue and can safely be recycled immediately after the enqueue call
        queue->EnqueueRequest(&request);
    }

    // enqueue the fence along with the value want to set when all previous enqueued reads have completed
    // Signal the fence when done
    queue->EnqueueSignal(fence.Get(), fenceValue);

    // Submit the contents of the queue to the hardware for processing.
    queue->Submit();

    // Wait until the fence is signalled which marks the requests as complete
    WaitForSingleObject(fenceEvent.get(), INFINITE);

    if (fence->GetCompletedValue() == (uint64_t)-1)
    {
        // Device removed!  Give DirectStorage a chance to detect the error.
        // In this case all pending GPU decompression requests should be considered failed and need be resubmitted once the device is restored
        // the most common cause of this state when using GPU decompression is bad data passed in for the request
        // To help debug this issue you can disable GPU decompression with the using DStorageSetConfiguration with the DisableGpuDecompression flag
        // Once the source of the corrupt data is resolved you can reenable GPU decompression for increased decompression speed
        Sleep(500);
    }

    // cleanup all buffers and release DirectStorage objects
    fence.Reset();
    queue.Reset();
    file.Reset();
    factory.Reset();
    return true;
}

#endif
