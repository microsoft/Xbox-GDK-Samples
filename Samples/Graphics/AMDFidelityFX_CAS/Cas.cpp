//--------------------------------------------------------------------------------------
// Cas.cpp
//
// Contrast Adaptive Sharpening (CAS)
//
// Modifications Copyright (C) 2020. Advanced Micro Devices, Inc. All Rights Reserved.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadCompressedData.h"
#include "ReadData.h"
#include "FindMedia.h"

#ifdef _GAMING_DESKTOP
#include <FidelityFX/host/backends/dx12/ffx_dx12.h>
#else
#include <FidelityFX/host/backends/gdk/ffx_gdk.h>
#endif // _GAMING_DESKTOP

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "Cas.h"
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
    const XMVECTORF32 c_eye = { 0.f, -119.f, -50.f };
    constexpr float c_pitch = 0.f;
    constexpr float c_yaw = 0.f;

    const float c_zoomSource[4] = { 0.47f, 0.47f, 0.53f, 0.53f };
    const float c_zoomDest[4] = { 0.65f, 0.65f, 0.95f, 0.95f };

    const float c_zoomDestCas[4] = { 0.65f, 0.15f, 0.95f, 0.45f };
}

static Sample* g_pSampleInstance = nullptr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_pitch(c_pitch),
    m_yaw(c_yaw),
    m_zoomViewport{},
    m_zoomViewportCas{},
    m_zoomRect{},
    m_zoomRectCas{},
    m_casMode(CASMode::Sharpen),
    m_sharpness(0.5f),
    m_renderScaleChanged(false),
    m_renderScale(0)
{
    // store the sample instance to use in the allocator callback
    g_pSampleInstance = this;

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

    UpdateFfxCasContext(false);
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

    UpdateFfxCasContext(true);
}

