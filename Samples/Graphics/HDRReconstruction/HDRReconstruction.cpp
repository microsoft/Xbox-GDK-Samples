//--------------------------------------------------------------------------------------
// HDRReconstruction.cpp
//
// This sample shows how HDR can be reconstructed from an already tonemapped SDR image as
// a simple postprocessing technique.The technique is useful for adding HDR to a game without
// disrupting the render pipeline, keeping the same artistic intent as the tonemapped image,
// and can also be applied to SDR videos and UI splash screens. The sample has a toggle to
// easily compare the difference between SDR and reconstructed HDR.
//
// One challenge with HDR is that videos and UI splash screens are mostly saved as SDR, and
// will look dull compared to the actual rendered scene. This technique can HDR'ify the SDR
// content automatically. Another challenge is that when you simply remove the game's tonemap
// operator to retain HDR scene values, the artistic intent of the image might get lost,
// especially if the tonemap operator is combined with operations like color grading, brightness
// and contrast. A good short-term solution is to reconstruct HDR scene values from the already
// tonemapped and color graded final image of the game, using an inverse tonemapper, thus keeping
// the artistic intent of the SDR image. We refer to this as SDR mastered, and cannot utilize
// HDR as an HDR mastered image, but it's still a good short-term solution with which games
// have already shipped. The pixel shader can easily be optimized using a 3D lookup table (LUT).
//
// Pros
//      -SDR mastered, i.e. the SDR artistic intent stays the same for HDR
//      -Postprocessing stays the same, e.g. color grading, tone mapping, AA
//      -Cut scene videos stay the same
//      -Simple to implement as a single postprocessing technique
// Cons
//      -Colors close to white look emissive, e.g. fog, particles
//      -Noise like film grain gets exaggerated
//      -Loss of precision, FP11:11:10 -> 10:10:10:2
//      -Not as good as HDR mastered
//
// NOTE: The sample uses several 8-bit compressed SDR images to show the result, so compression
// artifacts and banding might be visible, which is not caused by the HDR reconstruction technique.
//
// Refer to the Xfest 2017 presentation "HDR Tips and Tricks from the Trenches" http://aka.ms/XF17022
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HDRReconstruction.h"
#include "HDR\HDRCommon.h"
#include "ATGColors.h"
#include "ControllerFont.h"
#include "DDSTextureLoader.h"
#include "ReadData.h"
#include "DirectXHelpers.h"

extern void ExitSample() noexcept;

using namespace DX;
using namespace DirectX;
using namespace Colors;
using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float g_PaperWhiteNitsScene = 200.0f;         // How bright is white (1,1,1) in the scene, e.g. and white diffuse wall
    constexpr float g_PaperWhiteNitsUI = 300.0f;            // How bright is white (1,1,1) for UI, e.g. white text
    constexpr float g_MaxReconstructedNits = 1000.0f;       // How bright (in nits) should the tonemapped SDR value of 1.0 be
    constexpr float g_ReconstructedColorSaturation = 0.25f; // Lerping between per luma and per color channel reconstruction, where 0 = only use per luma reconstruction, 1 = use only per color channel reconstruction.
    constexpr float g_DisplayGamma = 1.1f;                  // Should be determined by calibration screen, see the HDRCalibration sample. This is a
}

