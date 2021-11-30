//--------------------------------------------------------------------------------------
// FrontPanelRenderTarget.cpp
//
// Microsoft GDK with Xbox extensions (DirectX 12)
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "FrontPanelRenderTarget.h"
#include "DirectXHelpers.h"

#include <algorithm>

using Microsoft::WRL::ComPtr;

using namespace ATG;
using namespace DirectX;

FrontPanelRenderTarget::FrontPanelRenderTarget()
    : m_displayWidth(0),
    m_displayHeight(0),
    m_blitRenderTargetFormat(DXGI_FORMAT_UNKNOWN)
{
}

FrontPanelRenderTarget::~FrontPanelRenderTarget()
{
}

void FrontPanelRenderTarget::CreateDeviceDependentResources(_In_ ID3D12Device *device, unsigned int numRenderTargets)
{
    // Determine the render target size in pixels
    DX::ThrowIfFailed(XFrontPanelGetScreenDimensions(&m_displayHeight, &m_displayWidth));

    // Get the render target format for the Front Panel
    DX::ThrowIfFailed(XFrontPanelGetScreenPixelFormat(&m_blitRenderTargetFormat));

    // Assuming either double or triple buffered
    assert(numRenderTargets == 2 || numRenderTargets == 3);
    m_numBlitRenderTargets = numRenderTargets;

    // Create rtv descriptor heap
    {
        // Describe and create a render target view (RTV) heap
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = m_numBlitRenderTargets;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        DX::ThrowIfFailed(
            device->CreateDescriptorHeap(&rtvHeapDesc, IID_GRAPHICS_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));
    }

    m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create memory blt buffer
    m_buffer.reset(new uint8_t[m_displayWidth * m_displayHeight]);

    // Create staging blt buffer
    {
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_READBACK);

        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
            m_blitRenderTargetFormat,
            m_displayWidth,
            m_displayHeight,
            1, // This resource has only one texture.
            1  // Use a single mipmap level.
        );
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE swapChainOptimizedClearValue = {};
        swapChainOptimizedClearValue.Format = m_blitRenderTargetFormat;

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &desc,
                D3D12_RESOURCE_STATE_COPY_DEST,
                &swapChainOptimizedClearValue,
                IID_GRAPHICS_PPV_ARGS(m_staging.ReleaseAndGetAddressOf())
            )
        );

        m_staging->SetName(L"FrontPanel Staging");
    }

    // Create the render targets
    {
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
            m_blitRenderTargetFormat,
            m_displayWidth,
            m_displayHeight,
            1, // This resource has only one texture.
            1  // Use a single mipmap level.
        );
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE swapChainOptimizedClearValue = {};
        swapChainOptimizedClearValue.Format = m_blitRenderTargetFormat;

        for(unsigned int rtIndex = 0; rtIndex < m_numBlitRenderTargets; ++rtIndex)
        {
            DX::ThrowIfFailed(
                device->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &desc,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    &swapChainOptimizedClearValue,
                    IID_GRAPHICS_PPV_ARGS(m_blitRenderTargets[rtIndex].ReleaseAndGetAddressOf())
                )
            );

            static const WCHAR* Names[3] = {
                L"FrontPanel RT 1",
                L"FrontPanel RT 2",
                L"FrontPanel RT 3",
            };
            m_blitRenderTargets[rtIndex]->SetName(Names[rtIndex]);
        }
    }

    // Create the render target views
    {
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_blitRenderTargetFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        for(unsigned int rtIndex = 0; rtIndex < m_numBlitRenderTargets; ++rtIndex)
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
                m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(rtIndex), m_rtvDescriptorSize);
            device->CreateRenderTargetView(m_blitRenderTargets[rtIndex].Get(), &rtvDesc, rtvDescriptor);
        }
    }

    // Create the monochrome post-processor
    {
        RenderTargetState rtState(m_blitRenderTargetFormat, DXGI_FORMAT_UNKNOWN);

        m_panelBlt = std::make_unique<BasicPostProcess>(device, rtState, BasicPostProcess::Effect::Copy);
    }
}

