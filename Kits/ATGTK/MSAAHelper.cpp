//--------------------------------------------------------------------------------------
// File: MSAAHelper.cpp
//
// Helper for managing MSAA render targets
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#include "pch.h"
#include "MSAAHelper.h"

#include "DirectXHelpers.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <utility>

#include <wrl/client.h>

using namespace DirectX;
using namespace DX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

using Microsoft::WRL::ComPtr;

namespace
{
    inline DXGI_FORMAT MakeDepthTypeless(DXGI_FORMAT fmt) noexcept
    {
        switch (fmt)
        {
        case DXGI_FORMAT_D32_FLOAT:             return DXGI_FORMAT_R32_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:             return DXGI_FORMAT_R16_TYPELESS;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:     return DXGI_FORMAT_R24G8_TYPELESS;
        default:                                return fmt;
        }
    }
}

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)
//======================================================================================
// Direct3D 12
//======================================================================================

MSAAHelper::MSAAHelper(DXGI_FORMAT backBufferFormat,
    DXGI_FORMAT depthBufferFormat,
    unsigned int sampleCount) noexcept(false) :
        m_clearColor{},
        m_rtvDescriptorHeap{},
        m_dsvDescriptorHeap{},
#if defined(_XBOX_ONE) && defined(_TITLE) && (_XDK_VER < 0x3F6803F3 /* XDK Edition 170600 */)
        m_resolveFormat(DXGI_FORMAT_UNKNOWN),
#endif
        m_backBufferFormat(backBufferFormat),
        m_depthBufferFormat(depthBufferFormat),
        m_sampleCount(0),
        m_targetSampleCount(sampleCount),
        m_width(0),
        m_height(0)
{
    if (sampleCount < 2 || sampleCount > D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT)
    {
        throw std::out_of_range("MSAA sample count invalid.");
    }
}


void MSAAHelper::SetDevice(_In_ ID3D12Device* device)
{
    if (device == m_device.Get())
        return;

    if (m_device)
    {
        ReleaseDevice();
    }

    {
        D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = { m_backBufferFormat, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport))))
        {
            throw std::exception();
        }

        UINT required = D3D12_FORMAT_SUPPORT1_RENDER_TARGET | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET;
        if ((formatSupport.Support1 & required) != required)
        {
#ifdef _DEBUG
            char buff[128] = {};
            sprintf_s(buff, "MSAAHelper: Device does not support MSAA for requested backbuffer format (%u)!\n", m_backBufferFormat);
            OutputDebugStringA(buff);
#endif
            throw std::exception();
        }
    }

    {
        D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = { m_depthBufferFormat, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE };
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport))))
        {
            throw std::exception();
        }

        UINT required = D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL | D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RENDERTARGET;
        if ((formatSupport.Support1 & required) != required)
        {
#ifdef _DEBUG
            char buff[128] = {};
            sprintf_s(buff, "MSAAHelper: Device does not support MSAA for requested depth/stencil format (%u)!\n", m_depthBufferFormat);
            OutputDebugStringA(buff);
#endif
            throw std::exception();
        }
    }

    for (m_sampleCount = m_targetSampleCount; m_sampleCount > 1; m_sampleCount--)
    {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels = { m_backBufferFormat, m_sampleCount, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE, 0u };
        if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels, sizeof(levels))))
            continue;

        if (levels.NumQualityLevels > 0)
            break;
    }

    if (m_sampleCount < 2)
    {
#ifdef _DEBUG
        OutputDebugStringA("MSAAHelper: Device does not support MSAA!\n");
#endif
        throw std::exception();
    }

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = 1;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ThrowIfFailed(device->CreateDescriptorHeap(&rtvDescriptorHeapDesc,
        IID_GRAPHICS_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));

    m_rtvDescriptorHeap->SetName(L"MSAAHelper");

    if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        ThrowIfFailed(device->CreateDescriptorHeap(&dsvDescriptorHeapDesc,
            IID_GRAPHICS_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf())));

        m_dsvDescriptorHeap->SetName(L"MSAAHelper");
    }

    m_device = device;
}


