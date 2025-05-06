//--------------------------------------------------------------------------------------
// Simple120Hz.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Simple120Hz.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

#include <XDisplay.h>

extern void ExitSample() noexcept;

using namespace ATG;
using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* c_sampleTitle = L"Simple 120 Hz";
    constexpr DXGI_FORMAT c_hdrRenderTargetFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    const UINT c_intervalFrames[Sample::FRAME_INTERVAL_COUNT] =
    {
        30,
        40,
        60,
        120,
#if _GXDK_VER >= 0x55F00C58 /* GDK Edition 220300 */
        24,
#endif
    };

    const UINT c_intervalInMicros[Sample::FRAME_INTERVAL_COUNT] =
    {
        D3D12XBOX_FRAME_INTERVAL_30_HZ,
        D3D12XBOX_FRAME_INTERVAL_40_HZ,
        D3D12XBOX_FRAME_INTERVAL_60_HZ,
        D3D12XBOX_FRAME_INTERVAL_120_HZ,
#if _GXDK_VER >= 0x55F00C58 /* GDK Edition 220300 */
        D3D12XBOX_FRAME_INTERVAL_24_HZ,
#endif
    };

    const wchar_t* c_frameIntervalStr[Sample::FRAME_INTERVAL_COUNT] =
    {
        L"30 Hz (33.3ms)",
        L"40 Hz (25.0ms)",
        L"60 Hz (16.6ms)",
        L"120 Hz (8.3ms)",
#if _GXDK_VER >= 0x55F00C58 /* GDK Edition 220300 */
        L"24 Hz (41.7ms)",
#endif
    };

    enum ShaderResourceViews
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_HdrScene,
        SRV_Count
    };
}

Sample::Sample() noexcept(false)
    : m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
    , m_preferHdr(false)
    , m_isDisplayInHDRMode(false)
    , m_frameInterval(FRAME_INTERVAL_120HZ) // Start the app at 120Hz if it's supported
    , m_refreshRateSupport{}
    , m_vrrSupported(false)
{
    if (XSystemGetDeviceType() < XSystemDeviceType::XboxOneS)
    {
        throw std::exception("This sample is only supported for Xbox One S and above.");
    }

    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD
        | DX::DeviceResources::c_EnableHDR);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_deviceResources->WaitForOrigin();

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
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        // Exit the sample
        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        // Move 'up' the list of available refresh rates
        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
        {
            uint32_t newFrameInterval = m_frameInterval;

            // Keep going until we reach the next supported format - with wrapping at the end.
            do
            {
                newFrameInterval = (newFrameInterval + FRAME_INTERVAL_COUNT - 1) % FRAME_INTERVAL_COUNT;
            } while (!m_refreshRateSupport[newFrameInterval]);

            m_frameInterval = FrameInterval(newFrameInterval);

            UpdateFrameInterval();
        }

        // Move 'down' the list of available refresh rates
        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            uint32_t newFrameInterval = m_frameInterval;

            // Keep going until we reach the next supported format - with wrapping at the end.
            do
            {
                newFrameInterval = (newFrameInterval + 1) % FRAME_INTERVAL_COUNT;
            } while (!m_refreshRateSupport[newFrameInterval]);

            m_frameInterval = FrameInterval(newFrameInterval);

            UpdateFrameInterval();
        }

        // Flip the HDR mode preference
        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_preferHdr = !m_preferHdr;

            TryEnableHDR();
            RefreshSupportedIntervals();
            UpdateFrameInterval();
        }
    }
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

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    // Initialize the root signature and pipeline state
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetPipelineState(m_pipelineState.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Set constants
    float time = float(m_timer.GetTotalSeconds());
    commandList->SetGraphicsRoot32BitConstants(0, 1, &time, 0);

    // Draw triangle.
    commandList->DrawInstanced(3, 1, 0, 0);

    // Draw the UI
    DrawHUD(commandList);

    // Copy linear color buffer to HDR 10 backbuffer
    ConvertToHDR10(commandList);

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // We need to sample from the HDR backbuffer
    TransitionResource(commandList, m_renderTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Clear the views.
    auto const rtvDescriptor = m_rtvPile->GetFirstCpuHandle();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

    // Use linear clear color for gamma-correct rendering.
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::DrawHUD(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"HUD");

    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(m_displayWidth, m_displayHeight);
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
    XMVECTOR textColor = DirectX::Colors::DarkKhaki;

    m_hudBatch->Begin(commandList);

    // Draw our settings and stats
    wchar_t textBuffer[128] = {};

    // Draw app state
    m_smallFont->DrawString(m_hudBatch.get(), c_sampleTitle, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing() * 2;

    swprintf_s(textBuffer, L"HDR Mode Preference: %s", m_preferHdr ? L"HDR" : L"Refresh Rate");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

#ifdef _GAMING_XBOX_SCARLETT
    swprintf_s(textBuffer, L"VRR Supported: %s", m_vrrSupported ? L"True" : L"False");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();
#endif

    m_smallFont->DrawString(m_hudBatch.get(), L"Refresh Rates:", textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    for (size_t i = 0; i < FRAME_INTERVAL_COUNT; ++i)
    {
        if (m_frameInterval == i)
        {
            m_smallFont->DrawString(m_hudBatch.get(), L">", textPos, textColor);
        }

        swprintf_s(textBuffer, L"    %s - %s", c_frameIntervalStr[i], m_refreshRateSupport[i] ? L"Supported" : L"Unsupported");
        m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
        textPos.y += m_smallFont->GetLineSpacing();
    }
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"FPS: %d", m_timer.GetFramesPerSecond());
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    swprintf_s(textBuffer, L"HDR Enabled: %s", m_isDisplayInHDRMode ? L"True" : L"False");
    m_smallFont->DrawString(m_hudBatch.get(), textBuffer, textPos, textColor);
    textPos.y += m_smallFont->GetLineSpacing();

    // Draw the controls
    textPos = XMFLOAT2(float(safe.left), float(safe.bottom - m_smallFont->GetLineSpacing()));

    const wchar_t* controlString = L"[DPad] - Select Refresh Rate    [A] - Toggle HDR Mode Preference    [View] - Exit Sample";
    DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), controlString, textPos, textColor);

    m_hudBatch->End();

    PIXEndEvent(commandList);
}

