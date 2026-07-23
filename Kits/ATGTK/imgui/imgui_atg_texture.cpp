//--------------------------------------------------------------------------------------
// imgui_atg_texture.cpp
//
// ImGuiAtg::Texture - load and display images in ImGui using WIC and D3D12
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "imgui_atg_texture.h"
#include "imgui_atg_device_context.h"

#include <wincodec.h>
#include <wrl/client.h>

#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;

namespace ImGuiAtg
{
    //--------------------------------------------------------------------------------------
    // WIC helpers
    //--------------------------------------------------------------------------------------

    static bool DecodeImageWIC(
        IWICImagingFactory* factory,
        IWICBitmapDecoder* decoder,
        std::vector<uint8_t>& outPixels,
        int& outWidth,
        int& outHeight,
        int& outStride)
    {
        ComPtr<IWICBitmapFrameDecode> frame;
        if (FAILED(decoder->GetFrame(0, &frame)))
            return false;

        UINT w = 0, h = 0;
        if (FAILED(frame->GetSize(&w, &h)))
            return false;

        ComPtr<IWICFormatConverter> converter;
        if (FAILED(factory->CreateFormatConverter(&converter)))
            return false;

        if (FAILED(converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeMedianCut)))
            return false;

        outWidth = static_cast<int>(w);
        outHeight = static_cast<int>(h);
        outStride = outWidth * 4;
        outPixels.resize(static_cast<size_t>(outStride) * outHeight);

        if (FAILED(converter->CopyPixels(nullptr, static_cast<UINT>(outStride), static_cast<UINT>(outPixels.size()), outPixels.data())))
            return false;

