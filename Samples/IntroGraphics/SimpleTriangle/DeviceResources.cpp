//
// DeviceResources.cpp - A wrapper for the Direct3D 12.X device and swapchain
//

#include "pch.h"
#include "DeviceResources.h"

#include <XSystem.h>

using namespace DirectX;
using namespace DX;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

using Microsoft::WRL::ComPtr;

// Constructor for DeviceResources.
DeviceResources::DeviceResources(
    DXGI_FORMAT backBufferFormat,
    DXGI_FORMAT depthBufferFormat,
    UINT backBufferCount,
    unsigned int flags) noexcept(false) :
        m_backBufferIndex(0),
        m_fenceValue(0),
        m_framePipelineToken{},
        m_rtvDescriptorSize(0),
        m_screenViewport{},
        m_scissorRect{},
        m_backBufferFormat(backBufferFormat),
        m_depthBufferFormat(depthBufferFormat),
        m_backBufferCount(backBufferCount),
        m_clearColor{},
        m_window(nullptr),
        m_d3dFeatureLevel(D3D_FEATURE_LEVEL_12_0),
        m_outputSize{0, 0, 1920, 1080},
        m_options(flags)
{
    if (backBufferCount < 2 || backBufferCount > MAX_BACK_BUFFER_COUNT)
    {
        throw std::out_of_range("invalid backBufferCount");
    }
}

// Destructor for DeviceResources.
DeviceResources::~DeviceResources()
{
    // Ensure that the GPU is no longer referencing resources that are about to be destroyed.
    WaitForGpu();

    // Ensure we present a blank screen before cleaning up resources.
    if (m_commandQueue)
    {
        std::ignore = m_commandQueue->PresentX(0, nullptr, nullptr);
    }
}

// Configures the Direct3D device, and stores handles to it and the device context.
#ifdef _GAMING_XBOX_SCARLETT
void DeviceResources::CreateDeviceResources(D3D12XBOX_CREATE_DEVICE_FLAGS createDeviceFlags)
#else
void DeviceResources::CreateDeviceResources()
#endif
{
    // Create the DX12 API device object.
    D3D12XBOX_CREATE_DEVICE_PARAMETERS params = {};
    params.Version = D3D12_SDK_VERSION;

#if defined(_DEBUG)
    // Enable the debug layer.
    params.ProcessDebugFlags = D3D12_PROCESS_DEBUG_FLAG_DEBUG_LAYER_ENABLED;
#elif defined(PROFILE)
    // Enable the instrumented driver.
    params.ProcessDebugFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
#endif

    params.GraphicsCommandQueueRingSizeBytes = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
    params.DisableGeometryShaderAllocations = (m_options & c_GeometryShaders) ? FALSE : TRUE;
    params.DisableTessellationShaderAllocations = (m_options & c_TessellationShaders) ? FALSE : TRUE;

#ifdef _GAMING_XBOX_SCARLETT
    params.DisableDXR = (m_options & c_EnableDXR) ? FALSE : TRUE;
    params.CreateDeviceFlags = createDeviceFlags;

#if (_GXDK_VER >= 0x585D070E /* GXDK Edition 221000 */)
    if (m_options & c_AmplificationShaders)
    {
        params.AmplificationShaderIndirectArgsBufferSize = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
        params.AmplificationShaderPayloadBufferSize = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
    }
#endif
#else // _GAMING_XBOX_XBOXONE
    m_options &= ~(c_AmplificationShaders | c_EnableDXR | c_ColorDcc | c_DepthTcc);
#endif

    HRESULT hr = D3D12XboxCreateDevice(
        nullptr,
        &params,
        IID_GRAPHICS_PPV_ARGS(m_d3dDevice.ReleaseAndGetAddressOf()));
#ifdef _DEBUG
    if (hr == D3D12_ERROR_DRIVER_VERSION_MISMATCH)
    {
#ifdef _GAMING_XBOX_SCARLETT
        OutputDebugStringA("ERROR: Running a d3d12_xs.lib (Xbox Series X|S) linked binary on an Xbox One is not supported\n");
#else
        OutputDebugStringA("ERROR: Running a d3d12_x.lib (Xbox One) linked binary on a Xbox Series X|S in 'Scarlett' mode is not supported\n");
#endif
    }
#endif
    ThrowIfFailed(hr);

    m_d3dDevice->SetName(L"DeviceResources");

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_GRAPHICS_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf())));

    m_commandQueue->SetName(L"DeviceResources");

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = m_backBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_GRAPHICS_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));

    m_rtvDescriptorHeap->SetName(L"DeviceResources");

    m_rtvDescriptorSize = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
        dsvDescriptorHeapDesc.NumDescriptors = 1;
        dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;

        ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_GRAPHICS_PPV_ARGS(m_dsvDescriptorHeap.ReleaseAndGetAddressOf())));

        m_dsvDescriptorHeap->SetName(L"DeviceResources");
    }

    // Create a command allocator for each back buffer that will be rendered to.
    for (UINT n = 0; n < m_backBufferCount; n++)
    {
        ThrowIfFailed(m_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocators[n].ReleaseAndGetAddressOf())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        m_commandAllocators[n]->SetName(name);
    }

    // Create a command list for recording graphics commands.
    ThrowIfFailed(m_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[0].Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandList.ReleaseAndGetAddressOf())));
    ThrowIfFailed(m_commandList->Close());

    m_commandList->SetName(L"DeviceResources");

    // Create a fence for tracking GPU execution progress.
    ThrowIfFailed(m_d3dDevice->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
    m_fenceValue++;

    m_fence->SetName(L"DeviceResources");

    m_fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!m_fenceEvent.IsValid())
    {
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
    }

    if (m_options & (c_Enable4K_UHD | c_EnableQHD))
    {
        switch (XSystemGetDeviceType())
        {
        case XSystemDeviceType::XboxOne:
        case XSystemDeviceType::XboxOneS:
            m_options &= ~(c_Enable4K_UHD | c_EnableQHD);
            break;

        case XSystemDeviceType::XboxScarlettLockhart /* Xbox Series S */:
            m_options &= ~c_Enable4K_UHD;
            if (m_options & c_EnableQHD)
            {
                m_outputSize = { 0, 0, 2560, 1440 };
            }
            break;

        case XSystemDeviceType::XboxScarlettAnaconda /* Xbox Series X */:
        case XSystemDeviceType::XboxOneXDevkit:
        case XSystemDeviceType::XboxScarlettDevkit:
        default:
            m_outputSize = (m_options & c_Enable4K_UHD) ? RECT{ 0, 0, 3840, 2160 } : RECT{ 0, 0, 2560, 1440 };
            break;
        }
    }

