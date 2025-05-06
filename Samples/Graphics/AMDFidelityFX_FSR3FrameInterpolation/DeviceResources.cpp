//
// DeviceResources.cpp - A wrapper for the Direct3D 12/12.X device and swapchain
//
// Modified to support HDR on Xbox.
//

#include "pch.h"
#include "DeviceResources.h"

#if defined(_GAMING_DESKTOP)

#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch"
#elif defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable : 4061 4062)
#endif

#include <ffx_api/dx12/ffx_api_dx12.hpp>

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif

#endif // #if defined(_GAMING_DESKTOP)

#include "SampleAssert.h"

using namespace DirectX;
using namespace DX;

using Microsoft::WRL::ComPtr;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

// Constructor for DeviceResources.
DeviceResources::DeviceResources(
    DXGI_FORMAT frameBackBufferFormat,
    DXGI_FORMAT uiBackBufferFormat,
    DXGI_FORMAT depthBufferFormat,
    UINT backBufferCount,
    unsigned int flags) noexcept(false) :
        m_backBufferIndex(0),
        m_fenceValue(0),
#ifdef _GAMING_XBOX
        m_framePipelineToken{},
#endif
        m_rtvDescriptorSize(0),
        m_screenViewport{},
        m_scissorRect{},
        m_frameBackBufferFormat(frameBackBufferFormat),
        m_uiBackBufferFormat(uiBackBufferFormat),
        m_depthBufferFormat(depthBufferFormat),
        m_backBufferCount(backBufferCount),
        m_window(nullptr),
        m_d3dFeatureLevel(D3D_FEATURE_LEVEL_12_0),
        m_outputSize{ 0, 0, 1, 1 },
        m_options(flags),
        m_deviceNotify(nullptr)
{
    if (backBufferCount < 2 || backBufferCount > MAX_BACK_BUFFER_COUNT)
    {
        throw std::out_of_range("invalid backBufferCount");
    }

#ifndef _GAMING_XBOX
    if (flags & c_EnableHDR)
    {
        throw std::out_of_range("HDR not supported on Desktop configurations");
    }
#endif
}

// Destructor for DeviceResources.
DeviceResources::~DeviceResources()
{
    // Ensure that the GPU is no longer referencing resources that are about to be destroyed.
    WaitForGpu();

#ifdef _GAMING_XBOX
    // Ensure we present a blank screen before cleaning up resources.
    if (m_commandQueue)
    {
        std::ignore = m_commandQueue->PresentX(0, nullptr, nullptr);
    }
#endif

	// Frame interpolation swap chain must be released
#ifdef _GAMING_XBOX
    ffxDestroyFrameinterpolationSwapchainX(m_ffxSwapChain);

    //////////////////////////////////////////////////////////////////////////
    //
    // AMD FidelityFX Super Resolution 3 + Frame Interpolation Guidelines
    //
    // Once the frame interpolation swapchain has been destroyed, the title should
    // change its frame interval length and period back to its original values.
    // 
    //////////////////////////////////////////////////////////////////////////
    RegisterFrameEvents(false);
#else
    // Destroy ffx swapchain context and swapchain
    DestroySwapChain();    
#endif // #ifdef _GAMING_XBOX
}

#ifdef _GAMING_DESKTOP
void DeviceResources::DestroySwapChain() noexcept
{
    ffx::DestroyContext(m_swapChainContext);
    m_swapChain.Reset();
}
#endif // _GAMING_DESKTOP

// Configures the Direct3D device, and stores handles to it and the device context.
void DeviceResources::CreateDeviceResources()
{
#ifdef _GAMING_XBOX

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

#if defined(_GAMING_XBOX_SCARLETT) && (_GRDK_VER >= 0x585D070E /* GXDK Edition 221000 */)
    if (m_options & c_AmplificationShaders)
    {
        params.AmplificationShaderIndirectArgsBufferSize = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
        params.AmplificationShaderPayloadBufferSize = static_cast<UINT>(D3D12XBOX_DEFAULT_SIZE_BYTES);
    }
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

    m_d3dFeatureLevel = D3D_FEATURE_LEVEL_12_0;

#else // _GAMING_DESKTOP

    DWORD dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    //
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugController.GetAddressOf()))))
        {
            debugController->EnableDebugLayer();
        }
        else
        {
            OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
        }

        ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
        {
            dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

            DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
            {
                80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
            };
            DXGI_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
        }
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));

    GetAdapter(m_adapter.GetAddressOf());

    // Create the DX12 API device object.
    HRESULT hr = D3D12CreateDevice(
        m_adapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(m_d3dDevice.ReleaseAndGetAddressOf())
        );
    ThrowIfFailed(hr);

    m_d3dDevice->SetName(L"DeviceResources");

