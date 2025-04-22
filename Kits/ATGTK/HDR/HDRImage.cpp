//--------------------------------------------------------------------------------------
// HDRImage.h
//
// Loads .HDR file and creates D3D resources for it
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HDRImage.h"
#include "DirectXTex.h"

using namespace DX;

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)

// Constructor
HDRImage::HDRImage()
{
    m_bFinishedLoading = false;
    m_maxLuminance = 0.0f;
    m_width = 0;
    m_height = 0;
    m_exposure = 0;
    m_srvCpuHandle.ptr = 0;
    m_srvGpuHandle.ptr = 0;
}

// Load image
void HDRImage::Load(const wchar_t* strPath, ID3D12Device* d3dDevice, ID3D12CommandQueue* d3dCommandQueue)
{
    using namespace DirectX;

    assert(strPath);
    assert(d3dDevice);
    assert(d3dCommandQueue);
    assert(m_srvCpuHandle.ptr != 0);
    assert(m_srvGpuHandle.ptr != 0);

    // Load the HDR image
    ScratchImage image;
    DX::ThrowIfFailed(LoadFromHDRFile(strPath, nullptr, image));

    // Calculate max luminance
    XMVECTOR maxLum = XMVectorZero();

    // Use paper white as 100 nits to load image. Could use higher nits for brighter image
    const float normalizeToPaperWhiteNits = 100.0;

    DX::ThrowIfFailed(EvaluateImage(*image.GetImage(0, 0, 0), [&](const XMVECTOR* pixels, size_t width, size_t y)
    {
        UNREFERENCED_PARAMETER(y);
        for (size_t j = 0; j < width; ++j)
        {
            static const XMVECTORF32 s_luminance = { { { 0.3f, 0.59f, 0.11f, 0.f } } };
            static const XMVECTORF32 s_nitsNormalize = { { { normalizeToPaperWhiteNits, normalizeToPaperWhiteNits, normalizeToPaperWhiteNits, 1.f } } };

            XMVECTOR v = *pixels++;

            v = XMVectorDivide(v, s_nitsNormalize);
            v = XMVectorSaturate(v);
            v = XMVector3Dot(v, s_luminance);

            maxLum = XMVectorMax(v, maxLum);
        }
    }));

    m_maxLuminance = XMVectorGetX(maxLum);
    m_maxLuminance *= normalizeToPaperWhiteNits;

    // Create fp16 texture data
    ScratchImage imagef16;
    DX::ThrowIfFailed(Convert(*image.GetImage(0, 0, 0), DXGI_FORMAT_R16G16B16A16_FLOAT, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, imagef16));
    auto imageData = imagef16.GetImages();

    m_width = static_cast<UINT>(imageData->width);
    m_height = static_cast<UINT>(imageData->height);

    // Create the D3D texture
    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(imageData->format, m_width, m_height, 1, 1);
    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    DX::ThrowIfFailed(d3dDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_GRAPHICS_PPV_ARGS(m_texture.ReleaseAndGetAddressOf())));
    m_texture->SetName(L"HDR Texture");

    // Copy the HDR image data to the D3D texture
    ResourceUploadBatch resourceUpload(d3dDevice);
    resourceUpload.Begin();
    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = imageData->pixels;
    textureData.RowPitch = LONG_PTR(imageData->rowPitch);
    textureData.SlicePitch = LONG_PTR(imageData->slicePitch);
    resourceUpload.Upload(m_texture.Get(), 0, &textureData, 1);
    resourceUpload.Transition(m_texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    auto uploadResourcesFinished = resourceUpload.End(d3dCommandQueue);
    uploadResourcesFinished.wait();     // Wait for resources to upload  

    // Create the SRV
    d3dDevice->CreateShaderResourceView(m_texture.Get(), nullptr, m_srvCpuHandle);

    m_bFinishedLoading = true;
}

#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)

// Constructor
HDRImage::HDRImage()
{
    m_bFinishedLoading = false;
    m_maxLuminance = 0.0f;
    m_width = 0;
    m_height = 0;
    m_exposure = 0;
}

// Load image
void HDRImage::Load(const wchar_t* strPath, ID3D11Device* d3dDevice)
{
    using namespace DirectX;

    assert(strPath);
    assert(d3dDevice);

    // Load image
    ScratchImage image;
    DX::ThrowIfFailed(LoadFromHDRFile(strPath, nullptr, image));

    // Calculate max luminance
    XMVECTOR maxLum = XMVectorZero();

    // Use paper white as 100 nits to load image. Could use higher nits for brighter image
    const float normalizeToPaperWhiteNits = 100.0;

    DX::ThrowIfFailed(EvaluateImage(*image.GetImage(0, 0, 0), [&](const XMVECTOR* pixels, size_t width, size_t y)
    {
        UNREFERENCED_PARAMETER(y);
        for (size_t j = 0; j < width; ++j)
        {
            static const XMVECTORF32 s_luminance = { 0.3f, 0.59f, 0.11f, 0.f };
            static const XMVECTORF32 s_nitsNormalize = { normalizeToPaperWhiteNits, normalizeToPaperWhiteNits, normalizeToPaperWhiteNits, 1.f };

            XMVECTOR v = *pixels++;

            v = XMVectorDivide(v, s_nitsNormalize);
            v = XMVectorSaturate(v);
            v = XMVector3Dot(v, s_luminance);

            maxLum = XMVectorMax(v, maxLum);
        }
    }));

    m_maxLuminance = XMVectorGetX(maxLum);
    m_maxLuminance *= normalizeToPaperWhiteNits;

    // Create fp16 texture
    ScratchImage imagef16;
    DX::ThrowIfFailed(Convert(*image.GetImage(0, 0, 0), DXGI_FORMAT_R16G16B16A16_FLOAT, TEX_FILTER_DEFAULT, TEX_THRESHOLD_DEFAULT, imagef16));

    auto imageData = imagef16.GetImages();
    m_width = static_cast<UINT>(imageData->width);
    m_height = static_cast<UINT>(imageData->height);

    DX::ThrowIfFailed(CreateShaderResourceView(d3dDevice, imageData, imagef16.GetImageCount(), imagef16.GetMetadata(), m_d3dSRV.ReleaseAndGetAddressOf()));

    m_bFinishedLoading = true;
}

#endif
