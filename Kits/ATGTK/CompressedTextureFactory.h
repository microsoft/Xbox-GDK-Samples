//--------------------------------------------------------------------------------------
// File: CompressedTextureFactory.h
//
// DirectX Tool Kit for DX12 IEffectTextureFactory that supports texture
// files compressed with the Microsoft SZDD/KWAJ-style compression tool for
// Windows & Xbox.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

#pragma once

#include "DDSTextureLoader.h"
#include "Effects.h"
#include "ResourceUploadBatch.h"
#include "WICTextureLoader.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "ReadCompressedData.h"

namespace DX
{
    class CompressedTextureFactory : public DirectX::EffectTextureFactory
    {
    public:
        CompressedTextureFactory(
            _In_ ID3D12Device* device,
            DirectX::ResourceUploadBatch& resourceUploadBatch,
            _In_ ID3D12DescriptorHeap* descriptorHeap) noexcept(false) :
            EffectTextureFactory(device, resourceUploadBatch, descriptorHeap),
            m_device(device),
            m_resourceUploadBatch(resourceUploadBatch),
            m_descriptorHeap(descriptorHeap),
            m_forceSRGB(false),
            m_autogen(false) {}

        CompressedTextureFactory(
            _In_ ID3D12Device* device,
            DirectX::ResourceUploadBatch& resourceUploadBatch,
            _In_ size_t numDescriptors,
            _In_ D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) noexcept(false) :
            EffectTextureFactory(device, resourceUploadBatch, numDescriptors, descriptorHeapFlags),
            m_device(device),
            m_resourceUploadBatch(resourceUploadBatch),
            m_descriptorHeap(Heap()),
            m_forceSRGB(false),
            m_autogen(false) {}

        CompressedTextureFactory(CompressedTextureFactory&&) = delete;
        CompressedTextureFactory& operator= (CompressedTextureFactory&&) = delete;

        CompressedTextureFactory(CompressedTextureFactory const&) = delete;
        CompressedTextureFactory& operator= (CompressedTextureFactory const&) = delete;

        size_t CreateTexture(_In_z_ const wchar_t* name, int descriptorIndex) override
        {
            // Sharing is not supported in this factory for compressed textures.

            wchar_t fullName[MAX_PATH] = {};
            wcscpy_s(fullName, m_path.c_str());
            wcscat_s(fullName, name);

            WIN32_FILE_ATTRIBUTE_DATA fileAttr = {};
            if (GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr))
            {
                return EffectTextureFactory::CreateTexture(name, descriptorIndex);
            }

            // Try CWD
            wcscpy_s(fullName, name);
            if (GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr))
            {
                return EffectTextureFactory::CreateTexture(name, descriptorIndex);
            }

            // See if a compressed version of the texture can be found.
            wchar_t ext[_MAX_EXT] = {};
            wchar_t fname[_MAX_FNAME] = {};
            _wsplitpath_s(name, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, ext, _MAX_EXT);

            wchar_t ext2[_MAX_EXT] = {};
            size_t len = wcslen(ext);
            if (len >= 3)
            {
                wcscpy_s(ext2, ext);
                ext2[len - 1] = L'_';
            }
            else
            {
                wcscpy_s(ext2, L"._");
            }

            wchar_t compressedName[MAX_PATH] = {};
            _wmakepath_s(compressedName, nullptr, nullptr, fname, ext2);

            wcscpy_s(fullName, m_path.c_str());
            wcscat_s(fullName, compressedName);

