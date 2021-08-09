//--------------------------------------------------------------------------------------
// File: Texture.cpp
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "Texture.h"

#include <exception>

#include "DirectXHelpers.h"
#include "WICTextureLoader.h"

using namespace DirectX;
using namespace DX;

using Microsoft::WRL::ComPtr;

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)

_Use_decl_annotations_
Texture::Texture(ID3D12Device* device,
    ResourceUploadBatch& resourceUpload,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
    const wchar_t* fileName,
    bool forceSRGB) :
    m_cubeMap(false),
    m_width(0),
    m_height(0),
    m_depth(0),
    m_mips(0),
    m_array(0),
    m_format(DXGI_FORMAT_UNKNOWN),
    m_alphaMode(DDS_ALPHA_MODE_UNKNOWN)
{
    assert(device != 0 && fileName != 0);

    WCHAR ext[_MAX_EXT];
    _wsplitpath_s(fileName, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);

    ComPtr<ID3D12Resource> resource;
    if (_wcsicmp(ext, L".dds") == 0)
    {
        ThrowIfFailed(
            CreateDDSTextureFromFileEx(device, resourceUpload, fileName, 0,
                D3D12_RESOURCE_FLAG_NONE, forceSRGB ? DDS_LOADER_FORCE_SRGB : DDS_LOADER_DEFAULT,
                resource.GetAddressOf(), &m_alphaMode, &m_cubeMap)
        );
    }
    else
    {
        ThrowIfFailed(
            CreateWICTextureFromFileEx(device, resourceUpload, fileName, 0,
                D3D12_RESOURCE_FLAG_NONE, forceSRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_DEFAULT,
                resource.GetAddressOf())
        );

        m_cubeMap = false;
        m_alphaMode = DDS_ALPHA_MODE_UNKNOWN;
    }

    InitializeFormatFromResourceDesc(resource->GetDesc());

    CreateShaderResourceView(device, resource.Get(), srvDescriptor);

    // Take ownership of resource
    m_resource.Swap(resource);
}

Texture::Texture(_In_ ID3D12Device* device,
    DirectX::ResourceUploadBatch& resourceUpload,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
    _In_reads_bytes_(wicDataSize) const uint8_t* wicData,
    size_t wicDataSize) noexcept(false)
{
    ComPtr<ID3D12Resource> resource;
    ThrowIfFailed(CreateWICTextureFromMemory(
        device,
        resourceUpload,
        wicData,
        wicDataSize,
        resource.GetAddressOf())
    );

    InitializeFormatFromResourceDesc(resource->GetDesc());

    CreateShaderResourceView(device, resource.Get(), srvDescriptor);

    // Take ownership of resource
    m_resource.Swap(resource);
}

