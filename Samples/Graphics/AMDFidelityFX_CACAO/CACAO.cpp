//--------------------------------------------------------------------------------------
// CACAO.cpp
//
// Combined Adaptive Compute Ambient Occlusion (CACAO)
//
// Modifications Copyright (C) 2021. Advanced Micro Devices, Inc. All Rights Reserved.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"
#include "FindMedia.h" 

#include "CACAO.h"
#ifdef _GAMING_DESKTOP
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#else
#include <FidelityFX/host/backends/gdk/ffx_gdk.h>
#endif // _GAMING_DESKTOP
#include <FidelityFX/host/ffx_types.h>

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

}

static Sample* g_pSampleInstance = nullptr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pitch(c_pitch),
    m_yaw(c_yaw),
    m_eye(c_eye),
    m_bIsDisplayInHDRMode(false),
    m_currentCamera(0),
    m_cacaoPreset(1),
    m_cacaoEnabled(true),
    m_compositeConstants({ 1.0f, 0 })
{
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
        m_deviceResources = std::make_unique<DX::DeviceResources>(
            hdrFormat,
            DXGI_FORMAT_D32_FLOAT,
            3,
            DX::DeviceResources::c_EnableHDR);
    }
    else
#endif
    {
        m_bIsDisplayInHDRMode = false;

        // PC just has an SDR path.
        m_deviceResources = std::make_unique<DX::DeviceResources>(
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            DXGI_FORMAT_D32_FLOAT,
            3);
    }
    
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
    
    DestroyCacaoContexts();

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

    CreateCacaoContexts();
}

void Sample::CreateCacaoContexts()
{
    const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_CACAO_CONTEXT_COUNT * 2);
    void* scratchBuffer = calloc(scratchBufferSize, 1u);
    FfxErrorCode errorCode = ffxGetInterfaceDX12(&m_ffxInterface, m_deviceResources->GetD3DDevice(), scratchBuffer,
        scratchBufferSize, FFX_CACAO_CONTEXT_COUNT * 2);
    FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Could not initialize FidelityFX SDK backend context.");
    FFX_ASSERT_MESSAGE(m_ffxInterface.fpGetSDKVersion(&m_ffxInterface) == FFX_SDK_MAKE_VERSION(1, 1, 4),
        "FidelityFX CACAO sample requires linking with a 1.1.4 version SDK backend");
    FFX_ASSERT_MESSAGE(ffxCacaoGetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 4, 0),
        "FidelityFX Cacao sample requires linking with a 1.4 version FidelityFX Cacao library");

    auto const size = m_deviceResources->GetOutputSize();

    FfxCacaoContextDescription description = {};
    description.backendInterface = m_ffxInterface;
    description.width = static_cast<uint32_t>(size.right);
    description.height = static_cast<uint32_t>(size.bottom);
    description.useDownsampledSsao = false;
    errorCode = ffxCacaoContextCreate(&m_cacaoContext, &description);
    FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Could not initialize FidelityFX SDK backend context.");
    description.useDownsampledSsao = true;
    errorCode = ffxCacaoContextCreate(&m_cacaoDownsampledContext, &description);
    FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Could not initialize FidelityFX SDK downsampled backend context.");

    // use a custom constant buffer allocator
    m_ffxInterface.fpRegisterConstantBufferAllocator(&m_ffxInterface, ffxAllocateConstantBuffer);
}