#ifdef _DEBUG
    const char* info = nullptr;
    switch (m_outputSize.bottom)
    {
    case 2160:    info = "INFO: Swapchain using 4k (3840 x 2160)\n"; break;
    case 1440:    info = "INFO: Swapchain using 1440p (2560 x 1440)\n"; break;
    default:      info = "INFO: Swapchain using 1080p (1920 x 1080)\n"; break;
    }
    OutputDebugStringA(info);
#endif

    RegisterFrameEvents();
}

// These resources need to be recreated every time the window size is changed.
void DeviceResources::CreateWindowSizeDependentResources()
{
    if (!m_window)
    {
        throw std::logic_error("Call SetWindow with a valid Win32 window handle");
    }

    // Wait until all previous GPU work is complete.
    WaitForGpu();

    // Ensure we present a blank screen before cleaning up resources.
    ThrowIfFailed(m_commandQueue->PresentX(0, nullptr, nullptr));

    // Release resources that are tied to the swap chain and update fence values.
    for (UINT n = 0; n < m_backBufferCount; n++)
    {
        m_renderTargets[n].Reset();
    }

    // Determine the render target size in pixels.
    const UINT backBufferWidth = std::max<UINT>(static_cast<UINT>(m_outputSize.right - m_outputSize.left), 1u);
    const UINT backBufferHeight = std::max<UINT>(static_cast<UINT>(m_outputSize.bottom - m_outputSize.top), 1u);

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    const CD3DX12_HEAP_PROPERTIES swapChainHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC swapChainBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        m_backBufferFormat,
        backBufferWidth,
        backBufferHeight,
        1, // This resource has only one texture.
        1  // Use a single mipmap level.
    );
    swapChainBufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

#ifdef _GAMING_XBOX_SCARLETT
    if (m_options & c_ColorDcc)
    {
        // Enable DCC (Delta Color Compression) for the swap buffer (requires a clear).
        swapChainBufferDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC;
    }
#endif

#ifdef _GAMING_XBOX_XBOXONE
    if (m_options & c_EnableHDR)
    {
        swapChainBufferDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP;
    }
#endif

    const CD3DX12_CLEAR_VALUE swapChainOptimizedClearValue(m_backBufferFormat, m_clearColor);

    for (UINT n = 0; n < m_backBufferCount; n++)
    {
        ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
            &swapChainHeapProperties,
            D3D12_HEAP_FLAG_ALLOW_DISPLAY,
            &swapChainBufferDesc,
            D3D12_RESOURCE_STATE_PRESENT,
            &swapChainOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_renderTargets[n].GetAddressOf())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        m_renderTargets[n]->SetName(name);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_backBufferFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        const auto cpuHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(cpuHandle, static_cast<INT>(n), m_rtvDescriptorSize);
        m_d3dDevice->CreateRenderTargetView(m_renderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    // Reset the index to the current back buffer.
    m_backBufferIndex = 0;

    if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
        // on this surface.
        const CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            m_depthBufferFormat,
            backBufferWidth,
            backBufferHeight,
            1, // Use a single array entry.
            1  // Use a single mipmap level.
            );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

#ifdef _GAMING_XBOX_SCARLETT
        if (m_options & c_DepthTcc)
        {
            // Enable Texture Compatibility for the depth buffer (avoids a decompress).
            depthStencilDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
        }
