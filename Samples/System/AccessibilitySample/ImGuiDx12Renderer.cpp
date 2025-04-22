// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ImGuiDx12Renderer.h"

// Include ImGui implementation headers
#ifdef _GAMING_XBOX
#include "ImGuiImplGdkDx12.h"
#else
#include "imgui_impl_dx12.h"
#endif

#ifdef _GAMING_XBOX
bool ImGuiDx12RendererInit(
    ID3D12Device* device,
    int numFramesInFlight,
    int rtvFormat,
    ID3D12DescriptorHeap* cbvSrvHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuDescHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuDescHandle)
{
    return ImGui_ImplGdkDX12_Init(
        device,
        numFramesInFlight,
        rtvFormat,
        cbvSrvHeap,
        fontSrvCpuDescHandle,
        fontSrvGpuDescHandle);
}

void ImGuiDx12RendererShutdown()
{
    ImGui_ImplGdkDX12_Shutdown();
}

void ImGuiDx12RendererNewFrame(ID3D12GraphicsCommandList* cmdList)
{
    ImGui_ImplGdkDX12_NewFrame(cmdList);
}

void ImGuiDx12RendererRenderDrawData(
    ImDrawData* drawData,
    ID3D12GraphicsCommandList* graphicsCommandList)
{
    ImGui_ImplGdkDX12_RenderDrawData(drawData, graphicsCommandList);
}

void ImGuiDx12RendererInvalidateDeviceObjects()
{
    ImGui_ImplGdkDX12_InvalidateDeviceObjects();
}

bool ImGuiDx12RendererCreateDeviceObjects(ID3D12GraphicsCommandList* cmdList)
{
    return ImGui_ImplGdkDX12_CreateDeviceObjects(cmdList);
}

#else
bool ImGuiDx12RendererInit(
    ID3D12Device* device,
    int numFramesInFlight,
    int rtvFormat,
    ID3D12DescriptorHeap* cbvSrvHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuDescHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuDescHandle)
{
    return ImGui_ImplDX12_Init(
        device,
        numFramesInFlight,
        static_cast<DXGI_FORMAT>(rtvFormat),
        cbvSrvHeap,
        fontSrvCpuDescHandle,
        fontSrvGpuDescHandle);
}

void ImGuiDx12RendererShutdown()
{
    ImGui_ImplDX12_Shutdown();
}

void ImGuiDx12RendererNewFrame(ID3D12GraphicsCommandList* /*cmdList*/)
{
    ImGui_ImplDX12_NewFrame();
}

void ImGuiDx12RendererRenderDrawData(
    ImDrawData* drawData,
    ID3D12GraphicsCommandList* graphicsCommandList)
{
    ImGui_ImplDX12_RenderDrawData(drawData, graphicsCommandList);
}

void ImGuiDx12RendererInvalidateDeviceObjects()
{
    ImGui_ImplDX12_InvalidateDeviceObjects();
}

bool ImGuiDx12RendererCreateDeviceObjects(ID3D12GraphicsCommandList* /*cmdList*/)
{
    return ImGui_ImplDX12_CreateDeviceObjects();
}

#endif // _GAMING_XBOX