void Sample::DestroyCacaoContexts()
{
    // Destroy CACAO Contexts
    ffxCacaoContextDestroy(&m_cacaoContext);
    ffxCacaoContextDestroy(&m_cacaoDownsampledContext);

    // Destroy the FidelityFX interface memory
    free(m_ffxInterface.scratchBuffer);
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

        if (m_gamePadButtons.a == ButtonState::PRESSED)
        {
            uint32_t nextPreset = m_cacaoPreset;
            nextPreset = (nextPreset + 1) % FFX_CACAO_PRESET_COUNT;
            m_cacaoPreset = nextPreset;
            m_taaOptions.reset = 1;
        }
        else if (m_gamePadButtons.b == ButtonState::PRESSED)
        {
            uint32_t nextPreset = m_cacaoPreset;
            if (nextPreset == 0)
            {
                nextPreset = FFX_CACAO_PRESET_COUNT - 1;
            }
            else
            {
                nextPreset--;
            }
            m_cacaoPreset = nextPreset;
            m_taaOptions.reset = 1;
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
            m_cacaoEnabled = !m_cacaoEnabled;
            m_taaOptions.reset = 1;
        }

        if (m_gamePadButtons.y == ButtonState::PRESSED)
        {
            m_compositeConstants.cacaoBufferView = !m_compositeConstants.cacaoBufferView;
            if (m_cacaoEnabled)
            {
                // if cacao view is currently enabled, the scene will change with this button press.
                // Reset the TAA history so we snap to the new scene instead of accumulating to it.
                m_taaOptions.reset = 1;
            }
        }

        if (pad.triggers.left > 0.0f)
        {
            m_compositeConstants.cacaoAOFactor -= 0.03f * pad.triggers.left;
        }
        if (pad.triggers.right > 0.0f)
        {
            m_compositeConstants.cacaoAOFactor += 0.03f * pad.triggers.right;
        }

        m_compositeConstants.cacaoAOFactor = std::max(std::min(m_compositeConstants.cacaoAOFactor, 1.5f), 0.5f);
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

    m_cacaoSettings = s_FfxCacaoPresets[m_cacaoPreset].settings;
    PIXEndEvent();
}
#pragma endregion

void Sample::SetTAAProjectionJitter(uint32_t width, uint32_t height, uint32_t &sampleIndex)
{
    static const auto CalculateHaltonNumber = [](uint32_t index, uint32_t base)
    {
        float f = 1.0f, result = 0.0f;

        for (uint32_t i = index; i > 0;)
        {
            f /= static_cast<float>(base);
            result = result + f * static_cast<float>(i % base);
            i = static_cast<uint32_t>(floorf(static_cast<float>(i) / static_cast<float>(base)));
        }

        return result;
    };

    sampleIndex = (sampleIndex + 1) % 8;

    float jitterX = 2.0f * CalculateHaltonNumber(sampleIndex+1, 2) - 1.0f;
    float jitterY = 2.0f * CalculateHaltonNumber(sampleIndex+1, 3) - 1.0f;

    jitterX /= static_cast<float>(width);
    jitterY /= static_cast<float>(height);

    m_proj = m_proj * XMMatrixTranslation(jitterX, jitterY, 0);
}

FfxSurfaceFormat GetFfxSurfaceFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case (DXGI_FORMAT_R32G32B32A32_TYPELESS):
        return FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
    case (DXGI_FORMAT_R32G32B32A32_FLOAT):
        return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case (DXGI_FORMAT_R16G16B16A16_FLOAT):
        return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    case (DXGI_FORMAT_R32G32_FLOAT):
        return FFX_SURFACE_FORMAT_R32G32_FLOAT;
    case (DXGI_FORMAT_R8_UINT):
        return FFX_SURFACE_FORMAT_R8_UINT;
    case (DXGI_FORMAT_R32_UINT):
        return FFX_SURFACE_FORMAT_R32_UINT;
    case (DXGI_FORMAT_R8G8B8A8_TYPELESS):
        return FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
    case (DXGI_FORMAT_R8G8B8A8_UNORM):
        return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case (DXGI_FORMAT_R8G8B8A8_UNORM_SRGB):
        return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
    case (DXGI_FORMAT_B8G8R8A8_UNORM_SRGB):
        return FFX_SURFACE_FORMAT_B8G8R8A8_SRGB;
    case (DXGI_FORMAT_R11G11B10_FLOAT):
        return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
    case (DXGI_FORMAT_R16G16_FLOAT):
        return FFX_SURFACE_FORMAT_R16G16_FLOAT;
    case (DXGI_FORMAT_R16G16_UINT):
        return FFX_SURFACE_FORMAT_R16G16_UINT;
    case (DXGI_FORMAT_R16_FLOAT):
        return FFX_SURFACE_FORMAT_R16_FLOAT;
    case (DXGI_FORMAT_R16_UINT):
        return FFX_SURFACE_FORMAT_R16_UINT;
    case (DXGI_FORMAT_R16_UNORM):
        return FFX_SURFACE_FORMAT_R16_UNORM;
    case (DXGI_FORMAT_R16_SNORM):
        return FFX_SURFACE_FORMAT_R16_SNORM;
    case (DXGI_FORMAT_R8_UNORM):
        return FFX_SURFACE_FORMAT_R8_UNORM;
    case DXGI_FORMAT_R8G8_UNORM:
        return FFX_SURFACE_FORMAT_R8G8_UNORM;
    case DXGI_FORMAT_R32_TYPELESS:
        return FFX_SURFACE_FORMAT_R32_UINT;
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT:
        return FFX_SURFACE_FORMAT_R32_FLOAT;
    case (DXGI_FORMAT_UNKNOWN):
        return FFX_SURFACE_FORMAT_UNKNOWN;
    case DXGI_FORMAT_R32G32B32A32_UINT:
        return FFX_SURFACE_FORMAT_R32G32B32A32_UINT;
    default:
        assert(false);
        return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

