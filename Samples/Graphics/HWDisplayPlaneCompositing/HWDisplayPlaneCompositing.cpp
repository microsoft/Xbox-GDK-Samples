//--------------------------------------------------------------------------------------
// HWDisplayPlaneCompositing.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HWDisplayPlaneCompositing.h"
#include <grdk.h>

extern void ExitSample() noexcept;

using namespace DirectX;

extern bool g_HDRMode;

namespace
{
    const wchar_t* c_sampleTitle = L"HW Display Plane Compositing";

    constexpr DXGI_FORMAT g_sceneBackBufferFormat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    constexpr DXGI_FORMAT g_uiBackBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;

    enum class RTVs
    {
        HDRScene,
        UI,
        NumRTVs
    };

    enum class SRVs
    {
        HDRScene,
        UI,
        TextFont,
        ControllerFont,
        NumSRVs
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(g_sceneBackBufferFormat, g_uiBackBufferFormat,
            DXGI_FORMAT_UNKNOWN,
            2,

// NOTE: GDKs prior to March 2024 only supports SDR compositing
#if _GRDK_VER < 0x63360C3B /* GDK Edition 240300 */
            DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD);
#else
            DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD | DX::DeviceResources::c_EnableHDR);
#endif
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

    // Render all UI at 1440p so that it's easy to switch between 1440p / 4K
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 2560;
    viewportUI.Height = 1440;
    m_spriteBatch->SetViewport(viewportUI);
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
    RenderScene(commandList);
    RenderUI(commandList);
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

    // Clear the views
    auto const sceneRTV = m_deviceResources->GetSceneRenderTargetView();
    auto const uiRTV = m_deviceResources->GetUIRenderTargetView();

    commandList->OMSetRenderTargets(1, &sceneRTV, FALSE, nullptr);
    commandList->ClearRenderTargetView(sceneRTV, DirectX::Colors::Black, 0, nullptr);

    commandList->OMSetRenderTargets(1, &uiRTV, FALSE, nullptr);
    commandList->ClearRenderTargetView(uiRTV, DirectX::Colors::Black, 0, nullptr);

    PIXEndEvent(commandList);
}

void Sample::RenderScene(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderScene");

    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    auto const sceneRTV = m_deviceResources->GetSceneRenderTargetView();
    auto const sceneSRV = m_hdrSceneImage->GetShaderResourceView();

    commandList->OMSetRenderTargets(1, &sceneRTV, FALSE, nullptr);
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    m_fullScreenQuad->Draw(commandList, g_HDRMode ? m_d3dHDRScenePSO.Get() : m_d3dSDRScenePSO.Get(), sceneSRV);

    PIXEndEvent(commandList);
}

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderUI");

    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    auto const uiRTV = m_deviceResources->GetUIRenderTargetView();
    auto const uiSRV = m_srvPile->GetGpuHandle(static_cast<size_t>(SRVs::UI));

    commandList->OMSetRenderTargets(1, &uiRTV, FALSE, nullptr);
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
    m_fullScreenQuad->Draw(commandList, m_d3dRenderUITexturePSO.Get(), uiSRV);

    wchar_t textBuffer[128] = {};
    const float lineSpacing = m_textFont->GetLineSpacing();
    XMFLOAT2 textPos = XMFLOAT2(50.0f, lineSpacing);

    m_spriteBatch->Begin(commandList);

    m_textFont->DrawString(m_spriteBatch.get(), c_sampleTitle, textPos, DirectX::Colors::White, 0.0f);

    textPos.y += lineSpacing * 2.0f;
    swprintf_s(textBuffer, L"TV in HDR mode: %ls", g_HDRMode ? L"True" : L"False");
    m_textFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, DirectX::Colors::White, 0.0f, DirectX::XMFLOAT2(0.0f, 0.0f), 0.8f);

    textPos.y += lineSpacing * 2.0f;
    const wchar_t* controlString = L"[View] - Exit Sample";
    DX::DrawControllerString(m_spriteBatch.get(), m_textFont.get(), m_controllerFont.get(), controlString, textPos, DirectX::Colors::White, 0.8f);

    m_spriteBatch->End();

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
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_rtvPile = std::make_unique<DescriptorPile>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 16, static_cast<size_t>(RTVs::NumRTVs));
    m_srvPile = std::make_unique<DescriptorPile>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 16, static_cast<size_t>(SRVs::NumSRVs));

    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(m_deviceResources->GetD3DDevice());

    // Upload assets
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        // Fonts
        {
            auto uiBackBufferRTS = RenderTargetState(g_uiBackBufferFormat, m_deviceResources->GetDepthBufferFormat());
            auto spritePSD = SpriteBatchPipelineStateDescription(uiBackBufferRTS, &CommonStates::AlphaBlend);
            m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

            auto index = static_cast<size_t>(SRVs::TextFont);
            m_textFont = std::make_unique<SpriteFont>(device, resourceUpload, L"SegoeUI_36.spritefont", m_srvPile->GetCpuHandle(index), m_srvPile->GetGpuHandle(index));

            index = static_cast<size_t>(SRVs::ControllerFont);
            m_controllerFont = std::make_unique<SpriteFont>(device, resourceUpload, L"XboxOneControllerLegend.spritefont", m_srvPile->GetCpuHandle(index), m_srvPile->GetGpuHandle(index));
        }

        // UI texture
        auto uiSRV = m_srvPile->GetCpuHandle(static_cast<size_t>(SRVs::UI));
        m_uiTexture = std::make_unique<DX::Texture>(device, resourceUpload, uiSRV, L"Assets\\UITexture.png", true);

        auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        uploadResourcesFinished.wait();
    }

    auto fullScreenVSBlob = DX::ReadData(L"FullScreenQuadVS.cso");
    auto fullScreenPSBlob = DX::ReadData(L"FullScreenQuadPS.cso");
    auto SDRScenePSBlob = DX::ReadData(L"SDRScenePS.cso");
    auto HDRScenePSBlob = DX::ReadData(L"HDRScenePS.cso");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    // PSO for rendering the UI texture
    {
        psoDesc.pRootSignature = m_fullScreenQuad->GetRootSignature();
        psoDesc.VS = { fullScreenVSBlob.data(), fullScreenVSBlob.size() };
        psoDesc.PS = { fullScreenPSBlob.data(), fullScreenPSBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DSVFormat = m_deviceResources->GetDepthBufferFormat();
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = g_uiBackBufferFormat;
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dRenderUITexturePSO.ReleaseAndGetAddressOf())));
    }

    // PSO for rendering the HDR scene
    {
        psoDesc.PS = { HDRScenePSBlob.data(), HDRScenePSBlob.size() };
        psoDesc.RTVFormats[0] = g_sceneBackBufferFormat;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dHDRScenePSO.ReleaseAndGetAddressOf())));
    }

    // PSO for rendering the SDR tonemapped scene
    {
        psoDesc.PS = { SDRScenePSBlob.data(), SDRScenePSBlob.size() };
        psoDesc.RTVFormats[0] = g_sceneBackBufferFormat;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dSDRScenePSO.ReleaseAndGetAddressOf())));
    }

    // Load HDR texture used as scene rendering
    m_hdrSceneImage = std::make_unique<DX::HDRImage>();
    auto index = static_cast<size_t>(SRVs::HDRScene);
    m_hdrSceneImage->SetShaderResourceView(m_srvPile->GetCpuHandle(index), m_srvPile->GetGpuHandle(index));
    m_hdrSceneImage->Load(L"graffiti_shelter_2k.hdr", m_deviceResources->GetD3DDevice(), m_deviceResources->GetCommandQueue());

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
}
#pragma endregion