#ifndef NDEBUG
    // Configure debug device (if active).
    ComPtr<ID3D12InfoQueue> d3dInfoQueue;
    if (SUCCEEDED(m_d3dDevice.As(&d3dInfoQueue)))
    {
#ifdef _DEBUG
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
        D3D12_MESSAGE_ID hide[] =
        {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
            // Workarounds for debug layer issues on hybrid-graphics systems
            D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
        };
        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
        filter.DenyList.pIDList = hide;
        d3dInfoQueue->AddStorageFilterEntries(&filter);
    }
#endif

    // Determine maximum supported feature level for this device
    static const D3D_FEATURE_LEVEL s_featureLevels[] =
    {
#if defined(NTDDI_WIN10_FE) && (NTDDI_VERSION >= NTDDI_WIN10_FE)
        D3D_FEATURE_LEVEL_12_2,
#endif
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
    {
        static_cast<UINT>(std::size(s_featureLevels)), s_featureLevels, D3D_FEATURE_LEVEL_11_0
    };

    hr = m_d3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
    if (SUCCEEDED(hr))
    {
        m_d3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
    }
    else
    {
        m_d3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    }

#endif

    // Create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&queueDesc, IID_GRAPHICS_PPV_ARGS(m_commandQueue.ReleaseAndGetAddressOf())));
    m_commandQueue->SetName(L"DeviceResources");

#ifdef _GAMING_XBOX
    // Create an async compute queue to use for frame interpolation 
    D3D12_COMMAND_QUEUE_DESC computeQueueDesc;
    computeQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
    computeQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    computeQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_HIGH;
    ThrowIfFailed(m_d3dDevice->CreateCommandQueue(&computeQueueDesc, IID_GRAPHICS_PPV_ARGS(m_asyncComputeQueue.ReleaseAndGetAddressOf())));
    m_asyncComputeQueue->SetName(L"DeviceResources_ComputeQueue");
#endif // _GAMING_XBOX

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
#ifdef _GAMING_XBOX
    // On Xbox there are x 3 descriptors:
    //      - one for each Interpolated back buffer by FidelityFX Super Resolution 3
    //      - one for each UI back buffer
    rtvDescriptorHeapDesc.NumDescriptors = m_backBufferCount * 3;
#else
    rtvDescriptorHeapDesc.NumDescriptors = m_backBufferCount;
#endif
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
#ifdef _GAMING_XBOX
    ThrowIfFailed(m_d3dDevice->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fenceAsync.ReleaseAndGetAddressOf())));
#endif
    m_fenceValue ++;

    m_fence->SetName(L"DeviceResources");
#ifdef _GAMING_XBOX
    m_fenceAsync->SetName(L"DeviceResources Async");
#endif

    m_fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!m_fenceEvent.IsValid())
    {
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
    }

#ifdef _GAMING_XBOX
    // Register frame events with interpolation disabled by default
    RegisterFrameEvents(false);
#endif
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

#ifdef _GAMING_XBOX
    // Ensure we present a blank screen before cleaning up resources.
    ThrowIfFailed(m_commandQueue->PresentX(0, nullptr, nullptr));
#endif

    // Release resources that are tied to the swap chain and update fence values.
    for (UINT n = 0; n < m_backBufferCount; n++)
    {
        m_frameRenderTargets[n].Reset();
#ifdef _GAMING_XBOX
        m_interpolatedRenderTargets[n].Reset();
        m_uiRenderTargets[n].Reset();
#endif
    }

    // Determine the render target size in pixels.
    const UINT backBufferWidth = std::max<UINT>(static_cast<UINT>(m_outputSize.right - m_outputSize.left), 1u);
    const UINT backBufferHeight = std::max<UINT>(static_cast<UINT>(m_outputSize.bottom - m_outputSize.top), 1u);

#ifdef _GAMING_XBOX

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    const CD3DX12_HEAP_PROPERTIES swapChainHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    const DXGI_FORMAT backBufferFormat = NoSRGB(m_frameBackBufferFormat);

    D3D12_RESOURCE_DESC swapChainBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        backBufferFormat,
        backBufferWidth,
        backBufferHeight,
        1, // This resource has only one texture.
        1  // Use a single mipmap level.
    );
    swapChainBufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

#ifdef _GAMING_XBOX_XBOXONE
    if (m_options & c_EnableHDR)
    {
        swapChainBufferDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP;
    }