FfxResourceDescription GetFfxResourceDescription(ID3D12Resource* pResource)
{
    FfxResourceDescription resourceDescription = { };

    // This is valid
    if (!pResource)
        return resourceDescription;

    D3D12_RESOURCE_DESC desc = pResource->GetDesc();

    if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
    {
        resourceDescription.flags = FFX_RESOURCE_FLAGS_NONE;
        resourceDescription.usage = FFX_RESOURCE_USAGE_UAV;
        resourceDescription.width = static_cast<uint32_t>(desc.Width);
        resourceDescription.height = static_cast<uint32_t>(desc.Height);
        resourceDescription.format = FFX_SURFACE_FORMAT_UNKNOWN;

        // Does not apply to buffers
        resourceDescription.depth = 0;
        resourceDescription.mipCount = 0;

        // Set the type
        resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
    }
    else
    {
        // Set flags properly for resource registration
        resourceDescription.flags = FFX_RESOURCE_FLAGS_NONE;
        resourceDescription.usage = (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) ? FFX_RESOURCE_USAGE_DEPTHTARGET : FFX_RESOURCE_USAGE_READ_ONLY;
        if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
            resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_UAV);

        resourceDescription.width = static_cast<uint32_t>(desc.Width);
        resourceDescription.height = static_cast<uint32_t>(desc.Height);
        resourceDescription.depth = desc.DepthOrArraySize;
        resourceDescription.mipCount = desc.MipLevels;
        resourceDescription.format = GetFfxSurfaceFormat(desc.Format);

        switch (desc.Dimension)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE1D;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
            break;
        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE3D;
            break;
        case D3D12_RESOURCE_DIMENSION_UNKNOWN:
        default:
            FFX_ASSERT(false);
            break;
        }
    }

    return resourceDescription;
}

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Apply TAA projection jitter
    // If we're in the process of changing renderscale, reuse previous frames matrix to reduce the appearance
    // of pixel offsets in the switch
    {
        static uint32_t JitterSeed = 0;
        auto size = m_deviceResources->GetOutputSize();
        m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 1000.0f);
        SetTAAProjectionJitter(uint32_t(size.right), uint32_t(size.bottom), JitterSeed);
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

    m_gpuTimer->Start(commandList, 0);

    TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_RENDER_TARGET);
    TransitionTo(commandList, m_taaOutput, &m_taaOutputState, D3D12_RESOURCE_STATE_RENDER_TARGET);

    const FLOAT clearcolors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTargetView(m_renderDescriptors->GetCpuHandle(RTDescriptors::Tonemap_OutputRTV), clearcolors, 0, nullptr);
    commandList->ClearRenderTargetView(m_renderDescriptors->GetCpuHandle(RTDescriptors::TaaOutputRTV), clearcolors, 0, nullptr);

    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GLTF");

    m_gltfModel->GetCommonPerFrameData().iblFactor = 0.80f;
    m_gltfModel->GetCommonPerFrameData().emmisiveFactor = 0.9f;

    m_gltfModel->SetAnimationTime(0, 0);
    m_gltfModel->TransformScene(0, XMMatrixScaling(-1, 1, 1));

    DirectX::XMMATRIX currentViewProj = m_view * m_proj;
    DirectX::XMMATRIX prevViewCurrProj = m_prevView * m_proj;

    m_gltfModel->GetCommonPerFrameData().mCameraViewProj = currentViewProj;
    m_gltfModel->GetCommonPerFrameData().mInverseCameraViewProj = XMMatrixInverse(nullptr, m_gltfModel->GetCommonPerFrameData().mCameraViewProj);
    m_gltfModel->GetCommonPerFrameData().cameraPos = m_eye;
    m_gltfModel->UpdatePerFrameLights();
    m_gltfResources.SetSkinningMatricesForSkeletons();
    m_gltfResources.SetPerFrameConstants();

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
    }

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Shadow Atlas");

    commandList->ClearDepthStencilView(m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    constexpr uint32_t maxShadowLights = 4u; // max shadow lights supported
    uint32_t numShadowLights = 0u;
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
        D3D12_VIEWPORT viewPort = { static_cast<float>(topLeftX), static_cast<float>(topLeftY), static_cast<float>(viewportWidth), static_cast<float>(viewportHeight), 0.0f, 1.0f };
        commandList->RSSetViewports(1, &viewPort);

        // Create scissor rectangle
        D3D12_RECT rectScissor = { (LONG)topLeftX, (LONG)topLeftY, (LONG)(topLeftX + viewportWidth), (LONG)(topLeftY + viewportHeight) };
        commandList->RSSetScissorRects(1, &rectScissor);

        auto shadowAtlasCPUHandle{ m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV) };
        commandList->OMSetRenderTargets(0, NULL, true, &shadowAtlasCPUHandle);

        AMDTK::GltfDepthPass::per_frame perPassConstants;

        perPassConstants.mViewProj = m_gltfModel->GetCommonPerFrameData().lights[i].mLightViewProj;

        m_gltfDepth->SetPerFrameCB(m_graphicsMemory->AllocateConstant(perPassConstants).GpuAddress());

        m_gltfDepth->Draw(commandList);

        if (++numShadowLights >= maxShadowLights)
        {
            // Early out, as we have no more shadow atlas available
            break;
        }
    }

    TransitionResource(commandList, m_shadowAtlas.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    PIXEndEvent(commandList);

    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Motion Vectors Pass");

        TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        TransitionTo(commandList, m_motionVectors, &m_motionVectorsState, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->ClearDepthStencilView(m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDepthDSV), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        float clearValues[4] = { 0.0f , 0.0f , 0.0f , 0.0f };
        commandList->ClearRenderTargetView(m_renderDescriptors->GetCpuHandle(RTDescriptors::MotionVectorsRTV), &clearValues[0], 0, nullptr);

        auto viewport = m_deviceResources->GetScreenViewport();
        auto scissorRect = m_deviceResources->GetScissorRect();

        viewport.Width = static_cast<float>(scissorRect.right);
        viewport.Height = static_cast<float>(scissorRect.bottom);

        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvs[1];
        rtvs[0] = m_renderDescriptors->GetCpuHandle(RTDescriptors::MotionVectorsRTV);

        auto motionDepthCPUHandle{ m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDepthDSV) };

        commandList->OMSetRenderTargets(1, &rtvs[0], false, &motionDepthCPUHandle);

        AMDTK::GltfMotionVectorsPass::per_frame perPassConstants;
        perPassConstants.mCurrViewProj = currentViewProj;
        perPassConstants.mPrevViewProj = prevViewCurrProj;
        perPassConstants.normalizedFormat = 0;

        m_gltfMotionVectors->SetPerFrameCB(m_graphicsMemory->AllocateConstant(perPassConstants).GpuAddress());

        m_gltfMotionVectors->Draw(commandList);
        PIXEndEvent(commandList);
    }

    TransitionTo(commandList, m_gltfScene, &m_gltfSceneState, D3D12_RESOURCE_STATE_RENDER_TARGET);

    Clear();

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"PBR Pass");
    m_gltfPBR->Draw(commandList, m_gltfResources.GetSrvPile()->GetGpuHandle(m_shadowAtlasIdx));
    TransitionResource(commandList, m_shadowAtlas.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    PIXEndEvent(commandList);


    PIXEndEvent(commandList);
    if (m_cacaoEnabled)
    {
        //CACAO 
        {
            m_gpuTimer->Start(commandList, 1);
            FfxFloat32x4x4 proj, normalsWorldToView;
            {
                XMFLOAT4X4 p;
                XMMATRIX xProj = (m_proj);
                XMStoreFloat4x4(&p, xProj);
                proj[0] = p._11;    proj[1] = p._12;    proj[2] = p._13;    proj[3] = p._14;
                
                //Set projection matrix.
                memcpy(proj, &p, sizeof(FfxFloat32x4x4));

                XMMATRIX xView = m_view;
                XMMATRIX xNormalsWorldToView = (XMMatrixInverse(NULL, xView));

                XMStoreFloat4x4(&p, xNormalsWorldToView);
                memcpy(normalsWorldToView, &p, sizeof(FfxFloat32x4x4));
            }

            m_cacaoSettings.generateNormals = false;
            FfxErrorCode errorCode = ffxCacaoUpdateSettings(m_useDownsampledSSAO ? &m_cacaoDownsampledContext : &m_cacaoContext, &m_cacaoSettings, m_useDownsampledSSAO);
            FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Error returned from ffxCacaoUpdateSettings");

            TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_normals, &m_normalsState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_cacaoOutput, &m_cacaoOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            FfxCacaoDispatchDescription dispatchDescription = {};
            dispatchDescription.commandList = ffxGetCommandListDX12(commandList);            
            dispatchDescription.depthBuffer = ffxGetResourceDX12(m_motionVectorDepth.Get(), GetFfxResourceDescription(m_motionVectorDepth.Get()), L"CacaoInputDepth", FFX_RESOURCE_STATE_COMPUTE_READ);
            dispatchDescription.normalBuffer = ffxGetResourceDX12(m_normals.Get(), GetFfxResourceDescription(m_normals.Get()), L"CacaoInputNormal", FFX_RESOURCE_STATE_COMPUTE_READ);
            dispatchDescription.outputBuffer = ffxGetResourceDX12((m_cacaoOutput.Get()), GetFfxResourceDescription(m_cacaoOutput.Get()), L"CacaoInputOutput", FFX_RESOURCE_STATE_UNORDERED_ACCESS);
            dispatchDescription.proj = &proj;
            dispatchDescription.normalsToView = &normalsWorldToView;
            dispatchDescription.normalUnpackMul = 2;
            dispatchDescription.normalUnpackAdd = -1;
            errorCode = ffxCacaoContextDispatch(m_useDownsampledSSAO ? &m_cacaoDownsampledContext : &m_cacaoContext, &dispatchDescription);
            FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Error returned from ffxCacaoContextDispatch");
            m_gpuTimer->Stop(commandList, 1);
        }

        {
            //Composite AO
            PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Compose AO");
            
            TransitionTo(commandList, m_finalScene, &m_finalSceneState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            TransitionTo(commandList, m_gltfScene, &m_gltfSceneState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            TransitionTo(commandList, m_cacaoOutput, &m_cacaoOutputState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

            auto size = m_deviceResources->GetOutputSize();

            commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
            commandList->SetComputeRootSignature(m_compositeAORS.Get());

            commandList->SetComputeRootConstantBufferView(0, m_graphicsMemory->AllocateConstant(m_compositeConstants).GpuAddress());
            commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::GltfSceneSRV));   // Scene
            commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::CacaoOutputSRV)); // AO
            commandList->SetComputeRootDescriptorTable(3, m_resourceDescriptors->GetGpuHandle(Descriptors::FinalSceneUAV)); // Output
            commandList->SetPipelineState(m_compositeAOPSO.Get());

            constexpr unsigned int threadGroupWorkRegionDim = 8u;
            unsigned int dispatchX = (size.right + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;
            unsigned int dispatchY = (size.bottom + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;

            commandList->Dispatch(dispatchX, dispatchY, 1u);

            TransitionTo(commandList, m_finalScene, &m_finalSceneState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            
            PIXEndEvent(commandList);
        }
    }

    {
        // TAA
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"TAA");

        TransitionTo(commandList, m_motionVectors, &m_motionVectorsState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionTo(commandList, m_motionVectorDepth, &m_motionVectorDepthState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        TransitionTo(commandList, m_taaIntermediateOutput, &m_taaIntermediateOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        TransitionTo(commandList, m_taaHistory, &m_taaHistoryState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
         
        commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
        commandList->SetComputeRootSignature(m_taaRS.Get());
         
        auto cbHandle = m_graphicsMemory->AllocateConstant(m_taaOptions);
         
        commandList->SetComputeRootConstantBufferView(0, cbHandle.GpuAddress());
        if (m_cacaoEnabled)
        {
            commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::FinalSceneSRV));          // ColorBuffer
        }
        else
        {
            TransitionTo(commandList, m_gltfScene, &m_gltfSceneState, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::GltfSceneSRV)); 
        }
        commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorDepthSRV));  // DepthBuffer
        commandList->SetComputeRootDescriptorTable(3, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaHistorySRV));         // HistoryBuffer
        commandList->SetComputeRootDescriptorTable(4, m_resourceDescriptors->GetGpuHandle(Descriptors::MotionVectorsSRV));      // VelocityBuffer
        commandList->SetComputeRootDescriptorTable(5, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaIntOutputUAV));
        commandList->SetPipelineState(m_taaPSO.Get());

        auto size = m_deviceResources->GetOutputSize();
        float inWidth = static_cast<float>(size.right );
        float inHeight = static_cast<float>(size.bottom );

        constexpr unsigned int threadGroupWorkRegionDim = 16u;
        UINT dispatchX = static_cast<UINT>((inWidth + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim);
        UINT dispatchY = static_cast<UINT>((inHeight + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim);

        commandList->Dispatch(dispatchX, dispatchY, 1u); 

        // Always clear the reset bit after each dispatch
        m_taaOptions.reset = 0;

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

        auto const size = m_deviceResources->GetOutputSize();
        float inWidth = static_cast<float>(size.right);
        float inHeight = static_cast<float>(size.bottom);

        constexpr unsigned int threadGroupWorkRegionDim = 8u;
        UINT dispatchX = static_cast<UINT>((inWidth + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim);
        UINT dispatchY = static_cast<UINT>((inHeight + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim);

        commandList->Dispatch(dispatchX, dispatchY, 1u);

        PIXEndEvent(commandList);
    }

    {
        // Tonemapping CS - also outputs Gamma2 space for FSR passes.
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"TonemappingCS");
        m_tonemapperConstants.mode = 0;
        m_tonemapperConstants.scale = 1.0f;

        auto cbHandle = m_graphicsMemory->AllocateConstant(m_tonemapperConstants);
        commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
        commandList->SetComputeRootSignature(m_tonemapperCSRS.Get());
        commandList->SetComputeRootConstantBufferView(0, cbHandle.GpuAddress());

        TransitionTo(commandList, m_taaOutput, &m_taaOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        commandList->SetComputeRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::TaaOutputUAV));
        commandList->SetComputeRootDescriptorTable(2, m_resourceDescriptors->GetGpuHandle(Descriptors::Tonemap_OutputUAV));
        commandList->SetPipelineState(m_tonemapperCSPSO.Get());

        auto const size = m_deviceResources->GetOutputSize();

        constexpr unsigned int threadGroupWorkRegionDim = 8u;
        unsigned int dispatchX = (size.right + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;
        unsigned int dispatchY = (size.bottom + (threadGroupWorkRegionDim - 1u)) / threadGroupWorkRegionDim;

        commandList->Dispatch(dispatchX, dispatchY, 1u);
        PIXEndEvent(commandList);
    }

    {
        // Color conversion (if in SDR mode, simply gamma2 conversion back occurs)
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Color Conversion");

        TransitionTo(commandList, m_tonemapperOutput, &m_tonemapperOutputState, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

        commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
        auto const viewport = m_deviceResources->GetScreenViewport();
        auto const scissorRect = m_deviceResources->GetScissorRect();

        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        auto cbHandle = m_graphicsMemory->AllocateConstant(m_colorConversionConstants);
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
        commandList->SetGraphicsRootSignature(m_colorConversionRS.Get());
        commandList->SetGraphicsRootConstantBufferView(0, cbHandle.GpuAddress());
        commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::Tonemap_OutputSRV)); //MagnifierOutputSRV));
        commandList->SetPipelineState(m_colorConversionPSO.Get());
        commandList->DrawInstanced(3, 1, 0, 0);

        PIXEndEvent(commandList);
    }

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

}

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    // When viewing the AO Buffer the screen is mostly white, so change the text color to grey
    auto color = m_cacaoEnabled && (m_compositeConstants.cacaoBufferView != 0) ? ATG::Colors::DarkGrey : ATG::Colors::White;

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
        m_font->DrawString(m_batch.get(), L"AMD FidelityFX CACAO Sample (HDR Display)",
            XMFLOAT2(float(safe.left), y), color);
    }
    else
    {
        m_font->DrawString(m_batch.get(), L"AMD FidelityFX CACAO Sample (SDR Display)",
            XMFLOAT2(float(safe.left), y), color);
    }

    wchar_t textBuffer[128] = {};

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Resolution %d x %d", size.right, size.bottom);
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), color);

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Frametime:  %0.3fms", m_gpuTimer->GetAverageMS(0));
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), color);

    if (m_cacaoEnabled)
    {
        y += m_font->GetLineSpacing();

        if (m_compositeConstants.cacaoBufferView)
        {
            swprintf_s(textBuffer, L" > CACAO - Viewing AO Buffer");
            m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), color);
        }
        else
        {
            swprintf_s(textBuffer, L" > CACAO - Composited over scene");
            m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), color);

            y += m_font->GetLineSpacing();
            swprintf_s(textBuffer, L"         - Scene Blend Factor:  %1.3f", m_compositeConstants.cacaoAOFactor);
            m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), color);
        }

        y += m_font->GetLineSpacing();
        swprintf_s(textBuffer, L" > CACAO - Quality Preset:  %ls", s_FfxCacaoPresetNames[m_cacaoPreset]);
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), color);

        y += m_font->GetLineSpacing();
        swprintf_s(textBuffer, L" > CACAO - Time:  %0.3fms", m_gpuTimer->GetAverageMS(1));
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), color);
    }
    else
    {
        y += m_font->GetLineSpacing();
        swprintf_s(textBuffer, L" > CACAO Disabled");
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), color);
    }

    const wchar_t* legendStr = L"[View] Exit [LThumb] View  [A]/[B] CACAO Quality Preset [X] Enable CACAO\n[Y] View AO Output [RT]/[LT] AO Blend Factor";

    DX::DrawControllerString(m_batch.get(),
        m_font.get(), m_ctrlFont.get(),
        legendStr, XMFLOAT2(float(safe.left), float(safe.bottom) - m_ctrlFont->GetLineSpacing()*2),
        color);

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
    auto const scissorRect = m_deviceResources->GetScissorRect();

    // Scale depending on current renderscale 
    viewport.Width = static_cast<float>(scissorRect.right);
    viewport.Height = static_cast<float>(scissorRect.bottom);

   
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = {};

    dsvDescriptor = m_deviceResources->GetDepthStencilView();

    TransitionTo(commandList, m_normals, &m_normalsState, D3D12_RESOURCE_STATE_RENDER_TARGET);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvs[2];
    rtvs[0] = m_renderDescriptors->GetCpuHandle(RTDescriptors::GltfSceneRTV);
    rtvs[1] = m_renderDescriptors->GetCpuHandle(RTDescriptors::NormalsRTV);

    commandList->OMSetRenderTargets(2, &rtvs[0], FALSE, &dsvDescriptor);

    const FLOAT clearcolors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTargetView(rtvs[0], clearcolors, 0, nullptr);

    float clearVal[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    commandList->ClearRenderTargetView(rtvs[1], clearVal, 0, nullptr);


    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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

    CreateWindowSizeDependentResources();

    DestroyCacaoContexts();
    CreateCacaoContexts();
}

