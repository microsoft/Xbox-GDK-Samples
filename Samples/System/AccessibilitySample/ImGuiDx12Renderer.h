// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

// Include necessary headers for ImGui types
#include "imgui.h"

// Forward declarations of structures used in function parameters
struct ID3D12Device;
struct ID3D12DescriptorHeap;
struct ID3D12GraphicsCommandList;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

// Function declarations
bool ImGuiDx12RendererInit(
    ID3D12Device* device,
    int numFramesInFlight,
    int rtvFormat,
    ID3D12DescriptorHeap* cbvSrvHeap,
    D3D12_CPU_DESCRIPTOR_HANDLE fontSrvCpuDescHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE fontSrvGpuDescHandle);

void ImGuiDx12RendererShutdown();

void ImGuiDx12RendererNewFrame(ID3D12GraphicsCommandList* cmdList);

void ImGuiDx12RendererRenderDrawData(
    ImDrawData* drawData,
    ID3D12GraphicsCommandList* graphicsCommandList);

void ImGuiDx12RendererInvalidateDeviceObjects();

bool ImGuiDx12RendererCreateDeviceObjects(ID3D12GraphicsCommandList* cmdList);
