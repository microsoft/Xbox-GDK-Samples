//--------------------------------------------------------------------------------------
// SimpleHDR.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleHDR.h"

extern void ExitSample() noexcept;

using namespace ATG;
using namespace DirectX;
using namespace SimpleMath;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* c_sampleTitle = L"Simple HDR";

    // Our triangle will render at max 1000 nits. The shader defines paper white nits as 200.0f. This means that the
    // brightness of white (1,1,1) is 200 nits. Therefore an HDR scene value of 5.0 will result in 1000 nits
    constexpr float g_hdrSceneValue = 5.0f;

    struct Vertex
    {
        XMFLOAT4 position;
        XMFLOAT4 color;
    };

#ifdef _GAMING_XBOX_SCARLETT
    // On Scarlett, it is recommended to use 999e5 as an HDR render target format, since it gives better precision that 11:11:10
    constexpr DXGI_FORMAT g_hdrBackBufferFormat = DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
#else
    constexpr DXGI_FORMAT g_hdrBackBufferFormat = DXGI_FORMAT_R11G11B10_FLOAT;
#endif
}

Sample::Sample() noexcept(false) :
    m_frame(0)
    , m_bIsDisplayInHDRMode(false)
{
#ifdef _GAMING_XBOX_SCARLETT
    // On Scarlett, it is recommended to use 999e5 as a swap buffer format, since it gives better precision than 10:10:10:2,
    // and some processing can be moved from the GPU to the display hardware
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
#else
    // On Xbox One, 10:10:10:2 is required for HDR
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R10G10B10A2_UNORM,
#endif
            DXGI_FORMAT_UNKNOWN, 2, DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableQHD | DX::DeviceResources::c_EnableHDR);
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

    // Try to switch the display into HDR mode
    TryEnableHDR();

    // Render all UI at 1080p so that it's easy to switch between 1080p/1440p/4K
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920;
    viewportUI.Height = 1080;
    m_spriteBatch->SetViewport(viewportUI);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

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

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        if (pad.IsViewPressed())
        {
            ExitSample();
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

    // Draw simple triangle
    m_effect->Apply(commandList);
    m_batch->Begin(commandList);
    VertexPositionColor v1(Vector3(0.f, 0.5f, 0.5f), Vector4(g_hdrSceneValue, 0.0f, 0.0f, 1.0f));
    VertexPositionColor v2(Vector3(0.5f, -0.5f, 0.5f), Vector4(0.0f, g_hdrSceneValue, 0.0f, 1.0f));
    VertexPositionColor v3(Vector3(-0.5f, -0.5f, 0.5f), Vector4(0.0f, 0.0f, g_hdrSceneValue, 1.0f));
    m_batch->DrawTriangle(v1, v2, v3);
    m_batch->End();

    // Render UI
    RenderUI(commandList);

    // Process the HDR scene so that it can be presented correct to the TV
    TransitionResource(commandList, m_hdrScene.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    TransitionResource(commandList, m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    FinalHDRShader(commandList);

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

    TransitionResource(commandList, m_hdrScene.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto rtvDescriptor = m_rtvPile->GetCpuHandle(HDRSceneRTV);
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderUI");

    auto rtvDescriptor = m_rtvPile->GetCpuHandle(HDRSceneRTV);
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    wchar_t textBuffer[128] = {};
    const float lineSpacing = m_smallFont->GetLineSpacing();
    XMFLOAT2 textPos = XMFLOAT2(50.0f, lineSpacing);

    m_spriteBatch->Begin(commandList);

    m_smallFont->DrawString(m_spriteBatch.get(), c_sampleTitle, textPos, ColorsHDR::White, 0.0f, DirectX::XMFLOAT2(0.0f, 0.0f), 1.5f);

    textPos.y += lineSpacing * 3.0f;
    swprintf_s(textBuffer, L"TV in HDR mode: %s", m_bIsDisplayInHDRMode ? L"True" : L"False");
    m_smallFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, ColorsHDR::White);

    if (!m_bIsDisplayInHDRMode)
    {
        textPos.y += lineSpacing * 2.0f;
        swprintf_s(textBuffer, L"This sample requires an HDR display!");
        m_smallFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, ColorsHDR::Orange);
    }

    textPos.y = 1080.0f - lineSpacing * 2.0f;
    const wchar_t* controlString = L"[View] - Exit Sample";
    DX::DrawControllerString(m_spriteBatch.get(), m_smallFont.get(), m_ctrlFont.get(), controlString, textPos, ColorsHDR::White, 1.0f, 1.0f);

    m_spriteBatch->End();

    PIXEndEvent(commandList);
}

void Sample::FinalHDRShader(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"FinalHDRShader");

    commandList->SetComputeRootSignature(m_finalHDRShaderRS.Get());
    commandList->SetPipelineState(m_finalHDRShaderPSO.Get());
    commandList->SetComputeRootDescriptorTable(0, m_srvPile->GetGpuHandle(HDRSceneSRV));
    commandList->SetComputeRootDescriptorTable(1, m_deviceResources->GetSwapBufferUAV());
    commandList->Dispatch(m_width / 8, m_height / 8, 1);

    PIXEndEvent(commandList);
}