#endif

    D3D12_CLEAR_VALUE swapChainOptimizedClearValue = {};
    swapChainOptimizedClearValue.Format = backBufferFormat;

    wchar_t name[64] = {};
    for (UINT n = 0; n < m_backBufferCount; n++)
    {
        ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
            &swapChainHeapProperties,
            D3D12_HEAP_FLAG_ALLOW_DISPLAY,
            &swapChainBufferDesc,
            D3D12_RESOURCE_STATE_PRESENT,
            &swapChainOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_frameRenderTargets[n].GetAddressOf())));

        swprintf_s(name, L"Frame Render Target (Back Buffer) %u", n);
        m_frameRenderTargets[n]->SetName(name);
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_frameBackBufferFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(n), m_rtvDescriptorSize);
        m_d3dDevice->CreateRenderTargetView(m_frameRenderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    for (UINT n = 0; n < m_backBufferCount; n++)
    {
        ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
            &swapChainHeapProperties,
            D3D12_HEAP_FLAG_ALLOW_DISPLAY,
            &swapChainBufferDesc,
            D3D12_RESOURCE_STATE_PRESENT,
            &swapChainOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_interpolatedRenderTargets[n].GetAddressOf())));

        swprintf_s(name, L"Interpolated Render Target (Back Buffer) %u", n);
        m_interpolatedRenderTargets[n]->SetName(name);
        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_frameBackBufferFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(m_backBufferCount + n), m_rtvDescriptorSize);
        m_d3dDevice->CreateRenderTargetView(m_interpolatedRenderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    const DXGI_FORMAT uiBackBufferFormat = NoSRGB(m_uiBackBufferFormat);

    swapChainBufferDesc.Format = uiBackBufferFormat;
    swapChainOptimizedClearValue.Format = uiBackBufferFormat;

    for (UINT n = 0; n < m_backBufferCount; n++)
    {
        ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
            &swapChainHeapProperties,
            D3D12_HEAP_FLAG_ALLOW_DISPLAY,
            &swapChainBufferDesc,
            D3D12_RESOURCE_STATE_PRESENT,
            &swapChainOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_uiRenderTargets[n].GetAddressOf())));

        swprintf_s(name, L"UI Render Target (Back Buffer) %u", n);
        m_uiRenderTargets[n]->SetName(name);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = uiBackBufferFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(2 * m_backBufferCount + n), m_rtvDescriptorSize);
        m_d3dDevice->CreateRenderTargetView(m_uiRenderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    // Reset the index to the current back buffer.
    m_backBufferIndex = 0;

    // Initialize Frame Interpolation
    
    //////////////////////////////////////////////////////////////////////////
    //
    // AMD FidelityFX Super Resolution 3 + Frame Interpolation XBOX Guidelines
    //
    // Once the frame interpolation swapchain has been created, the title should
    // change its frame interval length and period to the recommended values (See RegisterFrameEvents()).
    // 
    // AMD recommends only using frame interpolation when the title can maintain a 60 fps average and
    // is attached to a high refresh rate capable monitor (> 120Hz). While it can be used with lower 
    // frame rates and refresh rates, this may result in visual artifacting and fluidity issues.
    //
    // The PresentX callback function will receive the D3D12XBOX_FRAME_PIPELINE_TOKEN that each frame was
    // generated with. It is the title's responsibility to use those tokens to query performance statistics
    // to determine if frame generation should be used or not.
    // 
    //////////////////////////////////////////////////////////////////////////
    UpdatePrimaryOutputHighRefreshRateSupported();

    FfxPresentXFunc presentFunc = &PresentXCallback;
    FfxCommandQueue ffxGameQueue = ffxGetCommandQueueDX12(m_commandQueue.Get());
    FfxCommandQueue ffxComputeQueue = ffxGetCommandQueueDX12(m_asyncComputeQueue.Get());
    FfxCommandQueue ffxPresentQueue = ffxGetCommandQueueDX12(m_asyncComputeQueue.Get());

    ffxCreateFrameinterpolationSwapchainX(ffxGameQueue, ffxComputeQueue, ffxPresentQueue, presentFunc, reinterpret_cast<void*>(this), m_ffxSwapChain);


#else // _GAMING_DESKTOP

    const DXGI_FORMAT backBufferFormat = NoSRGB(m_frameBackBufferFormat);

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        // If the swap chain already exists, resize it.
        HRESULT hr = m_swapChain->ResizeBuffers(
            m_backBufferCount,
            backBufferWidth,
            backBufferHeight,
            backBufferFormat,
            0
        );

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
#ifdef _DEBUG
            char buff[64] = {};
            sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n",
                static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? m_d3dDevice->GetDeviceRemovedReason() : hr));
            OutputDebugStringA(buff);
