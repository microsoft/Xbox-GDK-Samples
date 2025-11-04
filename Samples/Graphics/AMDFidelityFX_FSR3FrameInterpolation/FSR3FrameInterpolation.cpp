//--------------------------------------------------------------------------------------
// SuperResolution.cpp
//
// Modifications Copyright (C) 2021, 2022. Advanced Micro Devices, Inc. All Rights Reserved.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h" 

#include "FSR3FrameInterpolation.h"

#ifdef _GAMING_XBOX
#include <FidelityFX/host/backends/gdk/ffx_gdk.h>
#else

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

#endif // #ifdef _GAMING_XBOX

#include "SampleAssert.h"

#pragma warning(disable : 4061) // warning C4061: enumerator 'A' in switch of enum 'B' is not explicitly handled by a case label

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

enum ColorSpace
{
    ColorSpace_NOP,
    ColorSpace_REC2020
};

#if defined(_GAMING_XBOX)

struct FrameTransientResourceMemoryStackAllocator
{
    static constexpr uint32_t kDefaultPageCount = 256;
    static constexpr uint32_t k2Mb = 2 * 1024 * 1024;
    static constexpr uint32_t k64Kb = 64 * 1024;

    ULONG_PTR m_memory;
    ULONG_PTR m_offset;
    ULONG_PTR m_limit;
    ULONG_PTR m_unused;

    FrameTransientResourceMemoryStackAllocator()
        : m_memory(0)
        , m_offset(0)
        , m_limit(0)
        , m_unused(0)
    { }

    ~FrameTransientResourceMemoryStackAllocator()
    {
        // Make sure shutdown was called before destructor
        SAMPLE_ASSERT(m_memory == 0);
    }

    void init(uint32_t pageCount, uint32_t pageSizeFlag)
    {
        if (m_memory == 0)
        {
            DWORD allocationFlags = MEM_RESERVE | pageSizeFlag;
            DWORD protectionFlags = PAGE_READWRITE | PAGE_WRITECOMBINE | PAGE_GRAPHICS_READWRITE;

            m_limit = (pageSizeFlag == MEM_2MB_PAGES ? k2Mb : k64Kb) * pageCount;
            m_memory = reinterpret_cast<ULONG_PTR>(XMemVirtualAlloc(NULL, m_limit, allocationFlags, XMEM_GRAPHICS | XMEM_MAPPABLE, protectionFlags));
            m_offset = 0;
        }
    }

    void shutdown()
    {
        if (m_memory != 0)
        {
            VirtualFree(reinterpret_cast<PVOID>(m_memory), 0, MEM_RELEASE);
            m_memory = 0;
        }
    }

    void mapPhysical(PULONG_PTR pages, uint64_t pageCount)
    {
        if (pageCount > 0)
        {
            ULONG_PTR mappedAddress = reinterpret_cast<ULONG_PTR>(XMemMapPhysicalPages(reinterpret_cast<PVOID>(m_memory), pageCount, pages));
            SAMPLE_ASSERT(mappedAddress == m_memory);
        }
    }

    void unmapPhysical(uint64_t pageCount)
    {
        if (pageCount > 0)
        {
            ULONG_PTR mappedAddress = reinterpret_cast<ULONG_PTR>(XMemMapPhysicalPages(reinterpret_cast<PVOID>(m_memory), pageCount, NULL));
            SAMPLE_ASSERT(mappedAddress == m_memory);
        }
    }

    D3D12_GPU_VIRTUAL_ADDRESS alloc(uint64_t size, uint64_t alignment)
    {
        ULONG_PTR resourceStart = (m_offset + alignment - 1) & ~(alignment - 1);
        ULONG_PTR resourceEnd = resourceStart + size;
        if (resourceEnd <= m_limit)
        {
            m_unused += resourceStart - m_offset;
            m_offset = resourceEnd;
            return m_memory + resourceStart;
        }
        else
        {
            SAMPLE_ASSERT(false);
            return NULL;
        }
    }

    uint32_t getUsedPageCount(uint32_t pageSize) const
    {
        // NOTE: pageSize must a power of 2
        SAMPLE_ASSERT((pageSize & (pageSize - 1)) == 0);
        return static_cast<uint32_t>((m_offset + static_cast<ULONG_PTR>(pageSize) - 1) / static_cast<ULONG_PTR>(pageSize));
    }

    static void allocatePhysicalPages(PULONG_PTR outPages, uint64_t pageCount, uint32_t pageSize)
    {
        SAMPLE_ASSERT(pageSize == k2Mb || pageSize == k64Kb);
        SAMPLE_ASSERT(pageSize == k2Mb || (pageCount / 32) == 0);
        if (pageCount > 0)
        {
            ULONG_PTR inoutPageCount = pageCount;
            BOOL physicalPagesAllocated = XMemAllocatePhysicalPages(static_cast<ULONG_PTR>(pageSize == k2Mb ? XMEM_2MB_CLUSTERS : MEM_64K_PAGES), &inoutPageCount, outPages);
            SAMPLE_ASSERT(inoutPageCount == pageCount);
            SAMPLE_ASSERT(physicalPagesAllocated);
        }
    }

    static void freePhysicalPages(PULONG_PTR pages, uint64_t pageCount)
    {
        if (pageCount > 0)
        {
            BOOL physicalPagesFreed = XMemFreePhysicalPages(pageCount, pages);
            SAMPLE_ASSERT(physicalPagesFreed);
        }
    }
};

#endif // #if defined(_GAMING_XBOX)

namespace
{
    /**
    Performs a rounded division.

    \param value The value to be divided.
    \param divisor The divisor to be used.
    \return The rounded divided value.
    */
    template<typename TYPE>
    static inline TYPE RoundedDivide(TYPE value, TYPE divisor)
    {
        return (value + divisor - 1) / divisor;
    }

    const XMVECTORF32 c_eye = { -0.8f, 2.0f, 2.0f };
    constexpr float c_pitch = -0.17f;
    constexpr float c_yaw = XM_PI - 0.28f;

    constexpr bool c_reverseDepth = true;

#ifdef _GAMING_XBOX
    FfxResource GetFfxResourceFromD3D12(ID3D12Resource *d3d12Resource, const wchar_t* resourceName, FfxResourceStates state)
    {
        FfxResourceDescription ffxResourceDescription = ffxGetResourceDescriptionDX12(d3d12Resource);
        return ffxGetResourceDX12(d3d12Resource, ffxResourceDescription, resourceName, state);
    }

    FfxResource GetFfxResourceFromNull()
    {
        return FfxResource({});
    }
#else //_GAMING_DESKTOP
    FfxApiResource GetFfxResourceFromD3D12(ID3D12Resource* d3d12Resource, uint32_t state = FFX_API_RESOURCE_STATE_COMPUTE_READ, uint32_t additionalUsages = 0)
    {
        FfxApiResource apiRes = ffxApiGetResourceDX12(d3d12Resource, state, additionalUsages);
        return apiRes;
    }
    FfxApiResource GetFfxResourceFromNull()
    {
        return FfxApiResource({});
    }
#endif // _GAMING_XBOX
}

static Sample* g_pSampleInstance = nullptr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pitch(c_pitch),
    m_yaw(c_yaw),
    m_eye(c_eye),
    m_bIsDisplayInHDRMode(true),
    m_cpuTimer(std::make_unique<DX::CPUTimer>()),
    m_initialResourceClearsRequired(true),
    m_rcasEnable(true),
    m_rcasSharpness(0.8f),
    m_deltaTime(0.0),
    m_frameInterpolationControls(true),
    m_hideMagnifier(true),
    m_useFSRReactiveMask(true),
    m_resetRequested(false),
    m_renderScaleChanged(false),
    m_renderScaleWantsChange(false),
    m_pendingRenderScale(3),
    m_renderScale(1),
    m_lodBias(-0.5f)
{
    unsigned int flags = (c_reverseDepth) ? DX::DeviceResources::c_ReverseDepth : 0;
    DXGI_FORMAT sdrFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    DXGI_FORMAT uiFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    (void)uiFormat;

    // store the sample instance to use in the UI callback
    g_pSampleInstance = this;

#ifdef _GAMING_XBOX
    // Determine if attached display is HDR or SDR, if HDR, also set the display to HDR mode.
    SetDisplayMode();

    if (m_bIsDisplayInHDRMode)
    {
#ifdef _GAMING_XBOX_SCARLETT
        DXGI_FORMAT hdrFormat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
#else
        DXGI_FORMAT hdrFormat = DXGI_FORMAT_R10G10B10A2_UNORM;
#endif
        flags |= DX::DeviceResources::c_EnableHDR;
        m_deviceResources = std::make_unique<DX::DeviceResources>(
            hdrFormat,
            uiFormat,
            DXGI_FORMAT_D32_FLOAT,
            3,
            flags);
    }
    else
#endif
    {
        m_bIsDisplayInHDRMode = false;

        // PC just has an SDR path.
        m_deviceResources = std::make_unique<DX::DeviceResources>(
            sdrFormat,
            uiFormat,
            DXGI_FORMAT_D32_FLOAT,
            3,
            flags);
    }

    // setup FidelityFX gdk backend allocators
#ifdef _GAMING_XBOX
    ffxRegisterResourceAllocatorX(&Sample::ffxAllocateResource);
    ffxRegisterResourceDestructorX(&Sample::ffxReleaseResource);
#endif // _GAMING_XBOX

    m_cpuTimer->Start();
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    // Destroy FSR context
    UpdateFSRContext(false);

    m_gltfPBR->OnDestroy();
    delete m_gltfPBR;

    m_gltfDepth->OnDestroy();
    delete m_gltfDepth;

    m_gltfMotionVectors->OnDestroy();
    delete m_gltfMotionVectors;

    m_gltfResources.OnDestroy();

    m_gltfModel->Unload();
    delete m_gltfModel;
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 3840;
    height = 2160;
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    // do graphics commit at the start of the frame instead of the end to avoid synchronization issues
    // with frame interpolation work on background threads
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

    m_timer.Tick([&]()
        {
            Update(m_timer);
        });

    Render();
    
    PIXEndEvent();

    m_frame++;
}

// updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

#ifdef _GAMING_XBOX
    auto pad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);
#else
    auto pad = m_gamePad->GetState(0);