void MSAAHelper::SizeResources(size_t width, size_t height)
{
    if (width == m_width && height == m_height)
        return;

    if (m_width > UINT32_MAX || m_height > UINT32_MAX)
    {
        throw std::out_of_range("Invalid width/height");
    }

    if (!m_device)
        return;

    m_width = m_height = 0;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    DXGI_FORMAT msaaFormat = m_backBufferFormat;

#if defined(_XBOX_ONE) && defined(_TITLE) && (_XDK_VER < 0x3F6803F3 /* XDK Edition 170600 */)

    switch (m_backBufferFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        msaaFormat = DXGI_FORMAT_R8G8B8A8_TYPELESS;
        m_resolveFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        msaaFormat = DXGI_FORMAT_B8G8R8A8_TYPELESS;
        m_resolveFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;

    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        msaaFormat = DXGI_FORMAT_B8G8R8X8_TYPELESS;
        m_resolveFormat = DXGI_FORMAT_B8G8R8X8_UNORM;
        break;

    default:
        m_resolveFormat = m_backBufferFormat;
        break;
    }

#endif

    // Create an MSAA render target
    D3D12_RESOURCE_DESC msaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        msaaFormat,
        static_cast<UINT64>(width),
        static_cast<UINT>(height),
        1, // This render target view has only one texture.
        1, // Use a single mipmap level
        m_sampleCount
    );
    msaaRTDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_CLEAR_VALUE msaaOptimizedClearValue = {};
    msaaOptimizedClearValue.Format = m_backBufferFormat;
    memcpy(msaaOptimizedClearValue.Color, m_clearColor, sizeof(float) * 4);

    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &msaaRTDesc,
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
        &msaaOptimizedClearValue,
        IID_GRAPHICS_PPV_ARGS(m_msaaRenderTarget.ReleaseAndGetAddressOf())
    ));

    m_msaaRenderTarget->SetName( L"MSAA Render Target" );

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = m_backBufferFormat;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;

    m_device->CreateRenderTargetView(
        m_msaaRenderTarget.Get(), &rtvDesc,
        m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        // Create an MSAA depth stencil view
        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            MakeDepthTypeless(m_depthBufferFormat),
            static_cast<UINT64>(width),
            static_cast<UINT>(height),
            1, // This depth stencil view has only one texture.
            1, // Use a single mipmap level.
            m_sampleCount
        );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = m_depthBufferFormat;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        ThrowIfFailed(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_msaaDepthStencil.ReleaseAndGetAddressOf())
        ));

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = m_depthBufferFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;

        m_device->CreateDepthStencilView(
            m_msaaDepthStencil.Get(), &dsvDesc,
            m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    }

    m_width = width;
    m_height = height;
}


void MSAAHelper::ReleaseDevice()
{
    m_rtvDescriptorHeap.Reset();
    m_dsvDescriptorHeap.Reset();

    m_msaaRenderTarget.Reset();
    m_msaaDepthStencil.Reset();

    m_device.Reset();

    m_width = m_height = 0;
}


void MSAAHelper::Prepare(_In_ ID3D12GraphicsCommandList* commandList,
    D3D12_RESOURCE_STATES beforeState)
{
    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_msaaRenderTarget.Get(),
        beforeState,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);
}


void MSAAHelper::Resolve(_In_ ID3D12GraphicsCommandList* commandList,
    _In_ ID3D12Resource* backBuffer,
    D3D12_RESOURCE_STATES beforeState,
    D3D12_RESOURCE_STATES afterState)
{
    D3D12_RESOURCE_BARRIER barriers[2] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(m_msaaRenderTarget.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(backBuffer,
            beforeState,
            D3D12_RESOURCE_STATE_RESOLVE_DEST)
    };

    commandList->ResourceBarrier((beforeState != D3D12_RESOURCE_STATE_RESOLVE_DEST) ? 2u : 1u, barriers);

#if defined(_XBOX_ONE) && defined(_TITLE) && (_XDK_VER < 0x3F6803F3 /* XDK Edition 170600 */)
    commandList->ResolveSubresource(backBuffer, 0, m_msaaRenderTarget.Get(), 0, m_resolveFormat);
#else
    commandList->ResolveSubresource(backBuffer, 0, m_msaaRenderTarget.Get(), 0, m_backBufferFormat);
#endif

    if (afterState != D3D12_RESOURCE_STATE_RESOLVE_DEST)
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer,
            D3D12_RESOURCE_STATE_RESOLVE_DEST,
            afterState);
        commandList->ResourceBarrier(1, &barrier);
    }
}

