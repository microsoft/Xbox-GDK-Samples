//--------------------------------------------------------------------------------------
// HDRPrecision.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "HDRPrecision.h"
#include "DirectXTex.h"
#include "DirectXTexXbox.h"

#include "ATGColors.h"
#include "HDRCommon.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace ATG;
using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* c_sampleTitle = L"HDR Precision";

    enum ShaderResourceViews
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_HdrScene,
        UAV_Ramp,
        UAV_SwapBuffer,
        SRV_Count
    };

    DXGI_FORMAT HDRBackBufferFormat[] = { DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
                                          DXGI_FORMAT_R11G11B10_FLOAT,
                                          DXGI_FORMAT_R32G32B32A32_FLOAT };

    const wchar_t* HDRBackBufferFormatText[] = { L"R9G9B9E5_SHAREDEXP",
                                                 L"R11G11B10_FLOAT",
                                                 L"R32G32B32A32_FLOAT" };

    enum ColorSpace
    {
        ColorSpace_Rec709,
        ColorSpace_P3,
        ColorSpace_Rec2020,
        ColorSpace_Count
    };

    const wchar_t* ColorSpaceText[] = { L"Rec.709",
                                        L"P3-D65",
                                        L"Rec.2020" };

    enum GammaCurve
    {
        GammaCurve_Linear,
        GammaCurve_ST2084,
        GammaCurve_Count
    };

    const wchar_t* GammaCurveText[] = { L"Linear",
                                        L"ST.2084", };

    DXGI_FORMAT SwapBufferFormat[] = { DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
                                       DXGI_FORMAT_R10G10B10A2_UNORM };

    const wchar_t* SwapBufferFormatText[] = { L"R9G9B9E5_SHAREDEXP",
                                              L"R10G10B10A2_UNORM", };

    DXGI_COLOR_SPACE_TYPE SwapBufferFlag[ColorSpace::ColorSpace_Count][GammaCurve::GammaCurve_Count] =
    {
        { DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709,  DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P709 },
        { DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_D65P3, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_D65P3 },
        { DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P2020, DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 }
    };
}

Sample::Sample() noexcept(false)
    : m_displayWidth(0)
    , m_displayHeight(0)
    , m_frame(0)
    , m_bIsDisplayInHDRMode(false)
    , m_bRenderRamp(true)
    , m_currentColorSpace(ColorSpace::ColorSpace_Rec2020)
    , m_currentGammaCurve(GammaCurve::GammaCurve_ST2084)
    , m_currentBackBufferFormat(0)
    , m_currentNitsValuesIndex(7)
    , m_currentHDRImage(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        SwapBufferFormat[m_currentGammaCurve],
        DXGI_FORMAT_UNKNOWN,
        2,
        DX::DeviceResources::c_Enable4K_UHD | DX::DeviceResources::c_EnableHDR);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources(SwapBufferFormat[m_currentGammaCurve]);
    CreateWindowSizeDependentResources();

    CreateFormatDependantResources();

    m_bIsDisplayInHDRMode = RequestHDRMode();

    // Render all UI at 1080p so that it's easy to switch between 4K/1080p
    auto viewportUI = m_deviceResources->GetScreenViewport();
    viewportUI.Width = 1920 * 2;
    viewportUI.Height = 1080 * 2;
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
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED ||
            m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentNitsValuesIndex++;
            m_currentNitsValuesIndex = std::min(m_currentNitsValuesIndex, g_NumNitsValues - 1);
        }

        if (m_gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED ||
            m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentNitsValuesIndex--;
            m_currentNitsValuesIndex = std::max(m_currentNitsValuesIndex, 0);
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentBackBufferFormat++;
            m_currentBackBufferFormat %= ARRAYSIZE(HDRBackBufferFormat);
            m_deviceResources->CreateWindowSizeDependentResources(SwapBufferFormat[m_currentGammaCurve]);
            CreateFormatDependantResources();
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentColorSpace++;
            m_currentColorSpace %= ColorSpace::ColorSpace_Count;
            CreateFormatDependantResources();
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentGammaCurve++;
            m_currentGammaCurve %= GammaCurve::GammaCurve_Count;
            m_deviceResources->CreateWindowSizeDependentResources(SwapBufferFormat[m_currentGammaCurve]);
            CreateFormatDependantResources();
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_bRenderRamp = !m_bRenderRamp;
        }

        if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentHDRImage = (m_currentHDRImage + m_NumImages - 1) % m_NumImages;
        }

        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED)
        {
            m_currentHDRImage = (m_currentHDRImage + 1) % m_NumImages;
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }
}
#pragma endregion