// Convert HDR scene to HDR10 and it to the swap chain buffer
void Sample::ConvertToHDR10(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"ConvertToHDR10");

    // We need to sample from the HDR backbuffer
    TransitionResource(commandList, m_renderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Set RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor[1] = { m_deviceResources->GetRenderTargetView() };
    commandList->OMSetRenderTargets(1, rtvDescriptor, FALSE, nullptr);

    // Render
    commandList->SetGraphicsRootSignature(m_d3dConvertToHDR10RS.Get());
    commandList->SetPipelineState(m_d3dConvertToHDR10PSO.Get());

    commandList->SetGraphicsRootDescriptorTable(0, m_srvPile->GetGpuHandle(SRV_HdrScene));
    commandList->DrawInstanced(3, 1, 0, 0);

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

    // While a title is suspended, the console TV settings could have changed, so we need to call the display APIs when resuming
    TryEnableHDR();
    RefreshSupportedIntervals();
    UpdateFrameInterval();
}

void Sample::OnConstrained()
{
}

void Sample::OnUnConstrained()
{
    // While a title is constrained, the console TV settings could have changed, so we need to call the display APIs when unconstraining
    TryEnableHDR();
    RefreshSupportedIntervals();
    UpdateFrameInterval();
}

#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_srvPile = std::make_unique<DescriptorPile>(
        device,
        128,
        SRV_Count);

    m_rtvPile = std::make_unique<DescriptorPile>(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        2,
        1);

    // Create the pipeline state, which includes loading shaders.
    {
        // Create root signature.
        auto vertexShaderBlob = DX::ReadData(L"VertexShader.cso");
        auto pixelShaderBlob = DX::ReadData(L"PixelShader.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(
            0,
            vertexShaderBlob.data(), vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())
        ));

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = c_hdrRenderTargetFormat;
        psoDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_pipelineState.ReleaseAndGetAddressOf())));
    }

    // Create PSO for rendering to the HDR10 swapchain buffer
    {
        auto vertexShaderBlob = DX::ReadData(L"ConvertToHDR10VS.cso");
        auto pixelShaderBlob = DX::ReadData(L"ConvertToHDR10PS.cso");

        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(
            0,
            vertexShaderBlob.data(), vertexShaderBlob.size(),
            IID_GRAPHICS_PPV_ARGS(m_d3dConvertToHDR10RS.ReleaseAndGetAddressOf())
        ));

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_d3dConvertToHDR10RS.Get();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;

        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dConvertToHDR10PSO.ReleaseAndGetAddressOf())));
    }

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();

    // Create the HUD sprite batch
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        const RenderTargetState backBufferRts(c_hdrRenderTargetFormat, m_deviceResources->GetDepthBufferFormat());
        const SpriteBatchPipelineStateDescription spritePSD(backBufferRts, &CommonStates::AlphaBlend);

        m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

        resourceUpload.End(m_deviceResources->GetCommandQueue());
    }

    TryEnableHDR();
    RefreshSupportedIntervals();
    UpdateFrameInterval();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    auto const size = m_deviceResources->GetOutputSize();
    m_displayWidth = static_cast<uint32_t>(size.right - size.left);
    m_displayHeight = static_cast<uint32_t>(size.bottom - size.top);

    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    // Create the HDR render target
    const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(
        c_hdrRenderTargetFormat,
        m_displayWidth,
        m_displayHeight,
        1, 1, 1, 0,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    // Use Delta Color Compression and Texture Compatibility on Xbox One X & Xbox Series X|S to leverage hardware resource decompression.
    if (XSystemGetDeviceType() > XSystemDeviceType::XboxOneS)
    {
        desc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC | D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
    }

    D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(c_hdrRenderTargetFormat, DirectX::Colors::Black);

    DX::ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
        &desc,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        &clearValue,
        IID_GRAPHICS_PPV_ARGS(m_renderTexture.ReleaseAndGetAddressOf())
    ));

    // Create RTV.
    device->CreateRenderTargetView(m_renderTexture.Get(), nullptr, m_rtvPile->GetFirstCpuHandle());

    // Create SRV.
    device->CreateShaderResourceView(m_renderTexture.Get(), nullptr, m_srvPile->GetCpuHandle(SRV_HdrScene));

    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1440) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
            m_srvPile->GetCpuHandle(SRV_Font),
            m_srvPile->GetGpuHandle(SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1440) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
            m_srvPile->GetCpuHandle(SRV_CtrlFont),
            m_srvPile->GetGpuHandle(SRV_CtrlFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }
}
#pragma endregion