Sample::Sample() noexcept(false) :
    m_HDRData{},
    m_UIBrightnessScale{},
    m_currentSDRTexture{},
    m_frame(0),
    m_bIsDisplayInHDRMode(false),
    m_savedUseGamutExpansion{}
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM /* GameDVR format */,
        DXGI_FORMAT_UNKNOWN,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD | DX::DeviceResources::c_EnableHDR);
    m_sdrScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
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
    // Try to switch the TV into HDR mode
    SetDisplayMode();

    m_UIBrightnessScale = 1.0f;
    m_currentSDRTexture = 0;
    m_savedUseGamutExpansion = true;

    m_HDRData.bApplyReconstruction = true;
    m_HDRData.bUseGamutExpansion = true;
    m_HDRData.MaxReconstructedNits = g_MaxReconstructedNits;
    m_HDRData.ReconstructedColorSaturation = g_ReconstructedColorSaturation;
    m_HDRData.DisplayGamma = g_DisplayGamma;
    m_HDRData.PaperWhiteNits = g_PaperWhiteNitsScene;

    for (int i = 0; i < c_NumImages; i++)
    {
        m_sdrTextureFinishedLoading[i] = false;
    }

    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Render all UI at 1080p so that it's easy to swtich between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_fontBatch->SetViewport(viewportUI);

    UpdateHDRData();
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

    auto gamepad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);
    if (gamepad.IsConnected())
    {
        if (gamepad.IsViewPressed())
        {
            ExitSample();
        }

        m_gamePadButtons.Update(gamepad);

        // Toggle HDR / SDR
        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_HDRData.bApplyReconstruction = !m_HDRData.bApplyReconstruction;

            // Don't use gamut expansion in SDR mode
            if (m_HDRData.bApplyReconstruction)
            {
                m_HDRData.bUseGamutExpansion = m_savedUseGamutExpansion;
            }
            else
            {
                m_savedUseGamutExpansion = m_HDRData.bUseGamutExpansion;
                m_HDRData.bUseGamutExpansion = false;
            }
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_HDRData.bApplyReconstruction)
            {
                m_HDRData.bUseGamutExpansion = !m_HDRData.bUseGamutExpansion;
            }
        }

        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
        {
            m_HDRData.DisplayGamma -= 0.05f;
            m_HDRData.DisplayGamma = std::max(m_HDRData.DisplayGamma, 0.8f);
        }

        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
        {
            m_HDRData.DisplayGamma += 0.05f;
            m_HDRData.DisplayGamma = std::min(m_HDRData.DisplayGamma, 1.4f);
        }

        if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            m_HDRData.ReconstructedColorSaturation -= 0.125f;
            m_HDRData.ReconstructedColorSaturation = std::max(m_HDRData.ReconstructedColorSaturation, 0.0f);
        }

        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            m_HDRData.ReconstructedColorSaturation += 0.125f;
            m_HDRData.ReconstructedColorSaturation = std::min(m_HDRData.ReconstructedColorSaturation, 0.5f);
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

    // Update data for the constant buffer that could have changed from user input
    UpdateHDRData();
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

    RenderSDRScene();
    RenderUI();
    ReconstructHDRAndConvertToHDR10();  // This sample does the HDR reconstruction right before the Present call, but you could do it earlier and have UI renderer on the linear HDR buffer

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Process the HDR scene so that the swapchains can correctly be sent to HDR or SDR display
void Sample::ReconstructHDRAndConvertToHDR10()
{
    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"ReconstructHDRAndConvertToHDR10");

    // We need to sample from the SDR backbuffer
    m_sdrScene->TransitionTo(d3dCommandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Set RTVs for HDR10 and GameDVR
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor[2] = { m_deviceResources->GetRenderTargetView(), m_deviceResources->GetGameDVRRenderTargetView() };
    d3dCommandList->OMSetRenderTargets(2, rtvDescriptor, FALSE, nullptr);

    // Update constant buffer and render
    auto hdrData = m_graphicsMemory->AllocateConstant<HDRData>(m_HDRData);
    auto srv = m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::SDRScene);
    m_fullScreenQuad->Draw(d3dCommandList, m_d3dReconstructHDRAndConvertToHDR10PSO.Get(), srv, hdrData.GpuAddress());

    PIXEndEvent(d3dCommandList);
}

// Render SDR scene. Represents the final tonemapped SDR backbuffer of a game. We simulate this by rendering an SDR tonemapped texture into this buffer. This could even be a photo.
// Note that these images are 8-bit compressed, so you might see compression artifacts and banding in the sample, which is not caused by the HDR reconstruction technique
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

