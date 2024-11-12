#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

struct ID3D12Device;
struct ID3D12DescriptorHeap;
struct ID3D12GraphicsCommandList;
struct D3D12_CPU_DESCRIPTOR_HANDLE;
struct D3D12_GPU_DESCRIPTOR_HANDLE;

IMGUI_IMPL_API bool ImGui_ImplGdkDX12_Init(ID3D12Device* device, int num_frames_in_flight, int rtv_format, ID3D12DescriptorHeap* cbv_srv_heap,
    D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle);
IMGUI_IMPL_API void ImGui_ImplGdkDX12_Shutdown();
IMGUI_IMPL_API void ImGui_ImplGdkDX12_NewFrame(ID3D12GraphicsCommandList* cmdList);
IMGUI_IMPL_API void ImGui_ImplGdkDX12_RenderDrawData(ImDrawData* draw_data, ID3D12GraphicsCommandList* graphics_command_list);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API void ImGui_ImplGdkDX12_InvalidateDeviceObjects();
IMGUI_IMPL_API bool ImGui_ImplGdkDX12_CreateDeviceObjects(ID3D12GraphicsCommandList* cmdList);