#endif

    if (pad.IsConnected())
    {
        using ButtonState = GamePad::ButtonStateTracker;

        m_gamePadButtons.Update(pad);

        // start with the global options

        // exit
        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        // rotate view
        m_yaw += pad.thumbSticks.leftX * 0.02f;
        m_pitch += pad.thumbSticks.leftY * 0.02f;

        // limit to avoid looking directly up or down
        constexpr float c_limit = XM_PI / 2.0f - 0.01f;
        m_pitch = std::max(-c_limit, std::min(+c_limit, m_pitch));

        if (m_yaw > XM_PI)
        {
            m_yaw -= XM_PI * 2.0f;
        }
        else if (m_yaw < -XM_PI)
        {
            m_yaw += XM_PI * 2.0f;
        }

        // reset camera
        if (m_gamePadButtons.leftStick == ButtonState::RELEASED)
        {
            m_pitch = c_pitch;
            m_yaw = c_yaw;
        }

        // hide UI
        if (m_gamePadButtons.rightStick == ButtonState::RELEASED)
        {
            m_hideMagnifier = !m_hideMagnifier;
        }

        // toggle control input (upscale controls vs frame interpolation controls)
        if (m_gamePadButtons.menu == ButtonState::PRESSED)
        {
            m_frameInterpolationControls = !m_frameInterpolationControls;
        }
        
        m_resetRequested = false;

        if (m_frameInterpolationControls)
        {
            // toggle interpolation
            if (m_gamePadButtons.a == ButtonState::PRESSED)
            {
                m_frameInterpolation = !m_frameInterpolation;
                m_resetRequested = true;

                // Disable warning in the event it was active
                m_showHighRefreshRateWarning = false;
            }

            // toggle interpolation only presentation
            if (m_gamePadButtons.b == ButtonState::PRESSED)
            {
                m_presentInterpolatedOnly = !m_presentInterpolatedOnly;
            }
            
            // draw tear lines
            if (m_gamePadButtons.x == ButtonState::PRESSED)
            {
                m_drawDebugTearLines = !m_drawDebugTearLines;
            }
            
            // draw debug view
            if (m_gamePadButtons.y == ButtonState::PRESSED)
            {
                m_drawDebugView = !m_drawDebugView;
            }

            // toggle asynchronous compute support
            if (m_gamePadButtons.dpadUp == ButtonState::PRESSED)
            {
                m_allowAsyncCompute = !m_allowAsyncCompute;
                m_resetRequested = true;
            }

            // toggle double buffered ui resource in swapchain
            if (m_gamePadButtons.dpadDown == ButtonState::PRESSED)
            {
                m_doublebufferInSwapchain = !m_doublebufferInSwapchain;
                m_resetRequested = true;
            }

            // toggle frame interpolation callback
            if (m_gamePadButtons.leftShoulder == ButtonState::PRESSED)
            {
                m_frameGenerationCallback = !m_frameGenerationCallback;
            }

#ifdef _GAMING_XBOX
            // toggle async compute presentation 
            if (m_gamePadButtons.rightShoulder == ButtonState::PRESSED)
            {
                m_asyncComputePresent = !m_asyncComputePresent;
            }
#endif // _GAMING_XBOX

            // toggle ui modes
            if (m_gamePadButtons.dpadLeft == ButtonState::PRESSED)
            {
                m_compositionMode = (m_compositionMode == COMP_MODE_NONE) ? COMP_MODE_HUDLESS_TEXTURE : FICompositionModes(m_compositionMode - 1);
            }
            else if (m_gamePadButtons.dpadRight == ButtonState::PRESSED)
            {
                m_compositionMode = (FICompositionModes)((uint32_t(m_compositionMode) + 1) % uint32_t(COMP_MODE_COUNT));
            }
        }
        else
        {
            // cycle Quality modes
            if (m_gamePadButtons.a == ButtonState::PRESSED || m_gamePadButtons.b == ButtonState::PRESSED)
            {
                if (m_gamePadButtons.a == ButtonState::PRESSED)
                {
                    m_pendingRenderScale = (m_renderScale + 1) % (m_numRenderScales);
                }
                else
                {
                    m_pendingRenderScale = (m_pendingRenderScale == 0) ? m_numRenderScales - 1 : m_pendingRenderScale - 1;
                }
                
                m_renderScaleWantsChange = true;
            }

            // rcas usage
            if (m_gamePadButtons.dpadUp == ButtonState::PRESSED)
            {
                m_rcasEnable = !m_rcasEnable;
            }

            // reactive mask usage
            if (m_gamePadButtons.dpadDown == ButtonState::PRESSED)
            {
                m_useFSRReactiveMask = !m_useFSRReactiveMask;
            }

            // sharpness
            if (m_rcasEnable)
            {
                // for FSR3 rcas 0 (fuzzy) to 1 (sharp)
                m_rcasSharpness -= 0.005f * (pad.IsLeftShoulderPressed() ? 1 : 0);
                m_rcasSharpness += 0.005f * (pad.IsRightShoulderPressed() ? 1 : 0);
                m_rcasSharpness = std::max(std::min(m_rcasSharpness, 1.0f), 0.f);
            }

            // exposure
            m_tonemapperConstants.exposure -= 0.03f * pad.triggers.left;
            m_tonemapperConstants.exposure += 0.03f * pad.triggers.right;
            m_tonemapperConstants.exposure = std::max(std::min(m_tonemapperConstants.exposure, 4.0f), 0.50f);

            // LOD bias
            if (m_gamePadButtons.dpadLeft == ButtonState::HELD)
            {
                m_lodBias -= 0.05f;
            }
            if (m_gamePadButtons.dpadRight == ButtonState::HELD)
            {
                m_lodBias += 0.05f;
            }

            if (m_lodBias < -3.0f) m_lodBias = -3.0f;
            if (m_lodBias > 0.0f) m_lodBias = 0.0f;
        }

        // handle render scale change
        if (m_renderScaleWantsChange)
        {
            m_renderScaleChanged = true;
            m_renderScale = m_pendingRenderScale;

            // As per FSR3 documentation
            if (!m_renderScale) {
                m_lodBias = 0.f;
            }
            else {
                m_lodBias = -log2f(m_renderScaleRatio[m_renderScale]) - 1.0f;
            }

            m_renderScaleWantsChange = false;
        }
        else
        {
            m_renderScaleChanged = false;
        }

        // because of the frame interpolation's interactions with the frame interval settings and pipeline token acquisition
        // process, we must fully re-init everything whenever a significant feature is enabled/disabled
        if (m_resetRequested)
        {
            m_deviceResources->WaitForGpu();
            UpdateFSRContext(false);
            UpdateFSRContext(true);

#ifdef _GAMING_XBOX
            // In case the frame interpolation was enabled/disabled, update Frame Event calls
            m_deviceResources->RegisterFrameEvents(m_frameInterpolation);
#endif // _GAMING_XBOX
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // update the camera
    m_lookAt = XMVectorSet(sinf(m_yaw), m_pitch, cosf(m_yaw), 0);
    m_view = XMMatrixLookToLH(m_eye, m_lookAt, g_XMIdentityR1);

    // update magnifier
    UpdateMagnifier(pad);

    PIXEndEvent();
}

#pragma endregion

void Sample::UpdateMagnifier(DirectX::GamePad::State& pad)
{
    m_magnifierConstants.iMousePos[0] += static_cast<int>(pad.thumbSticks.rightX * 20.0f);
    m_magnifierConstants.iMousePos[1] -= static_cast<int>(pad.thumbSticks.rightY * 20.0f);

    auto const size = m_deviceResources->GetOutputSize();
    m_magnifierConstants.uImageWidth = static_cast<uint32_t>(size.right);
    m_magnifierConstants.uImageHeight = static_cast<uint32_t>(size.bottom);

    m_magnifierConstants.iMousePos[0] = std::max(0, std::min(m_magnifierConstants.iMousePos[0], (int)m_magnifierConstants.uImageWidth));
    m_magnifierConstants.iMousePos[1] = std::max(0, std::min(m_magnifierConstants.iMousePos[1], (int)m_magnifierConstants.uImageHeight));

    const int IMAGE_SIZE[2] = { static_cast<int>(size.right), static_cast<int>(size.bottom) };
    const int& H = IMAGE_SIZE[1];

    const int radiusInPixelsMagifier = static_cast<int>(m_magnifierConstants.fMagnifierScreenRadius * H);
    const int radiusInPixelsMagifiedArea = static_cast<int>(m_magnifierConstants.fMagnifierScreenRadius * H / m_magnifierConstants.fMagnificationAmount);

    const bool bCirclesAreOverlapping = (radiusInPixelsMagifiedArea + radiusInPixelsMagifier) > std::sqrt(m_magnifierConstants.iMagnifierOffset[0] * m_magnifierConstants.iMagnifierOffset[0] + m_magnifierConstants.iMagnifierOffset[1] * m_magnifierConstants.iMagnifierOffset[1]);

    if (bCirclesAreOverlapping) // don't let the two circles overlap
    {
        m_magnifierConstants.iMagnifierOffset[0] = radiusInPixelsMagifiedArea + radiusInPixelsMagifier + 1;
        m_magnifierConstants.iMagnifierOffset[1] = radiusInPixelsMagifiedArea + radiusInPixelsMagifier + 1;
    }

    for (int i = 0; i < 2; ++i) // try to move the magnified area to be fully on screen, if possible
    {
        const bool bMagnifierOutOfScreenRegion = m_magnifierConstants.iMousePos[i] + m_magnifierConstants.iMagnifierOffset[i] + radiusInPixelsMagifier > IMAGE_SIZE[i]
            || m_magnifierConstants.iMousePos[i] + m_magnifierConstants.iMagnifierOffset[i] - radiusInPixelsMagifier < 0;

        if (bMagnifierOutOfScreenRegion)
        {
            if (!(m_magnifierConstants.iMousePos[i] - m_magnifierConstants.iMagnifierOffset[i] + radiusInPixelsMagifier > IMAGE_SIZE[i]
                || m_magnifierConstants.iMousePos[i] - m_magnifierConstants.iMagnifierOffset[i] - radiusInPixelsMagifier < 0))
            {
                // flip offset if possible
                m_magnifierConstants.iMagnifierOffset[i] = -m_magnifierConstants.iMagnifierOffset[i];
            }
            else
            {
                // otherwise clamp
                if (m_magnifierConstants.iMousePos[i] + m_magnifierConstants.iMagnifierOffset[i] + radiusInPixelsMagifier > IMAGE_SIZE[i])
                    m_magnifierConstants.iMagnifierOffset[i] = IMAGE_SIZE[i] - m_magnifierConstants.iMousePos[i] - radiusInPixelsMagifier;
                if (m_magnifierConstants.iMousePos[i] + m_magnifierConstants.iMagnifierOffset[i] - radiusInPixelsMagifier < 0)
                    m_magnifierConstants.iMagnifierOffset[i] = -m_magnifierConstants.iMousePos[i] + radiusInPixelsMagifier;
            }
        }
    }
}

void Sample::UpdateFSRContext(bool enabled)
{
    auto const size = m_deviceResources->GetOutputSize();
    UINT resourceSizeWidth{ static_cast<UINT>(size.right) };
    UINT resourceSizeHeight{ static_cast<UINT>(size.bottom) };

    if (enabled)
    {
#ifdef _GAMING_XBOX

        // setup FSR3 FidelityFX interface for frame interpolation, which is always enabled (disabled via config)
        if (!m_ffxBackendInitialized)
        {
            FfxErrorCode errorCode = 0;

            int effectCounts[] = { 1, 1, 2 };
            for (auto i = 0u; i < FSR3_BACKEND_COUNT; i++)
            {
                const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(size_t(effectCounts[i]));
                void* scratchBuffer = calloc(scratchBufferSize, 1);
                memset(scratchBuffer, 0, scratchBufferSize);
                FfxErrorCode localErr = ffxGetInterfaceDX12(&m_ffxFsr3Backends[i], m_deviceResources->GetD3DDevice(), scratchBuffer, scratchBufferSize, size_t(effectCounts[i]));

                SAMPLE_ASSERT_MESSAGE(m_ffxFsr3Backends[i].fpGetSDKVersion(&m_ffxFsr3Backends[i]) ==
                    FFX_SDK_MAKE_VERSION(1, 1, 4), "FidelityFX Frame Interpolation sample requires linking with a 1.1.4 version SDK backend");

                errorCode |= localErr;
            }

            m_ffxBackendInitialized = (errorCode == FFX_OK);
            SAMPLE_ASSERT(m_ffxBackendInitialized);

            m_fsr3ContextDescription.backendInterfaceSharedResources    = m_ffxFsr3Backends[FSR3_BACKEND_SHARED_RESOURCES];
            m_fsr3ContextDescription.backendInterfaceUpscaling          = m_ffxFsr3Backends[FSR3_BACKEND_UPSCALING];
            m_fsr3ContextDescription.backendInterfaceFrameInterpolation = m_ffxFsr3Backends[FSR3_BACKEND_FRAME_INTERPOLATION];
        }

        // On Xbox before initializing FSR3 context, create transient memory allocators from which ALIASABLE resources are allocated
        m_transientAlloc2MbFsr3 = std::make_unique<FrameTransientResourceMemoryStackAllocator>();
        m_transientAlloc2MbFI = std::make_unique<FrameTransientResourceMemoryStackAllocator>();

        m_transientAlloc64KFsr3 = std::make_unique<FrameTransientResourceMemoryStackAllocator>();
        m_transientAlloc64KFI = std::make_unique<FrameTransientResourceMemoryStackAllocator>();

        m_transientAlloc2MbFsr3->init(FrameTransientResourceMemoryStackAllocator::kDefaultPageCount, MEM_2MB_PAGES);
        m_transientAlloc2MbFI->init(FrameTransientResourceMemoryStackAllocator::kDefaultPageCount, MEM_2MB_PAGES);
        m_transientAlloc64KFsr3->init(FrameTransientResourceMemoryStackAllocator::kDefaultPageCount, MEM_64K_PAGES);
        m_transientAlloc64KFI->init(FrameTransientResourceMemoryStackAllocator::kDefaultPageCount, MEM_64K_PAGES);

        FfxErrorCode errorCode = FFX_OK;

        // always initialize fsr3 context (for frame interpolation) regardless of current mode
        // fsr3 frame interpolation supports running with a different upscaler (not present in current sample)
        {
            m_fsr3ContextDescription.maxRenderSize.width = resourceSizeWidth;
            m_fsr3ContextDescription.maxRenderSize.height = resourceSizeHeight;
            m_fsr3ContextDescription.maxUpscaleSize.width = resourceSizeWidth;
            m_fsr3ContextDescription.maxUpscaleSize.height = resourceSizeHeight;
            m_fsr3ContextDescription.displaySize.width = resourceSizeWidth;
            m_fsr3ContextDescription.displaySize.height = resourceSizeHeight;
            m_fsr3ContextDescription.flags = FFX_FSR3_ENABLE_AUTO_EXPOSURE | FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE |
                                             FFX_FSR3_ENABLE_DEPTH_INVERTED | FFX_FSR3_ENABLE_DEPTH_INFINITE;

            if (m_allowAsyncCompute)
            {
                m_fsr3ContextDescription.flags |= FFX_FSR3_ENABLE_ASYNC_WORKLOAD_SUPPORT;
            }

            // do error checking in debug
#if defined(_DEBUG)
            m_fsr3ContextDescription.flags |= FFX_FSR3_ENABLE_DEBUG_CHECKING;
            m_fsr3ContextDescription.fpMessage = &Sample::FfxMsgCallback;
#endif  // #if defined(_DEBUG)

            // create the FSR3 context
            {
                switch (m_deviceResources->GetFrameBackBufferFormat())
                {
                case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                case DXGI_FORMAT_R8G8B8A8_UNORM:
                    m_fsr3ContextDescription.backBufferFormat = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
                    break;
                case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                case DXGI_FORMAT_B8G8R8A8_UNORM:
                    m_fsr3ContextDescription.backBufferFormat = FFX_SURFACE_FORMAT_B8G8R8A8_UNORM;
                    break;
                case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
                    m_fsr3ContextDescription.backBufferFormat = FFX_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP;
                    break;
                case DXGI_FORMAT_R10G10B10A2_UNORM:
                    m_fsr3ContextDescription.backBufferFormat = FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
                    break;
                default:
                    SAMPLE_ASSERT_MESSAGE(false, "Unhandled format");
                    break;
                }

                // create the context.
                errorCode = ffxFsr3ContextCreate(&m_fsr3Context, &m_fsr3ContextDescription);
                SAMPLE_ASSERT(errorCode == FFX_OK);

                // use our thread-safe buffer allocator instead of the default one
                m_fsr3ContextDescription.backendInterfaceUpscaling.fpRegisterConstantBufferAllocator(&m_fsr3ContextDescription.backendInterfaceUpscaling, Sample::ffxAllocateConstantBuffer);
                m_fsr3ContextDescription.backendInterfaceFrameInterpolation.fpRegisterConstantBufferAllocator(&m_fsr3ContextDescription.backendInterfaceFrameInterpolation, Sample::ffxAllocateConstantBuffer);

                SAMPLE_ASSERT_MESSAGE(ffxFsr3UpscalerGetEffectVersion() == FFX_SDK_MAKE_VERSION(3, 1, 4),
                    "FidelityFX Frame Interpolation sample requires linking with a 3.1.4 version FidelityFX FSR3Upscaler library");
                SAMPLE_ASSERT_MESSAGE(ffxFsr3GetEffectVersion() == FFX_SDK_MAKE_VERSION(3, 1, 4),
                    "FidelityFX Frame Interpolation sample requires linking with a 3.1.4 version FidelityFX FSR3 library");
                SAMPLE_ASSERT_MESSAGE(ffxFrameInterpolationGetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 1, 3),
                    "FidelityFX Frame Interpolation sample requires linking with a 1.1.3 version FidelityFX FrameInterpolation library");
                SAMPLE_ASSERT_MESSAGE(ffxOpticalflowGetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 1, 2),
                    "FidelityFX Frame Interpolation sample requires linking with a 1.1.2 version FidelityFX OpticalFlow library");

                // configure UI composition
                
                // Composition modes: 
                // - 0 (none)
                // - 1 (UITexture)
                // - 2 (UICallback)
                // - 3 (Pre-UI Backbuffer)
                FfxResource hudlessResource = (m_compositionMode == COMP_MODE_HUDLESS_TEXTURE)
                                            ? GetFfxResourceFromD3D12(m_uiRenderTargets[m_currentUITarget].Get(), L"UI Hudless Resource", FFX_RESOURCE_STATE_UNORDERED_ACCESS)
                                            : GetFfxResourceFromNull();

                // Configure frame generation            
                FfxSwapchain ffxSwapChain = m_deviceResources->GetFFXSwapChain();
                
                m_frameGenerationConfig.frameGenerationEnabled = false;
                m_frameGenerationConfig.frameGenerationCallback = [](const FfxFrameGenerationDispatchDescription* desc, void*) -> FfxErrorCode { return ffxFsr3DispatchFrameGeneration(desc); };
                if (m_compositionMode == COMP_MODE_CALLBACK)
                {
                    m_frameGenerationConfig.presentCallback = RenderUI_FrameInterpolationCallback;
                    m_frameGenerationConfig.presentCallbackContext = this;
                }
                else
                {
                    m_frameGenerationConfig.presentCallback = nullptr;
                    m_frameGenerationConfig.presentCallbackContext = nullptr;
                }
                m_frameGenerationConfig.swapChain = ffxSwapChain;
                m_frameGenerationConfig.HUDLessColor = hudlessResource;

                errorCode = ffxFsr3ConfigureFrameGeneration(&m_fsr3Context, &m_frameGenerationConfig);
                SAMPLE_ASSERT(errorCode == FFX_OK);
            }
        }

        // On Xbox we try to reuse memory transient memory used by FSR3Upscale and FrameInterpolation by keeping two allocators for virtual address space
        // and aliasing them with the physical pages
        uint32_t pageCount64Kb = std::max(
            m_transientAlloc64KFsr3->getUsedPageCount(FrameTransientResourceMemoryStackAllocator::k64Kb),
            m_transientAlloc64KFI->getUsedPageCount(FrameTransientResourceMemoryStackAllocator::k64Kb)
        );

        uint32_t pageCount2Mb = std::max(
            m_transientAlloc2MbFsr3->getUsedPageCount(FrameTransientResourceMemoryStackAllocator::k2Mb),
            m_transientAlloc2MbFI->getUsedPageCount(FrameTransientResourceMemoryStackAllocator::k2Mb)
        );

        uint32_t physical64KbPageCountPerVirtual2MbPage = FrameTransientResourceMemoryStackAllocator::k2Mb / FrameTransientResourceMemoryStackAllocator::k64Kb;

        m_transientPhysicalPages64K.resize(pageCount64Kb);
        m_transientPhysicalPages2Mb.resize(pageCount2Mb);
        m_transientPhysicalPages64K_2Mb.resize(pageCount2Mb* physical64KbPageCountPerVirtual2MbPage);

        FrameTransientResourceMemoryStackAllocator::allocatePhysicalPages(m_transientPhysicalPages64K.data(), m_transientPhysicalPages64K.size(), FrameTransientResourceMemoryStackAllocator::k64Kb);
        FrameTransientResourceMemoryStackAllocator::allocatePhysicalPages(m_transientPhysicalPages64K_2Mb.data(), m_transientPhysicalPages64K_2Mb.size(), FrameTransientResourceMemoryStackAllocator::k2Mb);

        for (uint32_t i = 0, j = 0; i < m_transientPhysicalPages64K_2Mb.size(); i += physical64KbPageCountPerVirtual2MbPage, ++j)
        {
            m_transientPhysicalPages2Mb[j] = m_transientPhysicalPages64K_2Mb[i];
        }

        m_transientAlloc64KFsr3->mapPhysical(m_transientPhysicalPages64K.data(), pageCount64Kb);
        m_transientAlloc64KFI->mapPhysical(m_transientPhysicalPages64K.data(), pageCount64Kb);

        m_transientAlloc2MbFsr3->mapPhysical(m_transientPhysicalPages2Mb.data(), pageCount2Mb);
        m_transientAlloc2MbFI->mapPhysical(m_transientPhysicalPages2Mb.data(), pageCount2Mb);

#else // _GAMING_DESKTOP

        // Backend creation (for both FFXAPI contexts, FG and Upscale)
        ffx::CreateBackendDX12Desc backendDesc{};
        backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
        backendDesc.device = m_deviceResources->GetD3DDevice();

        // Create the Upscaler context
        {
            ffx::CreateContextDescUpscale createFsr{};
            createFsr.maxUpscaleSize = { resourceSizeWidth, resourceSizeHeight };
            createFsr.maxRenderSize = { resourceSizeWidth, resourceSizeHeight };
            createFsr.flags = FFX_UPSCALE_ENABLE_AUTO_EXPOSURE | FFX_FSR3_ENABLE_HIGH_DYNAMIC_RANGE |
                              FFX_FSR3_ENABLE_DEPTH_INVERTED | FFX_FSR3_ENABLE_DEPTH_INFINITE;

            // Do error checking in debug
#if defined(_DEBUG)
            createFsr.flags |= FFX_UPSCALE_ENABLE_DEBUG_CHECKING;
            createFsr.fpMessage = &Sample::FfxMsgCallback;
#endif  // #if defined(_DEBUG)

            // Create the FSR context
            {
                ffx::ReturnCode retCode;
                // lifetime of this must last until after CreateContext call!
                ffx::CreateContextDescOverrideVersion versionOverride{};
                retCode = ffx::CreateContext(m_upscalingContext, nullptr, createFsr, backendDesc);

                // Couldn't create the ffxapi upscaling context
                SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);

                // get the version ID
                //Query created version
                ffxQueryGetProviderVersion getVersion = { 0 };
                getVersion.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;

                ffxReturnCode_t retCode_t = ffxQuery(&m_upscalingContext, &getVersion.header);
                SAMPLE_ASSERT(retCode_t == FFX_API_RETURN_OK);

                int requiredSize = MultiByteToWideChar(CP_UTF8, 0, getVersion.versionName, -1, NULL, 0);
                MultiByteToWideChar(CP_UTF8, 0, getVersion.versionName, -1, &m_upscalingVersion[4], requiredSize);
            }

            FfxApiEffectMemoryUsage gpuMemoryUsageUpscaler;
            ffx::QueryDescUpscaleGetGPUMemoryUsage upscalerGetGPUMemoryUsage{};
            upscalerGetGPUMemoryUsage.gpuMemoryUsageUpscaler = &gpuMemoryUsageUpscaler;

            ffx::Query(m_upscalingContext, upscalerGetGPUMemoryUsage);
            wprintf_s(L"Upscaler Context VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageUpscaler.totalUsageInBytes / 1048576.f, gpuMemoryUsageUpscaler.aliasableUsageInBytes / 1048576.f);
        }

        // Create the FrameGen context
        {
            ffx::CreateContextDescFrameGeneration createFg{};
            createFg.displaySize = { resourceSizeWidth, resourceSizeHeight };
            createFg.maxRenderSize = { resourceSizeWidth, resourceSizeHeight };
            createFg.flags = FFX_FRAMEGENERATION_ENABLE_HIGH_DYNAMIC_RANGE |
                             FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED | FFX_FRAMEGENERATION_ENABLE_DEPTH_INFINITE;

            if (m_allowAsyncCompute)
            {
                createFg.flags |= FFX_FRAMEGENERATION_ENABLE_ASYNC_WORKLOAD_SUPPORT;
            }

            switch (m_deviceResources->GetFrameBackBufferFormat())
            {
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                createFg.backBufferFormat = FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
                break;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            case DXGI_FORMAT_B8G8R8A8_UNORM:
                createFg.backBufferFormat = FFX_SURFACE_FORMAT_B8G8R8A8_UNORM;
                break;
            case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
                createFg.backBufferFormat = FFX_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP;
                break;
            case DXGI_FORMAT_R10G10B10A2_UNORM:
                createFg.backBufferFormat = FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
                break;
            default:
                SAMPLE_ASSERT(false && "Unhandled format");
                break;
            }

            // create the context. We can reuse the backend description
            ffx::ReturnCode retCode = ffx::CreateContext(m_frameGenContext, nullptr, createFg, backendDesc);

            //Couldn't create the ffxapi framegen context
            SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);

            void* ffxSwapChain = m_deviceResources->GetSwapChain();

            // configure UI composition

            // Composition modes: 
            // - 0 (none)
            // - 1 (UITexture)
            // - 2 (UICallback)
            // - 3 (Pre-UI Backbuffer)
            FfxApiResource hudlessResource = (m_compositionMode == COMP_MODE_HUDLESS_TEXTURE)
                ? GetFfxResourceFromD3D12(m_uiRenderTargets[m_currentUITarget].Get(), FFX_RESOURCE_STATE_UNORDERED_ACCESS)
                : GetFfxResourceFromNull();

            m_frameGenerationConfig.frameGenerationEnabled = false;
            m_frameGenerationConfig.frameGenerationCallback = [](ffxDispatchDescFrameGeneration* params, void* pUserCtx) -> ffxReturnCode_t
                                                                {
                                                                    return ffxDispatch(reinterpret_cast<ffxContext*>(pUserCtx), &params->header);
                                                                };
            m_frameGenerationConfig.frameGenerationCallbackUserContext = &m_frameGenContext;
            if (m_compositionMode == COMP_MODE_CALLBACK)
            {
                m_frameGenerationConfig.presentCallback = RenderUI_FrameInterpolationCallback;
                m_frameGenerationConfig.presentCallbackUserContext = this;
            }
            else
            {
                m_frameGenerationConfig.presentCallback = nullptr;
                m_frameGenerationConfig.presentCallbackUserContext = nullptr;
            }
            m_frameGenerationConfig.swapChain = ffxSwapChain;
            m_frameGenerationConfig.HUDLessColor = hudlessResource;

            m_frameGenerationConfig.frameID = m_frame;

            retCode = ffx::Configure(m_frameGenContext, m_frameGenerationConfig);

            // Couldn't create the ffxapi upscaling context
            SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);

            FfxApiEffectMemoryUsage gpuMemoryUsageFrameGeneration;
            ffx::QueryDescFrameGenerationGetGPUMemoryUsage frameGenGetGPUMemoryUsage{};
            frameGenGetGPUMemoryUsage.gpuMemoryUsageFrameGeneration = &gpuMemoryUsageFrameGeneration;
            ffx::Query(m_frameGenContext, frameGenGetGPUMemoryUsage);
            wprintf_s(L"FrameGeneration Context VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageFrameGeneration.totalUsageInBytes / 1048576.f, gpuMemoryUsageFrameGeneration.aliasableUsageInBytes / 1048576.f);
        }

        FfxApiEffectMemoryUsage gpuMemoryUsageFrameGenerationSwapchain;
        ffx::QueryFrameGenerationSwapChainGetGPUMemoryUsageDX12 frameGenSwapchainGetGPUMemoryUsage{};
        frameGenSwapchainGetGPUMemoryUsage.gpuMemoryUsageFrameGenerationSwapchain = &gpuMemoryUsageFrameGenerationSwapchain;
        ffx::Query(m_deviceResources->GetFFXSwapChainContext(), frameGenSwapchainGetGPUMemoryUsage);
        wprintf_s(L"Swapchain Context VRAM totalUsageInBytes %f MB aliasableUsageInBytes %f MB", gpuMemoryUsageFrameGenerationSwapchain.totalUsageInBytes / 1048576.f, gpuMemoryUsageFrameGenerationSwapchain.aliasableUsageInBytes / 1048576.f);

