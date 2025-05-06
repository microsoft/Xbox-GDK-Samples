//--------------------------------------------------------------------------------------
// VariableShading.cpp
//
// Modifications Copyright (C) 2020. Advanced Micro Devices, Inc. All Rights Reserved.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"
#include "FindMedia.h"
#include "VariableShading.h"

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
 
namespace
{
    const XMVECTORF32 c_eye = { -0.8f, 2.0f, 2.0f };
    constexpr float c_pitch = -0.2f;
    constexpr float c_yaw = XM_PI - 0.3f;
}

static Sample* g_pSampleInstance = nullptr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pitch(c_pitch),
    m_yaw(c_yaw),
    m_variableShadingImgGenMode{VariableShadingImageGenMode::Enable},
    m_variableShadingImgGenTileSize{VariableShadingImageGenTileSize::TileSize_Disable},
    m_variableShadingImgGenAdditionalSizesSupported{VariableShadingImageGenAdditionalSizesSupported::Disable},
    m_variableShadingOverlayEnable(false),
    m_variableShadingEnable(true),
    m_variableShadingCombinerState{D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,D3D12_SHADING_RATE_COMBINER_PASSTHROUGH},
    m_eye(c_eye),
    m_currentCamera(0)
{
    // store the sample instance to use in the allocator callback
    g_pSampleInstance = this;

    m_vrsInitParams = { 0 };
    m_vrsMotionFactor = 0.015f;
    m_vrsThreshold = 0.008f;

    // Use gamma-correct rendering.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        DXGI_FORMAT_D32_FLOAT,
        3);

    // Set up for post-process rendering.
    m_scene = std::make_unique<DX::RenderTexture>(m_deviceResources->GetBackBufferFormat());
    m_scene->SetClearColor(ATG::ColorsLinear::Background);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    UpdateVRSContext(false);

    m_gltfPBR->OnDestroy();
    delete m_gltfPBR;

    m_gltfDepth->OnDestroy();
    delete m_gltfDepth;

    m_gltfMotionVectors->OnDestroy();
    delete m_gltfMotionVectors;

    m_GLTFResources.OnDestroy();

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

void Sample::UpdateVRSContext(bool enabled)
{
    if (enabled)
    {
        if (!m_vrsInitParams.backendInterface.scratchBuffer)
        {
            // Initialize the FFX backend
            const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_VRS_CONTEXT_COUNT);
            void* scratchBuffer = calloc(scratchBufferSize, 1u);
            FfxErrorCode errorCode = ffxGetInterfaceDX12(&m_vrsInitParams.backendInterface, m_deviceResources->GetD3DDevice(), scratchBuffer, scratchBufferSize, FFX_VRS_CONTEXT_COUNT);
            FFX_ASSERT(errorCode == FFX_OK);
            FFX_ASSERT_MESSAGE(m_vrsInitParams.backendInterface.fpGetSDKVersion(&m_vrsInitParams.backendInterface) == FFX_SDK_MAKE_VERSION(1, 1, 2),
                "FidelityFX VRS sample requires linking with a 1.1.2 version SDK backend");
            FFX_ASSERT_MESSAGE(ffxVrsGetEffectVersion() == FFX_SDK_MAKE_VERSION(1, 2, 0),
                "FidelityFX VRS sample requires linking with a 1.2 version FidelityFX VRS library");
        }

        // Init context
        static unsigned int sImageTileSize            = DeviceDefaultShadingRateImageTileSize();
        m_vrsInitParams.flags                       = 0;
        m_vrsInitParams.shadingRateImageTileSize    = sImageTileSize;
        ffxVrsContextCreate(&m_vrsContext, &m_vrsInitParams);

        // use our thread-safe buffer allocator instead of the default one
        m_vrsInitParams.backendInterface.fpRegisterConstantBufferAllocator(&m_vrsInitParams.backendInterface, ffxAllocateConstantBuffer);
    }
    else
    {
        // Destroy context
        ffxVrsContextDestroy(&m_vrsContext);

        // Destroy the FidelityFX interface memory
        if (m_vrsInitParams.backendInterface.scratchBuffer != nullptr)
        {
            free(m_vrsInitParams.backendInterface.scratchBuffer);
            m_vrsInitParams.backendInterface.scratchBuffer = nullptr;
        }
    }
}

