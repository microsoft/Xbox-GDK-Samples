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

#include "SuperResolution.h"

#ifdef _GAMING_DESKTOP
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#else
#include <FidelityFX/host/backends/gdk/ffx_gdk.h>
#endif // _GAMING_DESKTOP

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#pragma warning(disable : 4061)

enum ColorSpace
{
    ColorSpace_NOP,
    ColorSpace_REC2020
};

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
}

static Sample* g_pSampleInstance = nullptr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pitch(c_pitch),
    m_yaw(c_yaw),
    m_eye(c_eye),
    m_bIsDisplayInHDRMode(false),
    m_cpuTimer(std::make_unique<DX::CPUTimer>()),
    m_initialResourceClearsRequired(true),
    m_pendingMode(Mode::FSR2),
    m_currentMode(Mode::FSR2),
    m_rcasEnable(false),
    m_rcasSharpness(0.25f),
    m_deltaTime(0.0),
    m_useFSR2ReactiveMask(true),
    m_renderScaleChanged(false),
    m_renderScaleWantsChange(false),
    m_pendingRenderScale(3),
    m_renderScale(3),
    m_lodBias(-0.5f)
{
    unsigned int flags = (c_reverseDepth) ? DX::DeviceResources::c_ReverseDepth : 0;
    
    // store the sample instance to use in the allocator callback
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
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            DXGI_FORMAT_D32_FLOAT,
            3,
            flags);
    }

    m_cpuTimer->Start();
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    // Destroy FSR context
    EnableFSR(false);

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

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
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

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        Mode oldMode = m_currentMode;

        if (m_gamePadButtons.a == ButtonState::PRESSED)
        {
            int aa = static_cast<int>(m_currentMode);
            aa = (aa + 1) % static_cast<int>(Mode::Count);
            m_pendingMode = static_cast<Mode>(aa);
        }
        else if (m_gamePadButtons.b == ButtonState::PRESSED)
        {
            int aa = static_cast<int>(m_currentMode);
            aa = aa - 1;
            if (aa < 0)
                aa = static_cast<int>(Mode::Count) - 1;
            m_pendingMode = static_cast<Mode>(aa);
        }

        if (oldMode != m_pendingMode)
        {
            m_pendingRenderScale = m_renderScale;
            m_renderScaleWantsChange = true;
        }

        if (pad.IsLeftStickPressed())
        {
            m_pitch = c_pitch;
            m_yaw = c_yaw;
        }
        else
        {
            m_yaw += pad.thumbSticks.leftX * 0.02f;
            m_pitch += pad.thumbSticks.leftY * 0.02f;
        }

        if (m_gamePadButtons.x == ButtonState::PRESSED)
        {
            m_pendingRenderScale = (m_renderScale + 1) % (m_numRenderScales);
            m_renderScaleWantsChange = true;
            if (m_pendingRenderScale == 0) m_pendingRenderScale = 1;
        }
        else if (m_gamePadButtons.y == ButtonState::PRESSED)
        {
            if (m_renderScale <= 1)
            {
                m_pendingRenderScale = m_numRenderScales - 1;
            }
            else
            {
                m_pendingRenderScale = m_renderScale - 1;
            }
            m_renderScaleWantsChange = true;
        }

        if (pad.triggers.left > 0.0f)
        {
            m_tonemapperConstants.exposure -= 0.03f * pad.triggers.left;
        }
        if (pad.triggers.right > 0.0f)
        {
            m_tonemapperConstants.exposure += 0.03f * pad.triggers.right;
        }

        if (pad.IsRightShoulderPressed())
        {
            m_rcasSharpness -= 0.005f;
        }
        if (pad.IsLeftShoulderPressed())
        {
            m_rcasSharpness += 0.005f;
        }

        m_rcasSharpness = std::max(std::min(m_rcasSharpness, 1.0f), 0.f);

        if (m_gamePadButtons.dpadUp == ButtonState::PRESSED)
        {
            m_rcasEnable = !m_rcasEnable;
        }

        if (m_gamePadButtons.dpadDown == ButtonState::PRESSED)
        {
            m_useFSR2ReactiveMask = !m_useFSR2ReactiveMask;
        }
        
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

        // Clamp to 0.00f, 4.0f
        m_tonemapperConstants.exposure = std::max(std::min(m_tonemapperConstants.exposure, 4.0f), 0.50f);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    // Limit to avoid looking directly up or down
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