#endif // _GAMING_XBOX   
    }
    else
    {
#ifdef _GAMING_XBOX

        FfxSwapchain ffxSwapChain = m_deviceResources->GetFFXSwapChain();

        // disable frame generation before destroying contexts
        // also unset present callback, HUDLessColor and UiTexture to have the swapchain only present the backbuffer
        m_frameGenerationConfig.frameGenerationEnabled = false;
        m_frameGenerationConfig.swapChain = ffxSwapChain;
        m_frameGenerationConfig.presentCallback = nullptr;
        m_frameGenerationConfig.HUDLessColor = FfxResource({});
        ffxFsr3ConfigureFrameGeneration(&m_fsr3Context, &m_frameGenerationConfig);

        // always destroy fsr3 context
        ffxFsr3ContextDestroy(&m_fsr3Context);

        // On Xbox after destroying FSR3 context, clear transient memory allocators from which ALIASABLE resources are allocated
        m_transientAlloc2MbFI->unmapPhysical(m_transientPhysicalPages2Mb.size());
        m_transientAlloc2MbFsr3->unmapPhysical(m_transientPhysicalPages2Mb.size());
        m_transientAlloc64KFI->unmapPhysical(m_transientPhysicalPages64K.size());
        m_transientAlloc64KFsr3->unmapPhysical(m_transientPhysicalPages64K.size());

        FrameTransientResourceMemoryStackAllocator::freePhysicalPages(m_transientPhysicalPages64K_2Mb.data(), m_transientPhysicalPages64K_2Mb.size());
        FrameTransientResourceMemoryStackAllocator::freePhysicalPages(m_transientPhysicalPages64K.data(), m_transientPhysicalPages64K.size());

        m_transientPhysicalPages64K_2Mb.clear();
        m_transientPhysicalPages2Mb.clear();
        m_transientPhysicalPages64K.clear();

        m_transientAlloc64KFI->shutdown();
        m_transientAlloc64KFsr3->shutdown();
        m_transientAlloc2MbFI->shutdown();
        m_transientAlloc2MbFsr3->shutdown();

        m_transientAlloc64KFI.reset();
        m_transientAlloc64KFsr3.reset();
        m_transientAlloc2MbFI.reset();
        m_transientAlloc2MbFsr3.reset();

        // destroy the FidelityFX interface memory
        if (m_ffxBackendInitialized == true)
        {
            for (auto i = 0u; i < FSR3_BACKEND_COUNT; i++)
            {
                free(m_ffxFsr3Backends[i].scratchBuffer);
                m_ffxFsr3Backends[i].scratchBuffer = nullptr;
            }
            m_ffxBackendInitialized = false;
        }

#else // _GAMING_DESKTOP

        void* ffxSwapChain = m_deviceResources->GetSwapChain();

        // disable frame generation before destroying context
        // also unset present callback, HUDLessColor and UiTexture to have the swapchain only present the backbuffer
        m_frameGenerationConfig.frameGenerationEnabled = false;
        m_frameGenerationConfig.swapChain = ffxSwapChain;
        m_frameGenerationConfig.presentCallback = nullptr;
        m_frameGenerationConfig.HUDLessColor = FfxApiResource({});
        ffx::Configure(m_frameGenContext, m_frameGenerationConfig);

        ffx::ConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12 uiConfig{};
        uiConfig.uiResource = {};
        uiConfig.flags = 0;
        ffx::Configure(m_deviceResources->GetFFXSwapChainContext(), uiConfig);

        // Destroy the contexts
        if (m_upscalingContext)
        {
            ffx::DestroyContext(m_upscalingContext);
            m_upscalingContext = nullptr;
        }
        ffx::DestroyContext(m_frameGenContext);

#endif // _GAMING_XBOX
    }
}

#ifdef _GAMING_XBOX
void Sample::FfxMsgCallback(FfxMsgType type, const wchar_t* message)
#else // _GAMING_DESKTOP
void Sample::FfxMsgCallback(uint32_t type, const wchar_t* message)
#endif // _GAMING_XBOX
{
    if (type == FFX_MESSAGE_TYPE_ERROR)
    {
        wprintf(L"FSR_API_DEBUG_ERROR: %ls", message);
    }
    else if (type == FFX_MESSAGE_TYPE_WARNING)
    {
        wprintf(L"FSR_API_DEBUG_WARNING: %ls", message);
    }
}

void Sample::SetProjectionJitter(uint32_t width, uint32_t height)
{
    // only apply jitter if our Upscale mode is FSR2/3
    auto const displaysize = m_deviceResources->GetOutputSize();
    static int32_t jitterIndex = 0;

#ifdef _GAMING_XBOX

    const int32_t jitterPhaseCount = ffxFsr3GetJitterPhaseCount(static_cast<int32_t>(width), displaysize.right);
    ffxFsr3GetJitterOffset(&m_jitterX, &m_jitterY, jitterIndex, jitterPhaseCount);

#else // _GAMING_DESKTOP

    ffx::ReturnCode retCode;
    int32_t jitterPhaseCount;
    ffx::QueryDescUpscaleGetJitterPhaseCount getJitterPhaseDesc{};
    getJitterPhaseDesc.displayWidth = static_cast<uint32_t>(displaysize.right);
    getJitterPhaseDesc.renderWidth = width;
    getJitterPhaseDesc.pOutPhaseCount = &jitterPhaseCount;

    retCode = ffx::Query(m_upscalingContext, getJitterPhaseDesc);
    SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);

    ffx::QueryDescUpscaleGetJitterOffset getJitterOffsetDesc{};
    getJitterOffsetDesc.index = jitterIndex;
    getJitterOffsetDesc.phaseCount = jitterPhaseCount;
    getJitterOffsetDesc.pOutX = &m_jitterX;
    getJitterOffsetDesc.pOutY = &m_jitterY;

    retCode = ffx::Query(m_upscalingContext, getJitterOffsetDesc);
    SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);

#endif // _GAMING_XBOX

    jitterIndex++; // increase jitter index for next frame
    DirectX::XMMATRIX jitterTranslation = XMMatrixTranslation(2.0f * m_jitterX / (float)width, -2.0f * m_jitterY / (float)height, 0.0f);
    m_proj = m_proj * jitterTranslation;
}

void Sample::RenderShadowMaps(ID3D12GraphicsCommandList* commandList)
{
    // Setup shadows
    // Set shadowmaps bias and an index that indicates the rectangle of the atlas in which depth will be rendered
    uint32_t shadowMapIndex = 0;
    for (uint32_t i = 0; i < m_gltfModel->GetCommonPerFrameData().lightCount; i++)
    {
        if ((shadowMapIndex < 4) && (m_gltfModel->GetCommonPerFrameData().lights[i].type == tfLight::LIGHT_SPOTLIGHT))
        {
            m_gltfModel->GetCommonPerFrameData().lights[i].shadowMapIndex = shadowMapIndex++; // set the shadowmap index
            m_gltfModel->GetCommonPerFrameData().lights[i].depthBias = 1500.0f / 100000.0f;
        }
        else if ((shadowMapIndex < 4) && (m_gltfModel->GetCommonPerFrameData().lights[i].type == tfLight::LIGHT_DIRECTIONAL))
        {
            m_gltfModel->GetCommonPerFrameData().lights[i].shadowMapIndex = shadowMapIndex++; // set the shadowmap index
            m_gltfModel->GetCommonPerFrameData().lights[i].depthBias = 1000.0f / 100000.0f;
        }
        else
        {
            m_gltfModel->GetCommonPerFrameData().lights[i].shadowMapIndex = 0xffffffffu;   // no shadow for this light
        }
    }

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Shadow Atlas");

    TransitionResource(commandList, m_shadowAtlas.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->ClearDepthStencilView(m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV), D3D12_CLEAR_FLAG_DEPTH,
        1.0f, 0, 0, nullptr);

    for (uint32_t i = 0; i < m_gltfModel->GetCommonPerFrameData().lightCount; i++)
    {
        if (m_gltfModel->GetCommonPerFrameData().lights[i].type != tfLight::LIGHT_DIRECTIONAL
            && m_gltfModel->GetCommonPerFrameData().lights[i].type != tfLight::LIGHT_SPOTLIGHT)
            continue;

        if (m_gltfModel->GetCommonPerFrameData().lights[i].shadowMapIndex == 0xffffffffu) continue;

        // Set the RT's quadrant where to render the shadomap (these viewport offsets need to match the ones in shadowFiltering.h)
        uint32_t viewportOffsetsX[4] = { 0, 1, 0, 1 };
        uint32_t viewportOffsetsY[4] = { 0, 0, 1, 1 };
        uint32_t viewportWidth = (2 * 2048) / 2;
        uint32_t viewportHeight = (2 * 2048) / 2;

        uint32_t topLeftX = viewportOffsetsX[m_gltfModel->GetCommonPerFrameData().lights[i].shadowMapIndex] * viewportWidth;
        uint32_t topLeftY = viewportOffsetsY[m_gltfModel->GetCommonPerFrameData().lights[i].shadowMapIndex] * viewportHeight;

        // Set the viewport
        const D3D12_VIEWPORT viewPort = { static_cast<float>(topLeftX), static_cast<float>(topLeftY), static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), 0.0f, 1.0f };
        commandList->RSSetViewports(1, &viewPort);

        // Create scissor rectangle
        const D3D12_RECT rectScissor = { (LONG)topLeftX, (LONG)topLeftY, (LONG)(topLeftX + viewportWidth), (LONG)(topLeftY + viewportHeight) };
        commandList->RSSetScissorRects(1, &rectScissor);

        auto shadowAtlasCPUHandle{ m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV) };
        commandList->OMSetRenderTargets(0, NULL, true, &shadowAtlasCPUHandle);

        AMDTK::GltfDepthPass::per_frame perPassConstants;

        perPassConstants.mViewProj = m_gltfModel->GetCommonPerFrameData().lights[i].mLightViewProj;

        m_gltfDepth->SetPerFrameCB(m_graphicsMemory->AllocateConstant(perPassConstants).GpuAddress());

        m_gltfDepth->Draw(commandList);
    }

    TransitionResource(commandList, m_shadowAtlas.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    PIXEndEvent(commandList);
}


void Sample::RenderMotionVectors(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Motion Vectors Pass");

    TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    TransitionTo(commandList, m_motionVectors, &m_motionVectorsState, D3D12_RESOURCE_STATE_RENDER_TARGET);

    commandList->ClearDepthStencilView(m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDepthDSV), D3D12_CLEAR_FLAG_DEPTH,
        (c_reverseDepth) ? 0.0f : 1.0f, 0, 0, nullptr);

    const float clearValues[4] = { 0.0f , 0.0f , 0.0f , 0.0f };
    commandList->ClearRenderTargetView(m_renderDescriptors->GetCpuHandle(RTDescriptors::MotionVectorsRTV), &clearValues[0], 0, nullptr);

    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();

    scissorRect = GetScaledRect(scissorRect);
    viewport.Width = static_cast<float>(scissorRect.right);
    viewport.Height = static_cast<float>(scissorRect.bottom);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[1];
    rtvs[0] = m_renderDescriptors->GetCpuHandle(RTDescriptors::MotionVectorsRTV);

    auto motionDepthCPUHandle{ m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDepthDSV) };

    commandList->OMSetRenderTargets(1, &rtvs[0], false, &motionDepthCPUHandle);

    AMDTK::GltfMotionVectorsPass::per_frame perPassConstants;
    perPassConstants.mCurrViewProj = m_currentViewProj;
    perPassConstants.mPrevViewProj = (m_prevView * m_proj);
    perPassConstants.normalizedFormat = 1;

    m_gltfMotionVectors->SetPerFrameCB(m_graphicsMemory->AllocateConstant(perPassConstants).GpuAddress());

    m_gltfMotionVectors->Draw(commandList);
    PIXEndEvent(commandList);
}