bool Sample::DeviceSupportsAdditionalShadingRates() const
{
    auto device = m_deviceResources->GetD3DDevice();
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureData6;
    HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureData6, sizeof(featureData6));
    if (FAILED(hr))
    {
#ifdef _DEBUG
        OutputDebugStringA("Sample: Device does not support variable rate shading.\n");
#endif
        throw std::exception("Sample: Device does not support variable rate shading.");
    }

    return featureData6.AdditionalShadingRatesSupported != FALSE;
}

unsigned int Sample::DeviceDefaultShadingRateImageTileSize() const
{
    auto device = m_deviceResources->GetD3DDevice();
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureData6;
    HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureData6, sizeof(featureData6));
    if (FAILED(hr))
    {
#ifdef _DEBUG
        OutputDebugStringA("Sample: Device does not support variable rate shading.\n");
#endif
        throw std::exception("Sample: Device does not support variable rate shading.");
    }

    return featureData6.ShadingRateImageTileSize;
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
            unsigned int vrsMode = static_cast<unsigned int>(m_variableShadingImgGenMode);
            const unsigned int vrsModeCount = static_cast<unsigned int>(VariableShadingImageGenMode::Count);
            vrsMode = (vrsMode + 1) % vrsModeCount;

#ifndef _GAMING_XBOX_SCARLETT
            // Skip to next mode because Wave32 mode doesn't exist on other platforms.
            vrsMode = vrsMode == static_cast<unsigned int>(VariableShadingImageGenMode::Enable_Wave32) ? (vrsMode + 1) % vrsModeCount : vrsMode;