#endif
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            HandleDeviceLost();

            // Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        }
        else
        {
            ThrowIfFailed(hr);
        }
    }
    else
    {
        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = m_backBufferCount;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a swap chain for the window.
        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            &swapChain
        ));

        ThrowIfFailed(swapChain.As(&m_swapChain));

        // This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
        ThrowIfFailed(m_dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));

        // Check for high refresh rate display
        UpdatePrimaryOutputHighRefreshRateSupported();

        //////////////////////////////////////////////////////////////////////////
        //
        // AMD FidelityFX Super Resolution 3 + Frame Interpolation Desktop Guidelines
        //
        // Create FrameInterpolationSwapchain separate from FSR generation so it can be done
        // when the engine creates the swapchain.
        // This should not be created and destroyed with FSR, as it requires a switch to windowed mode
        //////////////////////////////////////////////////////////////////////////
        IDXGISwapChain4* dxgiSwapchain = m_swapChain.Get();
        dxgiSwapchain->AddRef();
        swapChain.Reset();
        m_swapChain.Reset();

        ffx::CreateContextDescFrameGenerationSwapChainForHwndDX12 createSwapChainDesc{};
        dxgiSwapchain->GetHwnd(&createSwapChainDesc.hwnd);
        DXGI_SWAP_CHAIN_DESC1 desc1;
        dxgiSwapchain->GetDesc1(&desc1);
        createSwapChainDesc.desc = &desc1;
        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc;
        dxgiSwapchain->GetFullscreenDesc(&fullscreenDesc);
        createSwapChainDesc.fullscreenDesc = &fullscreenDesc;
        dxgiSwapchain->GetParent(IID_PPV_ARGS(&createSwapChainDesc.dxgiFactory));
        createSwapChainDesc.gameQueue = m_commandQueue.Get();

        dxgiSwapchain->Release();
        dxgiSwapchain = nullptr;
        createSwapChainDesc.swapchain = &dxgiSwapchain;

        ffx::ReturnCode retCode = ffx::CreateContext(m_swapChainContext, nullptr, createSwapChainDesc);

        //Couldn't create the ffxapi fg swapchain
        SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);

        createSwapChainDesc.dxgiFactory->Release();

        m_swapChain = dxgiSwapchain;

        // In case the app is handling Alt-Enter manually we need to update the window association after creating a different swapchain
        IDXGIFactory7* factory = nullptr;
        if (SUCCEEDED(dxgiSwapchain->GetParent(IID_PPV_ARGS(&factory))))
        {
            factory->MakeWindowAssociation(m_window, DXGI_MWA_NO_WINDOW_CHANGES);
            factory->Release();
        }

        dxgiSwapchain->Release();
    }

    ThrowIfFailed(m_swapChain->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709));

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    for (UINT n = 0; n < m_backBufferCount; n++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(m_frameRenderTargets[n].GetAddressOf())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        m_frameRenderTargets[n]->SetName(name);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_frameBackBufferFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        const CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(n), m_rtvDescriptorSize);
        m_d3dDevice->CreateRenderTargetView(m_frameRenderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    // Reset the index to the current back buffer.
    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

#endif

    if (m_depthBufferFormat != DXGI_FORMAT_UNKNOWN)
    {
        // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
        // on this surface.
        const CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            m_depthBufferFormat,
            backBufferWidth,
            backBufferHeight,
            1, // This depth stencil view has only one texture.
            1  // Use a single mipmap level.
            );
        depthStencilDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = m_depthBufferFormat;
        depthOptimizedClearValue.DepthStencil.Depth = (m_options & c_ReverseDepth) ? 0.0f : 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

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

        m_d3dDevice->CreateDepthStencilView(m_depthStencil.Get(), &dsvDesc, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
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

// This method is called when the Win32 window is created (or re-created).
void DeviceResources::SetWindow(HWND window, int width, int height) noexcept
{
    m_window = window;

    m_outputSize.left = m_outputSize.top = 0;
    m_outputSize.right = width;
    m_outputSize.bottom = height;
}

// This method is called when the Win32 window changes size.
bool DeviceResources::WindowSizeChanged(int width, int height)
{
    RECT newRc;
    newRc.left = newRc.top = 0;
    newRc.right = width;
    newRc.bottom = height;

    bool sizeIdentical = newRc.left == m_outputSize.left
                       && newRc.top == m_outputSize.top
                       && newRc.right == m_outputSize.right
                       && newRc.bottom == m_outputSize.bottom;

    if (!sizeIdentical)
    {
        m_outputSize = newRc;
        CreateWindowSizeDependentResources();
    }

#ifdef _GAMING_DESKTOP
    // As WindowSizeChanged is called both when Window is moved or resized, we check if its primary output changed
    UpdatePrimaryOutputHighRefreshRateSupported();
#endif

    return !sizeIdentical;
}

void DeviceResources::UpdatePrimaryOutputHighRefreshRateSupported()
{
    m_hasHighRefreshRate = false;

    // Get the attached output
    ComPtr<IDXGIOutput> dxgiOutput;
#ifdef _GAMING_DESKTOP
    // On PC, we have `m_swapChain` object
    DX::ThrowIfFailed(m_swapChain->GetContainingOutput(dxgiOutput.GetAddressOf()));
#else
    // On Xbox, we get the default output from the adapter
    ComPtr<IDXGIDevice> dxgiDevice;
    DX::ThrowIfFailed(m_d3dDevice->QueryInterface(IID_GRAPHICS_PPV_ARGS(dxgiDevice.GetAddressOf())));

    ComPtr<IDXGIAdapter> dxgiAdapter;
    DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

    DX::ThrowIfFailed(dxgiAdapter->EnumOutputs(0, dxgiOutput.GetAddressOf()));
#endif

    if (nullptr != dxgiOutput)
    {
        uint32_t outputModeCount = 0;

#ifdef _GAMING_DESKTOP
        std::ignore = dxgiOutput->GetDisplayModeList(m_frameBackBufferFormat, 0, &outputModeCount, nullptr);
        auto outputModes = std::make_unique<DXGI_MODE_DESC[]>(outputModeCount);
        DX::ThrowIfFailed(dxgiOutput->GetDisplayModeList(m_frameBackBufferFormat, 0, &outputModeCount, outputModes.get()));
#else
        std::ignore = dxgiOutput->GetDisplayModeListX(m_frameBackBufferFormat, 0, &outputModeCount, nullptr);
        auto outputModes = std::make_unique<DXGIXBOX_MODE_DESC[]>(outputModeCount);
        DX::ThrowIfFailed(dxgiOutput->GetDisplayModeListX(m_frameBackBufferFormat, 0, &outputModeCount, outputModes.get()));
#endif

        // Check if the display supports 120Hz.
        for (uint32_t i = 0; i < outputModeCount; ++i)
        {
            if (outputModes[i].RefreshRate.Numerator / outputModes[i].RefreshRate.Denominator >= 120)
            {
                m_hasHighRefreshRate = true;
                break;
            }
        }
    }
}

// Recreate all device resources and set them back to the current state.
void DeviceResources::HandleDeviceLost()
{
#ifdef _GAMING_DESKTOP
    if (m_deviceNotify)
    {
        m_deviceNotify->OnDeviceLost();
    }

    for (UINT n = 0; n < m_backBufferCount; n++)
    {
        m_commandAllocators[n].Reset();
        m_frameRenderTargets[n].Reset();
    }

    // Need to release the Swapchain context
    DestroySwapChain();

    m_depthStencil.Reset();
    m_commandQueue.Reset();
    m_commandList.Reset();
    m_fence.Reset();
    m_rtvDescriptorHeap.Reset();
    m_dsvDescriptorHeap.Reset();
	
	//m_swapChain.Reset();
    
	m_d3dDevice.Reset();
    m_adapter.Reset();
    m_dxgiFactory.Reset();

#ifdef _DEBUG
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif

    CreateDeviceResources();
    CreateWindowSizeDependentResources();

    if (m_deviceNotify)
    {
        m_deviceNotify->OnDeviceRestored();
    }
#endif
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
        const D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_frameRenderTargets[m_backBufferIndex].Get(), beforeState, afterState),
#ifdef _GAMING_XBOX
            CD3DX12_RESOURCE_BARRIER::Transition(m_uiRenderTargets[m_backBufferIndex].Get(), beforeState, afterState)
#endif
        };
        m_commandList->ResourceBarrier(static_cast<UINT>(std::size(barriers)), barriers);
    }
}