void Sample::RenderParticlesIntoScene(ID3D12GraphicsCommandList* commandList)
{
    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Compose Particles&Reactivity");

    TransitionTo(commandList, m_gltfScene, &m_gltfSceneState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    TransitionTo(commandList, m_particles, &m_particlesState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionTo(commandList, m_reactive, &m_reactiveState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    auto const size = m_deviceResources->GetOutputSize();

    commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
    commandList->SetComputeRootSignature(m_resolveParticleRS.Get());

    m_resolveParticleConstants.factor = 1.0f;

    commandList->SetComputeRootConstantBufferView(0, m_graphicsMemory->AllocateConstant(m_resolveParticleConstants).GpuAddress());
    commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::ParticlesSRV)); // particles
    commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::GltfSceneUAV));   // Scene
    commandList->SetComputeRootDescriptorTable(3, m_resourceDescriptors->GetGpuHandle(Descriptors::FSR3ReactiveUAV)); // Output
    commandList->SetPipelineState(m_resolveParticlePSO.Get());

    constexpr unsigned int threadGroupWorkRegionDim = 8u;
    unsigned int dispatchX = (size.right + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;
    unsigned int dispatchY = (size.bottom + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;

    commandList->Dispatch(dispatchX, dispatchY, 1u);

    TransitionTo(commandList, m_gltfScene, &m_gltfSceneState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    PIXEndEvent(commandList);
}

void Sample::RenderTonemapping(ID3D12GraphicsCommandList* commandList)
{
    // Tonemapping CS - also outputs Gamma2 space

    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"TonemappingCS");
    m_tonemapperConstants.mode = 0;
    m_tonemapperConstants.gamma2 = 1;
    m_tonemapperConstants.scale = 1.0f;

    auto cbHandle = m_graphicsMemory->AllocateConstant(m_tonemapperConstants);
    commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
    commandList->SetComputeRootSignature(m_tonemapperCSRS.Get());
    commandList->SetComputeRootConstantBufferView(0, cbHandle.GpuAddress());

    TransitionTo(commandList, m_upsampleOutput, &m_upsampleOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::Upsample_OutputUAV));
    
    TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::Tonemap_OutputUAV));
    commandList->SetPipelineState(m_tonemapperCSPSO.Get());

    auto size = m_deviceResources->GetOutputSize();

    const unsigned int threadGroupWorkRegionDim = 8u;
    unsigned int dispatchX = (size.right + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;
    unsigned int dispatchY = (size.bottom + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;

    commandList->Dispatch(dispatchX, dispatchY, 1u);
    PIXEndEvent(commandList);
}

void Sample::RenderGTLFScene(ID3D12GraphicsCommandList* commandList)
{
    TransitionTo(commandList, m_gltfScene, &m_gltfSceneState, D3D12_RESOURCE_STATE_RENDER_TARGET);

    Clear();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GLTF PBR Pass");
    m_gltfPBR->Draw(commandList, m_gltfResources.GetSrvPile()->GetGpuHandle(m_shadowAtlasIdx));
    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Particles");
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
        D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = {};

        rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::ParticlesRTV);
        dsvDescriptor = m_deviceResources->GetDepthStencilView();

        commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);

        // Render the particle system
        m_particleSystem.Render(commandList, m_renderScaleRatio[(int)GetRenderScale()]);
    }
    // TransitionTo(commandList, m_gltfScene, &m_gltfSceneState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    TransitionTo(commandList, m_particles, &m_particlesState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    PIXEndEvent(commandList);
}

void Sample::UpdateGLTFState()
{
    m_gltfModel->GetCommonPerFrameData().iblFactor = 0.80f;
    m_gltfModel->GetCommonPerFrameData().emmisiveFactor = 0.9f;

    m_gltfModel->SetAnimationTime(0, 0);
    m_gltfModel->TransformScene(0, XMMatrixScaling(-1, 1, 1));

    m_currentViewProj = m_view * m_proj;

    m_gltfModel->GetCommonPerFrameData().lodBias = m_lodBias;
    
    m_gltfModel->GetCommonPerFrameData().mCameraViewProj = m_currentViewProj;
    m_gltfModel->GetCommonPerFrameData().mInverseCameraViewProj = XMMatrixInverse(nullptr, m_gltfModel->GetCommonPerFrameData().mCameraViewProj);
    m_gltfModel->GetCommonPerFrameData().cameraPos = m_eye;
    m_gltfModel->UpdatePerFrameLights();
    m_gltfResources.SetSkinningMatricesForSkeletons();
    m_gltfResources.SetPerFrameConstants();

}

void Sample::RenderMagnifier(ID3D12GraphicsCommandList* commandList)
{
    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Magnifier");

    TransitionTo(commandList, m_magnifierOutput, &m_magnifierOutputState, D3D12_RESOURCE_STATE_RENDER_TARGET);
    TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
    rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::MagnifierOutputRTV);

    commandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    auto cbHandle = m_graphicsMemory->AllocateConstant(m_magnifierConstants);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
    commandList->SetGraphicsRootSignature(m_magnifierRS.Get());
    commandList->SetGraphicsRootConstantBufferView(0, cbHandle.GpuAddress());

    TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::Tonemap_OutputSRV));
    
    commandList->SetPipelineState(m_magnifierPSO.Get());
    commandList->DrawInstanced(3, 1, 0, 0);
    PIXEndEvent(commandList);
}

void Sample::RenderColorConversion(ID3D12GraphicsCommandList* commandList)
{
    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    // Color conversion (if in SDR mode, simply gamma2 conversion back occurs)
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Color Conversion");

    // Pick the right input
    if (!m_hideMagnifier)
    {
        TransitionTo(commandList, m_magnifierOutput, &m_magnifierOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }
    else
    {
        TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    }

    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    m_colorConversionConstants.gamma2 = 1;
    auto cbHandle = m_graphicsMemory->AllocateConstant(m_colorConversionConstants);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
    commandList->SetGraphicsRootSignature(m_colorConversionRS.Get());
    commandList->SetGraphicsRootConstantBufferView(0, cbHandle.GpuAddress());
    
    // Set the right input
    if (!m_hideMagnifier)
    {
        commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::MagnifierOutputSRV));
    }
    else
    {
        commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::Tonemap_OutputSRV));
    }
    
    commandList->SetPipelineState(m_colorConversionPSO.Get());
    commandList->DrawInstanced(3, 1, 0, 0);

    PIXEndEvent(commandList);
}