#endif
            m_variableShadingImgGenMode = static_cast<VariableShadingImageGenMode>(vrsMode);
        }
        else if (m_gamePadButtons.b == ButtonState::PRESSED)
        {
            m_variableShadingEnable = !m_variableShadingEnable;
        }
        else if (m_gamePadButtons.x == ButtonState::PRESSED)
        {
            m_variableShadingOverlayEnable = !m_variableShadingOverlayEnable;
        }

        if (pad.IsLeftStickPressed())
        {
            m_pitch = c_pitch;
            m_yaw = c_yaw;
        }
        else
        {
            m_yaw += pad.thumbSticks.leftX * 0.1f;
            m_pitch += pad.thumbSticks.leftY * 0.1f;
        }

        if (pad.IsDPadUpPressed())
        {
            m_vrsThreshold += 0.0001f;
        }

        if (pad.IsDPadDownPressed())
        {
            m_vrsThreshold -= 0.0001f;
        }     

        if (pad.IsDPadRightPressed())
        {
            m_vrsMotionFactor += 0.0001f;
        }

        if (pad.IsDPadLeftPressed())
        {
            m_vrsMotionFactor -= 0.0001f;
        }

        if (m_gamePadButtons.x == ButtonState::PRESSED)
        {
        } 
        else if (m_gamePadButtons.y == ButtonState::PRESSED)
        {
        }
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
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }

    m_lookAt = XMVectorSet(
        sinf(m_yaw),
        m_pitch,
        cosf(m_yaw),
        0);

    m_view = XMMatrixLookToLH(m_eye, m_lookAt, g_XMIdentityR1);

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    ID3D12DescriptorHeap* resourceDescriptorHeap[] =
    {
        m_resourceDescriptors->Heap()
    };

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();
    m_gpuTimer->BeginFrame(commandList);
    m_gpuTimer->Start(commandList, 0);
    m_scene->BeginScene(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Setup Per frame data
    static float time = 0;
    time += 0.016f;
    m_gltfModel->GetCommonPerFrameData().iblFactor = 0.36f;
    m_gltfModel->GetCommonPerFrameData().emmisiveFactor = 1;
    m_gltfModel->SetAnimationTime(0, time);
    m_gltfModel->TransformScene(0, XMMatrixScaling(-1, 1, 1));
    m_gltfModel->GetCommonPerFrameData().mCameraViewProj = m_view * m_proj;
    m_gltfModel->GetCommonPerFrameData().mInverseCameraViewProj = XMMatrixInverse(nullptr, m_gltfModel->GetCommonPerFrameData().mCameraViewProj);
    m_gltfModel->GetCommonPerFrameData().cameraPos = m_eye;
    m_gltfModel->UpdatePerFrameLights();
    m_GLTFResources.SetSkinningMatricesForSkeletons();
    m_GLTFResources.SetPerFrameConstants();

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

    {
        // Render Shadow Atlas
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Shadow Atlas");

        // Clear shadow depths.
        commandList->ClearDepthStencilView(m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        {
            // Render shadow depths.
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

                auto shadowAtlasDescHandle{ m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV) };
                commandList->OMSetRenderTargets(0, NULL, true, &shadowAtlasDescHandle);

                AMDTK::GltfDepthPass::per_frame perPassConstants;

                perPassConstants.mViewProj = m_gltfModel->GetCommonPerFrameData().lights[i].mLightViewProj;

                m_gltfDepth->SetPerFrameCB(m_graphicsMemory->AllocateConstant(perPassConstants).GpuAddress());

                m_gltfDepth->Draw(commandList);

            }
        }

        // Transition shadow depths to read.
        TransitionResource(commandList, m_shadowAtlas.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        PIXEndEvent(commandList);
    }

    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Motion Vectors Pass");

        DirectX::XMMATRIX currentViewProj = m_view * m_proj;
        DirectX::XMMATRIX prevViewCurrProj = m_prevView * m_proj;

        commandList->ClearDepthStencilView(m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDV), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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

        auto motionDepthCPUHandle{ m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDV) };

        commandList->OMSetRenderTargets(1, &rtvs[0], false, &motionDepthCPUHandle);

        AMDTK::GltfMotionVectorsPass::per_frame perPassConstants;
        perPassConstants.mCurrViewProj = currentViewProj;
        perPassConstants.mPrevViewProj = prevViewCurrProj;
        perPassConstants.normalizedFormat = 0;

        m_gltfMotionVectors->SetPerFrameCB(m_graphicsMemory->AllocateConstant(perPassConstants).GpuAddress());

        m_gltfMotionVectors->Draw(commandList);
        PIXEndEvent(commandList);
    }

    Clear();
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> commandList5;
    commandList->QueryInterface(IID_GRAPHICS_PPV_ARGS(commandList5.GetAddressOf()));
    EnableVRS(commandList5.Get());
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"GLTF PBR Pass");
        m_gltfPBR->Draw(commandList, m_GLTFResources.GetSrvPile()->GetGpuHandle(m_shadowAtlasIdx));
        PIXEndEvent(commandList);
    }
    DisableVRS(commandList5.Get());

    GenerateShadingRateImage(commandList);

    RenderShadingRateImageOverlay(commandList);
    // Render final scene
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
    m_scene->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_fullScreenQuad->Draw(commandList, m_passthroughPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::SceneTex));

    // Render UI
    RenderUI(commandList);

    PIXEndEvent(commandList);

    m_prevView = m_view;

    // Transition shadow depths to write.
    TransitionResource(commandList, m_shadowAtlas.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_gpuTimer->Stop(commandList, 0);
    m_gpuTimer->EndFrame(commandList);
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
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

void Sample::GenerateShadingRateImage(ID3D12GraphicsCommandList* commandList)
{
    if ( m_variableShadingEnable && m_variableShadingImgGenMode != VariableShadingImageGenMode::Disable )
    {
        m_gpuTimer->Start(commandList, 1);
        
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Scene Transition");
        m_scene->TransitionTo(commandList, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        PIXEndEvent(commandList);

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"UAV Transition: Shading Rate Source to UAV");
        TransitionResource(commandList, m_ShadingRateImage.Get(), D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        PIXEndEvent(commandList);

        auto const size = m_deviceResources->GetOutputSize();

        FfxVrsDispatchDescription dispatchParameters = {};
        dispatchParameters.commandList      = ffxGetCommandListDX12(commandList);
        dispatchParameters.output           = ffxGetResourceDX12(m_ShadingRateImage.Get(), GetFfxResourceDescription(m_ShadingRateImage.Get()), L"VRSImage", FFX_RESOURCE_STATE_UNORDERED_ACCESS);
        dispatchParameters.historyColor     = ffxGetResourceDX12(m_scene->GetResource(), GetFfxResourceDescription(m_scene->GetResource()), L"HistoryColorBuffer", FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchParameters.motionVectors    = ffxGetResourceDX12(m_motionVectors.Get(), GetFfxResourceDescription(m_motionVectors.Get()), L"MotionVectors", FFX_RESOURCE_STATE_RENDER_TARGET);
        dispatchParameters.motionFactor     = m_vrsMotionFactor;
        dispatchParameters.varianceCutoff   = m_vrsThreshold;
        static unsigned int sImageTileSize    = DeviceDefaultShadingRateImageTileSize();
        dispatchParameters.tileSize         = sImageTileSize;
        dispatchParameters.renderSize       = { uint32_t(size.right), uint32_t(size.bottom) };
        dispatchParameters.motionVectorScale.x = -1.f;
        dispatchParameters.motionVectorScale.y = -1.f;

        // Disabled until remaining things are fixes
        FfxErrorCode errorCode = ffxVrsContextDispatch(&m_vrsContext, &dispatchParameters);
        FFX_ASSERT(errorCode == FFX_OK);

        m_gpuTimer->Stop(commandList, 1);
    }
}

void Sample::EnableVRS(ID3D12GraphicsCommandList5* commandList)
{
    if (m_variableShadingEnable)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"UAV Transition: UAV to Shading Rate Source");
        TransitionResource(commandList, m_ShadingRateImage.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE);
        PIXEndEvent(commandList);

        D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT]
        { D3D12_SHADING_RATE_COMBINER_PASSTHROUGH, D3D12_SHADING_RATE_COMBINER_OVERRIDE};
        commandList->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
        commandList->RSSetShadingRateImage(m_ShadingRateImage.Get());
    }
}