void FrontPanelRenderTarget::CreateWindowSizeDependentResources(
    _In_ ID3D12Device *device, 
    _In_ unsigned int numRenderTargets,
    _In_reads_(numRenderTargets) ID3D12Resource*const* pRenderTargets)
{
    // Describe and create a shader resource view (SRV) heap
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = m_numBlitRenderTargets + numRenderTargets;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    DX::ThrowIfFailed(
        device->CreateDescriptorHeap(&srvHeapDesc, IID_GRAPHICS_PPV_ARGS(m_srvDescriptorHeap.ReleaseAndGetAddressOf())));

    // Create SRVs
    {
        for(unsigned int rtIndex = 0; rtIndex < m_numBlitRenderTargets; ++rtIndex)
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE blitSRVDescriptor(
                m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                static_cast<INT>(rtIndex), m_srvDescriptorSize);
            DirectX::CreateShaderResourceView(device, m_blitRenderTargets[rtIndex].Get(), blitSRVDescriptor, false);
        }

        for(unsigned int rtIndex = 0; rtIndex < numRenderTargets; ++rtIndex)
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtSRVDescriptor(
                m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), 
                static_cast<INT>(m_numBlitRenderTargets + rtIndex), m_srvDescriptorSize);
            DirectX::CreateShaderResourceView(device, pRenderTargets[rtIndex], rtSRVDescriptor, false);
        }
    }
}

void FrontPanelRenderTarget::Clear(_In_ ID3D12GraphicsCommandList * commandList, const float ColorRGBA[4], unsigned int rtIndex)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
        m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        static_cast<INT>(rtIndex), m_rtvDescriptorSize);
    commandList->ClearRenderTargetView(rtvDescriptor, ColorRGBA, 0, nullptr);
}

void FrontPanelRenderTarget::SetAsRenderTarget(_In_ ID3D12GraphicsCommandList * commandList, unsigned int rtIndex)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
        m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
        static_cast<INT>(rtIndex), m_rtvDescriptorSize);
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
}

void FrontPanelRenderTarget::GPUBlit(
    _In_ ID3D12GraphicsCommandList* commandList, 
    _In_ ID3D12Resource* renderTargetResource, 
    _In_ unsigned int renderTargetIndex)
{
    assert(m_panelBlt);

    SetAsRenderTarget(commandList, renderTargetIndex);

    // Set the viewports for the off-screen render target
    D3D12_VIEWPORT viewport = { 0.0f, 0.0f, float(m_displayWidth), float(m_displayHeight), 0.0f, 1.0f };
    commandList->RSSetViewports(1, &viewport);

    auto heap = m_srvDescriptorHeap.Get();
    commandList->SetDescriptorHeaps(1, &heap);

    // Scene render target needs transition to a shader resource before using in post process
    CD3DX12_RESOURCE_BARRIER rtToSRVTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTargetResource, 
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList->ResourceBarrier(1, &rtToSRVTransition);

    // Convert pixels from srcSRV to grayscale
    CD3DX12_GPU_DESCRIPTOR_HANDLE srcSRV(
        m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), 
        static_cast<INT>(m_numBlitRenderTargets + renderTargetIndex), m_srvDescriptorSize);
    m_panelBlt->SetSourceTexture(srcSRV, renderTargetResource);
    m_panelBlt->Process(commandList);

    // Transition back to render target when finished with it
    CD3DX12_RESOURCE_BARRIER srvToRTTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        renderTargetResource, 
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &srvToRTTransition);
}