        return true;
    }

    static bool DecodeFromFile(
        const wchar_t* path,
        std::vector<uint8_t>& outPixels,
        int& outWidth,
        int& outHeight,
        int& outStride)
    {
        ComPtr<IWICImagingFactory> factory;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
            return false;

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(factory->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)))
            return false;

        return DecodeImageWIC(factory.Get(), decoder.Get(), outPixels, outWidth, outHeight, outStride);
    }

    static bool DecodeFromMemory(
        const void* data,
        size_t size,
        std::vector<uint8_t>& outPixels,
        int& outWidth,
        int& outHeight,
        int& outStride)
    {
        ComPtr<IWICImagingFactory> factory;
        if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
            return false;

        ComPtr<IWICStream> stream;
        if (FAILED(factory->CreateStream(&stream)))
            return false;

        if (FAILED(stream->InitializeFromMemory(static_cast<BYTE*>(const_cast<void*>(data)), static_cast<DWORD>(size))))
            return false;

        ComPtr<IWICBitmapDecoder> decoder;
        if (FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, &decoder)))
            return false;

        return DecodeImageWIC(factory.Get(), decoder.Get(), outPixels, outWidth, outHeight, outStride);
    }

    //--------------------------------------------------------------------------------------
    // D3D12 texture upload
    //--------------------------------------------------------------------------------------

    bool Texture::UploadPixels(
        const void* pixels,
        int width,
        int height,
        int stride,
        ID3D12Device* device,
        ID3D12CommandQueue* cmdQueue,
        DescriptorHeapAllocator* heapAlloc)
    {
        // Create the texture resource (default heap)
        D3D12_HEAP_PROPERTIES defaultHeapProps = {};
        defaultHeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        texDesc.Width = static_cast<UINT64>(width);
        texDesc.Height = static_cast<UINT>(height);
        texDesc.DepthOrArraySize = 1;
        texDesc.MipLevels = 1;
        texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        texDesc.SampleDesc.Count = 1;
        texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        ComPtr<ID3D12Resource> texture;
        if (FAILED(device->CreateCommittedResource(
            &defaultHeapProps, D3D12_HEAP_FLAG_NONE,
            &texDesc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, IID_GRAPHICS_PPV_ARGS(texture.ReleaseAndGetAddressOf()))))
            return false;

        // Calculate upload buffer size
        UINT64 uploadBufferSize = 0;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint = {};
        UINT numRows = 0;
        UINT64 rowSizeInBytes = 0;
        device->GetCopyableFootprints(&texDesc, 0, 1, 0, &footprint, &numRows, &rowSizeInBytes, &uploadBufferSize);

        // Create upload buffer (upload heap)
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Width = uploadBufferSize;
        bufferDesc.Height = 1;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.MipLevels = 1;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        ComPtr<ID3D12Resource> uploadBuffer;
        if (FAILED(device->CreateCommittedResource(
            &uploadHeapProps, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr, IID_GRAPHICS_PPV_ARGS(uploadBuffer.ReleaseAndGetAddressOf()))))
            return false;

        // Map and copy pixel data into the upload buffer
        void* mapped = nullptr;
        D3D12_RANGE readRange = { 0, 0 };
        if (FAILED(uploadBuffer->Map(0, &readRange, &mapped)))
            return false;

        auto* dst = static_cast<uint8_t*>(mapped) + footprint.Offset;
        auto* src = static_cast<const uint8_t*>(pixels);
        for (UINT row = 0; row < numRows; row++)
        {
            memcpy(dst + row * footprint.Footprint.RowPitch,
                   src + row * stride,
                   static_cast<size_t>(rowSizeInBytes));
        }
        uploadBuffer->Unmap(0, nullptr);

        // Create a temporary command allocator and command list for the copy
        ComPtr<ID3D12CommandAllocator> cmdAlloc;
        if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(cmdAlloc.ReleaseAndGetAddressOf()))))
            return false;

        ComPtr<ID3D12GraphicsCommandList> cmdList;
        if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, cmdAlloc.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(cmdList.ReleaseAndGetAddressOf()))))
            return false;

        // Copy upload buffer to texture
        D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
        dstLoc.pResource = texture.Get();
        dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLoc.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
        srcLoc.pResource = uploadBuffer.Get();
        srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLoc.PlacedFootprint = footprint;

        cmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);

        // Transition to shader resource state
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = texture.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        cmdList->ResourceBarrier(1, &barrier);

        cmdList->Close();

        // Execute and wait
        ID3D12CommandList* ppCmdLists[] = { cmdList.Get() };
        cmdQueue->ExecuteCommandLists(1, ppCmdLists);

        ComPtr<ID3D12Fence> fence;
        if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.ReleaseAndGetAddressOf()))))
            return false;

        HANDLE fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!fenceEvent)
            return false;

        cmdQueue->Signal(fence.Get(), 1);
        fence->SetEventOnCompletion(1, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);

        // Create SRV
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
        heapAlloc->Alloc(&cpuHandle, &gpuHandle);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;

        device->CreateShaderResourceView(texture.Get(), &srvDesc, cpuHandle);

        // Store results
        m_texture = std::move(texture);
        m_srvCpuHandle = cpuHandle;
        m_srvGpuHandle = gpuHandle;
        m_heapAlloc = heapAlloc;
        m_width = width;
        m_height = height;
        m_loaded = true;

        return true;
    }

    //--------------------------------------------------------------------------------------
    // Move semantics
    //--------------------------------------------------------------------------------------

    Texture::Texture(Texture&& other) noexcept
        : m_texture(std::move(other.m_texture))
        , m_srvCpuHandle(other.m_srvCpuHandle)
        , m_srvGpuHandle(other.m_srvGpuHandle)
        , m_heapAlloc(other.m_heapAlloc)
        , m_width(other.m_width)
        , m_height(other.m_height)
        , m_loaded(other.m_loaded)
    {
        other.m_srvCpuHandle = {};
        other.m_srvGpuHandle = {};
        other.m_heapAlloc = nullptr;
        other.m_width = 0;
        other.m_height = 0;
        other.m_loaded = false;
    }

    Texture& Texture::operator=(Texture&& other) noexcept
    {
        if (this != &other)
        {
            Release();
            m_texture = std::move(other.m_texture);
            m_srvCpuHandle = other.m_srvCpuHandle;
            m_srvGpuHandle = other.m_srvGpuHandle;
            m_heapAlloc = other.m_heapAlloc;
            m_width = other.m_width;
            m_height = other.m_height;
            m_loaded = other.m_loaded;

            other.m_srvCpuHandle = {};
            other.m_srvGpuHandle = {};
            other.m_heapAlloc = nullptr;
            other.m_width = 0;
            other.m_height = 0;
            other.m_loaded = false;
        }
        return *this;
    }

    //--------------------------------------------------------------------------------------
    // Public API
    //--------------------------------------------------------------------------------------

    bool Texture::LoadFromFile(
        const wchar_t* path,
        ID3D12Device* device,
        ID3D12CommandQueue* cmdQueue,
        DescriptorHeapAllocator* heapAlloc)
    {
        Release();

        std::vector<uint8_t> pixels;
        int w = 0, h = 0, stride = 0;
        if (!DecodeFromFile(path, pixels, w, h, stride))
            return false;

        return UploadPixels(pixels.data(), w, h, stride, device, cmdQueue, heapAlloc);
    }

    bool Texture::LoadFromMemory(
        const void* data,
        size_t size,
        ID3D12Device* device,
        ID3D12CommandQueue* cmdQueue,
        DescriptorHeapAllocator* heapAlloc)
    {
        Release();

        std::vector<uint8_t> pixels;
        int w = 0, h = 0, stride = 0;
        if (!DecodeFromMemory(data, size, pixels, w, h, stride))
            return false;

        return UploadPixels(pixels.data(), w, h, stride, device, cmdQueue, heapAlloc);
    }

    bool Texture::CreateFromPixels(
        const void* pixels,
        int width,
        int height,
        int stride,
        ID3D12Device* device,
        ID3D12CommandQueue* cmdQueue,
        DescriptorHeapAllocator* heapAlloc)
    {
        Release();
        return UploadPixels(pixels, width, height, stride, device, cmdQueue, heapAlloc);
    }

    bool Texture::LoadFromFile(const wchar_t* path, DeviceContext& ctx)
    {
        return LoadFromFile(path, ctx.GetDevice(), ctx.GetCommandQueue(), ctx.GetSrvDescHeapAlloc());
    }

    bool Texture::LoadFromMemory(const void* data, size_t size, DeviceContext& ctx)
    {
        return LoadFromMemory(data, size, ctx.GetDevice(), ctx.GetCommandQueue(), ctx.GetSrvDescHeapAlloc());
    }

    bool Texture::CreateFromPixels(const void* pixels, int width, int height, int stride, DeviceContext& ctx)
    {
        return CreateFromPixels(pixels, width, height, stride, ctx.GetDevice(), ctx.GetCommandQueue(), ctx.GetSrvDescHeapAlloc());
    }

    void Texture::Release()
    {
        if (m_loaded && m_heapAlloc)
            m_heapAlloc->Free(m_srvCpuHandle, m_srvGpuHandle);

        m_texture.Reset();

        m_srvCpuHandle = {};
        m_srvGpuHandle = {};
        m_heapAlloc = nullptr;
        m_width = 0;
        m_height = 0;
        m_loaded = false;
    }
} // namespace ImGuiAtg