void Sample::DisableVRS(ID3D12GraphicsCommandList5* commandList)
{
    if (m_variableShadingEnable)
    {
        D3D12_SHADING_RATE_COMBINER combiners[D3D12_RS_SET_SHADING_RATE_COMBINER_COUNT]
        { D3D12_SHADING_RATE_COMBINER_PASSTHROUGH, D3D12_SHADING_RATE_COMBINER_OVERRIDE };
        commandList->RSSetShadingRate(D3D12_SHADING_RATE_1X1, combiners);
        commandList->RSSetShadingRateImage(nullptr); // It's better to enable/disable using combiners only, if possible.
    }
}

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    ID3D12DescriptorHeap* descriptorHeaps[] { m_resourceDescriptors->Heap() };

    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    auto const size = m_deviceResources->GetOutputSize();
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(size.right), UINT(size.bottom));
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    m_batch->Begin(commandList);

    float y = float(safe.top);

    m_font->DrawString(m_batch.get(), L"AMD FidelityFX Variable Shading GLTF Sample",
        XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();

    wchar_t textBuffer[128] = {};

    y += m_font->GetLineSpacing();

    swprintf_s(textBuffer, L"Resolution %d x %d", size.right, size.bottom);
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();
    
    swprintf_s(textBuffer, L"Frametime:  %0.3fms", m_gpuTimer->GetElapsedMS(0));
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);
    
    y += m_font->GetLineSpacing();

    std::wstring shaderAAStr = L"FidelityFX Variable Shading Mode: ";
    if (m_variableShadingEnable)
    {
        shaderAAStr += GetShadingRateImageGenerationModeDescription(m_variableShadingImgGenMode);

    }
    else
    {
        shaderAAStr += L"Disabled";
    }

    m_font->DrawString(m_batch.get(), shaderAAStr.c_str(), XMFLOAT2(float(safe.left), y), ATG::Colors::White);
    

