//--------------------------------------------------------------------------------------
// HDRAutoToneMapping.cpp
//
// This sample shows how an HDR game can use auto tone mapping by the D3D driver to produce
// an SDR image for GameDVR, as opposed to the title rendering the SDR image itself. A title
// can use the default auto tone mapper, or provide its own tone mapper as a 3D lookup table.
//
// HDR games on Xbox One are required to output two swap chains, one with 10-bit HDR10 values
// presented to the HDR TV, and the other with an SDR image which can be used for GameDVR,
// screenshots, streaming and broadcasting. The SDR image can be rendered by the game, or be
// automatically rendered by the D3D driver when auto tone mapping is enabled by using the swap
// chain creation flag D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP. The default
// auto tone mapper produces a good SDR image, but will most likely not have the same look as
// the game's own tone mapped SDR image. The API SetHDRToneMapperX() gives a game the opportunity
// to provide its own tone mapper so that auto tone mapping by the D3D driver can produce an
// image similar to that of the game's own SDR image. It is safe to call the API every frame,
// allowing a game to even generate the 3D LUT at runtime to adjust the tone mapper dynamically
// for different scenes.
//
// For Scarlett, a title has two options for auto tone mapping
// 1) Legacy Xbox One auto tone mapping
//    - Uses a compute shader that runs on the GPU after Present()
//    - Everything is the same as Xbox One, same APIs and use of flag D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP
// 2) New Scarlett specific auto tone mapping
//    - Uses a hardware 3D LUT in the Scarlett display output hardware, i.e. no GPU or bandwidth cost
//    - Do NOT specify the flag D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP, just render the HDR10 swap buffer
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HDRAutoToneMapping.h"

#include "DirectXTex.h"
#include "DirectXTexXbox.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "HDRCommon.h"
#include "ReadData.h"

#include <XDisplay.h>

extern void ExitSample() noexcept;

using namespace DX;
using namespace DirectX;
using namespace SimpleMath;
using namespace Colors;
using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_currentAutoToneMapMethod{},
    m_bIsTVInHDRMode(false),
    m_frame(0),
    m_currentHDRImage(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM,  // Auto tone mapping will automatically create a 10:10:10:2 swap chain
        DXGI_FORMAT_UNKNOWN,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD
        | DX::DeviceResources::c_EnableHDR
        | DX::DeviceResources::c_GeometryShaders);

    m_hdrScene = std::make_unique<DX::RenderTexture>(DXGI_FORMAT_R16G16B16A16_FLOAT);
    m_hdrScene->SetClearColor(Black);
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
#ifdef _GAMING_XBOX_XBOXONE
    m_currentAutoToneMapMethod = AutoToneMapMethod::Default;
#endif

    m_currentHDRImage = 0;

    // We start by not switching the TV to HDR mode, since the goal of the sample is to see the SDR image
    // produced auto tone mapping. The OS will correctly show the SDR image in the GameDVR swapchain when
    // the TV is not in HDR mode
    m_bIsTVInHDRMode = false;

#ifdef _GAMING_XBOX_XBOXONE
    for (int i = 0; i < AutoToneMapMethod::NumAutoToneMapMethods; i++)
    {
        m_d3dToneMapLUT[i] = nullptr;
    }
#endif

    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Render all UI at 1080p so that it's easy to switch between 4K/1080p
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

    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() > 0)
    {
        Render();
    }

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(DirectX::GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

#ifdef _GAMING_XBOX_XBOXONE
        // Cycle through the different auto tone mapper methods
        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentAutoToneMapMethod = (AutoToneMapMethod)((m_currentAutoToneMapMethod + 1) % AutoToneMapMethod::NumAutoToneMapMethods);
        }

        // Save the currently selected tone mapper LUT as a DDS file on the title's scratch drive on the console.
        // i.e. \\<console>\TitleScratch\HDRToneMapperLUT.dds
        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            SaveToneMapLUT(m_currentAutoToneMapMethod);
        }
