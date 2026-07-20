//--------------------------------------------------------------------------------------
// imgui_device_context.cpp
//
// D3D12 device context for ImGui integration (Desktop + Xbox)
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "imgui_atg_device_context.h"
#include "imgui_atg.h"
#include "backends/imgui_impl_dx12.h"

#ifndef _GAMING_XBOX
#include <dxgi1_6.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif
#endif

using namespace ImGuiAtg;

//--------------------------------------------------------------------------------------
// DeviceContext lifecycle
//--------------------------------------------------------------------------------------

DeviceContext::~DeviceContext()
{
    CleanupDevice();
}

bool DeviceContext::CreateDevice(HWND hWnd, int width, int height)
{
    m_window = hWnd;

#ifdef _GAMING_XBOX

    // Xbox device creation -- no DXGI
    D3D12XBOX_CREATE_DEVICE_PARAMETERS params = {};
    params.Version = D3D12_SDK_VERSION;

#if defined(_DEBUG)
    params.ProcessDebugFlags = D3D12_PROCESS_DEBUG_FLAG_DEBUG_LAYER_ENABLED;
#elif defined(PROFILE)
    params.ProcessDebugFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
#endif

    params.GraphicsCommandQueueRingSizeBytes = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
    params.DisableGeometryShaderAllocations = TRUE;
    params.DisableTessellationShaderAllocations = TRUE;
#ifdef _GAMING_XBOX_SCARLETT
    params.DisableDXR = TRUE;
#endif

    if (FAILED(D3D12XboxCreateDevice(nullptr, &params, IID_GRAPHICS_PPV_ARGS(&m_device))))
        return false;

    // Use the window size or default to 1920x1080
    if (width <= 0 || height <= 0)
    {
        width = 1920;
        height = 1080;
    }

#else // Desktop

#ifdef _DEBUG
    {
        ID3D12Debug* pdx12Debug = nullptr;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        {
            pdx12Debug->EnableDebugLayer();
            pdx12Debug->Release();
        }
    }
#endif

    if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device))))
        return false;

#ifdef _DEBUG
    {
        ID3D12InfoQueue* pInfoQueue = nullptr;
        if (SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&pInfoQueue))))
        {
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            pInfoQueue->Release();
        }
    }
#endif

    // Get window size if not provided
    if (width <= 0 || height <= 0)
    {
        RECT rc;
        GetClientRect(hWnd, &rc);
        width = rc.right - rc.left;
        height = rc.bottom - rc.top;
    }

#endif // Desktop

    m_width = width;
    m_height = height;

    // RTV descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_GRAPHICS_PPV_ARGS(&m_rtvDescHeap))))
            return false;

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            m_renderTargetDescriptors[i] = rtvHandle;
            rtvHandle.ptr += m_rtvDescriptorSize;
        }
    }

    // SRV descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = SRV_HEAP_SIZE;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (FAILED(m_device->CreateDescriptorHeap(&desc, IID_GRAPHICS_PPV_ARGS(&m_srvDescHeap))))
            return false;
        m_srvDescHeapAlloc.Create(m_device, m_srvDescHeap);
    }

    // Command queue
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (FAILED(m_device->CreateCommandQueue(&desc, IID_GRAPHICS_PPV_ARGS(&m_commandQueue))))
            return false;
    }

    // Command allocators
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(&m_commandAllocators[i]))))
            return false;
    }

    // Command list
    if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0], nullptr, IID_GRAPHICS_PPV_ARGS(&m_commandList))))
        return false;
    m_commandList->Close();

    // Fence
    if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(&m_fence))))
        return false;

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent)
        return false;

#ifdef _GAMING_XBOX
    m_fenceValue = 1;
    RegisterFrameEvents();
#else
    for (UINT n = 0; n < NUM_BACK_BUFFERS; n++)
        m_fenceValues[n] = 1;
#endif