void Sample::EnableFSR(bool enable)
{
    if (!enable)
    {
        // tear down the current FSR Context as needed
        UpdateFSRContext(false);

        // destroy the FidelityFX interface memory
        if (m_fsr1ContextDescription.backendInterface.scratchBuffer)
        {
            free(m_fsr1ContextDescription.backendInterface.scratchBuffer);
            m_fsr1ContextDescription.backendInterface.scratchBuffer = nullptr;
        }
        if (m_fsr2ContextDescription.backendInterface.scratchBuffer)
        {
            free(m_fsr2ContextDescription.backendInterface.scratchBuffer);
            m_fsr2ContextDescription.backendInterface.scratchBuffer = nullptr;
        }
    }
    else
    {
        // re-init backends as needed
        // setup FSR1 FidelityFX interface.
        if (m_currentMode == Mode::FSR1)
        {
            const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_FSR1_CONTEXT_COUNT);
            void* scratchBuffer = calloc(scratchBufferSize, 1);
            FfxErrorCode errorCode = ffxGetInterfaceDX12(&m_fsr1ContextDescription.backendInterface, m_deviceResources->GetD3DDevice(), scratchBuffer, scratchBufferSize, FFX_FSR1_CONTEXT_COUNT);
            FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Could not initialize the FidelityFX SDK backend");

            FFX_ASSERT_MESSAGE(m_fsr1ContextDescription.backendInterface.fpGetSDKVersion(&m_fsr1ContextDescription.backendInterface) ==
                FFX_SDK_MAKE_VERSION(1, 1, 2), "FidelityFX Super Resolution sample requires linking with a 1.1.2 version SDK backend");
        }

        // setup FSR2 FidelityFX interface.
        if (m_currentMode == Mode::FSR2)
        {
            const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_FSR2_CONTEXT_COUNT);
            void* scratchBuffer = calloc(scratchBufferSize, 1);
            FfxErrorCode errorCode = ffxGetInterfaceDX12(&m_fsr2ContextDescription.backendInterface, m_deviceResources->GetD3DDevice(), scratchBuffer, scratchBufferSize, FFX_FSR2_CONTEXT_COUNT);
            FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Could not initialize the FidelityFX SDK backend");

            FFX_ASSERT_MESSAGE(m_fsr2ContextDescription.backendInterface.fpGetSDKVersion(&m_fsr2ContextDescription.backendInterface) ==
                FFX_SDK_MAKE_VERSION(1, 1, 2), "FidelityFX Super Resolution sample requires linking with a 1.1.2 version SDK backend");
        }

        // create the new FSR context as needed
        UpdateFSRContext(true);
    }
}

void Sample::UpdateFSRContext(bool enabled)
{
    auto const size = m_deviceResources->GetOutputSize();
    UINT resourceSizeWidth{ static_cast<UINT>(size.right) };
    UINT resourceSizeHeight{ static_cast<UINT>(size.bottom) };
    
    if (enabled)
    {
        FfxErrorCode errorCode = FFX_OK;

        // create the FSR1 context if needed
        if (m_currentMode == Mode::FSR1)
        {
            m_fsr1ContextDescription.outputFormat = FFX_SURFACE_FORMAT_R10G10B10A2_UNORM; // m_upscaledOutput is DXGI_FORMAT_R10G10B10A2_UNORM
            m_fsr1ContextDescription.maxRenderSize.width = resourceSizeWidth;
            m_fsr1ContextDescription.maxRenderSize.height = resourceSizeHeight;
            m_fsr1ContextDescription.displaySize.width = resourceSizeWidth;
            m_fsr1ContextDescription.displaySize.height = resourceSizeHeight;
            m_fsr1ContextDescription.flags = FFX_FSR1_ENABLE_HIGH_DYNAMIC_RANGE;

            // Create FSR1 context with a possibility to enable RCAS irrespectively of whether 'm_rcasEnable' is set.
            m_fsr1ContextDescription.flags |= FFX_FSR1_ENABLE_RCAS;

            // Create the FSR1 context
            errorCode = ffxFsr1ContextCreate(&m_fsr1Context, &m_fsr1ContextDescription);
            FFX_ASSERT(errorCode == FFX_OK);

            FFX_ASSERT_MESSAGE(ffxFsr1GetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 2, 0),
                "FidelityFX Super Resolution sample requires linking with a 1.2 version FidelityFX FSR1 library");

            // use our thread-safe buffer allocator instead of the default one (this needs to be done after the backend context created -- i.e. have 1 effect context created)
            m_fsr1ContextDescription.backendInterface.fpRegisterConstantBufferAllocator(&m_fsr1ContextDescription.backendInterface, Sample::ffxAllocateConstantBuffer);
        }

        // create the FSR2 context if needed
        if (m_currentMode == Mode::FSR2)
        {
            m_fsr2ContextDescription.maxRenderSize.width = resourceSizeWidth;
            m_fsr2ContextDescription.maxRenderSize.height = resourceSizeHeight;
            m_fsr2ContextDescription.displaySize.width = resourceSizeWidth;
            m_fsr2ContextDescription.displaySize.height = resourceSizeHeight;
            m_fsr2ContextDescription.flags = FFX_FSR2_ENABLE_HIGH_DYNAMIC_RANGE | FFX_FSR2_ENABLE_AUTO_EXPOSURE |
                                             FFX_FSR2_ENABLE_DEPTH_INVERTED | FFX_FSR2_ENABLE_DEPTH_INFINITE;

            // Do error checking in debug
#if defined(_DEBUG)
            m_fsr2ContextDescription.flags |= FFX_FSR2_ENABLE_DEBUG_CHECKING;
            m_fsr2ContextDescription.fpMessage = &Sample::FfxMsgCallback;
#endif // #if defined(_DEBUG)

            // Create the FSR2 context
            errorCode = ffxFsr2ContextCreate(&m_fsr2Context, &m_fsr2ContextDescription);
            FFX_ASSERT(errorCode == FFX_OK);

            FFX_ASSERT_MESSAGE(ffxFsr2GetEffectVersion() == FFX_SDK_MAKE_VERSION(2, 3, 2),
                "FidelityFX Super Resolution sample requires linking with a 2.3.2 version FidelityFX FSR2 library");

            // use our thread-safe buffer allocator instead of the default one (this needs to be done after the backend context created -- i.e. have 1 effect context created)
            m_fsr2ContextDescription.backendInterface.fpRegisterConstantBufferAllocator(&m_fsr2ContextDescription.backendInterface, Sample::ffxAllocateConstantBuffer);
        }
    }
    else
    {
        // destroy the FSR contexts
        if (m_currentMode == Mode::FSR1)
            ffxFsr1ContextDestroy(&m_fsr1Context);

        if (m_currentMode == Mode::FSR2)
            ffxFsr2ContextDestroy(&m_fsr2Context);
    }
}

