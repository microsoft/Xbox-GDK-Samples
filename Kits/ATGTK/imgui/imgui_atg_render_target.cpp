//--------------------------------------------------------------------------------------
// imgui_atg_render_target.cpp
//
// Implementation of the offscreen 3D-to-ImGui rendering pipeline.
// See imgui_atg_render_target.h for usage documentation.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "imgui_atg_render_target.h"
#include "imgui.h"

#ifndef _GAMING_XBOX
#include "d3dx12.h"
#endif

namespace ImGuiAtg
{
    void RenderTarget::Initialize(DeviceContext* deviceContext, UINT width, UINT height,
                                  const float clearColor[4])
    {
        m_deviceContext = deviceContext;
        m_device = deviceContext->GetDevice();
        m_commandQueue = deviceContext->GetCommandQueue();
        m_width = width;
        m_height = height;

        if (clearColor)
        {
            m_clearColor[0] = clearColor[0];
            m_clearColor[1] = clearColor[1];
            m_clearColor[2] = clearColor[2];
            m_clearColor[3] = clearColor[3];
        }

        // --- Offscreen render target ---
        D3D12_RESOURCE_DESC rtDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1);
        rtDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

        D3D12_CLEAR_VALUE cv = {};
        cv.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        cv.Color[0] = m_clearColor[0];
        cv.Color[1] = m_clearColor[1];
        cv.Color[2] = m_clearColor[2];
        cv.Color[3] = m_clearColor[3];

        auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
            &rtDesc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, &cv,
            IID_GRAPHICS_PPV_ARGS(m_renderTarget.ReleaseAndGetAddressOf()));

        // --- RTV descriptor heap ---
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.NumDescriptors = 1;
        m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_GRAPHICS_PPV_ARGS(m_rtvHeap.ReleaseAndGetAddressOf()));
        m_device->CreateRenderTargetView(m_renderTarget.Get(), nullptr,
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // --- Depth buffer ---
        D3D12_RESOURCE_DESC dsDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_D32_FLOAT, width, height, 1, 1);
        dsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE dsClear = {};
        dsClear.Format = DXGI_FORMAT_D32_FLOAT;
        dsClear.DepthStencil.Depth = 1.0f;

        m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
            &dsDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &dsClear,
            IID_GRAPHICS_PPV_ARGS(m_depthStencil.ReleaseAndGetAddressOf()));

        // --- DSV descriptor heap ---
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.NumDescriptors = 1;
        m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_GRAPHICS_PPV_ARGS(m_dsvHeap.ReleaseAndGetAddressOf()));
        m_device->CreateDepthStencilView(m_depthStencil.Get(), nullptr,
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

        // --- SRV on ImGui's descriptor heap ---
        // This is critical: ImGui's DX12 backend requires all textures passed to
        // ImGui::Image() to have their SRV on ImGui's own shader-visible SRV heap.
        m_deviceContext->GetSrvDescHeapAlloc()->Alloc(&m_srvCpu, &m_srvGpu);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        m_device->CreateShaderResourceView(m_renderTarget.Get(), &srvDesc, m_srvCpu);

        // --- Command allocator + list ---
        m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_GRAPHICS_PPV_ARGS(m_commandAllocator.ReleaseAndGetAddressOf()));
        m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(), nullptr,
            IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf()));
        m_commandList->Close();

        // --- Fence for GPU synchronization ---
        m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
            IID_GRAPHICS_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()));
        m_fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        m_fenceValue = 0;

        m_initialized = true;
    }

    void RenderTarget::Begin(const float clearColor[4])
    {
        if (!m_initialized)
            return;

        if (!clearColor)
            clearColor = m_clearColor;

        // Wait for the GPU to finish the previous frame's render
        if (m_fence->GetCompletedValue() < m_fenceValue)
        {
            m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        // Reset command allocator and list
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        // Transition render target to RENDER_TARGET state
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTarget.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(1, &barrier);

        // Clear render target and depth buffer
        auto rtv = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
        m_commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        m_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // Set render target
        m_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

        // Set viewport and scissor
        D3D12_VIEWPORT viewport = { 0, 0, static_cast<float>(m_width),
            static_cast<float>(m_height), 0.0f, 1.0f };
        D3D12_RECT scissor = { 0, 0, static_cast<LONG>(m_width),
            static_cast<LONG>(m_height) };
        m_commandList->RSSetViewports(1, &viewport);
        m_commandList->RSSetScissorRects(1, &scissor);
    }

    void RenderTarget::End()
    {
        if (!m_initialized)
            return;

        // Transition render target back to PIXEL_SHADER_RESOURCE for ImGui sampling
        auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTarget.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_commandList->ResourceBarrier(1, &barrier);

        // Execute command list
        m_commandList->Close();
        ID3D12CommandList* lists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(1, lists);

        // Signal fence for next frame's wait
        m_fenceValue++;
        m_commandQueue->Signal(m_fence.Get(), m_fenceValue);
    }

    void RenderTarget::DrawImage(float width, float height) const
    {
        if (!m_initialized)
            return;

        ImGui::Image(static_cast<ImTextureID>(m_srvGpu.ptr), ImVec2(width, height));
    }
}