            if (!GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr))
            {
                wcscpy_s(fullName, compressedName);
                if (!GetFileAttributesExW(fullName, GetFileExInfoStandard, &fileAttr))
                {
                    throw std::runtime_error("CompressedTextureFactory::CreateTexture");
                }
            }

            auto blob = ReadCompressedData(fullName);

            const bool isdds = _wcsicmp(ext, L".dds") == 0;

            using namespace DirectX;

            TextureEntry textureEntry = {};

            DDS_LOADER_FLAGS loadFlags = DDS_LOADER_DEFAULT;
            if (m_forceSRGB)
                loadFlags |= DDS_LOADER_FORCE_SRGB;
            if (m_autogen)
                loadFlags |= DDS_LOADER_MIP_AUTOGEN;

            if (isdds)
            {
                HRESULT hr = CreateDDSTextureFromMemoryEx(
                    m_device.Get(),
                    m_resourceUploadBatch,
                    blob.data(),
                    blob.size(),
                    0u,
                    D3D12_RESOURCE_FLAG_NONE,
                    loadFlags,
                    textureEntry.mResource.ReleaseAndGetAddressOf(),
                    nullptr,
                    &textureEntry.mIsCubeMap);
                if (FAILED(hr))
                {
                    throw std::runtime_error("CompressedTextureFactory::CreateDDSTextureFromFile");
                }
            }
            else
            {
                static_assert(static_cast<int>(DDS_LOADER_DEFAULT) == static_cast<int>(WIC_LOADER_DEFAULT), "DDS/WIC Load flags mismatch");
                static_assert(static_cast<int>(DDS_LOADER_FORCE_SRGB) == static_cast<int>(WIC_LOADER_FORCE_SRGB), "DDS/WIC Load flags mismatch");
                static_assert(static_cast<int>(DDS_LOADER_MIP_AUTOGEN) == static_cast<int>(WIC_LOADER_MIP_AUTOGEN), "DDS/WIC Load flags mismatch");

                HRESULT hr = CreateWICTextureFromMemoryEx(
                    m_device.Get(),
                    m_resourceUploadBatch,
                    blob.data(),
                    blob.size(),
                    0u,
                    D3D12_RESOURCE_FLAG_NONE,
                    static_cast<WIC_LOADER_FLAGS>(loadFlags),
                    textureEntry.mResource.ReleaseAndGetAddressOf());
                if (FAILED(hr))
                {
                    throw std::runtime_error("CompressedTextureFactory::CreateDDSTextureFromFile");
                }
            }

            assert(textureEntry.mResource != nullptr);

            const size_t slot = EffectTextureFactory::ResourceCount() + m_resources.size();
            m_resources.push_back(textureEntry);

            const auto textureDescriptor = m_descriptorHeap.GetCpuHandle(static_cast<size_t>(descriptorIndex));
            DirectX::CreateShaderResourceView(m_device.Get(), textureEntry.mResource.Get(), textureDescriptor, textureEntry.mIsCubeMap);

            return slot;
        }

        size_t ResourceCount() const noexcept
        {
            return EffectTextureFactory::ResourceCount() + m_resources.size();
        }

        void GetResource(size_t slot, _Out_ ID3D12Resource** resource, _Out_opt_ bool* isCubeMap = nullptr)
        {
            const size_t high = EffectTextureFactory::ResourceCount();
            if (slot < high)
            {
                EffectTextureFactory::GetResource(slot, resource, isCubeMap);
            }

            slot -= high;

            if (slot >= m_resources.size())
                throw std::invalid_argument("Resource slot is invalid");

            const auto& textureEntry = m_resources[slot];

            textureEntry.mResource.CopyTo(resource);

            if (isCubeMap)
            {
                *isCubeMap = textureEntry.mIsCubeMap;
            }
        }

        void EnableForceSRGB(bool forceSRGB) noexcept
        {
            m_forceSRGB = forceSRGB;
            EffectTextureFactory::EnableForceSRGB(forceSRGB);
        }

        void EnableAutoGenMips(bool generateMips) noexcept
        {
            m_autogen = generateMips;
            EffectTextureFactory::EnableAutoGenMips(generateMips);
        }

        void SetDirectory(_In_opt_z_ const wchar_t* path) noexcept
        {
            EffectTextureFactory::SetDirectory(path);

            if (path && *path != 0)
            {
                m_path = path;
                const size_t len = m_path.length();
                if (len > 0)
                {
                    // Ensure it has a trailing slash
                    if (m_path[len - 1] != L'\\')
                    {
                        m_path += L'\\';
                    }
                }
            }
            else
                m_path.clear();
        }

    private:
        struct TextureEntry
        {
            Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
            bool mIsCubeMap;

            TextureEntry() noexcept : mIsCubeMap(false) {}
        };

        Microsoft::WRL::ComPtr<ID3D12Device> m_device;
        DirectX::ResourceUploadBatch& m_resourceUploadBatch;
        DirectX::DescriptorHeap m_descriptorHeap;

        bool m_forceSRGB;
        bool m_autogen;
        std::wstring m_path;

        std::vector<TextureEntry> m_resources;
    };
}