#endif

        // The main goal of the sample is to see the auto tone mapped SDR image, therefore the sample does not switch
        // the TV to HDR mode at launch time. Only when using this option, the TV will switch to HDR mode, but then
        // you'll see the HDR image, not the SDR image.
        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            if (!m_bIsTVInHDRMode)
            {
                // Determine if attached display is HDR or SDR, if HDR, also set the TV in HDR mode.
                SetDisplayMode();
            }
        }

        // Show previous image
        if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentHDRImage = (m_currentHDRImage + m_NumImages - 1) % m_NumImages;
        }

        // Show next image
        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentHDRImage = (m_currentHDRImage + 1) % m_NumImages;
        }

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

// Convert HDR scene to HDR10 and it to the swap chain buffer
void Sample::ConvertToHDR10()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"ConvertToHDR10");

    // We need to sample from the HDR backbuffer
    m_hdrScene->TransitionTo(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    // Set RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor[1] = { m_deviceResources->GetRenderTargetView() };
    commandList->OMSetRenderTargets(1, rtvDescriptor, FALSE, nullptr);

    // Render
    m_fullScreenQuad->Draw(commandList, m_d3dConvertToHDR10PSO.Get(), m_resourceDescriptorHeap->GetGpuHandle(ResourceDescriptors::HDRScene));

    PIXEndEvent(commandList);
}

// Draws the scene.
void Sample::Render()
{
    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();

    auto commandList = m_deviceResources->GetCommandList();

    // Set the descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    Clear();
    RenderHDRScene();
    RenderUI();

    // Write HDR10 values into the swap chain
    ConvertToHDR10();

#ifdef _GAMING_XBOX_XBOXONE
    // For this tone mapper, we render the LUT at runtime, the others are loaded from disk
    RenderToneMapperLUT(m_ToneMapperToRender);
#endif

    PIXEndEvent(commandList);

#ifdef _GAMING_XBOX_XBOXONE
    // Set the tone mapper used during auto tone mapping. If nullptr, the driver's default auto tone mapper is used
    auto commandQueue = m_deviceResources->GetCommandQueue();
    commandQueue->SetHDRToneMapperX(m_d3dToneMapLUT[m_currentAutoToneMapMethod].Get());
#endif

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Render HDR image
void Sample::RenderHDRScene()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderHDRScene");

    // HDR images are pretty big, so load them async. Show a message when not done loading
    if (m_HDRImage[m_currentHDRImage].HasFinishedLoading())
    {
        m_fullScreenQuad->Draw(commandList, m_d3dRenderHDRImagePSO.Get(), m_HDRImage[m_currentHDRImage].GetShaderResourceView());
    }
    else
    {
        SimpleMath::Vector2 fontPos(300, 300);
        m_fontBatch->Begin(commandList);
        {
            m_textFont->DrawString(m_fontBatch.get(), L"Loading image ...", fontPos, White, 0.0f, g_XMZero, 1.0f);
        }
        m_fontBatch->End();
    }

    PIXEndEvent(commandList);
}