void Sample::RenderFSR3(ID3D12GraphicsCommandList* commandList)
{
    TransitionTo(commandList, m_gltfScene, &m_gltfSceneState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionTo(commandList, m_motionVectors, &m_motionVectorsState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionTo(commandList, m_upsampleOutput, &m_upsampleOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionTo(commandList, m_reactive, &m_reactiveState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"FidelityFX Super Resolution 3.0.1");
    m_gpuTimer->Start(commandList, 1);

    auto size = m_deviceResources->GetOutputSize();
    auto scaledSize = GetScaledRect(size);
    float inWidth = static_cast<float>(scaledSize.right);
    float inHeight = static_cast<float>(scaledSize.bottom);

#ifdef _GAMING_XBOX

    FfxResource hudlessResource = (m_compositionMode == COMP_MODE_HUDLESS_TEXTURE)
                                ? GetFfxResourceFromD3D12(m_uiRenderTargets[m_currentUITarget].Get(), L"UI Hudless Resource", FFX_RESOURCE_STATE_UNORDERED_ACCESS)
                                : GetFfxResourceFromNull();

    // Update frame generation config
    m_frameGenerationConfig.swapChain = m_deviceResources->GetFFXSwapChain();
    m_frameGenerationConfig.frameGenerationEnabled = m_frameInterpolation;
    m_frameGenerationConfig.flags = 0;
    m_frameGenerationConfig.flags |= m_drawDebugTearLines ? FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0;
    m_frameGenerationConfig.flags |= m_drawDebugView ? FFX_FSR3_FRAME_GENERATION_FLAG_DRAW_DEBUG_VIEW : 0;
    m_frameGenerationConfig.HUDLessColor = hudlessResource;
    m_frameGenerationConfig.allowAsyncWorkloads = m_allowAsyncCompute;
    m_frameGenerationConfig.allowAsyncPresent = m_asyncComputePresent;
    if (m_compositionMode == COMP_MODE_CALLBACK)
    {
        m_frameGenerationConfig.presentCallback = RenderUI_FrameInterpolationCallback;
        m_frameGenerationConfig.presentCallbackContext = this;
    }
    else
    {
        m_frameGenerationConfig.presentCallback = nullptr;
        m_frameGenerationConfig.presentCallbackContext = nullptr;
    }

    // assume symmetric letterbox (change this to allow letterboxing)
    m_frameGenerationConfig.interpolationRect.left = 0;
    m_frameGenerationConfig.interpolationRect.top = 0;
    m_frameGenerationConfig.interpolationRect.width = size.right;
    m_frameGenerationConfig.interpolationRect.height = size.bottom;
    m_frameGenerationConfig.frameID = m_frame;
    
    if (m_frameGenerationCallback)
    {
        m_frameGenerationConfig.frameGenerationCallback = [](const FfxFrameGenerationDispatchDescription* desc, void*) -> FfxErrorCode { return ffxFsr3DispatchFrameGeneration(desc); };
    }
    else
    {
        m_frameGenerationConfig.frameGenerationCallback = nullptr;
    }
    m_frameGenerationConfig.onlyPresentInterpolated = m_presentInterpolatedOnly;

    ffxFsr3ConfigureFrameGeneration(&m_fsr3Context, &m_frameGenerationConfig);

    // FSR3: for other upscalers FSR3 is initialized with FFX_FSR3_ENABLE_INTERPOLATION_ONLY so in that case this will only prepare the data for interpolation
    // if interpolation is disabled it will do nothing

    FfxFsr3DispatchUpscaleDescription dispatchParametersUpscaling = {};
    dispatchParametersUpscaling.frameID = m_frame;
    dispatchParametersUpscaling.commandList     = ffxGetCommandListDX12(commandList);
    dispatchParametersUpscaling.color           = GetFfxResourceFromD3D12(m_gltfScene.Get(),            L"FSR3Upscaler_InputColor",         FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchParametersUpscaling.depth           = GetFfxResourceFromD3D12(m_motionVectorDepth.Get(),    L"FSR3Upscaler_InputDepth",         FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchParametersUpscaling.motionVectors   = GetFfxResourceFromD3D12(m_motionVectors.Get(),        L"FSR3Upscaler_InputMotionVectors", FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchParametersUpscaling.exposure        = GetFfxResourceFromD3D12(nullptr,                      L"FSR3Upscaler_InputExposure",      FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchParametersUpscaling.upscaleOutput   = GetFfxResourceFromD3D12(m_upsampleOutput.Get(),       L"FSR3_UpsampledOutput",            FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchParametersUpscaling.flags = 0;
    
    // Uncomment this line to enable FSR3 upscaler debug view draw
    //dispatchParametersUpscaling.flags |= FFX_FSR3_UPSCALER_FLAG_DRAW_DEBUG_VIEW;

	if (m_useFSRReactiveMask)
	{
        dispatchParametersUpscaling.reactive = GetFfxResourceFromD3D12(m_reactive.Get(), L"FSR3Upscaler_InputReactiveMap",   FFX_RESOURCE_STATE_COMPUTE_READ);
	}
	else
	{
        dispatchParametersUpscaling.reactive = GetFfxResourceFromD3D12(nullptr, L"FSR3Upscaler_EmptyInputReactiveMap", FFX_RESOURCE_STATE_COMPUTE_READ);
	}

	// Sample does not make use of T&C mask
    dispatchParametersUpscaling.transparencyAndComposition = GetFfxResourceFromD3D12(nullptr, L"FSR3Upscaler_EmptyTransparencyAndCompositionMap", FFX_RESOURCE_STATE_COMPUTE_READ);

    dispatchParametersUpscaling.jitterOffset.x      = m_jitterX;
    dispatchParametersUpscaling.jitterOffset.y      = m_jitterY;
    dispatchParametersUpscaling.motionVectorScale.x = inWidth;
    dispatchParametersUpscaling.motionVectorScale.y = inHeight;
    dispatchParametersUpscaling.reset               = m_renderScaleChanged || m_resetRequested;
    dispatchParametersUpscaling.enableSharpening    = m_rcasEnable;
    dispatchParametersUpscaling.sharpness           = m_rcasSharpness;

    // frame time in milliseconds
    dispatchParametersUpscaling.frameTimeDelta      = (float)m_deltaTime;

    dispatchParametersUpscaling.preExposure         = 1.0f;
    dispatchParametersUpscaling.renderSize.width    = (unsigned int)inWidth;
    dispatchParametersUpscaling.renderSize.height   = (unsigned int)inHeight;
    dispatchParametersUpscaling.upscaleSize.width   = (unsigned int)size.right;
    dispatchParametersUpscaling.upscaleSize.height  = (unsigned int)size.bottom;
    
    dispatchParametersUpscaling.cameraFovAngleVertical = XM_PIDIV4;
    dispatchParametersUpscaling.cameraFar           = FLT_MAX;
    dispatchParametersUpscaling.cameraNear          = 0.1f;

    FfxErrorCode errorCode = ffxFsr3ContextDispatchUpscale(&m_fsr3Context, &dispatchParametersUpscaling);
    SAMPLE_ASSERT(errorCode == FFX_OK);

    FfxSwapchain ffxSwapChain = m_deviceResources->GetFFXSwapChain();

    if (m_frameInterpolation)
    {
        FfxFsr3DispatchFrameGenerationPrepareDescription frameGenerationPrepareDispatchParams{};
        frameGenerationPrepareDispatchParams.cameraFar                  = dispatchParametersUpscaling.cameraFar;
        frameGenerationPrepareDispatchParams.cameraFovAngleVertical     = dispatchParametersUpscaling.cameraFovAngleVertical;
        frameGenerationPrepareDispatchParams.cameraNear                 = dispatchParametersUpscaling.cameraNear;
        frameGenerationPrepareDispatchParams.commandList                = dispatchParametersUpscaling.commandList;
        frameGenerationPrepareDispatchParams.depth                      = dispatchParametersUpscaling.depth;
        frameGenerationPrepareDispatchParams.frameID                    = m_frame;
        frameGenerationPrepareDispatchParams.frameTimeDelta             = dispatchParametersUpscaling.frameTimeDelta;
        frameGenerationPrepareDispatchParams.jitterOffset               = dispatchParametersUpscaling.jitterOffset;
        frameGenerationPrepareDispatchParams.motionVectors              = dispatchParametersUpscaling.motionVectors;
        frameGenerationPrepareDispatchParams.motionVectorScale          = dispatchParametersUpscaling.motionVectorScale;
        frameGenerationPrepareDispatchParams.renderSize                 = dispatchParametersUpscaling.renderSize;
        frameGenerationPrepareDispatchParams.viewSpaceToMetersFactor    = dispatchParametersUpscaling.viewSpaceToMetersFactor;

        errorCode = ffxFsr3ContextDispatchFrameGenerationPrepare(&m_fsr3Context, &frameGenerationPrepareDispatchParams);
        SAMPLE_ASSERT(errorCode == FFX_OK);
    }

    // dispatch frame generation, if not using the callback
    if (m_frameInterpolation && !m_frameGenerationCallback)
    {
        FfxFrameGenerationDispatchDescription fgDesc{};
        ffxGetFrameinterpolationCommandlistDX12(ffxSwapChain, fgDesc.commandList);

        FfxResource realBackBuffer = GetFfxResourceFromD3D12(m_deviceResources->GetFrameRenderTarget(), L"Real Back Buffer Surface", FFX_RESOURCE_STATE_PRESENT);
        FfxResource interpolatedBackBuffer = GetFfxResourceFromD3D12(m_deviceResources->GetInterpolatedRenderTarget(), L"Interpolated Back Buffer Surface", FFX_RESOURCE_STATE_PRESENT);

        fgDesc.presentColor = realBackBuffer;
        fgDesc.numInterpolatedFrames = 1;
        fgDesc.outputs[0] = interpolatedBackBuffer;

        // this can be used to apply letterboxing effects
        fgDesc.interpolationRect.left = 0;
        fgDesc.interpolationRect.top = 0;
        fgDesc.interpolationRect.width = size.right;
        fgDesc.interpolationRect.height = size.bottom;
        fgDesc.frameID = m_frame;
        ffxFsr3DispatchFrameGeneration(&fgDesc);
    }

#else // _GAMING_DESKTOP

    // FSR3 Upscaling
    {
        ffx::DispatchDescUpscale dispatchUpscale{};
        dispatchUpscale.commandList = commandList;

        dispatchUpscale.color           = GetFfxResourceFromD3D12(m_gltfScene.Get(), FFX_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.depth           = GetFfxResourceFromD3D12(m_motionVectorDepth.Get(), FFX_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.motionVectors   = GetFfxResourceFromD3D12(m_motionVectors.Get(), FFX_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.exposure        = GetFfxResourceFromD3D12(nullptr, FFX_RESOURCE_STATE_COMPUTE_READ);
        dispatchUpscale.output          = GetFfxResourceFromD3D12(m_upsampleOutput.Get(), FFX_RESOURCE_STATE_COMPUTE_READ);

        if (m_useFSRReactiveMask)
        {
            dispatchUpscale.reactive = GetFfxResourceFromD3D12(m_reactive.Get(), FFX_RESOURCE_STATE_COMPUTE_READ);
        }
        else
        {
            dispatchUpscale.reactive = GetFfxResourceFromD3D12(nullptr, FFX_RESOURCE_STATE_COMPUTE_READ);
        }

        // Sample does not make use of T&C mask
        dispatchUpscale.transparencyAndComposition = GetFfxResourceFromD3D12(nullptr, FFX_RESOURCE_STATE_COMPUTE_READ);

        // Jitter is calculated earlier in the frame using a callback from the camera update
        dispatchUpscale.jitterOffset.x = m_jitterX;
        dispatchUpscale.jitterOffset.y = m_jitterY;
        dispatchUpscale.motionVectorScale.x = inWidth;
        dispatchUpscale.motionVectorScale.y = inHeight;
        dispatchUpscale.reset = m_renderScaleChanged || m_resetRequested;
        dispatchUpscale.enableSharpening = m_rcasEnable;
        dispatchUpscale.sharpness = m_rcasSharpness;

        // frame time in milliseconds
        dispatchUpscale.frameTimeDelta = (float)m_deltaTime;

        dispatchUpscale.preExposure = 1.0f;
        dispatchUpscale.renderSize.width   = (unsigned int)inWidth;
        dispatchUpscale.renderSize.height  = (unsigned int)inHeight;
        dispatchUpscale.upscaleSize.width  = (unsigned int)size.right;
        dispatchUpscale.upscaleSize.height = (unsigned int)size.bottom;

        // Setup camera params as required
        dispatchUpscale.cameraFovAngleVertical = XM_PIDIV4;
        dispatchUpscale.cameraFar = FLT_MAX;
        dispatchUpscale.cameraNear = 0.1f;
        
        dispatchUpscale.flags = 0;

        // Uncomment this line to enable FSR3 upscaler debug view draw
        //dispatchUpscale.flags |= FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW;

        ffx::ReturnCode retCode = ffx::Dispatch(m_upscalingContext, dispatchUpscale);
        SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);
    }

    // Frame generation
    {
        ffx::DispatchDescFrameGenerationPrepare dispatchFgPrep{};
        dispatchFgPrep.commandList = commandList;
        dispatchFgPrep.depth = GetFfxResourceFromD3D12(m_motionVectorDepth.Get(), FFX_RESOURCE_STATE_COMPUTE_READ);
        dispatchFgPrep.motionVectors = GetFfxResourceFromD3D12(m_motionVectors.Get(), FFX_RESOURCE_STATE_COMPUTE_READ);
        dispatchFgPrep.flags = 0;

        dispatchFgPrep.jitterOffset.x = m_jitterX;
        dispatchFgPrep.jitterOffset.y = m_jitterY;
        dispatchFgPrep.motionVectorScale.x = inWidth;
        dispatchFgPrep.motionVectorScale.y = inHeight;

        // frame time in milliseconds
        dispatchFgPrep.frameTimeDelta = (float)m_deltaTime;

        dispatchFgPrep.renderSize.width       = (unsigned int)inWidth;
        dispatchFgPrep.renderSize.height      = (unsigned int)inHeight;
        dispatchFgPrep.cameraFovAngleVertical = XM_PIDIV4;
        dispatchFgPrep.cameraFar = FLT_MAX;
        dispatchFgPrep.cameraNear = 0.1f;
        dispatchFgPrep.viewSpaceToMetersFactor = 0.f;
        dispatchFgPrep.frameID = m_frame;

        // Update frame generation config
        FfxApiResource hudlessResource = (m_compositionMode == COMP_MODE_HUDLESS_TEXTURE)
            ? GetFfxResourceFromD3D12(m_uiRenderTargets[m_currentUITarget].Get(), FFX_RESOURCE_STATE_UNORDERED_ACCESS)
            : GetFfxResourceFromNull();

        m_frameGenerationConfig.frameGenerationEnabled = m_frameInterpolation;
        m_frameGenerationConfig.flags = 0;
        m_frameGenerationConfig.flags |= m_drawDebugTearLines ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_TEAR_LINES : 0;
        // Uncomment to visually debug Reset Indicator
        //m_frameGenerationConfig.flags |= FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_RESET_INDICATORS;
        m_frameGenerationConfig.flags |= m_drawDebugView ? FFX_FRAMEGENERATION_FLAG_DRAW_DEBUG_VIEW : 0;
        dispatchFgPrep.flags = m_frameGenerationConfig.flags;
        m_frameGenerationConfig.HUDLessColor = hudlessResource;
        m_frameGenerationConfig.allowAsyncWorkloads = m_allowAsyncCompute;

        // assume symmetric letterbox (change this to allow letter boxing)
        m_frameGenerationConfig.generationRect.left   = 0;
        m_frameGenerationConfig.generationRect.top    = 0;
        m_frameGenerationConfig.generationRect.width  = size.right;
        m_frameGenerationConfig.generationRect.height = size.bottom;

        if (m_frameGenerationCallback)
        {
            m_frameGenerationConfig.frameGenerationCallback = [](ffxDispatchDescFrameGeneration* params, void* pUserCtx) -> ffxReturnCode_t
            {
                return ffxDispatch(reinterpret_cast<ffxContext*>(pUserCtx), &params->header);
            };
            m_frameGenerationConfig.frameGenerationCallbackUserContext = &m_frameGenContext;

            if (m_compositionMode == COMP_MODE_CALLBACK)
            {
                m_frameGenerationConfig.presentCallback = RenderUI_FrameInterpolationCallback;
                m_frameGenerationConfig.presentCallbackUserContext = this;
            }
            else
            {
                m_frameGenerationConfig.presentCallback = nullptr;
                m_frameGenerationConfig.presentCallbackUserContext = nullptr;
            }
        }
        else
        {
            m_frameGenerationConfig.frameGenerationCallback = nullptr;
            m_frameGenerationConfig.frameGenerationCallbackUserContext = nullptr;
        }

        m_frameGenerationConfig.onlyPresentGenerated = m_presentInterpolatedOnly;
        m_frameGenerationConfig.frameID = m_frame;

        void* ffxSwapChain = m_deviceResources->GetSwapChain();
        m_frameGenerationConfig.swapChain = ffxSwapChain;

        ffx::ReturnCode retCode = ffx::Configure(m_frameGenContext, m_frameGenerationConfig);
        SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);

        retCode = ffx::Dispatch(m_frameGenContext, dispatchFgPrep);
        SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);

        FfxApiResource uiColor = (m_compositionMode == COMP_MODE_TEXTURE)
            ? GetFfxResourceFromD3D12(m_uiRenderTargets[m_currentUITarget].Get(), FFX_RESOURCE_STATE_UNORDERED_ACCESS)
            : GetFfxResourceFromNull();

        ffx::ConfigureDescFrameGenerationSwapChainRegisterUiResourceDX12 uiConfig{};
        uiConfig.uiResource = uiColor;
        uiConfig.flags = m_doublebufferInSwapchain ? FFX_FRAMEGENERATION_UI_COMPOSITION_FLAG_ENABLE_INTERNAL_UI_DOUBLE_BUFFERING : 0u;
        ffx::Configure(m_deviceResources->GetFFXSwapChainContext(), uiConfig);

        // Dispatch frame generation, if not using the callback
        if (m_frameInterpolation && !m_frameGenerationCallback)
        {
            // The backbuffer currently is in RENDER_TARGET state, but at the time the FrameGenerationSwapChainInterpolationCommandList
            // gets executed by the FrameInterpolationSwapChain, it will be in PRESENT state
            FfxApiResource backbuffer = GetFfxResourceFromD3D12(m_deviceResources->GetFrameRenderTarget(), FFX_API_RESOURCE_STATE_PRESENT);

            ffx::DispatchDescFrameGeneration dispatchFg{};
            dispatchFgPrep.commandList = commandList;
            dispatchFg.presentColor = backbuffer;
            dispatchFg.numGeneratedFrames = 1;

            // assume symmetric letterbox
            dispatchFg.generationRect.left = 0;
            dispatchFg.generationRect.top = 0;
            dispatchFg.generationRect.width = size.right;
            dispatchFg.generationRect.height = size.bottom;

            ffx::QueryDescFrameGenerationSwapChainInterpolationCommandListDX12 queryCmdList{};
            queryCmdList.pOutCommandList = &dispatchFg.commandList;
            ffx::Query(m_deviceResources->GetFFXSwapChainContext(), queryCmdList);

            ffx::QueryDescFrameGenerationSwapChainInterpolationTextureDX12 queryFiTexture{};
            queryFiTexture.pOutTexture = &dispatchFg.outputs[0];
            ffx::Query(m_deviceResources->GetFFXSwapChainContext(), queryFiTexture);

            dispatchFg.frameID = m_frame;
            dispatchFg.reset = m_renderScaleChanged || m_resetRequested;

            retCode = ffx::Dispatch(m_frameGenContext, dispatchFg);
            SAMPLE_ASSERT(retCode == ffx::ReturnCode::Ok);
        }
    }

#endif // _GAMING_XBOX

    m_gpuTimer->Stop(commandList, 1);
	PIXEndEvent(commandList);
}

#pragma region Frame Render
// draws the scene.
void Sample::Render()
{
    m_cpuTimer->Stop();
    m_deltaTime = m_cpuTimer->GetElapsedMS();
    m_cpuTimer->Start();

    auto currentFrame = m_timer.GetFrameCount();
    
    // apply projection jitter
    // if we're in the process of changing renderscale, reuse previous frames matrix to reduce the appearance
    // of pixel offsets in the switch
    if ((!m_renderScaleWantsChange && !m_renderScaleChanged))
    {
        auto size = m_deviceResources->GetOutputSize();
        if (c_reverseDepth)
        {
            m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), FLT_MAX, 0.1f);
        }
        else
        {
            m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, FLT_MAX);
        }
        auto scaledSize = GetScaledRect(size);
        SetProjectionJitter(uint32_t(scaledSize.right), uint32_t(scaledSize.bottom));
    }

    // don't try to render anything before the first Update.
    if (currentFrame < 1)
    {
        m_deviceResources->WaitForGpu();
        return;
    }

    // perform the update here so it has updated projection
    m_particleSystem.Update(float(m_timer.GetElapsedSeconds()), m_view, m_proj);

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    m_gpuTimer->BeginFrame(commandList);

    // Whole-frame timer
    m_gpuTimer->Start(commandList, 0);

    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // setup the index for the ui texture for this frame
    m_currentUITarget = m_doublebufferInSwapchain ? 0 : (++m_currentUITarget) & 1;

    UpdateGLTFState();
    RenderShadowMaps(commandList);
    RenderMotionVectors(commandList);
    RenderGTLFScene(commandList);

    // pre TAA particle compose
    RenderParticlesIntoScene(commandList);

    // because FSR3 frame generation can be combined with other upsamplers, it needs to always run so that we can update the interpolation
    // for upscalers other than FSR3, FSR3 is initialized with FFX_FSR3_ENABLE_INTERPOLATION_ONLY so only interpolation data is used
    // if interpolation is disabled, it will do nothing
    RenderFSR3(commandList);

    // tonemapping should go after fsr3
    RenderTonemapping(commandList);

    // render magnifier if not hiding it 
    if (!m_hideMagnifier)
    {
        RenderMagnifier(commandList);
    }
    RenderColorConversion(commandList);

    m_prevProj = m_proj;
    m_prevView = m_view;
    m_prevViewProj = m_view * m_proj;

    PIXEndEvent(commandList);

    // Render UI only if not in callback ui composition mode (which will call it on another thread)
    if (m_compositionMode != COMP_MODE_CALLBACK)
    {
        RenderUI_NoCallback(commandList);
    }

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");

    m_gpuTimer->Stop(commandList, 0);
    m_gpuTimer->EndFrame(commandList);

    DX::DeviceResources::PresentUiCompositionMode uiCompositionMode = m_compositionMode == COMP_MODE_TEXTURE
                                                                    ? DX::DeviceResources::PresentUiCompositionMode::kUseUiBackBuffer
                                                                    : DX::DeviceResources::PresentUiCompositionMode::kNoUiBackBuffer;

    m_deviceResources->Present(uiCompositionMode);

    PIXEndEvent();
}

void Sample::RenderUI_NoCallback(ID3D12GraphicsCommandList* commandList)
{
    // if rendering ui to texture, temporarily swap the raster view to the ui texture
    if (m_compositionMode == COMP_MODE_TEXTURE)
    {
#ifdef _GAMING_XBOX
        // need to set a different resource on Xbox to avoid SRGB rendering issue and validation errors
        D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = m_deviceResources->GetUIRenderTargetView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);

#else // GAMING_DESKTOP

        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_uiRenderTargets[m_currentUITarget].Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
        rtvDescriptor = m_renderDescriptors->GetCpuHandle(m_currentUITarget ? RTDescriptors::UIOutput_RTV1 : RTDescriptors::UIOutput_RTV0);
        commandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);

#endif // _GAMING_XBOX

        // clear the target
        const FLOAT clearcolors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        commandList->ClearRenderTargetView(rtvDescriptor, clearcolors, 0, nullptr);
    }

    // if doing hudless ui render, we need to copy the contents of the ui-less backbuffer to the ui texture for isolated hud generation
    else if (m_compositionMode == COMP_MODE_HUDLESS_TEXTURE)
    {
        // transition the render target into the correct state to allow for drawing into it.
        D3D12_RESOURCE_BARRIER barriers[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetFrameRenderTarget(),
                                                    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(m_uiRenderTargets[m_currentUITarget].Get(),
                                                    D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST)
        };
        commandList->ResourceBarrier(2, barriers);

        // copy
        D3D12_TEXTURE_COPY_LOCATION dx12SourceLocation = {};
        dx12SourceLocation.pResource = m_deviceResources->GetFrameRenderTarget();
        dx12SourceLocation.SubresourceIndex = 0;
        dx12SourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        D3D12_TEXTURE_COPY_LOCATION dx12DestinationLocation = {};
        dx12DestinationLocation.pResource = m_uiRenderTargets[m_currentUITarget].Get();
        dx12DestinationLocation.SubresourceIndex = 0;
        dx12DestinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

        commandList->CopyTextureRegion(&dx12DestinationLocation, 0, 0, 0, &dx12SourceLocation, nullptr);

        // revert back to original state after copy
        D3D12_RESOURCE_STATES temp = barriers[0].Transition.StateBefore;
        barriers[0].Transition.StateBefore = barriers[0].Transition.StateAfter;
        barriers[0].Transition.StateAfter = temp;

        temp = barriers[1].Transition.StateBefore;
        barriers[1].Transition.StateBefore = barriers[1].Transition.StateAfter;
        barriers[1].Transition.StateAfter = temp;
        commandList->ResourceBarrier(2, barriers);
    }

    RenderUI(commandList);

    // if rendering ui to texture, return to the correct rtv and transition back the ui texture
    if (m_compositionMode == COMP_MODE_TEXTURE)
    {
#ifdef _GAMING_DESKTOP
        auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
        commandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);

        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_uiRenderTargets[m_currentUITarget].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->ResourceBarrier(1, &barrier);
#else // _GAMING_XBOX

    // We simply composite render UI into a separate back buffer (see DeviceResources::GetUIRenderTarget) and compose it with the main back buffer in PresentX on Xbox

#endif // #ifdef _GAMING_DESKTOP
    }
}

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_resourceDescriptors->Heap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    // Need to choose the correct batch renderer based on composition mode
    DirectX::DX12::SpriteBatch* pBatchPtr = m_compositionMode == COMP_MODE_TEXTURE
                                          ? m_batchForUIBufferRender.get()
                                          : m_batchForBackBufferRender.get();

    auto const size = m_deviceResources->GetOutputSize();
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    pBatchPtr->Begin(commandList);

    float y = float(safe.top);

    float resolutionScale[2] = { float(size.right) / 3840.f, float(size.bottom) / 2160.f };
    float scaleMultiplier = (resolutionScale[0] > resolutionScale[1]) ?  resolutionScale[0] : resolutionScale[1];
    float textScale = 0.85f * scaleMultiplier;

    float yLineSpacing = m_font->GetLineSpacing() * scaleMultiplier;

    // void XM_CALLCONV SpriteFont::DrawString(_In_ SpriteBatch* spriteBatch, _In_z_ wchar_t const* text, XMFLOAT2 const& position, FXMVECTOR color, 
    //float rotation, XMFLOAT2 const& origin, float scale, SpriteEffects effects, float layerDepth) const
    if (m_bIsDisplayInHDRMode)
    {
        m_font->DrawString(pBatchPtr, L"AMD FidelityFX Super Resolution Sample (HDR)",
            XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    }
    else
    {
        m_font->DrawString(pBatchPtr, L"AMD FidelityFX Super Resolution Sample (SDR)",
            XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    }

    wchar_t textBuffer[128] = {};

    y += yLineSpacing;
    swprintf_s(textBuffer, L"Target Resolution %d x %d", size.right, size.bottom);
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);

    y += yLineSpacing;
    swprintf_s(textBuffer, L"Frametime:  %0.3fms", m_gpuTimer->GetElapsedMS(0));
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);

#ifdef _GAMING_XBOX
    y += yLineSpacing;
    int32_t frameInterval = m_frameInterpolation ? 120 : 60;
    swprintf_s(textBuffer, L"FrameInterval: %d Hz", frameInterval);
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
#endif // _GAMING_XBOX

    y += yLineSpacing;
    y += yLineSpacing;

    wchar_t effectName[64];
#ifdef _GAMING_XBOX
    swprintf_s(effectName, L"FSR 3.1.4");
#else
    swprintf_s(effectName, m_upscalingVersion);
#endif // _GAMING_XBOX

    m_font->DrawString(pBatchPtr, effectName, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    auto scaled = GetScaledRect(size);
    swprintf_s(textBuffer, L"Render Resolution: %d x %d ( %ls Mode, %1.1fx scale)", (int)(scaled.right),
        (int)(scaled.bottom), m_fsrModes[GetRenderScale()], m_renderScaleRatio[GetRenderScale()]);
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    {
        if (m_rcasEnable)
        {
            swprintf_s(textBuffer, L"%ls Time:  %0.3fms (Sharpening Enabled)", effectName, m_gpuTimer->GetAverageMS(1));
        }
        else
        {
            swprintf_s(textBuffer, L"%ls Time:  %0.3fms", effectName, m_gpuTimer->GetAverageMS(1));
        }
        m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
        y += yLineSpacing;
    }

    // Reactive mask
    swprintf_s(textBuffer, L"Reactive Mask:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_useFSRReactiveMask ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_useFSRReactiveMask ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;
        
    // RCas
    swprintf_s(textBuffer, L"RCas:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_rcasEnable ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_rcasEnable ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // Frame interpolation
    swprintf_s(textBuffer, L"Frame Interpolation:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_frameInterpolation ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_frameInterpolation ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // Interpolation only
    swprintf_s(textBuffer, L"Show Only Interpolated:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_presentInterpolatedOnly ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_presentInterpolatedOnly ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // Debug Tear lines
    swprintf_s(textBuffer, L"Debug Tear Lines:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_drawDebugTearLines ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_drawDebugTearLines ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // Debug view
    swprintf_s(textBuffer, L"Debug View:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_drawDebugView ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_drawDebugView ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // Async compute
    swprintf_s(textBuffer, L"Async Compute:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_allowAsyncCompute ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_allowAsyncCompute ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // Frame generation callback
    swprintf_s(textBuffer, L"Frame Generation Callback:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_frameGenerationCallback ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_frameGenerationCallback ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

#ifdef _GAMING_XBOX
    // Async compute present
    swprintf_s(textBuffer, L"Async Compute Present:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_asyncComputePresent ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_asyncComputePresent ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;
#endif // _GAMING_XBOX

    // Internal UI double buffer
    swprintf_s(textBuffer, L"Internal UI Double Buffer:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, m_doublebufferInSwapchain ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), m_doublebufferInSwapchain ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // Show magnifier
    swprintf_s(textBuffer, L"Magnifier:");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    swprintf_s(textBuffer, !m_hideMagnifier ? L"(On)" : L"(Off)");
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left + 8 * yLineSpacing), y), !m_hideMagnifier ? ATG::Colors::Green : ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // Sharpness
    swprintf_s(textBuffer, L"Sharpness:  %1.3f", m_rcasEnable ? m_rcasSharpness : 0.00f);
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // Exposure
    swprintf_s(textBuffer, L"Exposure:  %1.3f", m_tonemapperConstants.exposure);
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    // LOD Bias
    swprintf_s(textBuffer, L"LOD Bias:  %1.3f", m_lodBias);
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);
    y += yLineSpacing;

    static wchar_t cCompModes[COMP_MODE_COUNT][64] = { L"None", L"Texture", L"Callback", L"Hudless Texture" };
    swprintf_s(textBuffer, L"UI Composition Mode:  %ls", cCompModes[m_compositionMode]);
    m_font->DrawString(pBatchPtr, textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White, 0.f, XMFLOAT2(0, 0), textScale);

    // Show high-refresh rate display warning if needed
    if (m_showHighRefreshRateWarning)
    {
        y += yLineSpacing;
        const wchar_t* warningStr = L"WARNING - Frame Interpolation Disabled\n\n"
            L"AMD recommends only using frame interpolation when the title can maintain a 60 fps average and\n"
            L"is attached to a high refresh rate capable monitor (> 120Hz). Enabling it with lower\n"
            L"frame rates and refresh rates will result in visual artifacts and fluidity issues.";
        m_font->DrawString(pBatchPtr, warningStr, XMFLOAT2(float(safe.left), y), ATG::Colors::Orange, 0.f, XMFLOAT2(0, 0), textScale);
    }

    // Let's try putting controls on the right to make things simpler
    y = float(safe.top);
    float x = safe.right - (m_ctrlFont->GetLineSpacing() * 6);
    const wchar_t* globalLegendStr = L"Global Controls\n\n"
        L"[View] Exit\n"
        L"[LThumb] Rotate Camera\n"
        L"[RThumb] Move Magnifier\n"
        L"[LThumb]-Click Reset Camera\n"
        L"[RThumb]-Click Toggle Magnifier\n"
        L"[Menu] Toggle Input Controls";
    DX::DrawControllerString(pBatchPtr,
        m_font.get(), m_ctrlFont.get(),
        globalLegendStr, XMFLOAT2(x, y),
        ATG::Colors::White, textScale);

    y += 9 * yLineSpacing;
    const wchar_t* upscalerLegendStr = L"Upscaler Controls\n\n"
        L"[DPad]-Up Sharpening On/Off\n"
        L"[LT] /[RT] Exposure\n"
        L"[A] /[B] Cycle Quality Modes\n"
        L"[LB] /[RB] Sharpness\n"
        L"[DPad]-Down Reactive Mask On/Off\n"
        L"[DPad]-Left/Right LOD Bias";
    DX::DrawControllerString(pBatchPtr,
        m_font.get(), m_ctrlFont.get(),
        upscalerLegendStr, XMFLOAT2(x, y),
        m_frameInterpolationControls ? ATG::Colors::DarkGrey : ATG::Colors::White, textScale);

    y += 9 * yLineSpacing;
    const wchar_t* fiLegendStr = L"Frame Interpolation Controls\n\n"
        L"[A]  Frame Interpolation On/Off\n"
        L"[B]  Frame Interpolation Only On/Off\n"
        L"[X]  Draw Tear Lines On/Off\n"
        L"[Y]  Draw Debug View On/Off\n"
        L"[DPad]-Up Async Compute On/Off\n"
        L"[DPad]-Down Internal UI double buffer On/Off\n"
        L"[LB] Frame Generation Callback On/Off\n"
#ifdef _GAMING_XBOX
        L"[RB] Async Compute Present On/Off\n"
#endif // _GAMING_XBOX
        L"[DPad]-Left/Right UI Composition Modes";
    DX::DrawControllerString(pBatchPtr,
        m_font.get(), m_ctrlFont.get(),
        fiLegendStr, XMFLOAT2(x, y),
        m_frameInterpolationControls ? ATG::Colors::White : ATG::Colors::DarkGrey, textScale);

    pBatchPtr->End();

    PIXEndEvent(commandList);
}

#ifdef _GAMING_XBOX
D3D12_RESOURCE_STATES GetDX12ResourceState(FfxResourceStates state)
#else // _GAMING_DESKTOP
D3D12_RESOURCE_STATES GetDX12ResourceState(uint32_t state)
#endif // _GAMING_XBOX
{
    switch (state)
    {
    case FFX_RESOURCE_STATE_UNORDERED_ACCESS:
        return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    case FFX_RESOURCE_STATE_COMPUTE_READ:
        return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    case FFX_RESOURCE_STATE_PIXEL_READ:
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ:
        return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case FFX_RESOURCE_STATE_COPY_SRC:
        return D3D12_RESOURCE_STATE_COPY_SOURCE;
    case FFX_RESOURCE_STATE_COPY_DEST:
        return D3D12_RESOURCE_STATE_COPY_DEST;
    case FFX_RESOURCE_STATE_GENERIC_READ:
        return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    case FFX_RESOURCE_STATE_INDIRECT_ARGUMENT:
        return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
    case FFX_RESOURCE_STATE_PRESENT:
        return D3D12_RESOURCE_STATE_PRESENT;
    default:
        // Unsupported resource state requested. Please implement.
        SAMPLE_ASSERT(false);
        return D3D12_RESOURCE_STATE_COMMON;
    }
}

#ifdef _GAMING_XBOX
FfxErrorCode Sample::RenderUI_FrameInterpolationCallback(const FfxPresentCallbackDescription* params, void* pContext)
#else // _GAMING_DESKTOP
ffxReturnCode_t Sample::RenderUI_FrameInterpolationCallback(ffxCallbackDescFrameGenerationPresent* params, void* pContext)
#endif // _GAMING_XBOX
{
    Sample* pSampleInstance = reinterpret_cast<Sample*>(pContext);

    if (pSampleInstance->m_compositionMode != COMP_MODE_CALLBACK)
    {
#ifdef _GAMING_XBOX
        return FFX_ERROR_INVALID_ARGUMENT;
#else
        return static_cast<ffxReturnCode_t>(FFX_ERROR_INVALID_ARGUMENT);
#endif
    }

    // get the command list (just need to cast it)
    ID3D12GraphicsCommandList* commandList = reinterpret_cast<ID3D12GraphicsCommandList*>(params->commandList);

    pSampleInstance->RenderUI_WithCallback(commandList, params);

    return FFX_OK;
}

#ifdef _GAMING_XBOX
void Sample::RenderUI_WithCallback(ID3D12GraphicsCommandList* commandList, const FfxPresentCallbackDescription* params)
#else //_GAMING_DESKTOP
void Sample::RenderUI_WithCallback(ID3D12GraphicsCommandList* commandList, const ffxCallbackDescFrameGenerationPresent* params)
#endif // _GAMING_XBOX
{
    // on PC, we need to blit the bb resource to the render target resource before compositing UI
    // this is not needed on console
    D3D12_RESOURCE_BARRIER barriers[2];
#ifdef _GAMING_DESKTOP
    D3D12_RESOURCE_STATES rtResourceState = GetDX12ResourceState(params->outputSwapChainBuffer.state);
    D3D12_RESOURCE_STATES bbResourceState = GetDX12ResourceState(params->currentBackBuffer.state);

    ID3D12Resource* pRTResource = reinterpret_cast<ID3D12Resource*>(params->outputSwapChainBuffer.resource);
    ID3D12Resource* pBBResource = reinterpret_cast<ID3D12Resource*>(params->currentBackBuffer.resource);

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pRTResource, rtResourceState, D3D12_RESOURCE_STATE_COPY_DEST);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(pBBResource, bbResourceState, D3D12_RESOURCE_STATE_COPY_SOURCE);
    commandList->ResourceBarrier(2, barriers);

    // copy
    D3D12_TEXTURE_COPY_LOCATION dx12SourceLocation = {};
    dx12SourceLocation.pResource = pBBResource;
    dx12SourceLocation.SubresourceIndex = 0;
    dx12SourceLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    D3D12_TEXTURE_COPY_LOCATION dx12DestinationLocation = {};
    dx12DestinationLocation.pResource = pRTResource;
    dx12DestinationLocation.SubresourceIndex = 0;
    dx12DestinationLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    commandList->CopyTextureRegion(&dx12DestinationLocation, 0, 0, 0, &dx12SourceLocation, nullptr);

    // transition bb resource back
    D3D12_RESOURCE_STATES temp = barriers[1].Transition.StateBefore;
    barriers[1].Transition.StateBefore = barriers[1].Transition.StateAfter;
    barriers[1].Transition.StateAfter = temp;

    // transition rt resource to render target
    barriers[0].Transition.StateBefore = barriers[0].Transition.StateAfter;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(2, barriers);

    // assign an RTV for the render target resource
    static size_t gRTVIndex = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_renderDescriptors->GetCpuHandle(RTDescriptors::RTCount + gRTVIndex);
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = m_deviceResources->GetFrameBackBufferFormat();
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    m_deviceResources->GetD3DDevice()->CreateRenderTargetView(pRTResource, &rtvDesc, rtvHandle);

    // cycle the index for the next time this is called
    gRTVIndex = (gRTVIndex + 1) % m_deviceResources->GetBackBufferCount();
#else
    // just transition the rt resource to render target
    D3D12_RESOURCE_STATES rtResourceState = GetDX12ResourceState(params->outputSwapChainBuffer.state);
    ID3D12Resource* pRTResource = reinterpret_cast<ID3D12Resource*>(params->outputSwapChainBuffer.resource);

    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(pRTResource, rtResourceState, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, barriers);

    DX::DeviceResources *deviceResources = m_deviceResources.get();

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = params->isInterpolatedFrame ? deviceResources->FindInterpolatedRenderTargetView(pRTResource)
                                                                        : deviceResources->FindFrameRenderTargetView(pRTResource);
#endif // _GAMING_DESKTOP

    // bind the render target
    commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

    // render the ui
    RenderUI(commandList);

    // transition the rt resource back to original state it was given to us in
    barriers[0].Transition.StateBefore = barriers[0].Transition.StateAfter;
    barriers[0].Transition.StateAfter = rtResourceState;
    commandList->ResourceBarrier(1, barriers);
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();

    // Scale depending on current render scale
    scissorRect = GetScaledRect(scissorRect);
    viewport.Width = static_cast<float>(scissorRect.right);
    viewport.Height = static_cast<float>(scissorRect.bottom);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = {};

    rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::GltfSceneRTV);
    dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, false, &dsvDescriptor);
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    const FLOAT clearcolors[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    commandList->ClearRenderTargetView(rtvDescriptor, clearcolors, 0, nullptr);

    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, (c_reverseDepth) ? 0.0f : 1.0f, 0, 0, nullptr);

    TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_RENDER_TARGET);
    TransitionTo(commandList, m_taaOutput, &m_taaOutputState, D3D12_RESOURCE_STATE_RENDER_TARGET);
    TransitionTo(commandList, m_particles, &m_particlesState, D3D12_RESOURCE_STATE_RENDER_TARGET);


    const FLOAT clearcolors0[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTargetView(m_renderDescriptors->GetCpuHandle(RTDescriptors::ParticlesRTV), clearcolors0, 0, nullptr);
    commandList->ClearRenderTargetView(m_renderDescriptors->GetCpuHandle(RTDescriptors::Tonemap_OutputRTV), clearcolors, 0, nullptr);
    commandList->ClearRenderTargetView(m_renderDescriptors->GetCpuHandle(RTDescriptors::TaaOutputRTV), clearcolors, 0, nullptr);

    if (m_initialResourceClearsRequired)
    {
        TransitionTo(commandList, m_taaHistory, &m_taaHistoryState, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ClearRenderTargetView(m_renderDescriptors->GetCpuHandle(RTDescriptors::TaaHistoryRTV), clearcolors, 0, nullptr);
        TransitionTo(commandList, m_taaHistory, &m_taaHistoryState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        m_initialResourceClearsRequired = false;
    }

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
#ifdef _GAMING_XBOX
    if (m_bIsDisplayInHDRMode)
    {
        SetDisplayMode();
    }
#endif

    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
}

void Sample::OnUnConstrained()
{
#ifdef _GAMING_XBOX
    if (m_bIsDisplayInHDRMode)
    {
        SetDisplayMode();
    }
#endif
}

void Sample::UpdateFrameInterpolationOnPotentialPrimaryOutputChange()
{
    const bool supportFrameInterpolation = m_frameInterpolation = m_deviceResources->SupportsHighRereshRate();

    m_resetRequested = supportFrameInterpolation != m_frameInterpolation;

    // Check if we're connected to a high refresh rate display. If not, disable frame interpolation
    // and show a warning.

    m_frameInterpolation = supportFrameInterpolation;

    // If we aren't connected to high refresh display, show a warning on-screen until we toggle 
    // frame interpolation next
    m_showHighRefreshRateWarning = !m_frameInterpolation;
}

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
    UpdateFrameInterpolationOnPotentialPrimaryOutputChange();
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    // Need to destroy the current context on resource resize
    m_deviceResources->WaitForGpu();
    UpdateFSRContext(false);

    // This will re-create the context
    CreateWindowSizeDependentResources();
}

void Sample::OnDeviceLost()
{
    UpdateFSRContext(false);

    m_gpuTimer.reset();
    m_graphicsMemory.reset();

    m_font.reset();
    m_colorCtrlFont.reset();
    m_ctrlFont.reset();

    m_resourceDescriptors.reset();
    m_renderDescriptors.reset();

    m_gltfModel->Unload();
    m_gltfResources.OnDestroy();
    m_gltfPBR->OnDestroy();
    m_gltfDepth->OnDestroy();
    m_gltfMotionVectors->OnDestroy();


}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();
}
#pragma endregion

#pragma region Direct3D Resources

// These are the resources that depend on the device.
FfxConstantAllocation Sample::ffxAllocateConstantBuffer(void* data, FfxUInt64 dataSize)
{
    FfxConstantAllocation allocation;
    allocation.resource = FfxResource(); // not needed in directx

    GraphicsResource alloc = g_pSampleInstance->m_graphicsMemory->Allocate(dataSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT, GraphicsMemory::TAG_CONSTANT);
    memcpy(alloc.Memory(), data, dataSize);
    allocation.handle = alloc.GpuAddress();

    return allocation;
}

#ifdef _GAMING_XBOX

FfxErrorCode Sample::ffxAllocateResource(FfxEffect ffxEffect, D3D12_RESOURCE_STATES initialState, const D3D12_HEAP_PROPERTIES* pHeapProps,
    const D3D12_RESOURCE_DESC* pD3DDesc, const FfxResourceDescription* pFfxDesc,
    const D3D12_CLEAR_VALUE* pOptimizedClear, ID3D12Resource** ppD3DResource)
{
#ifdef _GAMING_XBOX_SCARLETT

    D3D12XBOX_RESOURCE_DESC dx12ResourceDescriptionX = {};
    dx12ResourceDescriptionX.Dimension = pD3DDesc->Dimension;
    dx12ResourceDescriptionX.Alignment = pD3DDesc->Alignment;
    dx12ResourceDescriptionX.Width = pD3DDesc->Width;
    dx12ResourceDescriptionX.Height = pD3DDesc->Height;
    dx12ResourceDescriptionX.DepthOrArraySize = pD3DDesc->DepthOrArraySize;
    dx12ResourceDescriptionX.MipLevels = pD3DDesc->MipLevels;
    dx12ResourceDescriptionX.Format = pD3DDesc->Format;
    dx12ResourceDescriptionX.SampleDesc.Count = pD3DDesc->SampleDesc.Count;
    dx12ResourceDescriptionX.SampleDesc.Quality = pD3DDesc->SampleDesc.Quality;
    dx12ResourceDescriptionX.Layout = pD3DDesc->Layout;
    dx12ResourceDescriptionX.Flags = pD3DDesc->Flags;
    dx12ResourceDescriptionX.DccOptions.CompressionLevel = D3D12XBOX_DCC_COMPRESSION_LEVEL_DEFAULT;
    dx12ResourceDescriptionX.DccOptions.ColorTransform = D3D12XBOX_DCC_COLOR_TRANSFORM_AUTO_TEXTURE_COMPATIBLE;

    if (pFfxDesc->usage & FFX_RESOURCE_USAGE_DCC_RENDERTARGET)
    {
        /**
         *  We enforce RENDER_TARGET flag on select resources only to set swizzling mode to what RENDER_TARGET textures use
         *  for more optimal access
         *
         *  Everything else is DCC-related and we want DCC to be enabled to save on memory traffic.
         */
        dx12ResourceDescriptionX.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
                                       | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                                       | D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC
                                       | D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;

        dx12ResourceDescriptionX.DccOptions.CompressionLevel = D3D12XBOX_DCC_COMPRESSION_LEVEL_128B_TEXTURE_COMPATIBLE;
    }

    if ((pFfxDesc->flags & FFX_RESOURCE_FLAGS_ALIASABLE) && ffxEffect != FFX_EFFECT_OPTICALFLOW)
    {
        XG_RESOURCE_DESC xgResourceDescription = {};
        xgResourceDescription.Dimension         = static_cast<XG_RESOURCE_DIMENSION>(dx12ResourceDescriptionX.Dimension);
        xgResourceDescription.Alignment         = dx12ResourceDescriptionX.Alignment;
        xgResourceDescription.Width             = dx12ResourceDescriptionX.Width;
        xgResourceDescription.Height            = dx12ResourceDescriptionX.Height;
        xgResourceDescription.DepthOrArraySize  = dx12ResourceDescriptionX.DepthOrArraySize;
        xgResourceDescription.MipLevels         = dx12ResourceDescriptionX.MipLevels;
        xgResourceDescription.Format            = static_cast<XG_FORMAT>(dx12ResourceDescriptionX.Format);
        xgResourceDescription.SampleDesc.Count  = dx12ResourceDescriptionX.SampleDesc.Count;
        xgResourceDescription.SampleDesc.Quality= dx12ResourceDescriptionX.SampleDesc.Quality;
        xgResourceDescription.Layout            = static_cast<XG_TEXTURE_LAYOUT>(dx12ResourceDescriptionX.Layout);
        xgResourceDescription.MiscFlags         = static_cast<XG12_RESOURCE_MISC_FLAG>(dx12ResourceDescriptionX.Flags);

        // add an allocator for Optical Flow when it has ALIASABLE resources
        SAMPLE_ASSERT(ffxEffect != FFX_EFFECT_OPTICALFLOW);
        FrameTransientResourceMemoryStackAllocator *alloc2Mb = ffxEffect == FFX_EFFECT_FSR3UPSCALER ? g_pSampleInstance->m_transientAlloc2MbFsr3.get() : g_pSampleInstance->m_transientAlloc2MbFI.get();
        FrameTransientResourceMemoryStackAllocator *alloc64K = ffxEffect == FFX_EFFECT_FSR3UPSCALER ? g_pSampleInstance->m_transientAlloc64KFsr3.get() : g_pSampleInstance->m_transientAlloc64KFI.get();

        if (xgResourceDescription.Dimension != XG_RESOURCE_DIMENSION_BUFFER)
        {
            XGTextureAddressComputer *pComputer = NULL;
            if (S_OK == XGCreateTextureComputer(&xgResourceDescription, &pComputer))
            {
                XG_RESOURCE_LAYOUT resourceLayout;
                if (S_OK == pComputer->GetResourceLayout(&resourceLayout))
                {
                    D3D12XBOX_COMPONENT_PLACED_ADDRESSES resourceComponents = {};

                    for (uint32_t i = 0; i < resourceLayout.Planes; ++i)
                    {
                        D3D12_GPU_VIRTUAL_ADDRESS address = NULL;
                        if (resourceLayout.Plane[i].SizeBytes < 1024 * 1024)
                        {
                            address = alloc64K->alloc(resourceLayout.Plane[i].SizeBytes, resourceLayout.Plane[i].BaseAlignmentBytes);
                        }
                        else
                        {
                            address = alloc2Mb->alloc(resourceLayout.Plane[i].SizeBytes, resourceLayout.Plane[i].BaseAlignmentBytes);
                        }
                        switch (resourceLayout.Plane[i].Usage)
                        {
                            case XG_PLANE_USAGE_DEFAULT:
                                if (xgResourceDescription.SampleDesc.Count == 1)
                                    resourceComponents.NonAARenderTarget.ColorSamples = address;
                                else
                                    resourceComponents.AARenderTarget.ColorSamples = address;
                            break;

                            case XG_PLANE_USAGE_COLOR_MASK:
                                if (xgResourceDescription.SampleDesc.Count == 1)
                                    resourceComponents.NonAARenderTarget.CMask = address;
                                else
                                    resourceComponents.AARenderTarget.CMask = address;
                            break;

                            case XG_PLANE_USAGE_FRAGMENT_MASK:
                                resourceComponents.AARenderTarget.FragmentMask = address;
                            break;

                            case XG_PLANE_USAGE_HTILE:
                                resourceComponents.DepthTarget.HTile = address;
                            break;

                            case XG_PLANE_USAGE_LUMA:
                            break;

                            case XG_PLANE_USAGE_CHROMA:

                            break;

                            case XG_PLANE_USAGE_DEPTH:
                                resourceComponents.DepthTarget.DepthSamples = address;
                            break;

                            case XG_PLANE_USAGE_STENCIL:
                                resourceComponents.DepthStencilTarget.StencilSamples = address;
                            break;

                            case XG_PLANE_USAGE_DELTA_COLOR_COMPRESSION:
                                if (xgResourceDescription.SampleDesc.Count == 1)
                                    resourceComponents.NonAARenderTarget.DCC = address;
                                else
                                    resourceComponents.AARenderTarget.DCC = address;
                            break;

                            default:
                            break;
                        }
                    }
                    HRESULT hr = g_pSampleInstance->m_deviceResources->GetD3DDevice()->CreateComponentPlacedResourceX1(&resourceComponents, &dx12ResourceDescriptionX, initialState, pOptimizedClear, IID_GDK_PPV_ARGS(ppD3DResource));
                    if (hr != S_OK) {
                        return FFX_ERROR_BACKEND_API_ERROR;
                    }
                }
            }
        }
        else
        {
            D3D12_GPU_VIRTUAL_ADDRESS address = NULL;
            if (xgResourceDescription.Width < 1024 * 1024)
            {
                address = alloc64K->alloc(xgResourceDescription.Width, 64 * 1024);
            }
            else
            {
                address = alloc2Mb->alloc(xgResourceDescription.Width, 64 * 1024);
            }
            HRESULT hr = g_pSampleInstance->m_deviceResources->GetD3DDevice()->CreatePlacedResourceX(address, pD3DDesc, initialState, pOptimizedClear, IID_GDK_PPV_ARGS(ppD3DResource));
            if (hr != S_OK) {

                return FFX_ERROR_BACKEND_API_ERROR;
            }
        }
    }
    else
    {
        HRESULT hr = g_pSampleInstance->m_deviceResources->GetD3DDevice()->CreateCommittedResourceX(pHeapProps, D3D12_HEAP_FLAG_NONE, &dx12ResourceDescriptionX, initialState, pOptimizedClear, IID_GDK_PPV_ARGS(ppD3DResource));
        if (hr != S_OK) {

            return FFX_ERROR_BACKEND_API_ERROR;
        }
    }
#else
    (void)ffxEffect;
    (void)pFfxDesc;
    HRESULT hr = g_pSampleInstance->m_deviceResources->GetD3DDevice()->CreateCommittedResource(pHeapProps, D3D12_HEAP_FLAG_NONE, pD3DDesc, initialState, pOptimizedClear, IID_GDK_PPV_ARGS(ppD3DResource));
    if (hr != S_OK) {
        // note: should use GetLastError() to figure out what the error was to return something better
        return FFX_ERROR_BACKEND_API_ERROR;
    }
#endif // _GAMING_XBOX_SCARLETT

    return FFX_OK;
}

FfxErrorCode Sample::ffxReleaseResource(FfxEffect ffxEffect, ID3D12Resource* pResource)
{
    // NOTE: we use FrameTransientResourceMemoryStackAllocator which is stack allocator, so we don't handle any memory, just Release the D3D12 resource
    pResource->Release();
    (void)ffxEffect;
    return FFX_OK;
}
#endif // #ifdef _GAMING_XBOX

void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_3 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_3))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.3 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.3 is not supported!");
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS featureDataOptions = {};
    device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureDataOptions, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    m_renderDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        RTDescriptors::RTCount + m_deviceResources->GetBackBufferCount());  // allocate additional rtviews for ui callback

    m_depthDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        DSDescriptors::DSCount);

    m_states = std::make_unique<CommonStates>(device);
    m_gltfModel = new AMDTK::GLTFFile();

    wchar_t filepath[_MAX_PATH];
#ifdef _GAMING_XBOX
    DX::FindMediaFile(filepath, _MAX_PATH, L"Corridor.gltf");
#else
    DX::FindMediaFile(filepath, _MAX_PATH, L"AMDCorridor\\Corridor.gltf");
#endif

    bool status = m_gltfModel->Load(filepath);

    if (!status)
    {
        std::string message = "The model couldn't be found";
        SAMPLE_ASSERT(false);
    }

    // Force to use user defined lights below
    m_gltfModel->ClearLights();

    if (m_gltfModel->NumLights() == 0)
    {
        {
            tfNode n;
            XMVECTOR x{ 0.0f, 2.5f, 3.5f, 0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 2.0f, -3.1f, 0.0f));

            tfLight l;
            l.m_type = tfLight::LIGHT_SPOTLIGHT;
            l.m_intensity = 0.75f;
            l.m_color = XMVectorSet(0.70f, 0.70f, 1.0f, 0.0f);
            l.m_range = 15.0f;
            l.m_outerConeAngle = XM_PI / 3.0f;
            l.m_innerConeAngle = (XM_PI / 3.0f) * 0.6f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f, 2.5f, 3.5f, 0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(-1.0f, 2.0f, 2.5f, 0.0f));

            tfLight l;
            l.m_type = tfLight::LIGHT_SPOTLIGHT;
            l.m_intensity = 0.75f;
            l.m_color = XMVectorSet(0.70f, 0.70f, 1.0f, 0.0f);
            l.m_range = 15.0f;
            l.m_outerConeAngle = XM_PI / 3.0f;
            l.m_innerConeAngle = (XM_PI / 3.0f) * 0.6f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f, 2.2f, -4.5f, 0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 1.9f, -5.5f, 0.0f));

            tfLight l;
            l.m_type = tfLight::LIGHT_SPOTLIGHT;
            l.m_intensity = 0.95f;
            l.m_color = XMVectorSet(0.90f, 0.50f, 0.5f, 0.0f);
            l.m_range = 20.0f;
            l.m_outerConeAngle = XM_PI / 9.0f;
            l.m_innerConeAngle = (XM_PI / 9.0f) * 0.6f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f, 2.2f, 0.0f, 0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 0.2f, 0.0f, 0.0f));

            tfLight l;
            l.m_type = tfLight::LIGHT_SPOTLIGHT;
            l.m_intensity = 0.65f;
            l.m_color = XMVectorSet(0.90f, 0.90f, 1.0f, 0.0f);
            l.m_range = 18.0f;
            l.m_outerConeAngle = XM_PI / 3.0f;
            l.m_innerConeAngle = (XM_PI / 3.0f) * 0.6f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f, 2.5f,-2.0f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 1.0f, -2.1f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.7f;
            l.m_color = XMVectorSet(0.8f, 0.8f, 0.8f, 0.0f);
            l.m_range = 13.0f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ -0.5f, 1.8f, 3.5f, 0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 1.4f, 6.0f, 0.0f));

            tfLight l;
            l.m_type = tfLight::LIGHT_SPOTLIGHT;
            l.m_intensity = 0.7f;
            l.m_color = XMVectorSet(0.9f, 0.2f, 0.2f, 0.0f);
            l.m_range = 18.0f;
            l.m_outerConeAngle = XM_PI / 3.8f;
            l.m_innerConeAngle = (XM_PI / 3.8f) * 0.6f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 1.0f, 2.0f, 7.5f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 0.0f, 5.1f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.40f;
            l.m_color = XMVectorSet(0.9f, 0.9f, 0.9f, 0.0f);
            l.m_range = 5.0f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.4f, 0.8f, 4.1f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.4f, 0.8f, 4.2f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.30f;
            l.m_color = XMVectorSet(0.9f, 0.9f, 0.9f, 0.0f);
            l.m_range = 2.2f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f, 1.2f, -5.1f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.4f, 1.8f, -5.1f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.50f;
            l.m_color = XMVectorSet(0.9f, 0.9f, 0.9f, 0.0f);
            l.m_range = 1.2f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ -0.6f, 1.0f, -0.5f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(-0.6f, 1.0f, -0.2f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.80f;
            l.m_color = XMVectorSet(0.6f, 0.6f, 0.9f, 0.0f);
            l.m_range = 4.2f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

    }

    // Upload resources to video memory
    ResourceUploadBatch upload(device);
    upload.Begin();

    m_particleSystem.Initialize(m_deviceResources.get(), upload, c_reverseDepth);
    m_particleSystem.SetFloor(XMFLOAT4(0.0f, 0.1f, 0.0f, 10.0f));

    m_gltfResources.OnCreate(device, m_gltfModel, &upload, m_graphicsMemory.get());

    // Here we are loading onto the GPU all the textures and the inverse matrices
    // this data will be used to create the passes 
    m_gltfResources.LoadTextures(m_gltfModel->GetFilePath().c_str());

    m_gltfPBR = new AMDTK::GltfPbrPass();
    m_gltfPBR->OnCreate(
        device,
        &m_gltfResources,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        1, c_reverseDepth
    );

    m_gltfDepth = new AMDTK::GltfDepthPass();
    m_gltfDepth->OnCreate(
        device,
        &m_gltfResources,
        c_reverseDepth
    );

    m_gltfMotionVectors = new AMDTK::GltfMotionVectorsPass();
    m_gltfMotionVectors->OnCreate(
        device,
        &m_gltfResources,
        c_reverseDepth
    );

    const RenderTargetState rtStateUI(m_deviceResources->GetFrameBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
#ifdef _GAMING_XBOX
    const RenderTargetState rtStateTextureUI(NoSRGB(m_deviceResources->GetUIBackBufferFormat()), m_deviceResources->GetDepthBufferFormat());
#else // _GAMING_DESKTOP
    const RenderTargetState rtStateTextureUI(m_deviceResources->GetUIBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
#endif // _GAMING_XBOX

    {
        // regular ui render pipeline
        SpriteBatchPipelineStateDescription pd(rtStateUI, &CommonStates::AlphaBlend);
        m_batchForBackBufferRender = std::make_unique<SpriteBatch>(device, upload, pd);
        SpriteBatchPipelineStateDescription pdTexture(rtStateTextureUI, &CommonStates::AlphaBlend);
        m_batchForUIBufferRender = std::make_unique<SpriteBatch>(device, upload, pdTexture);
    }

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    m_deviceResources->WaitForGpu();
    finish.wait();

    // Post-process quad
    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(device);

    {
        EffectPipelineStateDescription pd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtStateUI);

        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");
        auto pixelShaderBlob = DX::ReadData(L"FullScreenQuadPS.cso");

        D3D12_SHADER_BYTECODE vertexShader =
        { vertexShaderBlob.data(), vertexShaderBlob.size() };
        D3D12_SHADER_BYTECODE pixelShader =
        { pixelShaderBlob.data(), pixelShaderBlob.size() };

        auto rootSig = m_fullScreenQuad->GetRootSignature();
        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_passthroughPSO.ReleaseAndGetAddressOf());
    }

    {
        // TAA Compute pipeline
        auto computeShaderBlob = DX::ReadData(L"taa.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_taaRS.ReleaseAndGetAddressOf())));

        m_taaRS->SetName(L"TAA RS");

        // Create compute pipeline state
        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_taaRS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO,
                IID_GRAPHICS_PPV_ARGS(m_taaPSO.ReleaseAndGetAddressOf())));

        m_taaPSO->SetName(L"TAA Compute PSO");
    }

    {
        // TAA Post pass Compute pipeline
        auto computeShaderBlob = DX::ReadData(L"post_taa.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_taaPostPassRS.ReleaseAndGetAddressOf())));

        m_taaPostPassRS->SetName(L"Post TAA RS");

        // Create compute pipeline state
        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_taaPostPassRS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO,
                IID_GRAPHICS_PPV_ARGS(m_taaPostPassPSO.ReleaseAndGetAddressOf())));

        m_taaPostPassPSO->SetName(L"Post TAA Compute PSO");
    }

    {
        // ColorConversion Graphics pipeline
        auto psShaderBlob = DX::ReadData(L"ColorConversionPS.cso");
        auto vsShaderBlob = DX::ReadData(L"FullscreenVS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, psShaderBlob.data(), psShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_colorConversionRS.ReleaseAndGetAddressOf())));

        m_colorConversionRS->SetName(L"ColorConversion PS RS");

        D3D12_GRAPHICS_PIPELINE_STATE_DESC descGraphicsPSO = {};
        descGraphicsPSO.InputLayout = { nullptr, 0 };
        descGraphicsPSO.pRootSignature = m_colorConversionRS.Get();
        descGraphicsPSO.VS.pShaderBytecode = vsShaderBlob.data();
        descGraphicsPSO.VS.BytecodeLength = vsShaderBlob.size();
        descGraphicsPSO.PS.pShaderBytecode = psShaderBlob.data();
        descGraphicsPSO.PS.BytecodeLength = psShaderBlob.size();
        descGraphicsPSO.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        descGraphicsPSO.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        descGraphicsPSO.DepthStencilState.DepthEnable = FALSE;
        descGraphicsPSO.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        descGraphicsPSO.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        descGraphicsPSO.SampleMask = UINT_MAX;
        descGraphicsPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        descGraphicsPSO.NumRenderTargets = 1;
        descGraphicsPSO.RTVFormats[0] = m_deviceResources->GetFrameBackBufferFormat(),
            descGraphicsPSO.SampleDesc.Count = 1;
        descGraphicsPSO.NodeMask = 0;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&descGraphicsPSO,
                IID_GRAPHICS_PPV_ARGS(m_colorConversionPSO.ReleaseAndGetAddressOf())));

        m_colorConversionPSO->SetName(L"ColorConversion Compute PSO");

        m_colorConversionConstants = {};
        m_colorConversionConstants.u_displayMode = ColorSpace_NOP;