#ifdef _GAMING_XBOX
    const wchar_t* legendStr = L"[View] Exit    [LThumb] Move Camera      [A] Variable Shading Image Generation Mode     [B] Variable Rate Shading On/Off    [X] Variable Rate Shading Overlay On/Off\n     [DPAD] Up/Down Adjust Variance Cutoff\n";
#else
    const wchar_t* legendStr = L"[View] Exit    [LThumb] Move Camera      [B] Variable Rate Shading On/Off    [X] Variable Rate Shading Overlay On/Off      \n     [DPAD] Up/Down Adjust Variance Cutoff\n";
#endif

    auto controllerBounds = DX::MeasureControllerDrawBounds(
        m_font.get(), m_ctrlFont.get(),
        legendStr, XMFLOAT2(float(safe.left), float(safe.bottom)),1.0f);
    DX::DrawControllerString(m_batch.get(),
        m_font.get(), m_ctrlFont.get(),
        legendStr, XMFLOAT2(float(safe.left), float(safe.bottom)-(controllerBounds.bottom-controllerBounds.top)),
        ATG::Colors::LightGrey,1.0f);

    if (m_variableShadingEnable && m_variableShadingImgGenMode < VariableShadingImageGenMode::Count)
    {
        y += m_font->GetLineSpacing();
        swprintf_s(textBuffer, L"Variable Shading Rate Image Generation Time: %0.3fms", m_gpuTimer->GetElapsedMS(1));
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        y += m_font->GetLineSpacing();
        swprintf_s(textBuffer, L"Variance Cutoff: %0.4f", m_vrsThreshold);
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);
    }

    m_batch->End();

    PIXEndEvent(commandList);
}

static uint32_t DivideRoundingUp(uint32_t a, uint32_t b)
{
    return (a + b - 1) / b;
}

void Sample::RenderShadingRateImageOverlay(ID3D12GraphicsCommandList* commandList)
{
    if (m_variableShadingOverlayEnable &&  m_variableShadingEnable && m_variableShadingImgGenMode < VariableShadingImageGenMode::Disable)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"FidelityFX Variable Shading Rate Image Overlay");

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Shading Rate Image Transition to Pixel Shader Resource");
        TransitionResource(commandList, m_ShadingRateImage.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        PIXEndEvent(commandList);

        // This only must occur during the overlay and has an additional impact on perf with overlay enabled.
        m_scene->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

        auto size = m_deviceResources->GetOutputSize();

        struct VRSOverlayConst
        {
            int             resolution[2];
            unsigned int    tileSize;
            float           varianceCutoff;
            float           motionFactor;
        };

        static unsigned int sImageTileSize = DeviceDefaultShadingRateImageTileSize();
        VRSOverlayConst consts = { static_cast<int>(size.right), static_cast<int>(size.bottom),
                                   sImageTileSize, m_vrsThreshold, m_vrsMotionFactor };

        auto cbHandle = m_graphicsMemory->AllocateConstant(consts);

        ID3D12DescriptorHeap* resourceDescriptorHeap[] =
        {
            m_resourceDescriptors->Heap()
        };

        commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
        commandList->SetGraphicsRootSignature(m_shadingRateImageOverlayRootSignature.Get());
        commandList->SetGraphicsRootConstantBufferView(0, cbHandle.GpuAddress());
        commandList->SetGraphicsRootDescriptorTable(1, m_resourceDescriptors->GetGpuHandle(Descriptors::VariableShadingImgOverlay_InputSRV));
        commandList->SetPipelineState(m_shadingRateImagePSO.Get());
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        commandList->DrawInstanced(3, 1, 0, 0);

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Shading Rate Image Transition to Shading Rate Source");
        TransitionResource(commandList, m_ShadingRateImage.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        PIXEndEvent(commandList);

        m_gpuTimer->Stop(commandList, 1);
        PIXEndEvent(commandList);
    }
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();

    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = {};
    D3D12_CPU_DESCRIPTOR_HANDLE dsvDescriptor = {};

    rtvDescriptor = m_renderDescriptors->GetCpuHandle(RTDescriptors::SceneRTV);
    dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);


    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);
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
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
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

    UpdateVRSContext(false);
    CreateWindowSizeDependentResources();
    UpdateVRSContext(true);
}