// Present the contents of the swap chain to the screen.
void DeviceResources::Present(PresentUiCompositionMode uiCompositionMode, D3D12_RESOURCE_STATES beforeState)
{
#ifndef _GAMING_XBOX
    (void)uiCompositionMode;
#endif
    if (beforeState != D3D12_RESOURCE_STATE_PRESENT)
    {
        // Transition the render target to the state that allows it to be presented to the display.
        const D3D12_RESOURCE_BARRIER barriers[] =
        {
            CD3DX12_RESOURCE_BARRIER::Transition(m_frameRenderTargets[m_backBufferIndex].Get(), beforeState, D3D12_RESOURCE_STATE_PRESENT),
#ifdef _GAMING_XBOX
            CD3DX12_RESOURCE_BARRIER::Transition(m_uiRenderTargets[m_backBufferIndex].Get(), beforeState, D3D12_RESOURCE_STATE_PRESENT)
#endif
        };
        m_commandList->ResourceBarrier(static_cast<UINT>(std::size(barriers)), barriers);
    }

    // Send the command list off to the GPU for processing.
    ThrowIfFailed(m_commandList->Close());
    m_commandQueue->ExecuteCommandLists(1, CommandListCast(m_commandList.GetAddressOf()));

#ifdef _GAMING_XBOX


    FfxResourceDescription realFrameDesc = ffxGetResourceDescriptionDX12(m_frameRenderTargets[m_backBufferIndex].Get());
    FfxResourceDescription interpolatedFrameDesc = ffxGetResourceDescriptionDX12(m_interpolatedRenderTargets[m_backBufferIndex].Get());
    FfxResourceDescription uiFrameDesc = ffxGetResourceDescriptionDX12(m_uiRenderTargets[m_backBufferIndex].Get());

    FfxResource realFrame = ffxGetResourceDX12(m_frameRenderTargets[m_backBufferIndex].Get(), realFrameDesc, L"Real Back Buffer Present Resource", FFX_RESOURCE_STATE_PRESENT);
    FfxResource interpolatedFrame = ffxGetResourceDX12(m_interpolatedRenderTargets[m_backBufferIndex].Get(), interpolatedFrameDesc, L"Interpolated Back Buffer Present Resource", FFX_RESOURCE_STATE_PRESENT);
    FfxResource uiFrame = ffxGetResourceDX12(m_uiRenderTargets[m_backBufferIndex].Get(), uiFrameDesc, L"UI Back Buffer Present Resource", FFX_RESOURCE_STATE_PRESENT);

    // Update the back buffer index before calling ffxPresentX because
    //      1. It might call UI callback immediately
    //      2. It might call UI callback on the other thread
    // and the UI callbacks would need to access 'Real' or 'Interpolated' back buffer of the current frame
    // so we advance the index to make sure the index is identical
    m_backBufferIndex = (m_backBufferIndex + 1) % m_backBufferCount;

    // call frame interpolation present - if frame interpolation is disabled, this will just call through to our PresentX callback
    // after doing specified UI composition
    FfxErrorCode errorCode = ffxPresentX(m_ffxSwapChain, m_framePipelineToken, realFrame, interpolatedFrame, uiCompositionMode == PresentUiCompositionMode::kUseUiBackBuffer ? uiFrame : FfxResource({}));
    SAMPLE_ASSERT(FFX_OK == errorCode);

    // Xbox apps do not need to handle DXGI_ERROR_DEVICE_REMOVED or DXGI_ERROR_DEVICE_RESET.

#else // _GAMING_DESKTOP

    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
#ifdef _DEBUG
        char buff[64] = {};
        sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n",
            static_cast<unsigned int>((hr == DXGI_ERROR_DEVICE_REMOVED) ? m_d3dDevice->GetDeviceRemovedReason() : hr));
        OutputDebugStringA(buff);