void MSAAHelper::ResolveDepth(_In_ ID3D12GraphicsCommandList* commandList,
    _In_ ID3D12Resource* depthBuffer,
    D3D12_RESOURCE_STATES beforeState,
    D3D12_RESOURCE_STATES afterState)
{
    D3D12_RESOURCE_BARRIER barriers[2] =
    {
        CD3DX12_RESOURCE_BARRIER::Transition(m_msaaDepthStencil.Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(depthBuffer,
            beforeState,
            D3D12_RESOURCE_STATE_RESOLVE_DEST)
    };

    commandList->ResourceBarrier((beforeState != D3D12_RESOURCE_STATE_RESOLVE_DEST) ? 2u : 1u, barriers);

    auto desc = depthBuffer->GetDesc();
    commandList->ResolveSubresource(depthBuffer, 0, m_msaaDepthStencil.Get(), 0, desc.Format);

    std::swap(barriers[0].Transition.StateBefore, barriers[0].Transition.StateAfter);

    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST;
    barriers[1].Transition.StateAfter = afterState;

    commandList->ResourceBarrier((afterState != D3D12_RESOURCE_STATE_RESOLVE_DEST) ? 2u : 1u, barriers);
}


#else
//======================================================================================
// Direct3D 11
//======================================================================================

MSAAHelper::MSAAHelper(DXGI_FORMAT backBufferFormat,
    DXGI_FORMAT depthBufferFormat,
    unsigned int sampleCount) noexcept(false) :
#if defined(_XBOX_ONE) && defined(_TITLE)
        m_fastSemantics(false),
#endif
        m_backBufferFormat(backBufferFormat),
        m_depthBufferFormat(depthBufferFormat),
        m_sampleCount(0),
        m_targetSampleCount(sampleCount),
        m_width(0),
        m_height(0)
{
    if (sampleCount < 2 || sampleCount > D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT)
    {
        throw std::out_of_range("MSAA sample count invalid.");
    }
}


void MSAAHelper::SetDevice(_In_ ID3D11Device* device)
{
    if (device == m_device.Get())
        return;

    if (m_device)
    {
        ReleaseDevice();
    }

    {
        UINT formatSupport = 0;
        if (FAILED(device->CheckFormatSupport(m_backBufferFormat, &formatSupport)))
        {
            throw std::exception();
        }

        UINT32 required = D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET;
        if ((formatSupport & required) != required)
        {
#ifdef _DEBUG
            char buff[128] = {};
            sprintf_s(buff, "MSAAHelper: Device does not support MSAA for requested backbuffer format (%u)!\n", m_backBufferFormat);
            OutputDebugStringA(buff);
#endif
            throw std::exception();
        }
    }

    {
        UINT formatSupport = 0;
        if (FAILED(device->CheckFormatSupport(m_depthBufferFormat, &formatSupport)))
        {
            throw std::exception();
        }

        UINT32 required = D3D11_FORMAT_SUPPORT_DEPTH_STENCIL | D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET;
        if ((formatSupport & required) != required)
        {
#ifdef _DEBUG
            char buff[128] = {};
            sprintf_s(buff, "MSAAHelper: Device does not support MSAA for requested depth/stencil format (%u)!\n", m_depthBufferFormat);
            OutputDebugStringA(buff);
#endif
            throw std::exception();
        }
    }

    for (m_sampleCount = m_targetSampleCount; m_sampleCount > 1; m_sampleCount--)
    {
        UINT levels = 0;
        if (FAILED(device->CheckMultisampleQualityLevels(m_backBufferFormat, m_sampleCount, &levels)))
            continue;

        if ( levels > 0)
            break;
    }

    if (m_sampleCount < 2)
    {
#ifdef _DEBUG
        OutputDebugStringA("MSAAHelper: Device does not support MSAA!\n");
#endif
        throw std::exception();
    }

    m_device = device;

#if defined(_XBOX_ONE) && defined(_TITLE)
    m_fastSemantics = (device->GetCreationFlags() & D3D11_CREATE_DEVICE_IMMEDIATE_CONTEXT_FAST_SEMANTICS) != 0;
#endif
}


void MSAAHelper::SizeResources(size_t width, size_t height)
{
    if (width == m_width && height == m_height)
        return;

    if (m_width > UINT32_MAX || m_height > UINT32_MAX)
    {
        throw std::out_of_range("Invalid width/height");
    }

    if (!m_device)
        return;

    m_width = m_height = 0;

    // Create an MSAA render target
    CD3D11_TEXTURE2D_DESC renderTargetDesc(
        m_backBufferFormat,
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1, // The render target view has only one texture.
        1, // Use a single mipmap level.
        D3D11_BIND_RENDER_TARGET,
        D3D11_USAGE_DEFAULT,
        0,
        m_sampleCount
    );

    ThrowIfFailed(m_device->CreateTexture2D(
        &renderTargetDesc,
        nullptr,
        m_msaaRenderTarget.ReleaseAndGetAddressOf()
    ));

    SetDebugObjectName(m_msaaRenderTarget.Get(), "MSAA Render Target");


    CD3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc(D3D11_RTV_DIMENSION_TEXTURE2DMS, m_backBufferFormat);

    ThrowIfFailed(m_device->CreateRenderTargetView(
        m_msaaRenderTarget.Get(),
        &renderTargetViewDesc,
        m_renderTargetView.ReleaseAndGetAddressOf()
    ));

    SetDebugObjectName(m_renderTargetView.Get(), "MSAA Render Target");

    if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        // Create an MSAA depth stencil view
        CD3D11_TEXTURE2D_DESC depthStencilDesc(
            MakeDepthTypeless(m_depthBufferFormat),
            static_cast<UINT>(width),
            static_cast<UINT>(height),
            1, // This depth stencil view has only one texture.
            1, // Use a single mipmap level.
            D3D11_BIND_DEPTH_STENCIL,
            D3D11_USAGE_DEFAULT,
            0,
            m_sampleCount
        );

        ThrowIfFailed(m_device->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            m_msaaDepthStencil.ReleaseAndGetAddressOf()
        ));

        SetDebugObjectName(m_msaaDepthStencil.Get(), "MSAA Depth/Stencil");

        CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(D3D11_DSV_DIMENSION_TEXTURE2DMS, m_depthBufferFormat);

        ThrowIfFailed(m_device->CreateDepthStencilView(
            m_msaaDepthStencil.Get(),
            &dsvDesc,
            m_depthStencilView.ReleaseAndGetAddressOf()
        ));

        SetDebugObjectName(m_depthStencilView.Get(), "MSAA Depth/Stencil");
    }

    m_width = width;
    m_height = height;
}