#pragma region Frame Render

void Sample::RenderImage()
{
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderImage");

    ID3D12DescriptorHeap* heaps[] = { m_resourceDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    // HDR images are pretty big, so load them async. Show a message when not done loading
    if (m_HDRImage[m_currentHDRImage].HasFinishedLoading())
    {
        m_fullScreenQuad->Draw(commandList, m_renderHDRImagePSO.Get(), m_HDRImage[m_currentHDRImage].GetShaderResourceView());
    }
    else
    {
        SimpleMath::Vector2 fontPos(300, 300);
        m_spriteBatch->Begin(commandList);
        {
            m_smallFont->DrawString(m_spriteBatch.get(), L"Loading image ...", fontPos, ATG::Colors::White, 0.0f, g_XMZero, 1.0f);
        }
        m_spriteBatch->End();
    }

    PIXEndEvent(commandList);
}

void Sample::RenderRamp()
{
    auto commandList = m_deviceResources->GetCommandList();
    m_deviceResources->Prepare(D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderRamp");

    commandList->SetComputeRootSignature(m_d3dRampRS.Get());
    commandList->SetComputeRootDescriptorTable(0, m_srvPile->GetGpuHandle(UAV_Ramp));
    commandList->SetPipelineState(m_d3dRampPSO.Get());
    commandList->Dispatch(3840 / 8, 2160 / 8, 1);

    TransitionResource(commandList, m_renderTexture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

    PIXEndEvent(commandList);
}

void Sample::Render()
{
    auto commandList = m_deviceResources->GetCommandList();

    if (m_bRenderRamp)
    {
        RenderRamp();
    }
    else
    {
        RenderImage();
    }

    RenderUI(commandList);

    TransitionResource(commandList, m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    TransitionResource(commandList, m_renderTexture.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    PrepareSwapBuffer(commandList);

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present(SwapBufferFlag[m_currentColorSpace][m_currentGammaCurve], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    TransitionResource(commandList, m_renderTexture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RENDER_TARGET);

    auto const rtvDescriptor = m_rtvPile->GetFirstCpuHandle();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}

void Sample::RenderUI(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"RenderUI");

    auto const rtvDescriptor = m_rtvPile->GetFirstCpuHandle();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    const float lineSpacing = m_smallFont->GetLineSpacing();
    auto const safe = SimpleMath::Viewport::ComputeTitleSafeArea(m_displayWidth, m_displayHeight);
    XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top - lineSpacing));
    float hdrTextColor = 1000000.0f / g_NitsValues[m_currentNitsValuesIndex];
    XMVECTORF32 Bright = { { { hdrTextColor, hdrTextColor, hdrTextColor, 1.0f } } };
    XMVECTOR textColor = m_bRenderRamp ? Bright : ColorsHDR::White;
    auto const hdrButtonBrightnessScale = m_bRenderRamp ? hdrTextColor : 3.0f;

    wchar_t textBuffer[128] = {};

    m_spriteBatch->Begin(commandList);

    m_smallFont->DrawString(m_spriteBatch.get(), c_sampleTitle, textPos, textColor);
    textPos.y += lineSpacing * 2.0f;

    swprintf_s(textBuffer, L"TV in HDR mode: %s", m_bIsDisplayInHDRMode ? L"True" : L"False");
    m_smallFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
    textPos.y += lineSpacing;

    if (!m_bIsDisplayInHDRMode)
    {
        textPos.y += lineSpacing;
        swprintf_s(textBuffer, L"This sample requires an HDR display!");
        m_smallFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
        textPos.y += lineSpacing;
    }
    else
    {
        textPos.y += lineSpacing;
        swprintf_s(textBuffer, L"Brigtness Range: %d nits", g_NitsValues[m_currentNitsValuesIndex]);
        m_smallFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
        textPos.y += lineSpacing;

        swprintf_s(textBuffer, L"HDR Back Buffer Format: %s", HDRBackBufferFormatText[m_currentBackBufferFormat]);
        m_smallFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
        textPos.y += lineSpacing;

        swprintf_s(textBuffer, L"Swap Buffer Format: %s", SwapBufferFormatText[m_currentGammaCurve]);
        m_smallFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
        textPos.y += lineSpacing;

        swprintf_s(textBuffer, L"Color Space: %s", ColorSpaceText[m_currentColorSpace]);
        m_smallFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
        textPos.y += lineSpacing;

        swprintf_s(textBuffer, L"Gamma Curve: %s", GammaCurveText[m_currentGammaCurve]);
        m_smallFont->DrawString(m_spriteBatch.get(), textBuffer, textPos, textColor);
        textPos.y += lineSpacing;
    }

    // Draw the controls
    textPos = XMFLOAT2(float(safe.left), float(safe.bottom - lineSpacing));

    const wchar_t* controlString = m_bIsDisplayInHDRMode ?  (m_bRenderRamp ?
            L"[A] Render target format    [X] Swap buffer color space    [Y] Swap buffer gamma curve    [B] Toggle ramp/image     [DPad] Adjust brightness range     [View] Exit Sample" :
            L"[A] Render target format    [X] Swap buffer color space    [Y] Swap buffer gamma curve    [B] Toggle ramp/image     [RB] Next image     [View] Exit Sample") :
            L"[View] - Exit Sample";

    DX::DrawControllerString(m_spriteBatch.get(), m_smallFont.get(), m_ctrlFont.get(), controlString, textPos, textColor, 1.0f, hdrButtonBrightnessScale);

    m_spriteBatch->End();

    PIXEndEvent(commandList);
}

void Sample::PrepareSwapBuffer(ID3D12GraphicsCommandList* commandList)
{
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"PrepareSwapBuffer");

    int renderRamp = static_cast<int>(m_bRenderRamp);
    float brightnessScale = m_bRenderRamp ? g_NitsValues[m_currentNitsValuesIndex] * 100.0f / 10000.0f : 1.0f;

    commandList->SetComputeRootSignature(m_prepareSwapBufferRS.Get());
    commandList->SetPipelineState(m_prepareSwapBufferPSO.Get());
    commandList->SetComputeRoot32BitConstants(0, 4, &renderRamp, 0);
    commandList->SetComputeRoot32BitConstants(0, 4, &m_currentColorSpace, 1);
    commandList->SetComputeRoot32BitConstants(0, 4, &m_currentGammaCurve, 2);
    commandList->SetComputeRoot32BitConstants(0, 4, &brightnessScale, 3);
    commandList->SetComputeRootDescriptorTable(1, m_srvPile->GetGpuHandle(UAV_Ramp));
    commandList->SetComputeRootDescriptorTable(2, m_deviceResources->GetSwapBufferUAV());
    commandList->Dispatch(3840 / 8, 2160 / 8, 1);

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

    m_fullScreenQuad = std::make_unique<DX::FullScreenQuad>();
    m_fullScreenQuad->Initialize(m_deviceResources->GetD3DDevice());

    m_srvPile = std::make_unique<DescriptorPile>(device, 128, SRV_Count);
    m_rtvPile = std::make_unique<DescriptorPile>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 2, 1);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_rtvDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, RTVDescriptors::NumRTVs);
        m_resourceDescriptorHeap = std::make_unique<DescriptorHeap>(device, ResourceDescriptors::NumSRVs);

        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload, L"SegoeUI_36.spritefont", m_srvPile->GetCpuHandle(SRV_Font),
        m_srvPile->GetGpuHandle(SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload, L"XboxOneControllerLegend.spritefont", m_srvPile->GetCpuHandle(SRV_CtrlFont),
        m_srvPile->GetGpuHandle(SRV_CtrlFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }
}

void Sample::CreateFormatDependantResources()
{
    static auto computeShaderBlob = DX::ReadData(L"PrepareSwapBufferCS.cso");
    static auto rampShaderBlob = DX::ReadData(L"RampCS.cso");
    static auto fullScreenPSBlob = DX::ReadData(L"FullScreenQuadPS.cso");
    static auto fullScreenVSBlob = DX::ReadData(L"FullScreenQuadVS.cso");

    auto device = m_deviceResources->GetD3DDevice();

    // PSO for rendering an HDR texture into the HDR backbuffer
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
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
        psoDesc.RTVFormats[0] = HDRBackBufferFormat[m_currentBackBufferFormat];
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_renderHDRImagePSO.ReleaseAndGetAddressOf())));
    }

    // Create PSO for rendering into the swap buffer
    {
        // Xbox One best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(), IID_GRAPHICS_PPV_ARGS(m_prepareSwapBufferRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_prepareSwapBufferRS.Get();
        psoDesc.CS.pShaderBytecode = computeShaderBlob.data();
        psoDesc.CS.BytecodeLength = computeShaderBlob.size();
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_prepareSwapBufferPSO.ReleaseAndGetAddressOf())));
    }

    // Create PSO for rendering the ramp
    {
        // Xbox One best practice is to use HLSL-based root signatures to support shader precompilation.
        DX::ThrowIfFailed(device->CreateRootSignature(0, rampShaderBlob.data(), rampShaderBlob.size(), IID_GRAPHICS_PPV_ARGS(m_d3dRampRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_d3dRampRS.Get();
        psoDesc.CS.pShaderBytecode = rampShaderBlob.data();
        psoDesc.CS.BytecodeLength = rampShaderBlob.size();
        DX::ThrowIfFailed(device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_d3dRampPSO.ReleaseAndGetAddressOf())));
    }

    // Wait until assets have been uploaded to the GPU.
    m_deviceResources->WaitForGpu();

    // Create the HUD sprite batch
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();
        auto backBufferRts = RenderTargetState(HDRBackBufferFormat[m_currentBackBufferFormat], m_deviceResources->GetDepthBufferFormat());
        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
        m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);
        resourceUpload.End(m_deviceResources->GetCommandQueue());
    }

    auto const size = m_deviceResources->GetOutputSize();
    m_displayWidth = static_cast<uint32_t>(size.right - size.left);
    m_displayHeight = static_cast<uint32_t>(size.bottom - size.top);

    m_spriteBatch->SetViewport(m_deviceResources->GetScreenViewport());

    // Create the HDR render target
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(HDRBackBufferFormat[m_currentBackBufferFormat], m_displayWidth, m_displayHeight, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE clearValue = CD3DX12_CLEAR_VALUE(HDRBackBufferFormat[m_currentBackBufferFormat], DirectX::Colors::Black);
    DX::ThrowIfFailed(device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &desc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &clearValue, IID_GRAPHICS_PPV_ARGS(m_renderTexture.ReleaseAndGetAddressOf())));
    m_renderTexture->SetName(L"RenderTexture");

    device->CreateRenderTargetView(m_renderTexture.Get(), nullptr, m_rtvPile->GetFirstCpuHandle());
    device->CreateShaderResourceView(m_renderTexture.Get(), nullptr, m_srvPile->GetCpuHandle(SRV_HdrScene));
    device->CreateUnorderedAccessView(m_renderTexture.Get(), nullptr, nullptr, m_srvPile->GetCpuHandle(UAV_Ramp));

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
            if (!m_HDRImage[i].HasFinishedLoading())
            {
                m_HDRImage[i].Load(m_HDRImageFiles[i], m_deviceResources->GetD3DDevice(), m_deviceResources->GetCommandQueue());
            }
        }

        return true;
    });

    // Setup HDR render target.
    m_hdrScene.release();
    m_hdrScene = std::make_unique<DX::RenderTexture>(HDRBackBufferFormat[m_currentBackBufferFormat]);
    m_hdrScene->SetClearColor(ATG::Colors::Background);
    m_hdrScene->SetDevice(device, m_resourceDescriptorHeap->GetCpuHandle(ResourceDescriptors::HDRScene), m_rtvDescriptorHeap->GetCpuHandle(RTVDescriptors::HDRSceneRTV));
}

#pragma endregion