void Sample::FfxMsgCallback(FfxMsgType type, const wchar_t* message)
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
    auto const displaysize = m_deviceResources->GetOutputSize();

    static int32_t index = 0;
    const int32_t jitterPhaseCount = ffxFsr2GetJitterPhaseCount((int32_t)width, displaysize.right);
    ffxFsr2GetJitterOffset(&m_jitterX, &m_jitterY, index, jitterPhaseCount);

    index++;
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
    perPassConstants.normalizedFormat = (m_currentMode == Mode::FSR2) ? 1 : 0;

    m_gltfMotionVectors->SetPerFrameCB(m_graphicsMemory->AllocateConstant(perPassConstants).GpuAddress());

    m_gltfMotionVectors->Draw(commandList);
    PIXEndEvent(commandList);
}

void Sample::RenderTAA(ID3D12GraphicsCommandList* commandList)
{
    auto const size = m_deviceResources->GetOutputSize();
    auto scaledSize = GetScaledRect(size);
    float inWidth = static_cast<float>(scaledSize.right);
    float inHeight = static_cast<float>(scaledSize.bottom);

    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    {
        // TAA
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"TAA");

        TransitionTo(commandList, m_motionVectors, &m_motionVectorsState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionTo(commandList, m_taaIntermediateOutput, &m_taaIntermediateOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        TransitionTo(commandList, m_taaHistory, &m_taaHistoryState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
        commandList->SetComputeRootSignature(m_taaRS.Get());

        m_taaOptions.scaleHistory = m_renderScaleChanged ? 1 : 0;
        float prevscale = 1.0f / m_renderScaleRatio[m_prevScale];
        float curscale = 1.0f / m_renderScaleRatio[GetRenderScale()];
        m_taaOptions.prevScale = prevscale / curscale;
        m_taaOptions.currentScale = curscale;
        m_taaOptions.reverseDepth = c_reverseDepth;

        auto cbHandle = m_graphicsMemory->AllocateConstant(m_taaOptions);

        commandList->SetComputeRootConstantBufferView(0, cbHandle.GpuAddress());
        commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::GltfSceneSRV));          // ColorBuffer
        commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorDepthSRV));  // DepthBuffer
        commandList->SetComputeRootDescriptorTable(3, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaHistorySRV));         // HistoryBuffer
        commandList->SetComputeRootDescriptorTable(4, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorsSRV));      // VelocityBuffer
        commandList->SetComputeRootDescriptorTable(5, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaIntOutputUAV));
        commandList->SetPipelineState(m_taaPSO.Get());


        const unsigned int threadGroupWorkRegionDim = 16u;
        UINT dispatchX = static_cast<UINT>((inWidth + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim);
        UINT dispatchY = static_cast<UINT>((inHeight + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim);

        commandList->Dispatch(dispatchX, dispatchY, 1u);

        PIXEndEvent(commandList);
    }

    {
        // Post TAA Fixup
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Post TAA");
        TransitionTo(commandList, m_taaIntermediateOutput, &m_taaIntermediateOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionTo(commandList, m_taaOutput, &m_taaOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        TransitionTo(commandList, m_taaHistory, &m_taaHistoryState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
        commandList->SetComputeRootSignature(m_taaPostPassRS.Get());
        commandList->SetComputeRootDescriptorTable(0, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaIntOutputSRV));
        commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaOutputUAV));
        commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaHistoryUAV));
        commandList->SetPipelineState(m_taaPostPassPSO.Get());

        const unsigned int threadGroupWorkRegionDim = 8u;
        UINT dispatchX = static_cast<UINT>((inWidth + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim);
        UINT dispatchY = static_cast<UINT>((inHeight + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim);

        commandList->Dispatch(dispatchX, dispatchY, 1u);

        PIXEndEvent(commandList);
    }

    m_prevScale = GetRenderScale();
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
    commandList->SetComputeRootDescriptorTable(3, m_resourceDescriptors->GetGpuHandle(Descriptors::FSR2ReactiveUAV)); // Output
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

    if (m_currentMode == Mode::FSR1)
    {
        m_tonemapperConstants.scale = 1.0f / m_renderScaleRatio[GetRenderScale()];
    }

    auto cbHandle = m_graphicsMemory->AllocateConstant(m_tonemapperConstants);
    commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
    commandList->SetComputeRootSignature(m_tonemapperCSRS.Get());
    commandList->SetComputeRootConstantBufferView(0, cbHandle.GpuAddress());

    if (m_currentMode == Mode::Native || m_currentMode == Mode::FSR1)
    {
        TransitionTo(commandList, m_taaOutput, &m_taaOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaOutputUAV));
    }
    else
    {
        TransitionTo(commandList, m_upsampleOutput, &m_upsampleOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::Upsample_OutputUAV));
    }

    TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::Tonemap_OutputUAV));
    commandList->SetPipelineState(m_tonemapperCSPSO.Get());

    auto size = m_deviceResources->GetOutputSize();

    if (m_currentMode == Mode::FSR1)
    {
        size = GetScaledRect(size);
    }

    const unsigned int threadGroupWorkRegionDim = 8u;
    unsigned int dispatchX = (size.right + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;
    unsigned int dispatchY = (size.bottom + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;

    commandList->Dispatch(dispatchX, dispatchY, 1u);
    PIXEndEvent(commandList);
}
 
void Sample::RenderMagnifier(ID3D12GraphicsCommandList* commandList)
{
    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Magnifier");

    TransitionTo(commandList, m_magnifierOutput, &m_magnifierOutputState, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
    rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::MagnifierOutputRTV);

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

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

    if (m_currentMode == Mode::FSR1)
    {
        TransitionTo(commandList, m_upsampleOutput, &m_upsampleOutputState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::Upsample_OutputSRV));
    }
    else
    {
        TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::Tonemap_OutputSRV));
    }

    commandList->SetPipelineState(m_magnifierPSO.Get());
    commandList->DrawInstanced(3, 1, 0, 0);
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

    if ((m_currentMode == Mode::FSR1) || (m_currentMode == Mode::FSR2))
    {
        m_gltfModel->GetCommonPerFrameData().lodBias = m_lodBias;
    }
    else
    {
        m_gltfModel->GetCommonPerFrameData().lodBias = 0.0f;
    }

    m_gltfModel->GetCommonPerFrameData().mCameraViewProj = m_currentViewProj;
    m_gltfModel->GetCommonPerFrameData().mInverseCameraViewProj = XMMatrixInverse(nullptr, m_gltfModel->GetCommonPerFrameData().mCameraViewProj);
    m_gltfModel->GetCommonPerFrameData().cameraPos = m_eye;
    m_gltfModel->UpdatePerFrameLights();
    m_gltfResources.SetSkinningMatricesForSkeletons();
    m_gltfResources.SetPerFrameConstants();

}