Texture::Texture(_In_ ID3D12Device* device,
    DirectX::ResourceUploadBatch& resourceUpload,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor,
    const D3D12_RESOURCE_DESC& texDesc,
    _In_ D3D12_SUBRESOURCE_DATA* data) :
    m_cubeMap(false),
    m_width(int(texDesc.Width)),
    m_height(int(texDesc.Height)),
    m_mips(int(texDesc.MipLevels)),
    m_format(texDesc.Format),
    m_alphaMode(DDS_ALPHA_MODE_STRAIGHT)
{
    if (texDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        m_depth = texDesc.DepthOrArraySize;
        m_array = 1;
    }
    else
    {
        m_depth = 1;
        m_array = texDesc.DepthOrArraySize;
    }

    CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    DX::ThrowIfFailed(
        device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_resource.GetAddressOf())));

    resourceUpload.Upload(GetResource(), 0, data, 1);

    resourceUpload.Transition(
        GetResource(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    CreateShaderResourceView(device, m_resource.Get(), srvDescriptor);
}

std::unique_ptr<Texture> Texture::CreateDefaultTexture(ID3D12Device* device,
    ResourceUploadBatch& resourceUpload,
    D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor)
{
    // Create 1x1 white default texture
    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Width = 1;
    texDesc.Height = 1;
    texDesc.MipLevels = texDesc.DepthOrArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    uint32_t white = 0xFFFFFFFF;
    D3D12_SUBRESOURCE_DATA initData = { &white, sizeof(uint32_t), 0 };

    return std::unique_ptr<Texture>(new Texture(device, resourceUpload, srvDescriptor, texDesc, &initData));
}

void Texture::InitializeFormatFromResourceDesc(D3D12_RESOURCE_DESC desc)
{
    m_width = static_cast<int>(desc.Width);
    m_height = static_cast<int>(desc.Height);
    m_mips = desc.MipLevels;
    m_format = desc.Format;

    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
    {
        m_depth = desc.DepthOrArraySize;
        m_array = 1;
    }
    else
    {
        m_depth = 1;
        m_array = desc.DepthOrArraySize;
    }
}

#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)

_Use_decl_annotations_
Texture::Texture(ID3D11Device* device, const wchar_t* fileName, bool forceSRGB) :
    m_width(0),
    m_height(0),
    m_depth(0),
    m_mips(0),
    m_array(0),
    m_format(DXGI_FORMAT_UNKNOWN),
    m_alphaMode(DDS_ALPHA_MODE_UNKNOWN)
{
    assert(device != 0 && fileName != 0);

    WCHAR ext[_MAX_EXT];
    _wsplitpath_s(fileName, nullptr, 0, nullptr, 0, nullptr, 0, ext, _MAX_EXT);

    ComPtr<ID3D11Resource> resource;
    ComPtr<ID3D11ShaderResourceView> resourceView;
    if (_wcsicmp(ext, L".dds") == 0)
    {
        ThrowIfFailed(
            CreateDDSTextureFromFileEx( device, fileName, 0,
                D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                forceSRGB, 
                resource.GetAddressOf(), resourceView.GetAddressOf(), &m_alphaMode)
        );
    }
    else
    {
        ThrowIfFailed(
            CreateWICTextureFromFileEx(device, fileName, 0, 
                D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                forceSRGB ? WIC_LOADER_FORCE_SRGB : WIC_LOADER_DEFAULT, 
                resource.GetAddressOf(), resourceView.GetAddressOf())
        );
    }

    GetDesc(resource.Get());

    // Take ownership of resource
    m_resourceView.Swap(resourceView);
}

Texture::Texture(_In_ ID3D11ShaderResourceView* resourceView) :
    m_width(0),
    m_height(0),
    m_depth(0),
    m_mips(0),
    m_array(0),
    m_format(DXGI_FORMAT_UNKNOWN),
    m_alphaMode(DDS_ALPHA_MODE_UNKNOWN)
{
    assert(resourceView != 0);

    ComPtr<ID3D11Resource> resource;
    m_resourceView->GetResource(resource.GetAddressOf());

    GetDesc(resource.Get());

    // Take ownership of resource
    m_resourceView = resourceView;
}

void Texture::GetDesc(_In_ ID3D11Resource* resource)
{
    D3D11_RESOURCE_DIMENSION dim;
    resource->GetType(&dim);

    switch (dim)
    {
    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            ComPtr<ID3D11Texture1D> tex;
            if (SUCCEEDED(resource->QueryInterface(IID_GRAPHICS_PPV_ARGS(tex.GetAddressOf()))))
            {
                D3D11_TEXTURE1D_DESC desc;
                tex->GetDesc(&desc);

                m_width = static_cast<int>(desc.Width);
                m_height = 1;
                m_depth = 1;
                m_mips = static_cast<int>(desc.MipLevels);
                m_array = static_cast<int>(desc.ArraySize);
                m_format = desc.Format;
            }
        }
        break;

    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            ComPtr<ID3D11Texture2D> tex;
            if (SUCCEEDED(resource->QueryInterface(IID_GRAPHICS_PPV_ARGS(tex.GetAddressOf()))))
            {
                D3D11_TEXTURE2D_DESC desc;
                tex->GetDesc(&desc);

                m_width = static_cast<int>(desc.Width);
                m_height = static_cast<int>(desc.Height);
                m_depth = 1;
                m_mips = static_cast<int>(desc.MipLevels);
                m_array = static_cast<int>(desc.ArraySize);
                m_format = desc.Format;
            }
        }
        break;

    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            ComPtr<ID3D11Texture3D> tex;
            if (SUCCEEDED(resource->QueryInterface(IID_GRAPHICS_PPV_ARGS(tex.GetAddressOf()))))
            {
                D3D11_TEXTURE3D_DESC desc;
                tex->GetDesc(&desc);

                m_width = static_cast<int>(desc.Width);
                m_height = static_cast<int>(desc.Height);
                m_depth = static_cast<int>(desc.Depth);
                m_mips = static_cast<int>(desc.MipLevels);
                m_array = 1;
                m_format = desc.Format;
            }
        }
        break;

    default:
        throw std::exception("Unknown resource dimension");
    }
}

#endif