#endif

        const CD3DX12_CLEAR_VALUE depthOptimizedClearValue(m_depthBufferFormat, (m_options & c_ReverseDepth) ? 0.0f : 1.0f, 0u);

        ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
            &depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_depthStencil.ReleaseAndGetAddressOf())
            ));

        m_depthStencil->SetName(L"Depth stencil");

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = m_depthBufferFormat;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        const auto cpuHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_d3dDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, cpuHandle);
    }

    // Set the 3D rendering viewport and scissor rectangle to target the entire window.
    m_screenViewport.TopLeftX = m_screenViewport.TopLeftY = 0.f;
    m_screenViewport.Width = static_cast<float>(backBufferWidth);
    m_screenViewport.Height = static_cast<float>(backBufferHeight);
    m_screenViewport.MinDepth = D3D12_MIN_DEPTH;
    m_screenViewport.MaxDepth = D3D12_MAX_DEPTH;

    m_scissorRect.left = m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(backBufferWidth);
    m_scissorRect.bottom = static_cast<LONG>(backBufferHeight);
}

// Prepare the command list and render target for rendering.
void DeviceResources::Prepare(D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    // Reset command list and allocator.
    ThrowIfFailed(m_commandAllocators[m_backBufferIndex]->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_backBufferIndex].Get(), nullptr));

    if (beforeState != afterState)
    {
        // Transition the render target into the correct state to allow for drawing into it.
        const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[m_backBufferIndex].Get(),
            beforeState, afterState);
        m_commandList->ResourceBarrier(1, &barrier);
    }
}

// Present the contents of the swap chain to the screen.
void DeviceResources::Present(D3D12_RESOURCE_STATES beforeState, _In_opt_ const D3D12XBOX_PRESENT_PARAMETERS* params)
{
    if (beforeState != D3D12_RESOURCE_STATE_PRESENT)
    {
        // Transition the render target to the state that allows it to be presented to the display.
        const D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[m_backBufferIndex].Get(),
            beforeState, D3D12_RESOURCE_STATE_PRESENT);
        m_commandList->ResourceBarrier(1, &barrier);
    }

    // Send the command list off to the GPU for processing.
    ThrowIfFailed(m_commandList->Close());
    m_commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

    // Present the backbuffer using the PresentX API.
    D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters = {};
    planeParameters.Token = m_framePipelineToken;
    planeParameters.ResourceCount = 1;
    planeParameters.ppResources = m_renderTargets[m_backBufferIndex].GetAddressOf();

    if (m_options & c_EnableHDR)
    {
        planeParameters.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
    }

    ThrowIfFailed(
        m_commandQueue->PresentX(1, &planeParameters, params)
    );

    // Xbox apps do not need to handle DXGI_ERROR_DEVICE_REMOVED or DXGI_ERROR_DEVICE_RESET.

    // Update the back buffer index.
    m_backBufferIndex = (m_backBufferIndex + 1) % m_backBufferCount;
}

// Handle GPU suspend/resume
void DeviceResources::Suspend()
{
    m_commandQueue->SuspendX(0);
}

void DeviceResources::Resume()
{
    m_commandQueue->ResumeX();

    RegisterFrameEvents();
}

// Wait for pending GPU work to complete.
void DeviceResources::WaitForGpu() noexcept
{
    if (m_commandQueue && m_fence && m_fenceEvent.IsValid())
    {
        // Schedule a Signal command in the GPU queue.
        const UINT64 fenceValue = m_fenceValue;
        if (SUCCEEDED(m_commandQueue->Signal(m_fence.Get(), fenceValue)))
        {
            // Wait until the Signal has been processed.
            if (SUCCEEDED(m_fence->SetEventOnCompletion(fenceValue, m_fenceEvent.Get())))
            {
                std::ignore = WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);

                // Increment the fence value for the current frame.
                m_fenceValue++;
            }
        }
    }
}

// For PresentX rendering, we should wait for the origin event just before processing input.
void DeviceResources::WaitForOrigin()
{
    // Wait until frame start is signaled
    m_framePipelineToken = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;
    ThrowIfFailed(m_d3dDevice->WaitFrameEventX(
        D3D12XBOX_FRAME_EVENT_ORIGIN,
        INFINITE,
        nullptr,
        D3D12XBOX_WAIT_FRAME_EVENT_FLAG_NONE,
        &m_framePipelineToken));
}

// Set frame interval and register for frame events
void DeviceResources::RegisterFrameEvents()
{
    // First, retrieve the underlying DXGI device from the D3D device.
    ComPtr<IDXGIDevice1> dxgiDevice;
    ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

    // Identify the physical adapter (GPU or card) this device is running on.
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

    // Retrieve the outputs for the adapter.
    ComPtr<IDXGIOutput> dxgiOutput;
    ThrowIfFailed(dxgiAdapter->EnumOutputs(0, dxgiOutput.GetAddressOf()));

    // Set frame interval and register for frame events
    ThrowIfFailed(m_d3dDevice->SetFrameIntervalX(
        dxgiOutput.Get(),
        D3D12XBOX_FRAME_INTERVAL_60_HZ,
        m_backBufferCount - 1u /* Allow n-1 frames of latency */,
        D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));

    ThrowIfFailed(m_d3dDevice->ScheduleFrameEventX(
        D3D12XBOX_FRAME_EVENT_ORIGIN,
        0U,
        nullptr,
        D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));
}