#ifdef _GAMING_XBOX_XBOXONE
// Render tone mapper LUT.
void Sample::RenderToneMapperLUT(AutoToneMapMethod method)
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderToneMapperLUT");

    // Set the viewport and scissor rect.
    const D3D12_VIEWPORT viewport = { 0, 0, m_LUTSize, m_LUTSize, 0, 1 };
    const D3D12_RECT scissorRect = { 0, 0, m_LUTSize, m_LUTSize };
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    // Set RTVs
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = m_rtvDescriptorHeap->GetCpuHandle(static_cast<size_t>(RTVDescriptors::ToneMapLUTRTV + method));
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    // Render the LUT
    DirectX::TransitionResource(commandList, m_d3dToneMapLUT[method].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_render3DTexture->Draw(commandList, m_d3dRender3DTexturePSO.Get());
    DirectX::TransitionResource(commandList, m_d3dToneMapLUT[method].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    PIXEndEvent(commandList);
}
#endif

// Render text with dark shadow which helps when text is on top of bright areas on the image
void Sample::DrawStringWithShadow(const wchar_t* string, DirectX::SimpleMath::Vector2& fontPos, DirectX::FXMVECTOR color, float fontScale)
{
    fontPos.x += 1.0f;
    fontPos.y += 1.0f;
    m_textFont->DrawString(m_fontBatch.get(), string, fontPos, Black, 0.0f, g_XMZero, fontScale);
    fontPos.x -= 1.0f;
    fontPos.y -= 1.0f;
    m_textFont->DrawString(m_fontBatch.get(), string, fontPos, color, 0.0f, g_XMZero, fontScale);
}

// Render the UI
void Sample::RenderUI()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderUI");

    const float startX = 50.0f;
    const float startY = 40.0f;
    const float fontScale = 0.75f;

    SimpleMath::Vector2 fontPos(startX, startY);

    m_fontBatch->Begin(commandList);
    DrawStringWithShadow(L"HDR Auto Tone Mapping Sample", fontPos, ATG::ColorsHDR::White, 1.0f);

    fontPos.y = startY + 100.0f;
    DrawStringWithShadow(m_bIsTVInHDRMode ? L"TV in HDR Mode: TRUE" : L"TV in HDR Mode: FALSE", fontPos, ATG::ColorsHDR::White, fontScale);

#ifdef _GAMING_XBOX_XBOXONE
    wchar_t strText[2048];

    fontPos.y += 40.0f;
    swprintf_s(strText, L"Method: %s", m_AutoToneMapMethodStrings[m_currentAutoToneMapMethod]);
    DrawStringWithShadow(strText, fontPos, ATG::ColorsHDR::White, fontScale);
#endif

    // Render all UI at 1080p so that it's easy to switch between 4K/1080p
    int step = static_cast<int>(((1920.0f - 2 * startX) / 4.0f));
    fontPos.x = startX;
    fontPos.y = 990 - 35;

#ifdef _GAMING_XBOX_SCARLETT
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[X] - Switch TV to HDR mode", fontPos, ATG::ColorsHDR::White, 0.65f); fontPos.y += 35;
#else
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[A] - Auto tone map method", fontPos, ATG::ColorsHDR::White, 0.65f); fontPos.y += 35;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[X] - Switch TV to HDR mode", fontPos, ATG::ColorsHDR::White, 0.65f); fontPos.y += 35;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[Y] - Save current tone mapper (\\\\<console>\\TitleScratch\\HDRToneMapperLUT.dds)", fontPos, ATG::ColorsHDR::White, 0.65f);
#endif

    fontPos.x = startX + step + step + step;
    fontPos.y = 990;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[RB] - Next image", fontPos, ATG::ColorsHDR::White, 0.65f); fontPos.y += 35;
    DX::DrawControllerString(m_fontBatch.get(), m_textFont.get(), m_controllerFont.get(), L"[LB] - Prev image", fontPos, ATG::ColorsHDR::White, 0.65f);

    m_fontBatch->End();

    PIXEndEvent(commandList);
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    m_hdrScene->TransitionTo(commandList, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto const rtv = m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::HDRSceneRTV);
    commandList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);

    commandList->ClearRenderTargetView(m_deviceResources->GetRenderTargetView(), Black, 0, nullptr);
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
    // While a title is constrained, the console TV settings could have changed,
    // so we need to call the display APIs when unconstraining
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

    m_render3DTexture = std::make_unique<Render3DTexture>();
    m_render3DTexture->Initialize(m_deviceResources->GetD3DDevice());

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Create descriptor heaps
    m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RTVDescriptors::NumRTVs);
    m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::NumSRVs);

    // Init fonts
    const RenderTargetState rtState(m_hdrScene->GetFormat(), m_deviceResources->GetDepthBufferFormat());
    InitializeSpriteFonts(device, resourceUpload, rtState);

    // PSO for rendering an HDR texture into the HDR backbuffer
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
        psoDesc.RTVFormats[0] = m_hdrScene->GetFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dRenderHDRImagePSO.ReleaseAndGetAddressOf())));
    }

