//--------------------------------------------------------------------------------------
// imgui_atg_texture.h
//
// ImGuiAtg::Texture - load and display images in ImGui using WIC and D3D12
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "imgui.h"
#include "imgui_atg_device_context.h"
#ifndef _GAMING_XBOX
#include <d3d12.h>
#endif
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace ImGuiAtg
{
    class Texture
    {
    public:
        Texture() = default;
        ~Texture() { Release(); }

        // Non-copyable
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        // Movable
        Texture(Texture&& other) noexcept;
        Texture& operator=(Texture&& other) noexcept;

        // Load an image from a file using WIC
        bool LoadFromFile(
            const wchar_t* path,
            ID3D12Device* device,
            ID3D12CommandQueue* cmdQueue,
            DescriptorHeapAllocator* heapAlloc);

        // Load an encoded image (PNG, JPEG, BMP, etc.) from a memory buffer using WIC
        bool LoadFromMemory(
            const void* data,
            size_t size,
            ID3D12Device* device,
            ID3D12CommandQueue* cmdQueue,
            DescriptorHeapAllocator* heapAlloc);

        // Create a texture from raw decoded RGBA8 pixel data
        bool CreateFromPixels(
            const void* pixels,
            int width,
            int height,
            int stride,
            ID3D12Device* device,
            ID3D12CommandQueue* cmdQueue,
            DescriptorHeapAllocator* heapAlloc);

        // Convenience overloads accepting a DeviceContext
        bool LoadFromFile(const wchar_t* path, DeviceContext& ctx);
        bool LoadFromMemory(const void* data, size_t size, DeviceContext& ctx);
        bool CreateFromPixels(const void* pixels, int width, int height, int stride, DeviceContext& ctx);

        // Release D3D12 resources and free the SRV descriptor
        void Release();

        // Implicit conversion to ImTextureRef for use with ImGui::Image() etc.
        operator ImTextureRef() const { return ImTextureRef(GetTextureID()); }

        // Returns native image size as ImVec2, convenient for ImGui::Image() calls
        ImVec2 GetSize() const { return ImVec2(static_cast<float>(m_width), static_cast<float>(m_height)); }

        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }
        bool IsLoaded() const { return m_loaded; }
        ImTextureID GetTextureID() const { return static_cast<ImTextureID>(m_srvGpuHandle.ptr); }

    private:
        bool UploadPixels(
            const void* pixels,
            int width,
            int height,
            int stride,
            ID3D12Device* device,
            ID3D12CommandQueue* cmdQueue,
            DescriptorHeapAllocator* heapAlloc);

        ComPtr<ID3D12Resource>          m_texture;
        D3D12_CPU_DESCRIPTOR_HANDLE     m_srvCpuHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE     m_srvGpuHandle = {};
        DescriptorHeapAllocator*        m_heapAlloc = nullptr;
        int                             m_width = 0;
        int                             m_height = 0;
        bool                            m_loaded = false;
    };
} // namespace ImGuiAtg