#ifdef _GAMING_XBOX
        if (m_bIsDisplayInHDRMode)
        {
            m_colorConversionConstants.u_displayMode = ColorSpace_REC2020;
            m_colorConversionConstants.u_displayMinLuminancePerNits = m_displayModeHdrInfo.minToneMapLuminance / 80.0f; // RGB(1, 1, 1) maps to 80 nits in scRGB;
            m_colorConversionConstants.u_displayMaxLuminancePerNits = m_displayModeHdrInfo.maxToneMapLuminance / 80.0f;
        }
#endif
    }

    {
        // Tonemapper Compute pipeline
        auto computeShaderBlob = DX::ReadData(L"TonemappingCS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_tonemapperCSRS.ReleaseAndGetAddressOf())));

        m_tonemapperCSRS->SetName(L"Tonemapper RS");

        // Create compute pipeline state
        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_tonemapperCSRS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO,
                IID_GRAPHICS_PPV_ARGS(m_tonemapperCSPSO.ReleaseAndGetAddressOf())));

        m_tonemapperCSPSO->SetName(L"Tonemapper Compute PSO");

        m_tonemapperConstants.exposure = 1.5f;
        m_tonemapperConstants.mode = 0;
    }

    {
        //Particle composition CS

        auto computeShaderBlob = DX::ReadData(L"ResolveParticles.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_resolveParticleRS.ReleaseAndGetAddressOf())));

        m_resolveParticleRS->SetName(L"ResolveParticles RS");

        // Create compute pipeline state
        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_resolveParticleRS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO,
                IID_GRAPHICS_PPV_ARGS(m_resolveParticlePSO.ReleaseAndGetAddressOf())));

        m_resolveParticlePSO->SetName(L"ResolveParticles PSO");
    }

    {
        // Magnifier Graphics pipeline
        auto psShaderBlob = DX::ReadData(L"MagnifierPS.cso");
        auto vsShaderBlob = DX::ReadData(L"FullscreenVS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, psShaderBlob.data(), psShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_magnifierRS.ReleaseAndGetAddressOf())));

        m_magnifierRS->SetName(L"Magnifier PS RS");

        D3D12_GRAPHICS_PIPELINE_STATE_DESC descGraphicsPSO = {};
        descGraphicsPSO.InputLayout = { nullptr, 0 };
        descGraphicsPSO.pRootSignature = m_magnifierRS.Get();
        descGraphicsPSO.VS.pShaderBytecode = vsShaderBlob.data();
        descGraphicsPSO.VS.BytecodeLength = vsShaderBlob.size();
        descGraphicsPSO.PS.pShaderBytecode = psShaderBlob.data();
        descGraphicsPSO.PS.BytecodeLength = psShaderBlob.size();
        descGraphicsPSO.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        descGraphicsPSO.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        descGraphicsPSO.DepthStencilState.DepthEnable = FALSE;
        descGraphicsPSO.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        descGraphicsPSO.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        descGraphicsPSO.SampleMask = UINT_MAX;
        descGraphicsPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        descGraphicsPSO.NumRenderTargets = 1;
        descGraphicsPSO.RTVFormats[0] = DXGI_FORMAT_R10G10B10A2_UNORM;
        descGraphicsPSO.SampleDesc.Count = 1;
        descGraphicsPSO.NodeMask = 0;

        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&descGraphicsPSO,
                IID_GRAPHICS_PPV_ARGS(m_magnifierPSO.ReleaseAndGetAddressOf())));

        m_magnifierPSO->SetName(L"Magnifier Compute PSO");

        //Defaults
        m_magnifierConstants = {};
        m_magnifierConstants.fBorderColorRGB[0] = 1.0f;
        m_magnifierConstants.fBorderColorRGB[3] = 1.0f;

        auto const size = m_deviceResources->GetOutputSize();
        m_magnifierConstants.uImageWidth = static_cast<uint32_t>(size.right);
        m_magnifierConstants.uImageHeight = static_cast<uint32_t>(size.bottom);
        m_magnifierConstants.iMousePos[0] = static_cast<int>(size.right / 2.35f);
        m_magnifierConstants.iMousePos[1] = static_cast<int>(size.bottom / 2.00f);
        m_magnifierConstants.fMagnificationAmount = 5.0f;
        m_magnifierConstants.fMagnifierScreenRadius = 0.25f;
        m_magnifierConstants.iMagnifierOffset[0] = 100;
        m_magnifierConstants.iMagnifierOffset[1] = 100;
    }

    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_batchForBackBufferRender->SetViewport(m_deviceResources->GetScreenViewport());
    m_batchForUIBufferRender->SetViewport(m_deviceResources->GetScreenViewport());

    auto const size = m_deviceResources->GetOutputSize();

    auto device = m_deviceResources->GetD3DDevice();
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_prevScale = GetRenderScale();

    wchar_t strFilePath[MAX_PATH] = {};

    if (size.bottom > 1080)
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_36.spritefont");
    }
    else
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    }

    m_font = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font));

    if (size.bottom > 1080)
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegend.spritefont");
    }
    else
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    }

    m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));

    if (size.bottom > 1080)
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneController.spritefont");
    }
    else
    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerSmall.spritefont");
    }

    m_colorCtrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ColorCtrlFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ColorCtrlFont));

    // Wait until assets have been uploaded to the GPU.
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    // Camera
    if (c_reverseDepth)
    {
        m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), FLT_MAX, 0.1f);
    }
    else
    {
        m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, FLT_MAX);
    }

    UINT resourceSizeWidth{ static_cast<UINT>(size.right) };
    UINT resourceSizeHeight{ static_cast<UINT>(size.bottom) };

    D3D12_CLEAR_VALUE clearValue = { DXGI_FORMAT_R16G16B16A16_FLOAT, {0.0f, 0.0f, 0.0f, 1.0f} };
    D3D12_CLEAR_VALUE clearValue0 = { DXGI_FORMAT_R16G16B16A16_FLOAT, {0.0f, 0.0f, 0.0f, 0.0f} };
    CreateResource(m_gltfScene, L"GltfScene", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &clearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_RENDER_TARGET, &m_gltfSceneState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::GltfSceneRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::GltfSceneSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::GltfSceneUAV), { 0 });

    CreateResource(m_particles, L"Particles", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &clearValue0,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_RENDER_TARGET, &m_particlesState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::ParticlesRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::ParticlesSRV), { 0 }, { 0 });

    CreateResource(m_reactive, L"FSR3Reactive", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_reactiveState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::FSR3ReactiveSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::FSR3ReactiveUAV), { 0 });

    CreateResource(m_transparency, L"FSR3TransparencyComposition", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_transparencyState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::FSR3TransparencySRV), m_resourceDescriptors->GetCpuHandle(Descriptors::FSR3TransparencyUAV), { 0 });


    D3D12_CLEAR_VALUE motionDepthClearValue = {};
    motionDepthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    motionDepthClearValue.DepthStencil.Depth = (c_reverseDepth) ? 0.0f : 1.0f;
    motionDepthClearValue.DepthStencil.Stencil = 0;

    CreateResource(m_motionVectorDepth, L"MotionVectorDepth", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &motionDepthClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_DEPTH_WRITE, &m_motionVectorDepthState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::MotionVectorDepthSRV), { 0 }, m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDepthDSV));

    D3D12_CLEAR_VALUE motionClearValue = { DXGI_FORMAT_R16G16_FLOAT, {0.0f,0.0f,0.0f,0.0f} };
    CreateResource(m_motionVectors, L"MotionVectors", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &motionClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_RENDER_TARGET, &m_motionVectorsState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::MotionVectorsRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::MotionVectorsSRV), { 0 }, { 0 });

    CreateResource(m_taaIntermediateOutput, L"TaaIntermediateOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_taaIntermediateOutputState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::TaaIntOutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::TaaIntOutputUAV), { 0 });

    CreateResource(m_taaOutput, L"TaaOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &clearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_taaOutputState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::TaaOutputRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::TaaOutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::TaaOutputUAV), { 0 });

    CreateResource(m_taaHistory, L"TaaHistory", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &clearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_taaHistoryState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::TaaHistoryRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::TaaHistorySRV), m_resourceDescriptors->GetCpuHandle(Descriptors::TaaHistoryUAV), { 0 });


    CreateResource(m_upsampleOutput, L"m_upsampleOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R10G10B10A2_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_upsampleOutputState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::Upsample_OutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::Upsample_OutputUAV), { 0 });

    CreateResource(m_magnifierOutput, L"m_magnifierOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R10G10B10A2_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_RENDER_TARGET, &m_magnifierOutputState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::MagnifierOutputRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::MagnifierOutputSRV), { 0 }, { 0 });

    // UI Targets for FSR3 composition modes
    D3D12_CLEAR_VALUE uiClearValue = { m_deviceResources->GetFrameBackBufferFormat(), {0.0f,0.0f,0.0f,0.0f} };
    CreateResource(m_uiRenderTargets[0], L"m_uiRenderTarget0", (UINT)size.right, (UINT)size.bottom,
        m_deviceResources->GetFrameBackBufferFormat(), D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &uiClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_uiRenderTargetStates[0],
        m_renderDescriptors->GetCpuHandle(RTDescriptors::UIOutput_RTV0), m_resourceDescriptors->GetCpuHandle(Descriptors::UIOutput_SRV0), { 0 }, { 0 });

    CreateResource(m_uiRenderTargets[1], L"m_uiRenderTarget1", (UINT)size.right, (UINT)size.bottom,
        m_deviceResources->GetFrameBackBufferFormat(), D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &uiClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_uiRenderTargetStates[1],
        m_renderDescriptors->GetCpuHandle(RTDescriptors::UIOutput_RTV1), m_resourceDescriptors->GetCpuHandle(Descriptors::UIOutput_SRV1), { 0 }, { 0 });

    D3D12_CLEAR_VALUE tonemapClearValue = { DXGI_FORMAT_R10G10B10A2_UNORM, {0.0f,0.0f,0.0f,1.0f} };
    CreateResource(m_tonemapperOutput, L"m_tonemapperOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R10G10B10A2_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &tonemapClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &m_tonemapperOutputState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::Tonemap_OutputRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::Tonemap_OutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::Tonemap_OutputUAV), { 0 });

    m_initialResourceClearsRequired = true;

    // shadow Atlas generation
    {
        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        CreateResource(m_shadowAtlas, L"Shadow Atlas", 4096, 4096,
            DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &depthOptimizedClearValue,
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, nullptr,
            { 0 }, { 0 }, { 0 }, m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV));

        // SRV of different format
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;

        m_shadowAtlasIdx = m_gltfResources.GetSrvPile()->Allocate();

        device->CreateShaderResourceView(m_shadowAtlas.Get(), &srvDesc, m_gltfResources.GetSrvPile()->GetCpuHandle(m_shadowAtlasIdx));
    }

    // Recalculate the mip bias
    // As per FSR 3.0 documentation
    if (!m_renderScale) {
        m_lodBias = 0.f;
    }
    else {
        m_lodBias = -log2f(m_renderScaleRatio[m_renderScale]) - 1.0f;
    }

    // Init FSR Context for current mode
    UpdateFSRContext(true);

    //////////////////////////////////////////////////////////////////////////
    //
    // AMD FidelityFX Super Resolution 3 + Frame Interpolation Guidelines
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
#ifdef _GAMING_XBOX
    m_deviceResources->RegisterFrameEvents(m_frameInterpolation);
#endif // _GAMING_XBOX

    UpdateFrameInterpolationOnPotentialPrimaryOutputChange();
}

#pragma endregion

void Sample::TransitionTo(ID3D12GraphicsCommandList* commandList, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES* stateTracker, D3D12_RESOURCE_STATES afterState)
{
    SAMPLE_ASSERT(stateTracker != nullptr);
    if (*stateTracker != afterState)
    {
        TransitionResource(commandList, resource.Get(), *stateTracker, afterState);
        *stateTracker = afterState;
    }
}

// Helper for 2D resources
void Sample::CreateResource(
    Microsoft::WRL::ComPtr<ID3D12Resource>& resource,
    LPCWSTR name,
    UINT64 width, UINT height,
    DXGI_FORMAT format,
    D3D12_RESOURCE_FLAGS flags,
    D3D12_CLEAR_VALUE* clearValue,
    CD3DX12_HEAP_PROPERTIES heapProps,
    D3D12_RESOURCE_STATES initalState,
    D3D12_RESOURCE_STATES* stateTracker,
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE uavHandle,
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle
)
{
    auto device = m_deviceResources->GetD3DDevice();

    {
        DXGI_FORMAT noSRGBFormat = format;
        noSRGBFormat = NoSRGB(format);
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.MipLevels = 1;
#ifdef _GAMING_XBOX
        if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
            texDesc.Format = noSRGBFormat;
        else
            texDesc.Format = format;
#else
        texDesc.Format = noSRGBFormat;
#endif // _GAMING_XBOX
        texDesc.Width = width;
        texDesc.Height = height;
        texDesc.Flags = flags;
        texDesc.DepthOrArraySize = 1;
        texDesc.SampleDesc.Count = 1;
        texDesc.SampleDesc.Quality = 0;
        texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                initalState,
                clearValue,
                IID_GRAPHICS_PPV_ARGS(resource.ReleaseAndGetAddressOf())));

        resource->SetName(name);

        if (rtvHandle.ptr)
        {
            SAMPLE_ASSERT(flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
#ifdef _GAMING_XBOX
            if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
                rtvDesc.Format = noSRGBFormat; // Must match creation format on Xbox
            else
                rtvDesc.Format = format;
#else
            rtvDesc.Format = format; //noSRGBFormat;
#endif // _GAMING_XBOX
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            device->CreateRenderTargetView(resource.Get(), &rtvDesc, rtvHandle);
        }

        if (uavHandle.ptr)
        {
            SAMPLE_ASSERT(flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = noSRGBFormat;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
            uavDesc.Texture2D.MipSlice = 0;
            uavDesc.Texture2D.PlaneSlice = 0;
            device->CreateUnorderedAccessView(resource.Get(), nullptr, &uavDesc, uavHandle);
        }

        if (srvHandle.ptr)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

            if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
            {
                srvDesc.Format = format == DXGI_FORMAT_R32_TYPELESS ? DXGI_FORMAT_R32_FLOAT : format;
            }
            else
            {
                srvDesc.Format = format;
            }

            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;
            srvDesc.Texture2D.PlaneSlice = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0;
            device->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);
        }

        if (dsvHandle.ptr)
        {
            SAMPLE_ASSERT(flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
            D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
            dsvDesc.Format = format == DXGI_FORMAT_R32_TYPELESS ? DXGI_FORMAT_D32_FLOAT : format;
            dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            device->CreateDepthStencilView(resource.Get(), &dsvDesc, dsvHandle);
        }

        if (stateTracker != nullptr)
        {
            *stateTracker = initalState;
        }
    }
}

#ifdef _GAMING_XBOX
void Sample::SetDisplayMode() noexcept
{
    // Request HDR mode. For frame interpolation, refresh rate is more important than HDR, so we use "PreferRefreshRate" mode.
    auto result = XDisplayTryEnableHdrMode(XDisplayHdrModePreference::PreferRefreshRate, &m_displayModeHdrInfo);

    m_bIsDisplayInHDRMode = (result == XDisplayHdrModeResult::Enabled);

#ifdef _DEBUG
    OutputDebugStringA((m_bIsDisplayInHDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif
}
#endif