#ifdef _GAMING_XBOX_XBOXONE
    // PSO for rendering the tone mapper LUT
    {
        auto vertexShaderBlob = DX::ReadData(L"Render3DTextureVS.cso");
        auto geometryShaderBlob = DX::ReadData(L"Render3DTextureGS.cso");
        auto pixelShaderBlob = DX::ReadData(L"RenderToneMapperLUT.cso");
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_render3DTexture->GetRootSignature();
        psoDesc.VS = { vertexShaderBlob.data(), vertexShaderBlob.size() };
        psoDesc.GS = { geometryShaderBlob.data(), geometryShaderBlob.size() };
        psoDesc.PS = { pixelShaderBlob.data(), pixelShaderBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_LUTFormat;
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dRender3DTexturePSO.ReleaseAndGetAddressOf())));
    }
#endif

    // PSO for rendering the HDR10 swapchain buffer
    {
        auto pixelShaderBlob = DX::ReadData(L"ConvertToHDR10PS.cso");
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
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dConvertToHDR10PSO.ReleaseAndGetAddressOf())));
    }

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();     // Wait for resources to upload  

#ifdef _GAMING_XBOX_SCARLETT
#else
    // Load the 3D lookup tables from disk
    LoadToneMapLUT(m_ToneMapperToLoad);
#endif

#ifdef _GAMING_XBOX_SCARLETT
#else
    // Create RTV.
    {
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex3D(m_LUTFormat, m_LUTSize, m_LUTSize, m_LUTSize, 1, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

        D3D12_CLEAR_VALUE clearValue = { m_LUTFormat, {} };
        ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, &clearValue,
                                                        IID_GRAPHICS_PPV_ARGS(m_d3dToneMapLUT[m_ToneMapperToRender].ReleaseAndGetAddressOf())));

        m_d3dToneMapLUT[m_ToneMapperToRender].Get()->SetName(L"ToneMapLUT");

        device->CreateRenderTargetView(m_d3dToneMapLUT[m_ToneMapperToRender].Get(), nullptr, m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::ToneMapLUTRTV + m_ToneMapperToRender));
        device->CreateShaderResourceView(m_d3dToneMapLUT[m_ToneMapperToRender].Get(), nullptr, m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::ToneMapLUT + m_ToneMapperToRender));        
    }
#endif

    // Set views for HDR textures
    for (int i = 0; i < m_NumImages; i++)
    {
        auto cpuHandle = m_resourceDescriptorHeap->GetCpuHandle(static_cast<size_t>(ResourceDescriptors::HDRTexture + i));
        auto gpuHandle = m_resourceDescriptorHeap->GetGpuHandle(static_cast<size_t>(ResourceDescriptors::HDRTexture + i));
        m_HDRImage[i].SetShaderResourceView(cpuHandle, gpuHandle);
    }

    // Load HDR images async
    concurrency::task<bool> t([this]()
    {
        for (int i = 0; i < m_NumImages; i++)
        {
            m_HDRImage[i].Load(m_HDRImageFiles[i], m_deviceResources->GetD3DDevice(), m_deviceResources->GetCommandQueue());
        }

        return true;
    });

    // Setup HDR render target.
    m_hdrScene->SetDevice(device, m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::HDRScene), m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::HDRSceneRTV));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Create HDR backbuffer resources.
    auto const outputSize = m_deviceResources->GetOutputSize();
    size_t width = size_t(outputSize.right - outputSize.left);
    size_t height = size_t(outputSize.bottom - outputSize.top);
    m_hdrScene->SizeResources(width, height);
}

// Initialize all the fonts used
void Sample::InitializeSpriteFonts(ID3D12Device* d3dDevice, ResourceUploadBatch& resourceUpload, const RenderTargetState& rtState)
{
    SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
    m_fontBatch = std::make_unique<SpriteBatch>(d3dDevice, resourceUpload, pd);

    size_t index = static_cast<size_t>(ResourceDescriptors::TextFont);
    m_textFont = std::make_unique<SpriteFont>(d3dDevice, resourceUpload, L"Courier_36.spritefont", m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));

    index = ResourceDescriptors::ControllerFont;
    m_controllerFont = std::make_unique<SpriteFont>(d3dDevice, resourceUpload, L"XboxOneControllerSmall.spritefont", m_resourceDescriptorHeap->GetCpuHandle(index), m_resourceDescriptorHeap->GetGpuHandle(index));
}