void Sample::RefreshSupportedIntervals()
{
    auto device = m_deviceResources->GetD3DDevice();

    ComPtr<IDXGIDevice> dxgiDevice;
    DX::ThrowIfFailed(device->QueryInterface(IID_GRAPHICS_PPV_ARGS(dxgiDevice.GetAddressOf())));

    ComPtr<IDXGIAdapter> dxgiAdapter;
    DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

    ComPtr<IDXGIOutput> dxgiOutput;
    DX::ThrowIfFailed(dxgiAdapter->EnumOutputs(0, dxgiOutput.GetAddressOf()));

    m_vrrSupported = false;

#ifdef _GAMING_XBOX_SCARLETT
    UINT outputModeCount = 0;
    std::ignore = dxgiOutput->GetDisplayModeListX(m_deviceResources->GetBackBufferFormat(), 0, &outputModeCount, nullptr);

    auto outputModes = std::make_unique<DXGIXBOX_MODE_DESC[]>(outputModeCount);

    DX::ThrowIfFailed(dxgiOutput->GetDisplayModeListX(m_deviceResources->GetBackBufferFormat(), 0, &outputModeCount, outputModes.get()));

    for (uint32_t i = 0; i < outputModeCount; ++i)
    {
        m_vrrSupported |= (outputModes[i].Flags & DXGIXBOX_MODE_FLAG_VARIABLE_REFRESH_RATE) != 0;
    }
#else
    UINT outputModeCount = 0;
    std::ignore = dxgiOutput->GetDisplayModeList(m_deviceResources->GetBackBufferFormat(), 0, &outputModeCount, nullptr);

    auto outputModes = std::make_unique<DXGI_MODE_DESC[]>(outputModeCount);

    DX::ThrowIfFailed(dxgiOutput->GetDisplayModeList(m_deviceResources->GetBackBufferFormat(), 0, &outputModeCount, outputModes.get()));
#endif

    // Reset support
    memset(m_refreshRateSupport, 0, sizeof(m_refreshRateSupport));
    m_refreshRateSupport[FRAME_INTERVAL_30HZ] = m_refreshRateSupport[FRAME_INTERVAL_60HZ] = true; // 30 & 60 Hz are always supported.

    for (size_t i = 0; i < outputModeCount; ++i)
    {
        for (size_t j = 0; j < FRAME_INTERVAL_COUNT; ++j)
        {
            if (outputModes[i].RefreshRate.Numerator == c_intervalFrames[j])
            {
                m_refreshRateSupport[j] = true;
                break;
            }
        }
    }

    // Reset frame interval to 60Hz if the current one isn't supported after the refresh.
    if (!m_refreshRateSupport[uint32_t(m_frameInterval)])
    {
        m_frameInterval = FRAME_INTERVAL_60HZ;
    }
}

void Sample::UpdateFrameInterval()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Set the frame interval in microseconds
    uint32_t intervalIndex = uint32_t(m_frameInterval);
    DX::ThrowIfFailed(device->SetFrameIntervalX(nullptr, c_intervalInMicros[intervalIndex], 1u, D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));

    // Reschedule the frame event origin
    DX::ThrowIfFailed(device->ScheduleFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, 0U, nullptr, D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));
}

void Sample::TryEnableHDR()
{
    auto preference = m_preferHdr ? XDisplayHdrModePreference::PreferHdr : XDisplayHdrModePreference::PreferRefreshRate;

    // Try to enable HDR, with preference of HDR or Refresh Rate if the display doesn't support simultaneous 120Hz & HDR.
    auto result = XDisplayTryEnableHdrMode(preference, nullptr);
    m_isDisplayInHDRMode = (result == XDisplayHdrModeResult::Enabled);
}