void Sample::UpdateFfxCasContext(bool enabled)
{
    if (enabled)
    {
        // reset
        memset(&m_InitializationParameters, 0, sizeof(m_InitializationParameters));

        const size_t scratchBufferSize = ffxGetScratchMemorySizeDX12(FFX_CAS_CONTEXT_COUNT);
        void* scratchBuffer = calloc(scratchBufferSize, 1);

        FfxErrorCode errorCode = ffxGetInterfaceDX12(&m_InitializationParameters.backendInterface, m_deviceResources->GetD3DDevice(),
            scratchBuffer, scratchBufferSize, FFX_CAS_CONTEXT_COUNT);
        FFX_ASSERT_MESSAGE(errorCode == FFX_OK, "Could not initialize the FidelityFX SDK backend");

        FFX_ASSERT_MESSAGE(m_InitializationParameters.backendInterface.fpGetSDKVersion(&m_InitializationParameters.backendInterface) ==
            FFX_SDK_MAKE_VERSION(1, 1, 2), "FidelityFX CAS sample requires linking with a 1.1.2 version SDK backend");

        if (GetUpscaleMode() == UpscaleMode::NativeRender) {
            m_InitializationParameters.flags |= FFX_CAS_SHARPEN_ONLY;
        }
        m_InitializationParameters.colorSpaceConversion = FFX_CAS_COLOR_SPACE_LINEAR;

        RECT size = m_deviceResources->GetOutputSize();

        float outWidth = static_cast<float>(size.right);
        float outHeight = static_cast<float>(size.bottom);

        float inWidth = static_cast<float>(size.right * m_renderScales[GetRenderScale()]);
        float inHeight = static_cast<float>(size.bottom * m_renderScales[GetRenderScale()]);

        m_InitializationParameters.maxRenderSize.width = static_cast<uint32_t>(inWidth);
        m_InitializationParameters.maxRenderSize.height = static_cast<uint32_t>(inHeight);
        m_InitializationParameters.displaySize.width = static_cast<uint32_t>(outWidth);
        m_InitializationParameters.displaySize.height = static_cast<uint32_t>(outHeight);

        // Create the cas context
        ffxCasContextCreate(&m_CasContext, &m_InitializationParameters);

        // use our thread-safe buffer allocator instead of the default one
        m_InitializationParameters.backendInterface.fpRegisterConstantBufferAllocator(&m_InitializationParameters.backendInterface, Sample::ffxAllocateConstantBuffer);
    }
    else
    {
        ffxCasContextDestroy(&m_CasContext);

        // Destroy the FidelityFX interface memory
        free(m_InitializationParameters.backendInterface.scratchBuffer);
    }
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

        CASMode oldMode = m_casMode;

        if (m_gamePadButtons.a == ButtonState::PRESSED)
        {
            int aa = static_cast<int>(m_casMode);
            aa = (aa + 1) % static_cast<int>(CASMode::Count);
            m_casMode = static_cast<CASMode>(aa);
        }
        else if (m_gamePadButtons.b == ButtonState::PRESSED)
        {
            int aa = static_cast<int>(m_casMode);
            aa = aa - 1;
            if (aa < 0)
                aa = static_cast<int>(CASMode::Count) - 1;
            m_casMode = static_cast<CASMode>(aa);
        }

        if ((oldMode != m_casMode) && 
            ((m_casMode == CASMode::Disabled) || (oldMode == CASMode::Disabled)))
        {
            // If we have gone to or from a CAS disabled mode, this also changes renderscale
            m_renderScaleChanged = true;
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

        if (pad.triggers.left > 0.0f)
        {
            m_sharpness -= 0.03f * pad.triggers.left;
        } 
        if (pad.triggers.right > 0.0f)
        {
            m_sharpness += 0.03f * pad.triggers.right;
        }

        if (m_gamePadButtons.x == ButtonState::PRESSED)
        {
            m_renderScale = (m_renderScale + 1) % m_numRenderScales;
            m_renderScaleChanged = true;
        } 
        else if (m_gamePadButtons.y == ButtonState::PRESSED)
        {
            if (m_renderScale == 0)
            {
                m_renderScale = m_numRenderScales - 1;
            } 
            else
            {
                m_renderScale--;
            }
            m_renderScaleChanged = true;
        }

        // Clamp to 0.05f, 1.0f
        m_sharpness = std::max(std::min(m_sharpness, 1.0f), 0.05f);

        // If we are applying a render scale change, re-init CAS
        if (m_renderScaleChanged)
        {
            m_deviceResources->WaitForGpu();
            UpdateFfxCasContext(false);
            UpdateFfxCasContext(true);
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

    XMVECTOR lookAt = XMVectorSet(
        sinf(m_yaw),
        m_pitch,
        cosf(m_yaw),
        0);

    m_view = XMMatrixLookToLH(c_eye, lookAt, g_XMIdentityR1);

    PIXEndEvent();
}
#pragma endregion

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

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    if (m_renderScaleChanged)
    {
        m_deviceResources->WaitForGpu();
        m_renderScaleChanged = false;

        auto scissorRect = m_deviceResources->GetScissorRect();

        // Scale depending on current renderscale
        scissorRect.bottom = static_cast<LONG>(scissorRect.bottom * m_renderScales[GetRenderScale()]);
        scissorRect.right = static_cast<LONG>(scissorRect.right * m_renderScales[GetRenderScale()]);
        m_scene->SetWindow(scissorRect);
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
    
    m_scene->BeginScene(commandList);


    Clear();

    PIXEndEvent(commandList);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    RenderScene(commandList);
    m_gpuTimer->Start(commandList, 1);
    if (m_casMode != CASMode::Disabled)
    {
        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"FidelityFX CAS");

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Scene Transition");
        m_scene->TransitionTo(commandList,  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        PIXEndEvent(commandList);

        PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"UAV Transition");
        TransitionCasUAVTo(commandList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        PIXEndEvent(commandList);

        RECT size = m_deviceResources->GetOutputSize();
        uint32_t inWidth = static_cast<uint32_t>(size.right * m_renderScales[GetRenderScale()]);
        uint32_t inHeight = static_cast<uint32_t>(size.bottom * m_renderScales[GetRenderScale()]);

        FfxCasDispatchDescription dispatchParameters = {};
        dispatchParameters.commandList = ffxGetCommandListDX12(commandList);
        dispatchParameters.renderSize = { inWidth, inHeight };
        dispatchParameters.sharpness = m_sharpness;

        // All cauldron resources come into a render module in a generic read state (ResourceState::NonPixelShaderResource | ResourceState::PixelShaderResource)
        dispatchParameters.color = ffxGetResourceDX12(m_scene->GetResource(), GetFfxResourceDescription(m_scene->GetResource()), nullptr, FFX_RESOURCE_STATE_PIXEL_COMPUTE_READ);
        dispatchParameters.output = ffxGetResourceDX12(m_casOutput.Get(), GetFfxResourceDescription(m_casOutput.Get()), nullptr, FFX_RESOURCE_STATE_UNORDERED_ACCESS);

        FfxErrorCode errorCode = ffxCasContextDispatch(&m_CasContext, &dispatchParameters);
        FFX_ASSERT(errorCode == FFX_OK);

        PIXEndEvent(commandList);
    }

    m_gpuTimer->Stop(commandList,1);

    // Render final scene
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();

    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);


    commandList->SetDescriptorHeaps(_countof(resourceDescriptorHeap), resourceDescriptorHeap);
    if (m_casMode != CASMode::Disabled)
    {
        TransitionCasUAVTo(commandList,  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_fullScreenQuad->Draw(commandList, m_passthroughPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::CAS_OutputSRV));
    }
    else
    {
        m_scene->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_fullScreenQuad->Draw(commandList, m_passthroughPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::SceneTex));
    }

    // Render zoom
    RenderZoom(commandList);

    // Render UI
    RenderUI(commandList);

    PIXEndEvent(commandList);


    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_gpuTimer->Stop(commandList, 0);
    m_gpuTimer->EndFrame(commandList);
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::RenderScene(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render scene");

    ID3D12DescriptorHeap* descriptorHeaps[] =
    {
        m_modelResources->Heap(),
        m_states->Heap()
    };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    XMMATRIX world = SimpleMath::Matrix::Identity;

    Model::UpdateEffectMatrices(m_modelEffect, world, m_view, m_proj);

    m_model->Draw(commandList, m_modelEffect.cbegin());

    PIXEndEvent(commandList);
}

void Sample::RenderZoom(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render Zoom");

    commandList->RSSetViewports(1, &m_zoomViewport);
    commandList->RSSetScissorRects(1, &m_zoomRect);

    m_fullScreenQuad->Draw(commandList, m_zoomPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::SceneTex));

    commandList->RSSetViewports(1, &m_zoomViewportCas);
    commandList->RSSetScissorRects(1, &m_zoomRectCas);

    if (m_casMode != CASMode::Disabled)
    {
        m_fullScreenQuad->Draw(commandList, m_zoomPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::CAS_OutputSRV));
    }
    else
    {
        m_fullScreenQuad->Draw(commandList, m_zoomPSO.Get(), m_resourceDescriptors->GetGpuHandle(Descriptors::SceneTex));
    }

    auto const vp = m_deviceResources->GetScreenViewport();
    commandList->RSSetViewports(1, &vp);

    auto const scissors = m_deviceResources->GetScissorRect();
    commandList->RSSetScissorRects(1, &scissors);

    m_lineEFfect->Apply(commandList);

    m_lines->Begin(commandList);

    XMFLOAT4 color(ATG::ColorsLinear::Orange);

    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomSource[0], c_zoomSource[1], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomSource[0], c_zoomSource[3], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomSource[0], c_zoomSource[3], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomSource[2], c_zoomSource[3], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomSource[2], c_zoomSource[3], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomSource[2], c_zoomSource[1], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomSource[2], c_zoomSource[1], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomSource[0], c_zoomSource[1], 0}, color });

    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDest[0], c_zoomDest[1], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomDest[0], c_zoomDest[3], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDest[0], c_zoomDest[3], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomDest[2], c_zoomDest[3], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDest[2], c_zoomDest[3], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomDest[2], c_zoomDest[1], 0}, color });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDest[2], c_zoomDest[1], 0}, color }, VertexPositionColor{ XMFLOAT3{c_zoomDest[0], c_zoomDest[1], 0}, color });

    XMFLOAT4 color2(ATG::ColorsLinear::Green);

    if (m_casMode == CASMode::Disabled)
    {
        // If CAS is disabled, change the color to the existing scene texture border
        color2 = color;
    }

    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDestCas[0], c_zoomDestCas[1], 0}, color2 }, VertexPositionColor{ XMFLOAT3{c_zoomDestCas[0], c_zoomDestCas[3], 0}, color2 });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDestCas[0], c_zoomDestCas[3], 0}, color2 }, VertexPositionColor{ XMFLOAT3{c_zoomDestCas[2], c_zoomDestCas[3], 0}, color2 });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDestCas[2], c_zoomDestCas[3], 0}, color2 }, VertexPositionColor{ XMFLOAT3{c_zoomDestCas[2], c_zoomDestCas[1], 0}, color2 });
    m_lines->DrawLine(VertexPositionColor{ XMFLOAT3{c_zoomDestCas[2], c_zoomDestCas[1], 0}, color2 }, VertexPositionColor{ XMFLOAT3{c_zoomDestCas[0], c_zoomDestCas[1], 0}, color2 });
    
    m_lines->End();

    PIXEndEvent(commandList);
}