void Sample::RenderColorConversion(ID3D12GraphicsCommandList* commandList)
{
    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    // Color conversion (if in SDR mode, simply gamma2 conversion back occurs)
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Color Conversion");

    TransitionTo(commandList, m_magnifierOutput, &m_magnifierOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
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
    commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::MagnifierOutputSRV));
    commandList->SetPipelineState(m_colorConversionPSO.Get());
    commandList->DrawInstanced(3, 1, 0, 0);

    PIXEndEvent(commandList);
}

void Sample::RenderFSR1(ID3D12GraphicsCommandList* commandList)
{
	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"FidelityFX Super Resolution 1.0 - Spatial");

    TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionTo(commandList, m_upsampleOutput, &m_upsampleOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	auto size = m_deviceResources->GetOutputSize();
	auto scaledSize = GetScaledRect(size);
	float inWidth = static_cast<float>(scaledSize.right);
	float inHeight = static_cast<float>(scaledSize.bottom);

    FfxFsr1DispatchDescription dispatchParameters = {};
    dispatchParameters.commandList = ffxGetCommandListDX12(commandList);
    dispatchParameters.renderSize.width = (unsigned int)inWidth;
    dispatchParameters.renderSize.height = (unsigned int)inHeight;
    dispatchParameters.enableSharpening = m_rcasEnable;
    dispatchParameters.sharpness = m_rcasSharpness;

    dispatchParameters.color = ffxGetResourceDX12(m_tonemapperOutput.Get(),
		ffxGetResourceDescriptionDX12(m_tonemapperOutput.Get()),
		(wchar_t*)L"FSR1_InputColor", FFX_RESOURCE_STATE_COMPUTE_READ);

    dispatchParameters.output = ffxGetResourceDX12(m_upsampleOutput.Get(),
		ffxGetResourceDescriptionDX12(m_upsampleOutput.Get()),
		(wchar_t*)L"FSR1_OutputUpscaledColor", FFX_RESOURCE_STATE_COMPUTE_READ);

	m_gpuTimer->Start(commandList, 1);

	FfxErrorCode errorCode = ffxFsr1ContextDispatch(&m_fsr1Context, &dispatchParameters);
	FFX_ASSERT(errorCode == FFX_OK);

	m_gpuTimer->Stop(commandList, 1);

	PIXEndEvent(commandList);
}