#ifndef _GAMING_XBOX
    // Create swap chain (desktop only)
    {
        IDXGIFactory4* dxgiFactory = nullptr;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory))))
            return false;

        DXGI_SWAP_CHAIN_DESC1 sd = {};
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = static_cast<UINT>(m_width);
        sd.Height = static_cast<UINT>(m_height);
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;

        IDXGISwapChain1* swapChain1 = nullptr;
        if (FAILED(dxgiFactory->CreateSwapChainForHwnd(m_commandQueue, hWnd, &sd, nullptr, nullptr, &swapChain1)))
        {
            dxgiFactory->Release();
            return false;
        }

        swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapChain));
        swapChain1->Release();

        dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
        dxgiFactory->Release();

        m_swapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
    }
#endif

    CreateRenderTarget();

    // Set viewport and scissor
    m_screenViewport.TopLeftX = m_screenViewport.TopLeftY = 0.f;
    m_screenViewport.Width = static_cast<float>(m_width);
    m_screenViewport.Height = static_cast<float>(m_height);
    m_screenViewport.MinDepth = D3D12_MIN_DEPTH;
    m_screenViewport.MaxDepth = D3D12_MAX_DEPTH;

    m_scissorRect.left = m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(m_width);
    m_scissorRect.bottom = static_cast<LONG>(m_height);

    return true;
}

void DeviceContext::CleanupDevice()
{
    WaitForGpu();

#ifdef _GAMING_XBOX
    if (m_commandQueue)
    {
        m_commandQueue->PresentX(0, nullptr, nullptr);
    }
#endif

    CleanupRenderTarget();

#ifndef _GAMING_XBOX
    if (m_swapChain) { m_swapChain->SetFullscreenState(false, nullptr); m_swapChain->Release(); m_swapChain = nullptr; }
#endif

    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        if (m_commandAllocators[i]) { m_commandAllocators[i]->Release(); m_commandAllocators[i] = nullptr; }

    if (m_commandQueue) { m_commandQueue->Release(); m_commandQueue = nullptr; }
    if (m_commandList) { m_commandList->Release(); m_commandList = nullptr; }
    if (m_rtvDescHeap) { m_rtvDescHeap->Release(); m_rtvDescHeap = nullptr; }
    m_srvDescHeapAlloc.Destroy();
    if (m_srvDescHeap) { m_srvDescHeap->Release(); m_srvDescHeap = nullptr; }
    if (m_fence) { m_fence->Release(); m_fence = nullptr; }
    if (m_fenceEvent) { CloseHandle(m_fenceEvent); m_fenceEvent = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }

#if defined(_DEBUG) && !defined(_GAMING_XBOX)
    IDXGIDebug1* pDebug = nullptr;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

//--------------------------------------------------------------------------------------
// Render target management
//--------------------------------------------------------------------------------------

void DeviceContext::CreateRenderTarget()
{
#ifdef _GAMING_XBOX
    // Xbox: create committed resources (no swap chain)
    const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        static_cast<UINT64>(m_width),
        static_cast<UINT>(m_height),
        1, 1);
    bufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    const float clearColor[4] = { 0.45f, 0.55f, 0.60f, 1.0f };
    const CD3DX12_CLEAR_VALUE clearValue(DXGI_FORMAT_R8G8B8A8_UNORM, clearColor);

    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_ALLOW_DISPLAY,
            &bufferDesc,
            D3D12_RESOURCE_STATE_PRESENT,
            &clearValue,
            IID_GRAPHICS_PPV_ARGS(&m_renderTargets[i]));

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        m_device->CreateRenderTargetView(m_renderTargets[i], &rtvDesc, m_renderTargetDescriptors[i]);
    }

    m_backBufferIndex = 0;

#else
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = nullptr;
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        m_device->CreateRenderTargetView(pBackBuffer, nullptr, m_renderTargetDescriptors[i]);
        m_renderTargets[i] = pBackBuffer;
    }

    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
#endif
}

void DeviceContext::CleanupRenderTarget()
{
    WaitForGpu();
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        if (m_renderTargets[i]) { m_renderTargets[i]->Release(); m_renderTargets[i] = nullptr; }
    }
}

