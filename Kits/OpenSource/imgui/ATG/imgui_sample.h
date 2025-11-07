//--------------------------------------------------------------------------------------
// imgui_sample.h
//
// ImGui sample-specific code
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "imgui_sample.h"
#include "../imgui.h"

// Forward declarations of helper functions
void ImGui_Sample_DX12_Init();
void ImGui_Sample_DX12_PreRender();
void ImGui_Sample_DX12_PostRender();
void ImGui_Sample_DX12_Resize(LPARAM lParam, WPARAM wParam);
void ImGui_Sample_DX12_Shutdown();

bool CreateDeviceD3D(HWND hWnd, int width, int height);
void CleanupDeviceD3D();