#endif
        HandleDeviceLost();
    }
    else
    {
        ThrowIfFailed(hr);
    }

    MoveToNextFrame();
#endif
}

#ifdef _GAMING_XBOX
void DeviceResources::PresentXCallback(FfxPresentXParams* ffxPresentXParams)
{
    // FfxPresentXParams.framePipelineTokenOriginal is the token used for this rendered frame.
    // Use it to do any performance-related queries needed.

    // FfxPresentXParams.framePipelineTokenToSubmit is the token used that is
    // either the same as FfxPresentXParams.framePipelineTokenOriginal or newly generated token.
    // It must be used present FfxPresentXParams.sceneBackBufferToPresent
    
    // Present the backbuffer using the PresentX API.
    DeviceResources *deviceResources = reinterpret_cast<DeviceResources *>(ffxPresentXParams->presentContext);

    DXGI_FORMAT backBufferFormat = deviceResources->GetFrameBackBufferFormat();
    (void)backBufferFormat;

#define BACK_BUFFER_HAS_NO_GAMMA_CURVE_APPLIED 0 // Enable this define applying gamma curve was skipped and the back buffer has DXGI_FORMAT_R9G9B9E5_SHAREDEXP format

    // Present the backbuffer using the PresentX API.
    if (ffxPresentXParams->uiBackBufferToPresent == nullptr)
    {
        D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters = {};
        planeParameters.Token = ffxPresentXParams->framePipelineTokenToSubmit;
        planeParameters.ResourceCount = 1;
        planeParameters.ppResources = &ffxPresentXParams->sceneBackBufferToPresent;
        planeParameters.pSrcViewRects = &deviceResources->m_outputSize;
        if (deviceResources->m_options & DeviceResources::c_EnableHDR)
        {
            planeParameters.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;    // Scene buffer is HDR, contains values rendered in Rec.2020 color space with applied ST.2084 gamma curve on top

#if defined(_GAMING_XBOX_SCARLETT) && BACK_BUFFER_HAS_NO_GAMMA_CURVE_APPLIED
            if (backBufferFormat == DXGI_FORMAT_R9G9B9E5_SHAREDEXP)
                planeParameters.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P2020;  // Scene buffer is HDR, contains values rendered in Rec.2020 color space with no gamma curve applied (linear)
#endif
        }
        else
        {
            planeParameters.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;       // Scene buffer is HDR, contains values rendered in Rec.709 color space with applied sRGB gamma curve on top

#if defined(_GAMING_XBOX_SCARLETT) && BACK_BUFFER_HAS_NO_GAMMA_CURVE_APPLIED
            if (backBufferFormat == DXGI_FORMAT_R9G9B9E5_SHAREDEXP)
                planeParameters.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;   // Scene buffer is SDR, contains values rendered in Rec.709 color space with no gamma curve applied (linear)
#endif

        }

        ThrowIfFailed(
            ffxPresentXParams->presentQueue->PresentX(1, &planeParameters, nullptr)
        );
    }
    else
    {
        D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters[2] = {};
        planeParameters[0].Token = planeParameters[1].Token = ffxPresentXParams->framePipelineTokenToSubmit;
        planeParameters[0].ResourceCount = planeParameters[1].ResourceCount = 1;
        planeParameters[0].pSrcViewRects = planeParameters[1].pSrcViewRects = &deviceResources->m_outputSize;

        // When compositing in HDR, the system level auto tone mapping will be used to generate an SDR image for GameDVR
        // NOTE: For SDR, present the scene in display plane 0 and UI in display plane 1
        //       For HDR, present the scene in display plane 1 and UI in display plane 0
        if (deviceResources->m_options & DeviceResources::c_EnableHDR)
        {
            planeParameters[0].ppResources = &ffxPresentXParams->uiBackBufferToPresent;
            planeParameters[0].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;    // UI is SDR in sRGB gamma space using Rec.709

            planeParameters[1].ppResources = &ffxPresentXParams->sceneBackBufferToPresent;
            planeParameters[1].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020; // Scene buffer is HDR, contains values rendered in Rec.2020 color space with applied ST.2084 gamma curve on top

#if defined(_GAMING_XBOX_SCARLETT) && BACK_BUFFER_HAS_NO_GAMMA_CURVE_APPLIED
            if (backBufferFormat == DXGI_FORMAT_R9G9B9E5_SHAREDEXP)
                planeParameters[1].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P2020;   // Scene buffer is HDR, contains values rendered in Rec.2020 color space with no gamma curve applied (linear)
#endif
        }
        else
        {
            planeParameters[0].ppResources = &ffxPresentXParams->sceneBackBufferToPresent;
            planeParameters[0].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;        // Scene buffer is HDR, contains values rendered in Rec.709 color space with applied sRGB gamma curve on top

#if defined(_GAMING_XBOX_SCARLETT) && BACK_BUFFER_HAS_NO_GAMMA_CURVE_APPLIED
            if (backBufferFormat == DXGI_FORMAT_R9G9B9E5_SHAREDEXP)
                planeParameters[0].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;    // Scene buffer is SDR, contains values rendered in Rec.709 color space with no gamma curve applied (linear)
#endif

            planeParameters[1].ppResources = &ffxPresentXParams->uiBackBufferToPresent;
            planeParameters[1].ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;        // UI is SDR in sRGB gamma space using Rec.709
        }
#undef BACK_BUFFER_HAS_NO_GAMMA_CURVE_APPLIED

        ThrowIfFailed(
            ffxPresentXParams->presentQueue->PresentX(2, planeParameters, nullptr)
        );
    }
}
#endif // #ifdef _GAMING_XBOX