const wchar_t* Sample::GetCasModeDescription()
{
    const wchar_t* mode = nullptr;

    switch (m_casMode)
    {
    case CASMode::Sharpen: mode = L"Sharpen"; break; 
#ifdef _GAMING_XBOX_SCARLETT
	case CASMode::Sharpen_Wave32: mode = L"Sharpen_Wave32"; break;
#endif
    case CASMode::Disabled:
    case CASMode::Count:
    default:  mode = L"Disabled"; break;
    }
    return mode;
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

    m_font->DrawString(m_batch.get(), L"AMD FidelityFX CAS Sample",
        XMFLOAT2(float(safe.left), y), ATG::Colors::White);


    wchar_t textBuffer[128] = {};

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Resolution %d x %d", size.right, size.bottom);
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();
    swprintf_s(textBuffer, L"Frametime:  %0.3fms", m_gpuTimer->GetElapsedMS(0));
    m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

    y += m_font->GetLineSpacing();

    std::wstring shaderAAStr = L"FidelityFX CAS Mode: ";

    shaderAAStr.append(GetCasModeDescription());

    if (GetRenderScale() > 0)
    {
        shaderAAStr.append(L" + Upscale");
    }

    y += m_font->GetLineSpacing();
    m_font->DrawString(m_batch.get(), shaderAAStr.c_str(), XMFLOAT2(float(safe.left), y), ATG::Colors::White);


    if (m_casMode != CASMode::Disabled)
    {
        y += m_font->GetLineSpacing();
        swprintf_s(textBuffer, L"CAS Time: %0.3fms", m_gpuTimer->GetElapsedMS(1));
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        y += m_font->GetLineSpacing();
        swprintf_s(textBuffer, L"CAS Sharpness:  %0.2f", m_sharpness);
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        y += m_font->GetLineSpacing();
        swprintf_s(textBuffer, L"Render scale %d x %d (%.f%%)", (int)(size.right*m_renderScales[GetRenderScale()]), 
            (int)(size.bottom*m_renderScales[GetRenderScale()]), m_renderScales[GetRenderScale()] * 100.f);
        m_font->DrawString(m_batch.get(), textBuffer, XMFLOAT2(float(safe.left), y), ATG::Colors::White);

        if (GetRenderScale() != 0)
        {
            shaderAAStr = L"FidelityFX CAS (Upscaled):";
        }
        else
        {
            shaderAAStr = L"FidelityFX CAS scene:";
        }
    }
    else
    {
        shaderAAStr = L"Base output scene:";
    }

    DX::DrawControllerString(m_batch.get(), m_font.get(), m_colorCtrlFont.get(),
        shaderAAStr.c_str(),
        XMFLOAT2(float(c_zoomDestCas[0]*size.right), float(c_zoomDestCas[1] * size.bottom)), ATG::Colors::LightGrey);


    m_font->DrawString(m_batch.get(),  L"Base input scene:",
        XMFLOAT2(float(c_zoomDest[0] * size.right), float(c_zoomDest[1] * size.bottom)), ATG::Colors::LightGrey);

    const wchar_t* legendStr = L"[View] Exit   [LThumb] Move Camera  [A]/[B] FidelityFX CAS Mode [X]/[Y] Render Scale [LT]/[RT] Sharpness";

    DX::DrawControllerString(m_batch.get(),
        m_font.get(), m_ctrlFont.get(),
        legendStr, XMFLOAT2(float(safe.left), float(safe.bottom)),
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
    viewport.Height *= m_renderScales[GetRenderScale()];
    viewport.Width *= m_renderScales[GetRenderScale()];

    scissorRect.bottom = static_cast<LONG>(scissorRect.bottom * m_renderScales[GetRenderScale()]);
    scissorRect.right = static_cast<LONG>(scissorRect.right * m_renderScales[GetRenderScale()]);

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

    CreateWindowSizeDependentResources();
}

void Sample::OnDeviceLost()
{
    m_gpuTimer.reset();
    m_graphicsMemory.reset();

    m_computeRootSignature.Reset();
    
    GetCasPipelineState(UpscaleMode::NativeRender, CASMode::Sharpen).Reset();
    GetCasPipelineState(UpscaleMode::Upscaled, CASMode::Sharpen).Reset();

#ifdef _GAMING_XBOX_SCARLETT
    GetCasPipelineState(UpscaleMode::NativeRender, CASMode::Sharpen_Wave32).Reset();
    GetCasPipelineState(UpscaleMode::Upscaled, CASMode::Sharpen_Wave32).Reset();
#endif

    m_casOutput.Reset(); 

    m_font.reset();
    m_colorCtrlFont.reset();
    m_ctrlFont.reset();

    m_model = nullptr;
    m_resourceDescriptors.reset();
    m_renderDescriptors.reset();
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
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    m_renderDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        RTDescriptors::RTCount);

    m_states = std::make_unique<CommonStates>(device);
    wchar_t filepath[MAX_PATH];
#ifdef _GAMING_XBOX
    DX::FindMediaFile(filepath, _countof(filepath), L"AbstractCathedral.sdkmes_");
#else
    DX::FindMediaFile(filepath, _countof(filepath), L"AbstractCathedral\\AbstractCathedral.sdkmes_");
#endif

    {
        auto modelBlob = DX::ReadCompressedData(filepath);
        m_model = Model::CreateFromSDKMESH(device, modelBlob.data(), modelBlob.size());
    }

    // Upload resources to video memory
    ResourceUploadBatch upload(device);
    upload.Begin();

    m_modelResources = std::make_unique<EffectTextureFactory>(device, upload, m_model->textureNames.size());
    m_modelResources->EnableForceSRGB(true);

    //Set the resource directory to the same one found for the .sdkmesh
    wchar_t dirpath[MAX_PATH];
    wchar_t* strFilePart = nullptr;
    swprintf_s(dirpath, MAX_PATH, L"%ls\\..", filepath);
    GetFullPathNameW(dirpath, MAX_PATH, filepath, &strFilePart); 
    m_modelResources->SetDirectory(filepath);
    m_model->LoadTextures(*m_modelResources);

    m_fxFactory = std::make_unique<EffectFactory>(m_modelResources->Heap(), m_states->Heap());

    m_model->LoadStaticBuffers(device, upload);

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

    // Scene effects
    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    {
        EffectPipelineStateDescription pdOpaque(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        EffectPipelineStateDescription pdAlpha(
            nullptr,
            CommonStates::AlphaBlend,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        m_modelEffect = m_model->CreateEffects(*m_fxFactory, pdOpaque, pdAlpha);

    }

    // Line drawing for Zoom window
    {
        m_lines = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

        EffectPipelineStateDescription pd(
            &VertexPositionColor::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthNone,
            CommonStates::CullNone,
            rtStateUI,
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);

        m_lineEFfect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);

        Matrix proj = Matrix::CreateScale(2.f, -2.f, 1.f)
            * Matrix::CreateTranslation(-1.f, 1.f, 0.f);
        m_lineEFfect->SetProjection(proj);
    }

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

        // Zoom View
        vertexShaderBlob = DX::ReadData(L"ZoomViewVS.cso");
        pixelShaderBlob = DX::ReadData(L"ZoomViewPS.cso");

        vertexShader = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        pixelShader = { pixelShaderBlob.data(), pixelShaderBlob.size() };

        pd.CreatePipelineState(
            device,
            rootSig,
            vertexShader,
            pixelShader,
            m_zoomPSO.ReleaseAndGetAddressOf());
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

    auto downscalesize = size;
    downscalesize.bottom = static_cast<LONG>(downscalesize.bottom * m_renderScales[GetRenderScale()]);
    downscalesize.right = static_cast<LONG>(downscalesize.right * m_renderScales[GetRenderScale()]);

    // Offscreen render targets
    m_scene->SetWindow(downscalesize);

    auto res = m_scene->GetResource();
    if (res)
        res->SetName(L"Scene");

    // Camera
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 10000.f);

    // Set zoom rectangles
    m_zoomViewport.TopLeftX = c_zoomDest[0] * float(size.right);
    m_zoomViewport.TopLeftY = c_zoomDest[1] * float(size.bottom);
    m_zoomViewport.Width = (c_zoomDest[2] - c_zoomDest[0]) * float(size.right);
    m_zoomViewport.Height = (c_zoomDest[3] - c_zoomDest[1]) * float(size.bottom);
    m_zoomViewport.MinDepth = 0.0f;
    m_zoomViewport.MaxDepth = 1.0f;

    m_zoomRect.left = LONG(m_zoomViewport.TopLeftX);
    m_zoomRect.top = LONG(m_zoomViewport.TopLeftY);
    m_zoomRect.right = m_zoomRect.left + LONG(m_zoomViewport.Width);
    m_zoomRect.bottom = m_zoomRect.top + LONG(m_zoomViewport.Height);


    // Set zoom rectangles
    m_zoomViewportCas.TopLeftX = c_zoomDestCas[0] * float(size.right);
    m_zoomViewportCas.TopLeftY = c_zoomDestCas[1] * float(size.bottom);
    m_zoomViewportCas.Width = (c_zoomDestCas[2] - c_zoomDestCas[0]) * float(size.right);
    m_zoomViewportCas.Height = (c_zoomDestCas[3] - c_zoomDestCas[1]) * float(size.bottom);
    m_zoomViewportCas.MinDepth = 0.0f;
    m_zoomViewportCas.MaxDepth = 1.0f;

    m_zoomRectCas.left = LONG(m_zoomViewportCas.TopLeftX);
    m_zoomRectCas.top = LONG(m_zoomViewportCas.TopLeftY);
    m_zoomRectCas.right = m_zoomRectCas.left + LONG(m_zoomViewportCas.Width);
    m_zoomRectCas.bottom = m_zoomRectCas.top + LONG(m_zoomViewportCas.Height);

    const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC texDesc = {}; 
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    texDesc.Width = static_cast<UINT64>(size.right);
    texDesc.Height = static_cast<UINT>(size.bottom);
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    texDesc.DepthOrArraySize = 1;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    DX::ThrowIfFailed(
        device->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &texDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_casOutput.ReleaseAndGetAddressOf())));

    m_casOutput->SetName(L"m_casOutput");

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    uavDesc.Texture2D.PlaneSlice = 0;

    device->CreateUnorderedAccessView(m_casOutput.Get(), nullptr, &uavDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::CAS_OutputUAV));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0;

    device->CreateShaderResourceView(m_casOutput.Get(), &srvDesc, m_resourceDescriptors->GetCpuHandle(Descriptors::CAS_OutputSRV));

    m_casOutputState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
}
#pragma endregion

void Sample::TransitionCasUAVTo(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES afterState)
{
    if (m_casOutputState != afterState)
    {
        TransitionResource(commandList, m_casOutput.Get(), m_casOutputState, afterState);
        m_casOutputState = afterState;
    }
}