void Sample::RenderFSR2(ID3D12GraphicsCommandList* commandList)
{
	TransitionTo(commandList, m_gltfScene, &m_gltfSceneState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionTo(commandList, m_motionVectors, &m_motionVectorsState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionTo(commandList, m_upsampleOutput, &m_upsampleOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionTo(commandList, m_reactive, &m_reactiveState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"FidelityFX Super Resolution 2.2.1 - Temporal");

	auto size = m_deviceResources->GetOutputSize();
	auto scaledSize = GetScaledRect(size);
	float inWidth = static_cast<float>(scaledSize.right);
	float inHeight = static_cast<float>(scaledSize.bottom);

    FfxFsr2DispatchDescription dispatchParameters = {};
    dispatchParameters.commandList = ffxGetCommandListDX12(commandList);
    dispatchParameters.color = ffxGetResourceDX12(m_gltfScene.Get(),
		ffxGetResourceDescriptionDX12(m_gltfScene.Get()),
		(wchar_t*)L"FSR2_Input_OutputColor", FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchParameters.depth = ffxGetResourceDX12(m_motionVectorDepth.Get(),
		ffxGetResourceDescriptionDX12(m_motionVectorDepth.Get()),
		(wchar_t*)L"FSR2_InputDepth", FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchParameters.motionVectors = ffxGetResourceDX12(m_motionVectors.Get(),
		ffxGetResourceDescriptionDX12(m_motionVectors.Get()),
		(wchar_t*)L"FSR2_InputMotionVectors", FFX_RESOURCE_STATE_COMPUTE_READ);
    dispatchParameters.exposure = ffxGetResourceDX12(nullptr,
		ffxGetResourceDescriptionDX12(nullptr),
		(wchar_t*)L"FSR2_InputExposure", FFX_RESOURCE_STATE_COMPUTE_READ);

	if (m_useFSR2ReactiveMask)
	{
        dispatchParameters.reactive = ffxGetResourceDX12(m_reactive.Get(),
			ffxGetResourceDescriptionDX12(m_reactive.Get()),
			(wchar_t*)L"FSR2_InputReactiveMap", FFX_RESOURCE_STATE_COMPUTE_READ);
	}
	else
	{
        dispatchParameters.reactive = ffxGetResourceDX12(nullptr,
			ffxGetResourceDescriptionDX12(nullptr),
			(wchar_t*)L"FSR2_EmptyInputReactiveMap", FFX_RESOURCE_STATE_COMPUTE_READ);
	}

	// Sample does not make use of T&C mask
    dispatchParameters.transparencyAndComposition = ffxGetResourceDX12(nullptr,
		ffxGetResourceDescriptionDX12(nullptr),
		(wchar_t*)L"FSR2_EmptyTransparencyAndCompositionMap", FFX_RESOURCE_STATE_COMPUTE_READ);

    dispatchParameters.output = ffxGetResourceDX12(m_upsampleOutput.Get(),
		ffxGetResourceDescriptionDX12(m_upsampleOutput.Get()),
		(wchar_t*)L"FSR2_UPsampledOutput", FFX_RESOURCE_STATE_COMPUTE_READ);

    dispatchParameters.jitterOffset.x = m_jitterX;
    dispatchParameters.jitterOffset.y = m_jitterY;

    dispatchParameters.frameTimeDelta = (float)m_deltaTime;
    dispatchParameters.preExposure = 1.0f;
    dispatchParameters.reset = m_renderScaleChanged;

    dispatchParameters.motionVectorScale.x = inWidth;
    dispatchParameters.motionVectorScale.y = inHeight;

    dispatchParameters.renderSize.width = (unsigned int)inWidth;
    dispatchParameters.renderSize.height = (unsigned int)inHeight;

    dispatchParameters.cameraFovAngleVertical = XM_PIDIV4;

    dispatchParameters.cameraFar = FLT_MAX;
    dispatchParameters.cameraNear = 0.1f;

    dispatchParameters.enableSharpening = m_rcasEnable;
    dispatchParameters.sharpness = m_rcasSharpness;

	m_gpuTimer->Start(commandList, 1);

	FfxErrorCode errorCode = ffxFsr2ContextDispatch(&m_fsr2Context, &dispatchParameters);
	FFX_ASSERT(errorCode == FFX_OK);

	m_gpuTimer->Stop(commandList, 1);

	PIXEndEvent(commandList);
}

#pragma region Frame Render

static unsigned int AU1_AF1(float a) { union { float f; unsigned int u; }bits; bits.f = a; return bits.u; }

// Draws the scene.
void Sample::Render()
{
    m_cpuTimer->Stop();
    m_deltaTime = m_cpuTimer->GetElapsedMS();
    m_cpuTimer->Start();

    auto currentFrame = m_timer.GetFrameCount();
    // Apply projection jitter
    // If we're in the process of changing renderscale, reuse previous frames matrix to reduce the appearance
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

    // Don't try to render anything before the first Update.
    if (currentFrame < 1)
    {
        m_deviceResources->WaitForGpu();
        return;
    } 

    // Perform the update here so it has updated projection
    m_particleSystem.Update(float(m_timer.GetElapsedSeconds()), m_view, m_proj);

    if (m_renderScaleChanged)
    {
        m_deviceResources->WaitForGpu();
    }

    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    m_gpuTimer->BeginFrame(commandList);

    // Whole-frame timer
    m_gpuTimer->Start(commandList, 0);

    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");
     
    UpdateGLTFState();
    RenderShadowMaps(commandList);
    RenderMotionVectors(commandList);
    RenderGTLFScene(commandList);

    // pre TAA particle compose
    RenderParticlesIntoScene(commandList);

    {
        // TAA timer
        m_gpuTimer->Start(commandList, 3);

        // FSR2 replaces TAA
        if (m_currentMode != Mode::FSR2)
        {
            RenderTAA(commandList);
        }

        m_gpuTimer->Stop(commandList, 3);
    }

    if (m_currentMode == Mode::FSR2)
    {
        RenderFSR2(commandList);
    }

    // Move to render function
    if (m_currentMode == Mode::Bilinear)
    {

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Bilinear Upsample");

        TransitionTo(commandList, m_upsampleOutput, &m_upsampleOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        TransitionTo(commandList, m_taaOutput, &m_taaOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        auto size = m_deviceResources->GetOutputSize();
        float outWidth = static_cast<float>(size.right);
        float outHeight = static_cast<float>(size.bottom);

        auto scaledSize = GetScaledRect(size);
        float inWidth = static_cast<float>(scaledSize.right);
        float inHeight = static_cast<float>(scaledSize.bottom);

        //Bilinear upsample uses the FSR base shader with the same constant structure.
        m_bilinearConsts.Const0.x = AU1_AF1(inWidth * 1.f / outWidth);
        m_bilinearConsts.Const0.y = AU1_AF1(inHeight * 1.f / outHeight);
        m_bilinearConsts.Const0.z = AU1_AF1((0.5f * inWidth * 1.f / outWidth) - 0.5f);
        m_bilinearConsts.Const0.w = AU1_AF1((0.5f * inHeight * 1.f / outHeight) - 0.5f);

        // Viewport pixel position to normalized image space.
        // This is used to get upper-left of 'F' tap.
        m_bilinearConsts.Const1.x = AU1_AF1(1.f / outWidth);
        m_bilinearConsts.Const1.y = AU1_AF1(1.f / outHeight);

        // Centers of gather4, first offset from upper-left of 'F'.
        //      +---+---+
        //      |   |   |
        //      +--(0)--+
        //      | b | c |
        //  +---F---+---+---+
        //  | e | f | g | h |
        //  +--(1)--+--(2)--+
        //  | i | j | k | l |
        //  +---+---+---+---+
        //      | n | o |
        //      +--(3)--+
        //      |   |   |
        //      +---+---+
        m_bilinearConsts.Const1.z = AU1_AF1(1.f / outWidth);
        m_bilinearConsts.Const1.w = AU1_AF1(-1.f / outHeight);

        auto cbHandle = m_graphicsMemory->AllocateConstant(m_bilinearConsts);

        commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
        commandList->SetComputeRootSignature(m_bilinearUpsampleRS.Get());
        commandList->SetComputeRootConstantBufferView(0, cbHandle.GpuAddress());
        commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaOutputSRV));
        commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::Upsample_OutputUAV));
        commandList->SetPipelineState(m_bilinearUpsamplePSO.Get());

        const unsigned int threadGroupWorkRegionDim = 16u;
        unsigned int dispatchX = (size.right + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;
        unsigned int dispatchY = (size.bottom + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;

        commandList->Dispatch(dispatchX, dispatchY, 1u);
        PIXEndEvent(commandList);
    }

    // tonemapping should go after fsr2 but before fsr1
    RenderTonemapping(commandList);

    if (m_currentMode == Mode::FSR1)
    {
        RenderFSR1(commandList);
    }
     
    RenderMagnifier(commandList);
    RenderColorConversion(commandList);

    m_prevProj = m_proj;
    m_prevView = m_view;
    m_prevViewProj = m_view * m_proj;
     
    PIXEndEvent(commandList);

    // Render UI
    RenderUI(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");

    m_gpuTimer->Stop(commandList, 0);
    m_gpuTimer->EndFrame(commandList);
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

    PIXEndEvent();

    if (m_renderScaleWantsChange)
    { 
        m_deviceResources->WaitForGpu();
        
        m_renderScaleChanged = true;
        EnableFSR(false);
        m_renderScale = m_pendingRenderScale;
        m_currentMode = m_pendingMode;
        EnableFSR(true);

        // Whilst we allow changing of the bias- when we change FSR renderscale, we snap back to
        // the default bias
        if (m_currentMode == Mode::FSR2)
        {
            // As per FSR 2.0 documentation
            m_lodBias = -log2f(m_renderScaleRatio[m_renderScale]) - 1.0f;
        }
        else
        {
            // As per FSR 1.0 documentation: Mipbias = -log2(displayres/sourceres)
            m_lodBias = -log2f(m_renderScaleRatio[m_renderScale]);
        }
        m_renderScaleWantsChange = false;
    }
    else
    {
        m_renderScaleChanged = false;
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

    auto const size = m_deviceResources->GetOutputSize();
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    m_batch->Begin(commandList);

    float y = float(safe.top);

    if (m_bIsDisplayInHDRMode)
    {
        m_font->DrawString(m_batch.get(), L"AMD FidelityFX Super Resolution Sample (HDR Display)",
            XMFLOAT2(float(safe.left), y), ATG::Colors::White);
    }
    else
    {
        m_font->DrawString(m_batch.get(), L"AMD FidelityFX Super Resolution Sample (SDR Display)",
            XMFLOAT2(float(safe.left), y), ATG::Colors::White);
    }

    wchar_t textBuffer[128] = {};

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Target Resolution %d x %d", size.right, size.bottom);
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Frametime:  %0.3fms", m_gpuTimer->GetElapsedMS(0));
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L" TAA Time:  %0.3fms", m_gpuTimer->GetElapsedMS(3));
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);


    if (m_currentMode == Mode::FSR1)
    {
        y += m_font->GetLineSpacing();
        y += m_font->GetLineSpacing();

        m_font->DrawString(m_batch.get(), L"FSR 1.2 Upscale", XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        y += m_font->GetLineSpacing();

        auto scaled = GetScaledRect(size);
        swprintf_s(textBuffer, L"Render Resolution: %d x %d ( %ls Mode, %1.1fx scale)", (int)(scaled.right),
            (int)(scaled.bottom), m_fsrModes[GetRenderScale()], m_renderScaleRatio[GetRenderScale()]);

        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        y += m_font->GetLineSpacing();
        if (m_rcasEnable)
            swprintf_s(textBuffer, L"FSR (EASU + RCAS) Time:  %0.3fms", m_gpuTimer->GetAverageMS(1));
        else
            swprintf_s(textBuffer, L"FSR (EASU) Time:  %0.3fms", m_gpuTimer->GetAverageMS(1));

        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        if (m_rcasEnable)
        {
            y += m_font->GetLineSpacing();
            swprintf_s(textBuffer, L"RCAS Sharpness:  %1.3f", m_rcasSharpness);
            m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);
        }


        y += m_font->GetLineSpacing();
    }
    else if (m_currentMode == Mode::FSR2)
    {
        y += m_font->GetLineSpacing();
        y += m_font->GetLineSpacing();

        m_font->DrawString(m_batch.get(), L"FSR 2.3.2 Upscale", XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        y += m_font->GetLineSpacing();

        auto scaled = GetScaledRect(size);
        swprintf_s(textBuffer, L"Render Resolution: %d x %d ( %ls Mode, %1.1fx scale)", (int)(scaled.right),
            (int)(scaled.bottom), m_fsrModes[GetRenderScale()], m_renderScaleRatio[GetRenderScale()]);

        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        y += m_font->GetLineSpacing();
        if (m_rcasEnable)
        {
            swprintf_s(textBuffer, L"FSR 2.3.2 Time:  %0.3fms (Sharpening Enabled)", m_gpuTimer->GetAverageMS(1));
        }
        else
        {
            swprintf_s(textBuffer, L"FSR 2.3.2 Time:  %0.3fms", m_gpuTimer->GetAverageMS(1));
        }
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);


        if (m_useFSR2ReactiveMask)
        {
            y += m_font->GetLineSpacing();

            m_font->DrawString(m_batch.get(), L"Using Reactive Mask for particles", XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        }
        else
        {
            y += m_font->GetLineSpacing();

            swprintf_s(textBuffer, L"No reactive mask input");
            m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        }

        if (m_rcasEnable) {
            y += m_font->GetLineSpacing();
            swprintf_s(textBuffer, L"Sharpness:  %1.3f", 1.0f-m_rcasSharpness);
            m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);
        }
    }
    else if (m_currentMode == Mode::Bilinear)
    {
        y += m_font->GetLineSpacing();
        y += m_font->GetLineSpacing();
        m_font->DrawString(m_batch.get(), L"Bilinear Upscale", XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        y += m_font->GetLineSpacing();

        auto scaled = GetScaledRect(size);
        swprintf_s(textBuffer, L"Render Resolution:  %d x %d ",  (int)(scaled.right), (int)(scaled.bottom));

        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);
    }
    else
    {
        y += m_font->GetLineSpacing();
        y += m_font->GetLineSpacing();
        m_font->DrawString(m_batch.get(), L"Native", XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        y += m_font->GetLineSpacing();
        swprintf_s(textBuffer, L"Render Resolution:  %d x %d ", (int)(size.right), (int)(size.bottom));
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    }

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Exposure:  %1.3f", m_tonemapperConstants.exposure);
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Mip Bias:  %1.2f", m_gltfModel->GetCommonPerFrameData().lodBias);
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    const wchar_t* legendStr = L"[View] Exit [LThumb] View  [A]/[B] FSR/Bilinear/Native [X]/[Y] Change Render Resolution \n[DPad]-UP Sharpening On/Off [RB]/[LB] Sharpness [RT]/[LT] Exposure\n[DPad]-Left/Right Mip Bias FSR adjustment [RThumb] Move magnifier [DPad]-Down FSR2 Reactive Mask Usage";

    DX::DrawControllerString(m_batch.get(),
        m_font.get(), m_ctrlFont.get(),
        legendStr, XMFLOAT2(float(safe.left), float(safe.bottom) - m_ctrlFont->GetLineSpacing()*2),
        ATG::Colors::LightGrey);

    m_batch->End();

    PIXEndEvent(commandList);
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();

    // Scale depending on current renderscale
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

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    // Need to destroy the current context on resource resize
    m_deviceResources->WaitForGpu();
    EnableFSR(false);

    // This will re-create the context
    CreateWindowSizeDependentResources();
}

void Sample::OnDeviceLost()
{
    EnableFSR(false);

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
        RTDescriptors::RTCount);

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
        assert(false);
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

    const RenderTargetState rtStateUI(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    {
        SpriteBatchPipelineStateDescription pd(rtStateUI, &CommonStates::AlphaBlend);
        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
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
        // Bilinear upsample pipeline
        auto computeShaderBlob = DX::ReadData(L"bilinear_upsample.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_bilinearUpsampleRS.ReleaseAndGetAddressOf())));

        m_bilinearUpsampleRS->SetName(L"bilinear_upsample RS");

        // Create compute pipeline state
        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_bilinearUpsampleRS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO,
                IID_GRAPHICS_PPV_ARGS(m_bilinearUpsamplePSO.ReleaseAndGetAddressOf())));

        m_bilinearUpsamplePSO->SetName(L"bilinear_upsample Compute PSO");
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
        descGraphicsPSO.RTVFormats[0] = m_deviceResources->GetBackBufferFormat(),
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
        m_magnifierConstants.iMousePos[0] = static_cast<int>(size.right/2.35f);
        m_magnifierConstants.iMousePos[1] = static_cast<int>(size.bottom/2.00f);
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
    m_batch->SetViewport(m_deviceResources->GetScreenViewport());

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

    CreateResource(m_reactive, L"FSR2Reactive", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_reactiveState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::FSR2ReactiveSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::FSR2ReactiveUAV), { 0 });

    CreateResource(m_transparency, L"FSR2TransparencyComposition", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_transparencyState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::FSR2TransparencySRV), m_resourceDescriptors->GetCpuHandle(Descriptors::FSR2TransparencyUAV), { 0 });


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

    CreateResource(m_rcasOutput, L"m_rcasOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R10G10B10A2_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_rcasOutputState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::RCAS_OutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::RCAS_OutputUAV), { 0 });

    CreateResource(m_magnifierOutput, L"m_magnifierOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R10G10B10A2_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_RENDER_TARGET, &m_magnifierOutputState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::MagnifierOutputRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::MagnifierOutputSRV), { 0 }, { 0 });

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
    if (m_currentMode == Mode::FSR2)
    {
        // As per FSR 2.0 documentation
        m_lodBias = -log2f(m_renderScaleRatio[m_renderScale]) - 1.0f;
    }
    else
    {
        // As per FSR 1.0 documentation: Mipbias = -log2(displayres/sourceres)
        m_lodBias = -log2f(m_renderScaleRatio[m_renderScale]);
    }

    // Init FSR Context for current mode
    EnableFSR(true);
}

#pragma endregion

void Sample::TransitionTo(ID3D12GraphicsCommandList* commandList, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES* stateTracker, D3D12_RESOURCE_STATES afterState)
{
    assert(stateTracker != nullptr);
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

    const CD3DX12_HEAP_PROPERTIES heapProperties(heapProps);

    {
        D3D12_RESOURCE_DESC texDesc = {};
        texDesc.MipLevels = 1;
        texDesc.Format = format;
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
            assert(flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
            D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
            rtvDesc.Format = format;
            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
            device->CreateRenderTargetView(resource.Get(), &rtvDesc, rtvHandle);
        }

        if (uavHandle.ptr)
        {
            assert(flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.Format = format;
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
            assert(flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
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
    // Request HDR mode.
    auto result = XDisplayTryEnableHdrMode(XDisplayHdrModePreference::PreferHdr, &m_displayModeHdrInfo);

    m_bIsDisplayInHDRMode = (result == XDisplayHdrModeResult::Enabled);

#ifdef _DEBUG
    OutputDebugStringA((m_bIsDisplayInHDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif
}
#endif
