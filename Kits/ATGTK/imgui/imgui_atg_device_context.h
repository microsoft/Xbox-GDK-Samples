//--------------------------------------------------------------------------------------
// imgui_device_context.h
//
// D3D12 device context for ImGui integration (Desktop + Xbox)
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "imgui.h"
#include <array>

#pragma warning(push)
#pragma warning(disable : 4265 4365)
#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#elif defined(_GAMING_XBOX)
#include <d3d12_x.h>
#include <d3dx12_x.h>
#else
#include <d3d12.h>
#endif
#pragma warning(pop)

// Xbox D3D12 interfaces derive from IGraphicsUnknown, not IUnknown.
// IID_GRAPHICS_PPV_ARGS handles both; on desktop it maps to IID_PPV_ARGS.
#ifndef IID_GRAPHICS_PPV_ARGS
#define IID_GRAPHICS_PPV_ARGS(x) IID_PPV_ARGS(x)
#endif

struct IDXGISwapChain3;

namespace ImGuiAtg
{
    // Simple free list based allocator
    struct DescriptorHeapAllocator
    {
        ID3D12DescriptorHeap*       Heap = nullptr;
        D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
        D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu {};
        D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu {};
        UINT                        HeapHandleIncrement = 0;
        ImVector<int>               FreeIndices;

        void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
        {
            IM_ASSERT(Heap == nullptr && FreeIndices.empty());
            Heap = heap;
            D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
            HeapType = desc.Type;
            HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
            HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
            HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
            FreeIndices.reserve((int)desc.NumDescriptors);
            for (UINT n = desc.NumDescriptors; n > 0; n--)
                FreeIndices.push_back((int)(n - 1));
        }
        void Destroy()
        {
            Heap = nullptr;
            FreeIndices.clear();
        }
        void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
        {
            IM_ASSERT(FreeIndices.Size > 0);
            int idx = FreeIndices.back();
            FreeIndices.pop_back();
            out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
            out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
        }
        void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
        {
            int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
            int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);
            IM_ASSERT(cpu_idx == gpu_idx); (void)gpu_idx;
            FreeIndices.push_back(cpu_idx);
        }
    };

    // Encapsulates all D3D12 device state and ImGui rendering integration
    class DeviceContext
    {
    public:
        DeviceContext() = default;
        ~DeviceContext();

        // Non-copyable
        DeviceContext(const DeviceContext&) = delete;
        DeviceContext& operator=(const DeviceContext&) = delete;

        // Lifecycle
        bool CreateDevice(HWND hWnd, int width = 0, int height = 0);
        void CleanupDevice();

        // ImGui DX12 integration
        void DX12_Init();
        void DX12_PreRender();
        void DX12_PostRender();
        void DX12_Resize(LPARAM lParam, WPARAM wParam);
        void DX12_Shutdown();

#ifdef _GAMING_XBOX
        void Suspend();
        void Resume();
#endif

        // Getters
        ID3D12Device*                   GetDevice() const { return m_device; }
        ID3D12CommandQueue*             GetCommandQueue() const { return m_commandQueue; }
        ID3D12GraphicsCommandList*      GetCommandList() const { return m_commandList; }
        ID3D12DescriptorHeap*           GetSrvDescHeap() const { return m_srvDescHeap; }
        DescriptorHeapAllocator*        GetSrvDescHeapAlloc() { return &m_srvDescHeapAlloc; }

    private:
        static constexpr int NUM_BACK_BUFFERS = 2;
        static constexpr int SRV_HEAP_SIZE = 64;

        void CreateRenderTarget();
        void CleanupRenderTarget();
        void WaitForGpu();

#ifdef _GAMING_XBOX
        void RegisterFrameEvents();
#else
        void MoveToNextFrame();
#endif

        // D3D12 state
        ID3D12Device*                   m_device = nullptr;
        ID3D12DescriptorHeap*           m_rtvDescHeap = nullptr;
        ID3D12DescriptorHeap*           m_srvDescHeap = nullptr;
        DescriptorHeapAllocator         m_srvDescHeapAlloc;
        ID3D12CommandQueue*             m_commandQueue = nullptr;
        ID3D12GraphicsCommandList*      m_commandList = nullptr;
        ID3D12CommandAllocator*         m_commandAllocators[NUM_BACK_BUFFERS] = {};
        ID3D12Fence*                    m_fence = nullptr;
        HANDLE                          m_fenceEvent = nullptr;
        UINT                            m_rtvDescriptorSize = 0;
        UINT                            m_backBufferIndex = 0;

        ID3D12Resource*                 m_renderTargets[NUM_BACK_BUFFERS] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE     m_renderTargetDescriptors[NUM_BACK_BUFFERS] = {};

        D3D12_VIEWPORT                  m_screenViewport = {};
        D3D12_RECT                      m_scissorRect = {};

        HWND                            m_window = nullptr;
        int                             m_width = 0;
        int                             m_height = 0;

#ifdef _GAMING_XBOX
        UINT64                          m_fenceValue = 0;
        D3D12XBOX_FRAME_PIPELINE_TOKEN  m_framePipelineToken = {};
#else
        UINT64                          m_fenceValues[NUM_BACK_BUFFERS] = {};
        IDXGISwapChain3*                m_swapChain = nullptr;
        bool                            m_swapChainOccluded = false;
#endif
    };
} // namespace ImGuiAtg