void Sample::TryEnableHDR()
{
    if ((m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 )
    {
        // Request HDR mode.
        auto result = XDisplayTryEnableHdrMode(XDisplayHdrModePreference::PreferHdr, nullptr);

        m_bIsDisplayInHDRMode = (result == XDisplayHdrModeResult::Enabled);

#ifdef _DEBUG
        OutputDebugStringA((m_bIsDisplayInHDRMode) ? "INFO: Display in HDR Mode\n" : "INFO: Display in SDR Mode\n");
#endif
    }
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
    TryEnableHDR();

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
    TryEnableHDR();
}

#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // Use a primitive batch to draw a simple triangle
    {
        RenderTargetState rtState(g_hdrBackBufferFormat, m_deviceResources->GetDepthBufferFormat());

        m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);
        EffectPipelineStateDescription pd(&VertexPositionColor::InputLayout, CommonStates::Opaque,
            CommonStates::DepthDefault, CommonStates::CullNone, rtState);

        m_effect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
    }


    m_rtvPile = std::make_unique<DescriptorPile>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 2, NumRTVs);
    m_srvPile = std::make_unique<DescriptorPile>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 128, NumSRVs);

    static auto computeShaderBlob = DX::ReadData(L"FinalHDRShaderCS.cso");

    // PSO for rendering into the swap buffer
    {
        // Xbox best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                                                        IID_GRAPHICS_PPV_ARGS(m_finalHDRShaderRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_finalHDRShaderRS.Get();
        psoDesc.CS.pShaderBytecode = computeShaderBlob.data();
        psoDesc.CS.BytecodeLength = computeShaderBlob.size();
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_finalHDRShaderPSO.ReleaseAndGetAddressOf())));
    }

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    RECT size = m_deviceResources->GetOutputSize();
    m_width = static_cast<uint32_t>(size.right - size.left);
    m_height = static_cast<uint32_t>(size.bottom - size.top);

    // Load fonts
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();
        m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, NumRTVs);
        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload, L"SegoeUI_18.spritefont", m_srvPile->GetCpuHandle(FontSRV), m_srvPile->GetGpuHandle(FontSRV));
        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload, L"XboxOneControllerSmall.spritefont", m_srvPile->GetCpuHandle(CtrlFontSRV), m_srvPile->GetGpuHandle(CtrlFontSRV));
        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }
    
    // Create the HUD sprite batch
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();
        auto backBufferRts = RenderTargetState(g_hdrBackBufferFormat, m_deviceResources->GetDepthBufferFormat());
        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
        m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);
        resourceUpload.End(m_deviceResources->GetCommandQueue());
    }

    // Create the HDR scene render target
    {
        auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(g_hdrBackBufferFormat, m_width, m_height, 1, 1, 1, 0,
                                                                    D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

        // Use Delta Color Compression (DCC) and Texture Compatibility on Scarlett to leverage hardware resource decompression, otherwise
        // there's an expensive Fast Clear Eliminate before sampling the HDR scene buffer in FinalHDRShader
        if (XSystemGetDeviceType() >= XSystemDeviceType::XboxScarlettLockhart)
        {
            desc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_DCC | D3D12XBOX_RESOURCE_FLAG_FORCE_TEXTURE_COMPATIBILITY;
        }

        D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(g_hdrBackBufferFormat, DirectX::Colors::Black);
        DX::ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &clearValue, IID_GRAPHICS_PPV_ARGS(m_hdrScene.ReleaseAndGetAddressOf())));
        m_hdrScene->SetName(L"HDR Scene");

        device->CreateRenderTargetView(m_hdrScene.Get(), nullptr, m_rtvPile->GetCpuHandle(HDRSceneRTV));
        device->CreateShaderResourceView(m_hdrScene.Get(), nullptr, m_srvPile->GetCpuHandle(HDRSceneSRV));
    }

    m_spriteBatch->SetViewport(m_deviceResources->GetScreenViewport());
}
#pragma endregion
