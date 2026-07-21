//--------------------------------------------------------------------------------------
// imgui_atg_render_target.h
//
// Renders 3D content to an offscreen texture and displays it as an ImGui::Image.
//
// This class encapsulates the D3D12 resources needed to render 3D content into an
// ImGui window: an offscreen render target, depth buffer, command allocator/list,
// fence for GPU synchronization, and an SRV allocated from ImGui's descriptor heap.
//
// Usage:
//   // During initialization:
//   ImGuiAtg::RenderTarget renderer;
//   renderer.Initialize(deviceContext, 512, 512);
//
//   // Each frame, before ImGui rendering:
//   renderer.Begin();
//   // ... issue D3D12 draw calls to renderer.GetCommandList() ...
//   renderer.End();
//
//   // During ImGui drawing:
//   renderer.DrawImage(width, height);  // or use GetTextureID() directly
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "imgui_atg_device_context.h"
#ifndef _GAMING_XBOX
#include <d3d12.h>
#endif
#include <wrl/client.h>

namespace ImGuiAtg
{
    class RenderTarget
    {
    public:
        RenderTarget() = default;
        ~RenderTarget() = default;

        RenderTarget(RenderTarget&&) = delete;
        RenderTarget& operator=(RenderTarget&&) = delete;
        RenderTarget(const RenderTarget&) = delete;
        RenderTarget& operator=(const RenderTarget&) = delete;

        // Initialize the offscreen render target, depth buffer, and command list.
        // Allocates an SRV on ImGui's descriptor heap for display.
        // clearColor sets the optimized clear value (default: dark grey).
        void Initialize(DeviceContext* deviceContext, UINT width, UINT height,
                        const float clearColor[4] = nullptr);

        // Begin a new frame - waits for GPU, resets command list, transitions RT,
        // clears render target and depth buffer, sets viewport/scissor.
        // After calling Begin(), issue draw commands to GetCommandList().
        void Begin(const float clearColor[4] = nullptr);

        // End the frame - transitions RT back for ImGui sampling, executes commands,
        // signals fence.
        void End();

        // Display the rendered texture as an ImGui::Image widget.
        void DrawImage(float width, float height) const;

        // Accessors for custom rendering
        ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList.Get(); }
        ID3D12Device*              GetDevice() const { return m_device; }
        ID3D12CommandQueue*        GetCommandQueue() const { return m_commandQueue; }
        ImTextureID                GetTextureID() const { return static_cast<ImTextureID>(m_srvGpu.ptr); }
        UINT                       GetWidth() const { return m_width; }
        UINT                       GetHeight() const { return m_height; }
        DXGI_FORMAT                GetRTFormat() const { return DXGI_FORMAT_R8G8B8A8_UNORM; }
        DXGI_FORMAT                GetDSFormat() const { return DXGI_FORMAT_D32_FLOAT; }
        bool                       IsInitialized() const { return m_initialized; }

    private:
        DeviceContext* m_deviceContext = nullptr;
        ID3D12Device* m_device = nullptr;
        ID3D12CommandQueue* m_commandQueue = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_renderTarget;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
        Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
        HANDLE m_fenceEvent = nullptr;
        UINT64 m_fenceValue = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpu = {};
        D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpu = {};

        UINT m_width = 0;
        UINT m_height = 0;
        float m_clearColor[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
        bool m_initialized = false;
    };
}
