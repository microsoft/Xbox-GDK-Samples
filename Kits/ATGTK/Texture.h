//--------------------------------------------------------------------------------------
// File: Texture.h
//
// Helper class for loading and managing texture resources
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include "DDSTextureLoader.h"
#include "ResourceUploadBatch.h"


namespace DX
{
    class Texture
    {
    public:
        Texture(_In_ ID3D12Device* device,
            DirectX::ResourceUploadBatch& resourceUpload,
            D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
            _In_z_ const wchar_t* fileName, bool forceSRGB = false) noexcept(false);

        Texture(_In_ ID3D12Device* device,
            DirectX::ResourceUploadBatch& resourceUpload,
            D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
            const D3D12_RESOURCE_DESC& texDesc,
            _In_ D3D12_SUBRESOURCE_DATA* data) noexcept(false);

        Texture(_In_ ID3D12Device* device,
            DirectX::ResourceUploadBatch& resourceUpload,
            D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
            _In_reads_bytes_(wicDataSize) const uint8_t* wicData,
            size_t wicDataSize) noexcept(false);

        Texture(_In_ ID3D12Device* device,
            DXGI_FORMAT format,
            int width,
            int height,
            D3D12_RESOURCE_FLAGS resFlags,
            D3D12_RESOURCE_STATES initialState,
            LPCWSTR name,
            CD3DX12_HEAP_PROPERTIES& heap,
            D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
            D3D12_CPU_DESCRIPTOR_HANDLE uavHandle,
            int arraySize = 1);

        Texture(Texture&&) = default;
        Texture& operator= (Texture&&) = default;

        Texture(Texture const&) = delete;
        Texture& operator= (Texture const&) = delete;

        int Width() const { return m_width; }
        int Height() const { return m_height; }
        int Depth() const { return m_depth; }
        int LevelCount() const { return m_mips; }
        int ArrayCount() const { return m_array; }
        DXGI_FORMAT Format() const { return m_format; }
        DirectX::DDS_ALPHA_MODE AlphaMode() const { return m_alphaMode; }

        bool IsCubeMap() const { return m_cubeMap; }
        DirectX::XMUINT2 GetTextureSize() const { return DirectX::XMUINT2(static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height)); }

        ID3D12Resource* GetResource() const { return m_resource.Get(); }

        static std::unique_ptr<Texture> CreateDefaultTexture(ID3D12Device* device,
            DirectX::ResourceUploadBatch& resourceUpload,
            D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor);

    private:
        void InitializeFormatFromResourceDesc(D3D12_RESOURCE_DESC desc);

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
        bool m_cubeMap;

        int                     m_width;
        int                     m_height;
        int                     m_depth;
        int                     m_mips;
        int                     m_array;
        DXGI_FORMAT             m_format;
        DirectX::DDS_ALPHA_MODE m_alphaMode;
    };
}