void FrontPanelRenderTarget::CopyToBuffer(
    _In_ ID3D12Device* device,
    _In_ ID3D12CommandQueue* commandQueue,
    _In_ unsigned int renderTargetIndex, 
    _Inout_ BufferDesc &desc)
{
    // Create a new commandlist to upload to GPU immediately
    ComPtr<ID3D12CommandAllocator> commandAlloc;
    ComPtr<ID3D12GraphicsCommandList> commandList;
    {
        DX::ThrowIfFailed(
            device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(commandAlloc.ReleaseAndGetAddressOf())));
        SetDebugObjectName(commandAlloc.Get(), L"FrontPanelRenderTarget::CopyToBuffer");

        DX::ThrowIfFailed(
            device->CreateCommandList(1, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc.Get(), nullptr, IID_GRAPHICS_PPV_ARGS(commandList.ReleaseAndGetAddressOf())));
        SetDebugObjectName(commandList.Get(), L"FrontPanelRenderTarget::CopyToBuffer");
    }

    unsigned int prevRenderTargetIndex = renderTargetIndex == 0 ? m_numBlitRenderTargets - 1 : renderTargetIndex - 1;

    // Transition blit target to a copy source
    CD3DX12_RESOURCE_BARRIER rtToCopySourceTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_blitRenderTargets[prevRenderTargetIndex].Get(), 
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(1, &rtToCopySourceTransition);

    // Copy data to staging
    assert(m_staging && m_blitRenderTargets[prevRenderTargetIndex]);
    commandList->CopyResource(m_staging.Get(), m_blitRenderTargets[prevRenderTargetIndex].Get());

    // Transition blit target back to normal
    CD3DX12_RESOURCE_BARRIER copySourceToRTTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_blitRenderTargets[prevRenderTargetIndex].Get(), 
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &copySourceToRTTransition);

    // Close and submit command list to GPU
    DX::ThrowIfFailed(commandList->Close());
    commandQueue->ExecuteCommandLists(1, CommandListCast(commandList.GetAddressOf()));

    // Create GPU synchronization primitives
    ComPtr<ID3D12Fence> fence;
    HANDLE gpuCompletedEvent;
    {
        DX::ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(fence.GetAddressOf())));
        SetDebugObjectName(fence.Get(), L"FrontPanelRenderTarget::CopyToBuffer");

        gpuCompletedEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        if (!gpuCompletedEvent)
            throw std::exception("CreateEventEx");

        DX::ThrowIfFailed(commandQueue->Signal(fence.Get(), 1ULL));
        DX::ThrowIfFailed(fence->SetEventOnCompletion(1ULL, gpuCompletedEvent));
    }

    // Synchronize to GPU completion
    DWORD wr = WaitForSingleObject(gpuCompletedEvent, INFINITE);
    if (wr != WAIT_OBJECT_0)
    {
        if (wr == WAIT_FAILED)
        {
            DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }
        else
        {
            throw std::exception("WaitForSingleObject");
        }
    }
    
    // Finally map and read data into CPU buffer
    {
        void* pData = nullptr;
        m_staging->Map(0, nullptr, &pData);
        assert(pData);

        auto sptr = reinterpret_cast<uint8_t*>(pData);
        uint8_t *dptr = desc.data;

        memcpy_s(dptr, desc.size, sptr, m_displayWidth * m_displayHeight);

        m_staging->Unmap(0, nullptr);
    }

    CloseHandle(gpuCompletedEvent);
    commandList.Reset();
    commandAlloc.Reset();
}

void FrontPanelRenderTarget::PresentToFrontPanel(
    _In_ ID3D12Device* device, 
    _In_ ID3D12CommandQueue* commandQueue, 
    _In_ unsigned int renderTargetIndex)
{
    unsigned bufSize = m_displayWidth * m_displayHeight;

    BufferDesc desc;
    desc.data = m_buffer.get();
    desc.size = bufSize;
    desc.width = m_displayWidth;
    desc.height = m_displayHeight;

    CopyToBuffer(device, commandQueue, renderTargetIndex, desc);
    
    XFrontPanelPresentBuffer(bufSize, m_buffer.get());
}