#ifdef _GAMING_XBOX_SCARLETT
#else
// Load a tone mapper LUT
void Sample::LoadToneMapLUT(AutoToneMapMethod method)
{
    auto pd3dDevice = m_deviceResources->GetD3DDevice();

    void* grfxMemory = nullptr;
    Xbox::XboxImage xImage;
    TexMetadata metadata;

    if (m_AutoToneMapMethodFileNames[method])
    {
        DX::ThrowIfFailed(Xbox::LoadFromDDSFile(m_AutoToneMapMethodFileNames[method], &metadata, xImage));
        DX::ThrowIfFailed(Xbox::CreateTexture(pd3dDevice, xImage, (ID3D12Resource**)(m_d3dToneMapLUT[method].ReleaseAndGetAddressOf()), &grfxMemory));
    }
}

// Save a tone mapper LUT
void Sample::SaveToneMapLUT(AutoToneMapMethod method)
{
    if (m_d3dToneMapLUT[method] == nullptr)
    {
        return;
    }

    Xbox::XboxImage xboxImage;
    DirectX::ScratchImage image;
    DX::ThrowIfFailed(CaptureTexture(m_deviceResources->GetCommandQueue(), m_d3dToneMapLUT[method].Get(), true, image, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    XG_TEXTURE3D_DESC xgDesc = {};
    D3D12_RESOURCE_DESC desc = m_d3dToneMapLUT[method]->GetDesc();
    xgDesc.Width = static_cast<UINT>(desc.Width);
    xgDesc.Height = static_cast<UINT>(desc.Height);
    xgDesc.Depth = static_cast<UINT>(desc.DepthOrArraySize);
    xgDesc.MipLevels = static_cast<UINT>(desc.MipLevels);
    xgDesc.Format = static_cast<XG_FORMAT>(desc.Format);
    xgDesc.Usage = XG_USAGE_DEFAULT;
    xgDesc.BindFlags = XG_BIND_SHADER_RESOURCE;
#ifdef _GAMING_XBOX_SCARLETT
    xgDesc.SwizzleMode = XGComputeOptimalSwizzleMode(XG_RESOURCE_DIMENSION_TEXTURE3D, XG_FORMAT_R10G10B10A2_UNORM, m_LUTSize, m_LUTSize, m_LUTSize, 1, XG_BIND_SHADER_RESOURCE);
#else
    xgDesc.TileMode = XGComputeOptimalTileMode(XG_RESOURCE_DIMENSION_TEXTURE3D, XG_FORMAT_R10G10B10A2_UNORM, m_LUTSize, m_LUTSize, m_LUTSize, 1, XG_BIND_SHADER_RESOURCE);
#endif
    xgDesc.Pitch = 0;

    XG_RESOURCE_LAYOUT layout;
    XGComputeTexture3DLayout(&xgDesc, &layout);

    TexMetadata metaData = image.GetMetadata();
    xboxImage.Initialize(xgDesc, layout);

    // CaptureTexture has no tiling, so tile now
    Xbox::Tile(image.GetImage(0, 0, 0), m_LUTSize, metaData, xboxImage);

    // Save the texture to the TitleScratch folder
    DX::ThrowIfFailed(Xbox::SaveToDDSFile(xboxImage, L"d:\\HDRToneMapperLUT.dds"));
}
#endif
#pragma endregion

// HDR Helper
void Sample::SetDisplayMode()
{
    // Request HDR mode.
    auto result = XDisplayTryEnableHdrMode(XDisplayHdrModePreference::PreferHdr, nullptr);

    m_bIsTVInHDRMode = (result == XDisplayHdrModeResult::Enabled);

#ifdef _DEBUG
    OutputDebugStringA((m_bIsTVInHDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif
}