// Handle GPU suspend/resume
void DeviceResources::Suspend()
{
#ifdef _GAMING_XBOX
    m_commandQueue->SuspendX(0);
    m_asyncComputeQueue->SuspendX(0);
#endif
}

void DeviceResources::Resume()
{
#ifdef _GAMING_XBOX
    m_commandQueue->ResumeX();
    m_asyncComputeQueue->ResumeX();

    // Re-enable frame interpolation on resume
    // (The title will want to save the state of frame interpolation to use when resuming)
    RegisterFrameEvents(true);
#endif
}

// Wait for pending GPU work to complete.
void DeviceResources::WaitForGpu() noexcept
{
    if (m_commandQueue && m_fence && m_fenceEvent.IsValid())
    {
        // Schedule a Signal command in the GPU queue.
        if (SUCCEEDED(m_commandQueue->Signal(m_fence.Get(), m_fenceValue)))
        {
#ifdef _GAMING_XBOX
            // Schedule a Signal command in the Async GPU queue.
            m_asyncComputeQueue->Signal(m_fenceAsync.Get(), m_fenceValue);

            ID3D12Fence *fences[] = { m_fence.Get(), m_fenceAsync.Get() };
            UINT64 fenceValues[] = { m_fenceValue, m_fenceValue };

            // Wait until both Signals has been processed
            if (SUCCEEDED(m_d3dDevice->SetEventOnMultipleFenceCompletion(fences, fenceValues, 2, D3D12_MULTIPLE_FENCE_WAIT_FLAG_ALL, m_fenceEvent.Get())))
#else
            // Wait until the Signal has been processed.
            if (SUCCEEDED(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent.Get())))
#endif
            {
                std::ignore = WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);

                // Increment the fence value for the current frame.
                m_fenceValue ++;
            }
        }
    }
}

