//--------------------------------------------------------------------------------------
// imgui_sample_dx12.h
//
// ImGui sample-specific code
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <d3d12.h>
#include "imgui_sample.h"
#include "../imgui.h"

struct FrameContext
{
    ID3D12CommandAllocator*     CommandAllocator;
    UINT64                      FenceValue;
};

// Forward declarations of helper functions
void ImGui_Sample_DX12_Init();
bool ImGui_Sample_DX12_PreRender();
void ImGui_Sample_DX12_PostRender();
void ImGui_Sample_DX12_Resize(LPARAM lParam, WPARAM wParam);

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();
