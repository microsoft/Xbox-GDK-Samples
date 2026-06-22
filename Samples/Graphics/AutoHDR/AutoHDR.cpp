//--------------------------------------------------------------------------------------
// AutoHDR.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AutoHDR.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "ReadData.h"

#include <XDisplay.h>

extern void ExitSample() noexcept;

using namespace DX;
using namespace DirectX;
using namespace Colors;
using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_bAdjustUIBrighness(true),
    m_bIsTVInHDRMode(false),
    m_UIBrightnessScale(1.f),
    m_currentSDRTexture(0),
    m_frame(0)
{
    // NOTE: DXGI_FORMAT_R9G9B9E5_SHAREDEXP gives highest precision. Alternatively, you could use DXGI_FORMAT_R10G10B10A2_UNORM,
    // but it's NOT recommended to use DXGI_FORMAT_R8G8B8A8_UNORM which can cause banding in HDR mode
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
        DXGI_FORMAT_UNKNOWN,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    for (int i = 0; i < c_NumImages; i++)
    {
        m_sdrTextureFinishedLoading[i] = false;
    }
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    // Try to enter HDR mode before creating the Direct3D device.
    SetDisplayMode();

    auto createDeviceFlags = D3D12XBOX_CREATE_DEVICE_FLAG_NONE;

    if (m_bIsTVInHDRMode)
    {
        // Specify the Auto HDR flag during D3D device creation
        createDeviceFlags |= D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR;
    }

    m_deviceResources->CreateDeviceResources(createDeviceFlags);
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Render all UI at 1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_fontBatch->SetViewport(viewportUI);
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

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        // Toggle UI brightness adjustment
        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_bAdjustUIBrighness = !m_bAdjustUIBrighness;
        }

        if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentSDRTexture--;
            if (m_currentSDRTexture < 0)
            {
                m_currentSDRTexture = c_NumImages - 1;
            }
        }

        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentSDRTexture++;
            m_currentSDRTexture = m_currentSDRTexture % c_NumImages;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
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

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    // The sample renders and presents in SDR
    RenderSDRScene();
    RenderUI();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Render SDR images
void Sample::RenderSDRScene()
{
    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"RenderSDRScene");

    // 4K textures are pretty big, so load them async. Show a message when not done loading
    if (m_sdrTextureFinishedLoading[m_currentSDRTexture])
    {
        auto srv = m_resourceDescriptorHeap->GetGpuHandle(size_t(ResourceDescriptors::SDRTexture + m_currentSDRTexture));
        m_fullScreenQuad->Draw(d3dCommandList, m_d3dRenderSDRTexturePSO.Get(), srv);
    }
    else
    {
        m_fontBatch->Begin(d3dCommandList);
        m_textFont->DrawString(m_fontBatch.get(), L"Loading texture ...", SimpleMath::Vector2(500, 500), XMVectorScale(White, m_UIBrightnessScale), 0.0f, g_XMZero, 1.0f);
        m_fontBatch->End();
    }

    PIXEndEvent(d3dCommandList);
}

// Render text with dark gray shadow which helps when text is on top of bright areas on the image
void Sample::DrawStringWithShadow(const wchar_t* string, DirectX::SimpleMath::Vector2& fontPos, DirectX::FXMVECTOR color, float fontScale)
{
    fontPos.x += 1;
    fontPos.y += 1.0f;
    m_textFont->DrawString(m_fontBatch.get(), string, fontPos, XMVectorScale(DarkSlateGray, m_UIBrightnessScale), 0.0f, g_XMZero, fontScale);
    fontPos.x -= 1;
    fontPos.y -= 1.0f;
    m_textFont->DrawString(m_fontBatch.get(), string, fontPos, color, 0.0f, g_XMZero, fontScale);
}