void Sample::OnDeviceLost()
{
    UpdateVRSContext(false);

    m_gpuTimer.reset();
    m_graphicsMemory.reset();

    m_computeRootSignature.Reset();
    m_shadingRateImageOverlayRootSignature.Reset();
    m_shadingRateImagePSO.Reset();
    m_ShadingRateImage.Reset();

    m_motionVectorDepth.Reset();
    m_motionVectors.Reset();
    
    m_font.reset();
    m_colorCtrlFont.reset();
    m_ctrlFont.reset();
    m_gltfModel->Unload();
    m_GLTFResources.OnDestroy();
    m_resourceDescriptors.reset();
    m_renderDescriptors.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
    CreateWindowSizeDependentResources();

    UpdateVRSContext(true);
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

    D3D12_FEATURE_DATA_D3D12_OPTIONS6 featureData6 = {};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &featureData6, sizeof(featureData6)))
        || (featureData6.VariableShadingRateTier == D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED))
    {
        extern LPCWSTR g_szAppName;
        std::ignore = MessageBoxW(nullptr, L"D3D12 device does not support variable rate shading", g_szAppName, MB_ICONERROR | MB_OK);
        throw std::runtime_error("VRS is not supported!");
    }
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

    wchar_t filepath[MAX_PATH];
#ifdef _GAMING_XBOX
    DX::FindMediaFile(filepath, _countof(filepath), L"Corridor.gltf");