void MSAAHelper::ReleaseDevice()
{
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_msaaDepthStencil.Reset();
    m_msaaRenderTarget.Reset();

    m_device.Reset();

    m_width = m_height = 0;
}


#if defined(_XBOX_ONE) && defined(_TITLE)
void MSAAHelper::Resolve(_In_ ID3D11DeviceContextX* context, _In_ ID3D11Texture2D* backBuffer)
#else
void MSAAHelper::Resolve(_In_ ID3D11DeviceContext* context, _In_ ID3D11Texture2D* backBuffer)
#endif
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    if (m_fastSemantics)
    {
        context->DecompressResource(
            m_msaaRenderTarget.Get(), 0, nullptr,
            m_msaaRenderTarget.Get(), 0, nullptr,
            m_backBufferFormat, D3D11X_DECOMPRESS_ALL);
    }
#endif

    context->ResolveSubresource(backBuffer, 0, m_msaaRenderTarget.Get(), 0, m_backBufferFormat);
}


#if defined(_XBOX_ONE) && defined(_TITLE)
void MSAAHelper::ResolveDepth(_In_ ID3D11DeviceContextX* context, _In_ ID3D11Texture2D* depthBuffer)
#else
void MSAAHelper::ResolveDepth(_In_ ID3D11DeviceContext* context, _In_ ID3D11Texture2D* depthBuffer)
#endif
{
#if defined(_XBOX_ONE) && defined(_TITLE)
    if (m_fastSemantics)
    {
        context->DecompressResource(
            m_msaaDepthStencil.Get(), 0, nullptr,
            m_msaaDepthStencil.Get(), 0, nullptr,
            m_depthBufferFormat, D3D11X_DECOMPRESS_ALL);
    }
#endif

    context->ResolveSubresource(depthBuffer, 0, m_msaaDepthStencil.Get(), 0, m_depthBufferFormat);
}
#endif


void MSAAHelper::SetWindow(const RECT& output)
{
    // Determine the render target size in pixels.
    auto width = size_t(std::max<LONG>(output.right - output.left, 1));
    auto height = size_t(std::max<LONG>(output.bottom - output.top, 1));

    SizeResources(width, height);
}
