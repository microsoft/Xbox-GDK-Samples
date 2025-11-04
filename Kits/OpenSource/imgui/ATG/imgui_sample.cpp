//--------------------------------------------------------------------------------------
// imgui_sample.cpp
//
// ImGui sample-specific code
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

// DirectX
#ifdef _GAMING_XBOX_SCARLETT
#include <d3d12_xs.h>
#include <d3dx12_xs.h>
#elif defined(_GAMING_XBOX)
#include <d3d12_x.h>
#include <d3dx12_x.h>
#else
#ifndef _GAMING_XBOX
#include "d3dx12.h"
#endif
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgi1_4.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#endif

#include "../imgui.h"
#include "../backends/imgui_impl_dx12.h"
#include "imgui_allocator.h"
#include "imgui_sample.h"

static constexpr ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
static constexpr float  ClearColorWithAlpha[4] = { ClearColor.x * ClearColor.w, ClearColor.y * ClearColor.w, ClearColor.z * ClearColor.w, ClearColor.w };
static constexpr int APP_NUM_BACK_BUFFERS = 2;

// D3D device is handled by modified ATG DeviceResources object
static std::unique_ptr<DX::DeviceResources> g_deviceResources;
static DescriptorHeapAllocator              g_pd3dSrvDescHeapAlloc;

bool CreateDeviceD3D(HWND hWnd, int width, int height)
{
    g_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_D32_FLOAT, APP_NUM_BACK_BUFFERS);
    g_deviceResources->SetWindow(hWnd, width, height);
    g_deviceResources->CreateDeviceResources();
    g_deviceResources->CreateWindowSizeDependentResources();

    g_pd3dSrvDescHeapAlloc.Create(g_deviceResources->GetD3DDevice(), g_deviceResources->GetSRVHeap());

    return true;
}

void CleanupDeviceD3D()
{
    g_deviceResources->WaitForGpu();
    g_deviceResources.reset();
    g_pd3dSrvDescHeapAlloc.Destroy();
}

void ImGui_Sample_DX12_Init()
{
    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = g_deviceResources->GetD3DDevice();
    init_info.CommandQueue = g_deviceResources->GetCommandQueue();
    init_info.NumFramesInFlight = APP_NUM_BACK_BUFFERS;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    init_info.SrvDescriptorHeap = g_deviceResources->GetSRVHeap();
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return g_pd3dSrvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle); };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)            { return g_pd3dSrvDescHeapAlloc.Free(cpu_handle, gpu_handle); };
    ImGui_ImplDX12_Init(&init_info);
}

void ImGui_Sample_DX12_PreRender()
{
#ifdef _GAMING_XBOX
    g_deviceResources->WaitForOrigin();
#endif

    g_deviceResources->Prepare();

    auto commandList = g_deviceResources->GetCommandList();

    // Clear the views.
    auto const rtvDescriptor = g_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = g_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ClearColorWithAlpha, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = g_deviceResources->GetScreenViewport();
    auto const scissorRect = g_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    commandList = g_deviceResources->GetCommandList();
    ID3D12DescriptorHeap* descriptorHeaps[]{ g_deviceResources->GetSRVHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
}

void ImGui_Sample_DX12_PostRender()
{
    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplDX12_RenderDrawData(drawData, g_deviceResources->GetCommandList());
    g_deviceResources->Present();
}

void ImGui_Sample_DX12_Resize(LPARAM lParam, WPARAM)
{
    // it's possible to get a WM_SIZE on the CreateWindowEx before this has been initialized
    if(g_deviceResources == nullptr)
        return;

    g_deviceResources->WindowSizeChanged(LOWORD(lParam), HIWORD(lParam));
}

void ImGui_Sample_DX12_Shutdown()
{
    g_deviceResources->WaitForGpu();
}