//--------------------------------------------------------------------------------------
// ImGui DX12 integration
//--------------------------------------------------------------------------------------

void DeviceContext::DX12_Init()
{
    ImGui_ImplDX12_InitInfo init_info = {};
    init_info.Device = m_device;
    init_info.CommandQueue = m_commandQueue;
    init_info.NumFramesInFlight = NUM_BACK_BUFFERS;
    init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
    init_info.SrvDescriptorHeap = m_srvDescHeap;
    init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
    {
        static_cast<DeviceContext*>(info->UserData)->m_srvDescHeapAlloc.Alloc(out_cpu_handle, out_gpu_handle);
    };
    init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
    {
        static_cast<DeviceContext*>(info->UserData)->m_srvDescHeapAlloc.Free(cpu_handle, gpu_handle);
    };
    init_info.UserData = this;
    ImGui_ImplDX12_Init(&init_info);
}

void DeviceContext::DX12_PreRender()
{
#ifdef _GAMING_XBOX
    // Wait for frame origin (Xbox frame pacing)
    m_framePipelineToken = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;
    m_device->WaitFrameEventX(
        D3D12XBOX_FRAME_EVENT_ORIGIN,
        INFINITE,
        nullptr,
        D3D12XBOX_WAIT_FRAME_EVENT_FLAG_NONE,
        &m_framePipelineToken);
#endif

    // Reset command list for this frame
    m_commandAllocators[m_backBufferIndex]->Reset();
    m_commandList->Reset(m_commandAllocators[m_backBufferIndex], nullptr);

    // Transition render target to RENDER_TARGET state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_renderTargets[m_backBufferIndex];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    m_commandList->ResourceBarrier(1, &barrier);

    // Clear render target
    const auto& clearColor = ImGuiAtg::GetClearColor();
    m_commandList->ClearRenderTargetView(m_renderTargetDescriptors[m_backBufferIndex], clearColor.data(), 0, nullptr);
    m_commandList->OMSetRenderTargets(1, &m_renderTargetDescriptors[m_backBufferIndex], FALSE, nullptr);

    // Set viewport and scissor
    m_commandList->RSSetViewports(1, &m_screenViewport);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);

    // Bind SRV heap
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescHeap };
    m_commandList->SetDescriptorHeaps(1, descriptorHeaps);
}

void DeviceContext::DX12_PostRender()
{
    // Render ImGui draw data
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_commandList);

    // Transition render target to PRESENT state
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = m_renderTargets[m_backBufferIndex];
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    m_commandList->ResourceBarrier(1, &barrier);

    m_commandList->Close();
    m_commandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&m_commandList);

#ifdef _GAMING_XBOX
    // Present via PresentX
    D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParams = {};
    planeParams.Token = m_framePipelineToken;
    planeParams.ResourceCount = 1;
    planeParams.ppResources = &m_renderTargets[m_backBufferIndex];

    m_commandQueue->PresentX(1, &planeParams, nullptr);

    // Advance frame
    m_backBufferIndex = (m_backBufferIndex + 1) % NUM_BACK_BUFFERS;

#else
    // Present via swap chain (vsync)
    HRESULT hr = m_swapChain->Present(1, 0);
    m_swapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);

    MoveToNextFrame();
#endif
}