// Update the data for the constant buffer
void Sample::UpdateHDRData()
{
    // In this sample the UI will be rendered on top of the final tonemapped SDR image, i.e. white text will be the value of (1.0f, 1.0f, 1.0f), which
    // as a tonemapped value, is just as bright as a tonemapped sun. Because the value of (1.0f, 1.0f, 1.0f) is reconstructed to the max nits, e.g. 1000 nits,
    // the white UI text will be much too bright and fatiguing to the consumer. We therefore calculate a linear scale to dim down the UI rendering so
    // that when reconstructed, white will be g_PaperWhiteNitsUI.
    if (m_HDRData.bApplyReconstruction)
    {
        // The reconstruction technique uses the Reinhard inverse tonemapper y = x / (1 - x), so we can use the forward Reinhard y = x / (x + 1) to calculate the scale.
        float hdrSceneValueForWhiteText = g_PaperWhiteNitsUI / g_PaperWhiteNitsScene;
        m_UIBrightnessScale = hdrSceneValueForWhiteText / (hdrSceneValueForWhiteText + 1.0f);
    }
    else
    {   // When rendering SDR, no brightness adjustment is needed, white text will just be rendered as paper white
        m_UIBrightnessScale = 1.0f;
    }
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
    auto d3dCommandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(d3dCommandList, PIX_COLOR_DEFAULT, L"RenderUI");

    wchar_t strText[2048];
    float fontScale = 0.75f;
    SimpleMath::Vector2 fontPos;

    m_fontBatch->Begin(d3dCommandList);

    fontPos.x = 50.0f;
    fontPos.y = 50.0f;
    DrawStringWithShadow(L"HDR Reconstruction Sample", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontScale = 0.6f;
    fontPos.y += 100.0f;
    DrawStringWithShadow(m_HDRData.bApplyReconstruction ? L"Mode: Reconstructed HDR" : L"Mode: SDR", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.y += 40.0f;
    DrawStringWithShadow(m_HDRData.bUseGamutExpansion ? L"Gamut Expansion: TRUE" : L"Gamut Expansion: FALSE", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.y += 40.0f;
    swprintf_s(strText, m_bIsDisplayInHDRMode ? L"Max reconstructed brightness: %1.0f nits" : L"Max reconstructed brightness: N/A", m_HDRData.MaxReconstructedNits);
    DrawStringWithShadow(strText, fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.y += 40.0f;
    swprintf_s(strText, m_bIsDisplayInHDRMode ? L"Reconstructed color saturation: %1.2f" : L"Reconstructed color saturation: N/A", m_HDRData.ReconstructedColorSaturation);
    DrawStringWithShadow(strText, fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.y += 40.0f;
    swprintf_s(strText, L"Display Gamma: %1.2f", m_HDRData.DisplayGamma);
    DrawStringWithShadow(strText, fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.y += 40.0f;
    DrawStringWithShadow(m_bIsDisplayInHDRMode ? L"TV in HDR Mode: TRUE" : L"TV in HDR Mode: FALSE", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.x = 25;
    fontPos.y = 1000;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[A] Toggle HDR / SDR\n[B] Toggle Gamut Expansion", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

    fontPos.x = 600;
    fontPos.y = 1000;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[Dpad] Left/Right Reconstruced Color Saturation\n[Dpad] Up/Down Display Gamma", fontPos, XMVectorScale(White, m_UIBrightnessScale), fontScale);

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

    m_sdrScene->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);
    auto const rtv = m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::SDRSceneRTV);
    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    // Clear the views.
    commandList->ClearRenderTargetView(m_deviceResources->GetRenderTargetView(), Black, 0, nullptr);
    commandList->ClearRenderTargetView(m_deviceResources->GetGameDVRRenderTargetView(), Black, 0, nullptr);
    commandList->ClearRenderTargetView(rtv, Black, 0, nullptr);

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
    // While a title is suspended, the console TV settings could have changed, so we need to call the display APIs when resuming
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
    // While a title is constrained, the console TV settings could have changed, so we need to call the display APIs when unconstraining
    SetDisplayMode();
}

#pragma endregion

#pragma region Direct3D Resources
void Sample::SetDisplayMode()
{
    if ((m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0)
    {
        // Request HDR mode.
        auto result = XDisplayTryEnableHdrMode(XDisplayHdrModePreference::PreferHdr, nullptr);

        m_bIsDisplayInHDRMode = (result == XDisplayHdrModeResult::Enabled);

#ifdef _DEBUG
        OutputDebugStringA((m_bIsDisplayInHDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif
    }
}

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
    m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RTVDescriptors::CountRTV);
    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::Count);

    // Init fonts
    const RenderTargetState rtState(m_sdrScene->GetFormat(), m_deviceResources->GetDepthBufferFormat());
    InitializeSpriteFonts(device, resourceUpload, rtState);

    // PSO for rendering the tonemapped SDR texture into the SDR backbuffer
    {
        auto pixelShaderBlob = DX::ReadData(L"FullScreenQuadPS.cso");
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
        psoDesc.RTVFormats[0] = m_sdrScene->GetFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dRenderSDRTexturePSO.ReleaseAndGetAddressOf())));
    }

    // PSO for reconstructing HDR from SDR, then converint to HDR10
    {
        auto pixelShaderBlob = DX::ReadData(L"ReconstructHDRAndConvertToHDR10PS.cso");
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
        psoDesc.NumRenderTargets = 2;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.RTVFormats[1] = m_deviceResources->GetGameDVRFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dReconstructHDRAndConvertToHDR10PSO.ReleaseAndGetAddressOf())));
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
                m_sdrTexture[i] = std::make_unique<DX::Texture>(device, *(resourceUpload.get()), srv, m_sdrTextureFiles[i], true);

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
    auto device = m_deviceResources->GetD3DDevice();

    // Create HDR backbuffer resources
    auto const outputSize = m_deviceResources->GetOutputSize();
    auto width = size_t(outputSize.right - outputSize.left);
    auto height = size_t(outputSize.bottom - outputSize.top);
    m_sdrScene->SetDevice(device, m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::SDRScene), m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::SDRSceneRTV));
    m_sdrScene->SizeResources(width, height);
}

// Initialize all the fonts used
void Sample::InitializeSpriteFonts(ID3D12Device* d3dDevice, ResourceUploadBatch& resourceUpload, const RenderTargetState& rtState)
{
    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
    m_fontBatch = std::make_unique<SpriteBatch>(d3dDevice, resourceUpload, pd);

    auto index = static_cast<size_t>(ResourceDescriptors::TextFont);
    m_textFont = std::make_unique<SpriteFont>(d3dDevice, resourceUpload, L"Courier_36.spritefont", m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));

    index = static_cast<size_t>(ResourceDescriptors::ControllerFont);
    m_controllerFont = std::make_unique<SpriteFont>(d3dDevice, resourceUpload, L"XboxOneControllerSmall.spritefont", m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));
}
#pragma endregion