// Render the UI
void Sample::RenderUI()
{
    if (m_bIsTVInHDRMode)
    {
        // Auto HDR will show pure white pixels as 1000 nits, so text/UI/HUD will become much too bright
        // We linearly scale down the brightness of the UI
        m_UIBrightnessScale = m_bAdjustUIBrighness ? 0.8f : 1.0f;
    }
    else
    {
        // If the TV is in SDR mode, we don't do any brightness scaling
        m_UIBrightnessScale = 1.0f;
        m_bAdjustUIBrighness = false;
    }

    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"RenderUI");

    float fontScale = 1.0f;
    SimpleMath::Vector2 fontPos;

    m_fontBatch->Begin(d3dCommandList);

    fontPos.x = 50.0f;
    fontPos.y = 50.0f;
    DrawStringWithShadow(L"Auto HDR Sample", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontScale = 0.8f;
    fontPos.y += 100.0f;
    DrawStringWithShadow(m_bIsTVInHDRMode ? L"Display in HDR Mode: TRUE" : L"Display in HDR Mode: FALSE", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.y += 50.0f;
    DrawStringWithShadow(m_bAdjustUIBrighness ? L"Adjust UI Brightness: TRUE" : L"Adjust UI Brightness: FALSE", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.x = 25;
    fontPos.y = 1000;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[A] Toggle UI Brightness Adjustment", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.x = 1400;
    fontPos.y = 1000;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[RB] Next image\n[LB] Prev image", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    m_fontBatch->End();

    PIXEndEvent(d3dCommandList);
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::ColorsLinear::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

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
    // Display modes could have changed while title was suspended, so we need to make sure
    // that the TV is still in HDR mode. This is required for native HDR and Auto HDR.
    SetDisplayMode();

    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
}

void Sample::OnConstrained()
{
}

void Sample::OnUnConstrained()
{
    // Display modes could have changed while title was constrained, so we need to make sure
    // that the TV is still in HDR mode. This is required for native HDR and Auto HDR.
    SetDisplayMode();
}

#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_fullScreenQuad = std::make_unique<FullScreenQuad>();
    m_fullScreenQuad->Initialize(m_deviceResources->GetD3DDevice());

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create descriptor heaps
    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::Count);

    // Init fonts
    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    InitializeSpriteFonts(device, resourceUpload, rtState);

    // PSO for rendering the SDR texture
    {
        auto pixelShaderBlob = DX::ReadData(L"FullScreenQuadGammaPS.cso");
        auto vertexShaderBlob = DX::ReadData(L"FullScreenQuadVS.cso");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_fullScreenQuad->GetRootSignature();
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
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dRenderSDRTexturePSO.ReleaseAndGetAddressOf())));
    }

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();     // Wait for resources to upload

    // Load SDR textures async
    concurrency::task<bool> t([this]()
    {
        for (int i = 0; i < c_NumImages; i++)
        {
            auto device = m_deviceResources->GetD3DDevice();
            std::unique_ptr<ResourceUploadBatch> resourceUpload = std::make_unique<ResourceUploadBatch>(device);
            resourceUpload->Begin();

            auto srv = m_resourceDescriptorHeap->GetCpuHandle(size_t(ResourceDescriptors::SDRTexture + i));
            m_sdrTexture[i] = std::make_unique<DX::Texture>(device, *(resourceUpload.get()), srv, m_sdrTextureFiles[i], false);

            auto uploadResourcesFinished = resourceUpload->End(m_deviceResources->GetCommandQueue());
            uploadResourcesFinished.wait();

            m_sdrTextureFinishedLoading[i] = true;
        }

        return true;
    });
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
}

// Initialize all the fonts used
void Sample::InitializeSpriteFonts(ID3D12Device* d3dDevice, ResourceUploadBatch& resourceUpload, const RenderTargetState& rtState)
{
    const SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
    m_fontBatch = std::make_unique<SpriteBatch>(d3dDevice, resourceUpload, pd);

    auto index = static_cast<size_t>(ResourceDescriptors::TextFont);
    m_textFont = std::make_unique<SpriteFont>(d3dDevice, resourceUpload, L"Courier_36.spritefont",
            m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));

    index = static_cast<size_t>(ResourceDescriptors::ControllerFont);
    m_controllerFont = std::make_unique<SpriteFont>(d3dDevice, resourceUpload, L"XboxOneControllerSmall.spritefont",
            m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));
}
#pragma endregion

// HDR Helper
void Sample::SetDisplayMode()
{
    // AutoHDR is only supported on Xbox Series X|S

    // Request HDR mode.
    auto result = XDisplayTryEnableHdrMode(XDisplayHdrModePreference::PreferHdr, nullptr);

    m_bIsTVInHDRMode = (result == XDisplayHdrModeResult::Enabled);

#ifdef _DEBUG
    OutputDebugStringA((m_bIsTVInHDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif
}