void DeviceContext::DX12_Resize(LPARAM lParam, WPARAM wParam)
{
#ifndef _GAMING_XBOX
    if (m_device && wParam != SIZE_MINIMIZED)
    {
        int newWidth = LOWORD(lParam);
        int newHeight = HIWORD(lParam);
        if (newWidth == m_width && newHeight == m_height)
            return;

        m_width = newWidth;
        m_height = newHeight;

        CleanupRenderTarget();

        // CleanupRenderTarget waited for the GPU and advanced m_fenceValues[m_backBufferIndex].
        // After ResizeBuffers, GetCurrentBackBufferIndex resets (typically to 0), so the other
        // slots in m_fenceValues are stale and lower than the fence's current value. If we don't
        // sync them, the next MoveToNextFrame will signal a stale (lower) value, then wait on a
        // value that will never be reached -- hanging the app. Mirror what the standard
        // DeviceResources::CreateWindowSizeDependentResources does.
        for (UINT n = 0; n < NUM_BACK_BUFFERS; n++)
        {
            m_fenceValues[n] = m_fenceValues[m_backBufferIndex];
        }

        DXGI_SWAP_CHAIN_DESC1 desc = {};
        m_swapChain->GetDesc1(&desc);
        m_swapChain->ResizeBuffers(0, static_cast<UINT>(m_width), static_cast<UINT>(m_height), desc.Format, desc.Flags);
        CreateRenderTarget();

        // Update viewport/scissor
        m_screenViewport.Width = static_cast<float>(m_width);
        m_screenViewport.Height = static_cast<float>(m_height);
        m_scissorRect.right = static_cast<LONG>(m_width);
        m_scissorRect.bottom = static_cast<LONG>(m_height);
    }
#else
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
#endif
}

void DeviceContext::DX12_Shutdown()
{
    WaitForGpu();
}

//--------------------------------------------------------------------------------------
// GPU synchronization
//--------------------------------------------------------------------------------------

void DeviceContext::WaitForGpu()
{
    if (!m_commandQueue || !m_fence || !m_fenceEvent)
        return;

#ifdef _GAMING_XBOX
    UINT64 fenceValue = m_fenceValue;
    m_commandQueue->Signal(m_fence, fenceValue);
    m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
    WaitForSingleObject(m_fenceEvent, INFINITE);
    m_fenceValue++;
#else
    UINT64 fenceValue = m_fenceValues[m_backBufferIndex];
    m_commandQueue->Signal(m_fence, fenceValue);
    m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent);
    WaitForSingleObject(m_fenceEvent, INFINITE);
    m_fenceValues[m_backBufferIndex]++;
#endif
}

#ifdef _GAMING_XBOX

//--------------------------------------------------------------------------------------
// Xbox: Suspend/Resume + frame events
//--------------------------------------------------------------------------------------

void DeviceContext::Suspend()
{
    m_commandQueue->SuspendX(0);
}

void DeviceContext::Resume()
{
    m_commandQueue->ResumeX();
    RegisterFrameEvents();
}

void DeviceContext::RegisterFrameEvents()
{
    Microsoft::WRL::ComPtr<IDXGIDevice1> dxgiDevice;
    m_device->QueryInterface(IID_GRAPHICS_PPV_ARGS(dxgiDevice.GetAddressOf()));

    Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;
    dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());

    Microsoft::WRL::ComPtr<IDXGIOutput> dxgiOutput;
    dxgiAdapter->EnumOutputs(0, dxgiOutput.GetAddressOf());

    m_device->SetFrameIntervalX(
        dxgiOutput.Get(),
        D3D12XBOX_FRAME_INTERVAL_60_HZ,
        NUM_BACK_BUFFERS - 1u,
        D3D12XBOX_FRAME_INTERVAL_FLAG_NONE);

    m_device->ScheduleFrameEventX(
        D3D12XBOX_FRAME_EVENT_ORIGIN,
        0U,
        nullptr,
        D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE);
}

#else // Desktop

//--------------------------------------------------------------------------------------
// Desktop: frame advancement
//--------------------------------------------------------------------------------------

void DeviceContext::MoveToNextFrame()
{
    const UINT64 currentFenceValue = m_fenceValues[m_backBufferIndex];
    m_commandQueue->Signal(m_fence, currentFenceValue);

    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    if (m_fence->GetCompletedValue() < m_fenceValues[m_backBufferIndex])
    {
        m_fence->SetEventOnCompletion(m_fenceValues[m_backBufferIndex], m_fenceEvent);
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    m_fenceValues[m_backBufferIndex] = currentFenceValue + 1;
}

#endif