#ifdef _GAMING_XBOX
// For PresentX rendering, we should wait for the origin event just before processing input.
void DeviceResources::WaitForOrigin()
{
    // when frame interpolation is enabled, we have to allow the backend to perform frame interpolation presentation + token acquisition
    // before starting to wait on the next frame origin token
    // when frame interpolation is disabled, this function will immediately return
    ffxWaitForPresentX(m_ffxSwapChain);

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
void DeviceResources::RegisterFrameEvents(bool frameInterpolationEnabled)
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

    //////////////////////////////////////////////////////////////////////////
    //
    // AMD FidelityFX Super Resolution 3 + Frame Interpolation Guidelines
    //
    // When enabling frame interpolation, it is recommended that the following
    // values be used to maintain a good latency window:
    // 
    //  - LengthInMicroseconds should be double the original value
    //      (i.e. D3D12XBOX_FRAME_INTERVAL_60_HZ -> D3D12XBOX_FRAME_INTERVAL_120_HZ
    // 
    //  - PeriodInIntervals should be 2*X + 1 of original value X
    //      (i.e. 2 -> 5)
    // 
    //////////////////////////////////////////////////////////////////////////
    if (frameInterpolationEnabled && m_hasHighRefreshRate)
    {
        ThrowIfFailed(m_d3dDevice->SetFrameIntervalX(
            dxgiOutput.Get(),
            D3D12XBOX_FRAME_INTERVAL_120_HZ,
#if _GRDK_VER < 0x65F40743  // The maximum value for PeriodInIntervals was increased to 7 with the Oct 2024 GDK
            std::min((m_backBufferCount - 1u) * 2 + 1, 4u),
#else
            std::min((m_backBufferCount - 1u) * 2 + 1, 7u),
#endif
            D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));

        ThrowIfFailed(m_d3dDevice->ScheduleFrameEventX(
            D3D12XBOX_FRAME_EVENT_ORIGIN,
            0U,
            nullptr,
            D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));
    }
    else
    {
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
}

#else // _GAMING_DESKTOP

// Prepare to render the next frame.
void DeviceResources::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));

    // Update the back buffer index.
    m_backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValue)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent.Get()));
        std::ignore = WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fenceValue ++;
}

// This method acquires the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, try WARP. Otherwise throw an exception.
void DeviceResources::GetAdapter(IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0;
        SUCCEEDED(m_dxgiFactory->EnumAdapterByGpuPreference(
            adapterIndex,
            DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
            IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf())));
        adapterIndex++)
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc));

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
        {
#ifdef _DEBUG
            wchar_t buff[256] = {};
            swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterIndex, desc.VendorId, desc.DeviceId, desc.Description);
            OutputDebugStringW(buff);
#endif
            break;
        }
    }

#if !defined(NDEBUG)
    if (!adapter)
    {
        // Try WARP12 instead
        if (FAILED(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(adapter.ReleaseAndGetAddressOf()))))
        {
            throw std::runtime_error("WARP12 not available. Enable the 'Graphics Tools' optional feature");
        }

        OutputDebugStringA("Direct3D Adapter - WARP12\n");
    }
#endif

    if (!adapter)
    {
        throw std::runtime_error("No Direct3D 12 device found");
    }

    *ppAdapter = adapter.Detach();
}
#endif