#else
    DX::FindMediaFile(filepath, _countof(filepath), L"AMDCorridor\\Corridor.gltf");
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
            XMVECTOR x{ 0.0f, 2.5f, 2.5f, 0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 2.0f, -0.1f, 0.0f));

            tfLight l;
            l.m_type = tfLight::LIGHT_SPOTLIGHT;
            l.m_intensity = 1.0f;
            l.m_color = XMVectorSet(0.70f, 0.80f, 1.0f, 0.0f);
            l.m_range = 10.0f;
            l.m_outerConeAngle = XM_PI / 6.0f;
            l.m_innerConeAngle = (XM_PI / 6.0f) * 0.9f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f, 1.5f, 1.5f, 0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 0.1f, -3.6f, 0.0f));

            tfLight l;
            l.m_type = tfLight::LIGHT_SPOTLIGHT;
            l.m_intensity = 0.9f;
            l.m_color = XMVectorSet(0.70f, 0.80f, 1.0f, 0.0f);
            l.m_range = 15.0f;
            l.m_outerConeAngle = XM_PI / 6.0f;
            l.m_innerConeAngle = (XM_PI / 12.0f) * 0.9f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f, 2.5f, -2.5f, 0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 2.0f, -0.1f, 0.0f));

            tfLight l;
            l.m_type = tfLight::LIGHT_SPOTLIGHT;
            l.m_intensity = 1.0f;
            l.m_color = XMVectorSet(0.70f, 0.80f, 1.0f, 0.0f);
            l.m_range = 10.0f;
            l.m_outerConeAngle = XM_PI / 6.0f;
            l.m_innerConeAngle = (XM_PI / 6.0f) * 0.9f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f,2.5f,2.0f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 0.0f, -0.1f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.9f;
            l.m_color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
            l.m_range = 8.0f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f,2.5f,-2.0f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 0.0f, -0.1f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.9f;
            l.m_color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
            l.m_range = 5.0f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f,2.0f,-3.0f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 0.0f, -0.1f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.9f;
            l.m_color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
            l.m_range = 5.0f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }
        {
            tfNode n;
            XMVECTOR x{ 0.0f,2.0f,-4.0f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 0.0f, -0.1f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.9f;
            l.m_color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
            l.m_range = 5.0f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

        {
            tfNode n;
            XMVECTOR x{ 0.0f,2.0f,-5.0f,0.0f };
            n.m_tranform.LookAt(x, XMVectorSet(0.0f, 0.0f, -0.1f, 0.0f));
            tfLight l;
            l.m_type = tfLight::LIGHT_POINTLIGHT;
            l.m_intensity = 0.9f;
            l.m_color = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
            l.m_range = 5.0f;
            l.m_outerConeAngle = l.m_innerConeAngle = 0.0f;

            m_gltfModel->AddLight(n, l);
        }

    }

    // Upload resources to video memory
    ResourceUploadBatch upload(device);
    upload.Begin();

    m_GLTFResources.OnCreate(device, m_gltfModel, &upload, m_graphicsMemory.get());
    // here we are loading onto the GPU all the textures and the inverse matrices
    // this data will be used to create the passes 

    m_GLTFResources.LoadTextures(m_gltfModel->GetFilePath().c_str());

    m_gltfPBR = new AMDTK::GltfPbrPass();
    m_gltfPBR->OnCreate(
        device,
        &m_GLTFResources,
        m_scene->GetFormat(),
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        1
    );

    m_gltfDepth = new AMDTK::GltfDepthPass();
    m_gltfDepth->OnCreate(
        device,
        &m_GLTFResources
    );

    m_gltfMotionVectors = new AMDTK::GltfMotionVectorsPass();
    m_gltfMotionVectors->OnCreate(
        device,
        &m_GLTFResources
    );

    const RenderTargetState rtStateUI(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    {
        SpriteBatchPipelineStateDescription pd(rtStateUI, &CommonStates::AlphaBlend);
        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    m_deviceResources->WaitForGpu();
    finish.wait();

    // Render targets
    m_scene->SetDevice(device,
        m_resourceDescriptors->GetCpuHandle(Descriptors::SceneTex),
        m_renderDescriptors->GetCpuHandle(RTDescriptors::SceneRTV));

 
    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(device);

    {
        const EffectPipelineStateDescription pd(
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
  
    m_gpuTimer = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());

    // Overlay root signature and pipeline state object.
    auto shadingRateOverlayShaderBlobPS = DX::ReadData(L"VRS_ImageGen_Overlay_PS.cso");
    DX::ThrowIfFailed(
        device->CreateRootSignature(0, shadingRateOverlayShaderBlobPS.data(), shadingRateOverlayShaderBlobPS.size(),
            IID_GRAPHICS_PPV_ARGS(m_shadingRateImageOverlayRootSignature.ReleaseAndGetAddressOf())));

    m_shadingRateImageOverlayRootSignature->SetName(L"VRS_ImageGen_Overlay RS");

    auto shadingRateOverlayShaderBlobVS = DX::ReadData(L"VRS_ImageGen_Overlay_VS.cso");  

    D3D12_GRAPHICS_PIPELINE_STATE_DESC shadingRateOverlayPsoDesc = {};
    shadingRateOverlayPsoDesc.pRootSignature = m_shadingRateImageOverlayRootSignature.Get();
    shadingRateOverlayPsoDesc.VS.pShaderBytecode = shadingRateOverlayShaderBlobVS.data();
    shadingRateOverlayPsoDesc.VS.BytecodeLength = shadingRateOverlayShaderBlobVS.size();
    shadingRateOverlayPsoDesc.PS.pShaderBytecode = shadingRateOverlayShaderBlobPS.data();
    shadingRateOverlayPsoDesc.PS.BytecodeLength = shadingRateOverlayShaderBlobPS.size();
    shadingRateOverlayPsoDesc.BlendState = CommonStates::NonPremultiplied;
    shadingRateOverlayPsoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
    shadingRateOverlayPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    shadingRateOverlayPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    shadingRateOverlayPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    shadingRateOverlayPsoDesc.NumRenderTargets = 1;
    shadingRateOverlayPsoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
    shadingRateOverlayPsoDesc.SampleDesc.Count = 1;

    DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&shadingRateOverlayPsoDesc, IID_GRAPHICS_PPV_ARGS(m_shadingRateImagePSO.ReleaseAndGetAddressOf())));
    m_shadingRateImagePSO->SetName(L"VRS_ImageGen_Overlay PSO");

    UpdateVRSContext(true);
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

    if (size.bottom > 1440)
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

    if (size.bottom > 1440)
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

    if (size.bottom > 1440)
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

    // Offscreen render targets
    m_scene->SetWindow(size);

    auto res = m_scene->GetResource();
    if (res)
        res->SetName(L"Scene");

    // Camera
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 10000.f);

    // shadow Atlas generation
    {
        // Allocate a 2-D surface as the depth/stencil buffer and create a depth/stencil view
        // on this surface.
        CD3DX12_HEAP_PROPERTIES depthHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC depthStencilDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            DXGI_FORMAT_R32_TYPELESS,
            2 * 2048,
            2 * 2048,
            1,
            1,
            1,
            0,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        );

        D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
        depthOptimizedClearValue.DepthStencil.Stencil = 0;

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &depthHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_shadowAtlas.ReleaseAndGetAddressOf())
        ));

        m_shadowAtlas->SetName(L"Depth stencil");

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

        device->CreateDepthStencilView(m_shadowAtlas.Get(), &dsvDesc, m_depthDescriptors->GetCpuHandle(DSDescriptors::ShadowAtlasDV));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;

        m_shadowAtlasIdx = m_GLTFResources.GetSrvPile()->Allocate();

        device->CreateShaderResourceView(m_shadowAtlas.Get(), &srvDesc, m_GLTFResources.GetSrvPile()->GetCpuHandle(m_shadowAtlasIdx));
    }

    const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    
    // VRS Shading Image
    {
        uint32_t vrsImageWidth, vrsImageHeight;
        ffxVrsGetImageSizeFromeRenderResolution(&vrsImageWidth, &vrsImageHeight, size.right, size.bottom, DeviceDefaultShadingRateImageTileSize());

        CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UINT, vrsImageWidth, vrsImageHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_ShadingRateImage.ReleaseAndGetAddressOf())));

        m_ShadingRateImage->SetName(L"m_ShadingRateImage");

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = texDesc.Format;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;
        uavDesc.Texture2D.PlaneSlice = 0;

        device->CreateUnorderedAccessView(m_ShadingRateImage.Get(), nullptr, &uavDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::VariableShadingImg_OutputUAV));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R8_UINT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0, D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0);
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        device->CreateShaderResourceView(m_ShadingRateImage.Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::VariableShadingImgOverlay_InputSRV));
    }

    // Motion vector depth
    {
        D3D12_CLEAR_VALUE motionDepthClearValue = {};
        motionDepthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        motionDepthClearValue.DepthStencil.Depth = 1.0f;
        motionDepthClearValue.DepthStencil.Stencil = 0;

        CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R32_TYPELESS, size.right, size.bottom, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_DEPTH_WRITE,
                &motionDepthClearValue,
                IID_GRAPHICS_PPV_ARGS(m_motionVectorDepth.ReleaseAndGetAddressOf())));

        m_motionVectorDepth->SetName(L"m_motionVectorDepth");

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        device->CreateDepthStencilView(m_motionVectorDepth.Get(), &dsvDesc, m_depthDescriptors->GetCpuHandle(DSDescriptors::MotionVectorDV));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        device->CreateShaderResourceView(m_motionVectorDepth.Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::MotionDepthSRV));
    }

    // Motion vectors
    {
        D3D12_CLEAR_VALUE motionClearValue = { DXGI_FORMAT_R16G16_FLOAT, {0.0f,0.0f,0.0f,0.0f} };

        CD3DX12_RESOURCE_DESC texDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R16G16_FLOAT, size.right, size.bottom, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &texDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                &motionClearValue,
                IID_GRAPHICS_PPV_ARGS(m_motionVectors.ReleaseAndGetAddressOf())));

        m_motionVectors->SetName(L"m_motionVectors");

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        device->CreateRenderTargetView(m_motionVectors.Get(), &rtvDesc, m_renderDescriptors->GetCpuHandle(RTDescriptors::MotionVectorsRTV));

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        device->CreateShaderResourceView(m_motionVectors.Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::MotionVectorsSRV));
    }
}
#pragma endregion