void Sample::OnDeviceLost()
{
    DestroyCacaoContexts();

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
    CreateCacaoContexts();
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
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    // Init settings
    m_cacaoSettings = s_FfxCacaoPresets[m_cacaoPreset].settings;

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

    wchar_t filepath[MAX_PATH];
#ifdef _GAMING_XBOX
    DX::FindMediaFile(filepath, _countof(filepath), L"Corridor.gltf");
#else
    DX::FindMediaFile(filepath, _countof(filepath), L"AMDCorridor\\Corridor.gltf");
#endif

    bool modelStatus = m_gltfModel->Load(filepath);

    if (!modelStatus)
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
            l.m_range = 25.0f;
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
            l.m_intensity = 0.85f;
            l.m_color = XMVectorSet(0.8f, 0.8f, 0.8f, 0.0f);
            l.m_range = 55.0f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

    }
     
    // Upload resources to video memory
    ResourceUploadBatch upload(device);
    upload.Begin();

    m_gltfResources.OnCreate(device, m_gltfModel, &upload , m_graphicsMemory.get());

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
        DXGI_FORMAT_R8G8B8A8_UNORM,
        1
    ); 
      
    m_gltfDepth = new AMDTK::GltfDepthPass();
    m_gltfDepth->OnCreate(
        device,
        &m_gltfResources
    ); 

    m_gltfMotionVectors = new AMDTK::GltfMotionVectorsPass();
    m_gltfMotionVectors->OnCreate(
        device,
        &m_gltfResources
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
        //AO composition CS

        auto computeShaderBlob = DX::ReadData(L"CompositeAOCS.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_compositeAORS.ReleaseAndGetAddressOf())));

        m_compositeAORS->SetName(L"CompositeAOCS RS");

        // Create compute pipeline state
        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_compositeAORS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO,
                IID_GRAPHICS_PPV_ARGS(m_compositeAOPSO.ReleaseAndGetAddressOf())));

        m_compositeAOPSO->SetName(L"CompositeAOCS PSO");
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
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 1000.0f);

    UINT resourceSizeWidth{ static_cast<UINT>(size.right) };
    UINT resourceSizeHeight{ static_cast<UINT>(size.bottom) };

    D3D12_CLEAR_VALUE clearValue = { DXGI_FORMAT_R16G16B16A16_FLOAT, {0.0f, 0.0f, 0.0f, 0.0f} };
    CreateResource(m_gltfScene, L"GltfScene", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &clearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_RENDER_TARGET, &m_gltfSceneState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::GltfSceneRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::GltfSceneSRV), { 0 }, { 0 });

    CreateResource(m_finalScene, L"FinalScene", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_finalSceneState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::FinalSceneSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::FinalSceneUAV), { 0 });

    
    D3D12_CLEAR_VALUE normalsClearValue = { DXGI_FORMAT_R8G8B8A8_UNORM, {0.0f, 0.0f, 0.0f, 0.0f} };
    CreateResource(m_normals, L"Normals", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &normalsClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_RENDER_TARGET, &m_normalsState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::NormalsRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::NormalsSRV), { 0 }, { 0 });

    D3D12_CLEAR_VALUE motionDepthClearValue = {};
    motionDepthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    motionDepthClearValue.DepthStencil.Depth = 1.0f;
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
     
    CreateResource(m_cacaoOutput, L"CacaoOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_GENERIC_READ, &m_cacaoOutputState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::CacaoOutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::CacaoOutputUAV), { 0 });

    CreateResource(m_taaIntermediateOutput, L"TaaIntermediateOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_taaIntermediateOutputState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::TaaIntOutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::TaaIntOutputUAV), { 0 });

    CreateResource(m_taaOutput, L"TaaOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, &clearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_taaOutputState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::TaaOutputRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::TaaOutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::TaaOutputUAV), { 0 });

    CreateResource(m_taaHistory, L"TaaHistory", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R16G16B16A16_FLOAT, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, nullptr,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &m_taaHistoryState,
        { 0 }, m_resourceDescriptors->GetCpuHandle(Descriptors::TaaHistorySRV), m_resourceDescriptors->GetCpuHandle(Descriptors::TaaHistoryUAV), { 0 });

    D3D12_CLEAR_VALUE tonemapClearValue = { DXGI_FORMAT_R10G10B10A2_UNORM, {0.0f,0.0f,0.0f,0.0f} };
    CreateResource(m_tonemapperOutput, L"m_tonemapperOutput", resourceSizeWidth, resourceSizeHeight,
        DXGI_FORMAT_R10G10B10A2_UNORM, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, &tonemapClearValue,
        CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &m_tonemapperOutputState,
        m_renderDescriptors->GetCpuHandle(RTDescriptors::Tonemap_OutputRTV), m_resourceDescriptors->GetCpuHandle(Descriptors::Tonemap_OutputSRV), m_resourceDescriptors->GetCpuHandle(Descriptors::Tonemap_OutputUAV),  { 0 });
     

    // shadow Atlas generation
    {
        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        CreateResource(m_shadowAtlas, L"Shadow Atlas", 4096, 4096,
            DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, &depthOptimizedClearValue,
            CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr,
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
