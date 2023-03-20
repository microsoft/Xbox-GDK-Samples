//--------------------------------------------------------------------------------------
// SmokeSimulation.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SmokeSimulation.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const wchar_t* g_sampleTitle = L"Smoke Simulation";
}

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_theta(0)
    , m_phi(XM_PIDIV4)
    , m_radius(4)
    , m_paused(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
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
void Sample::Update(const DX::StepTimer& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_fluid.Reset();
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_paused = !m_paused;
        }

        m_theta += pad.thumbSticks.rightX * XM_PI * elapsedTime;
        m_phi = std::max(1e-2f, std::min(XM_PIDIV2, m_phi - pad.thumbSticks.rightY * XM_PI * elapsedTime));

        m_fluid.Update(elapsedTime, pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

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

    auto pos = XMVectorSet(
        m_radius * sinf(m_phi) * cosf(m_theta),
        m_radius * cosf(m_phi),
        m_radius * sinf(m_phi) * sinf(m_theta),
        0);
    auto viewProj = XMMatrixMultiply(XMMatrixLookAtLH(pos, g_XMZero, g_XMIdentityR1), m_proj);

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();

    if (!m_paused)
    {
        m_fluid.Step(commandList, 2);
    }

    {
        ScopedPixEvent Render(commandList, PIX_COLOR_DEFAULT, L"Render");

        m_fluid.Render(commandList, viewProj, pos);

        {
            ScopedPixEvent hud(commandList, PIX_COLOR_DEFAULT, L"HUD");

            auto safe = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(m_displayWidth), UINT(m_displayHeight));

            wchar_t textBuffer[1024] = {};
            XMFLOAT2 textPos = XMFLOAT2(float(safe.left), float(safe.top));
            XMVECTOR textColor = ATG::Colors::Green;

            m_hudBatch->Begin(commandList);

            m_smallFont->DrawString(m_hudBatch.get(), g_sampleTitle, textPos, textColor);
            textPos.y += m_smallFont->GetLineSpacing();

            swprintf_s(textBuffer, _countof(textBuffer),
                L"[RThumb] Camera Movement \n\
                [LThumb] Emitter Rotation \n\
                [LT][RT] Emitter Movement \n\
                [B] Reset Simulation \n\
                [X] Toggle Simulation Pause");

            textPos.y = float(safe.bottom - m_smallFont->GetLineSpacing() * 5);
            DX::DrawControllerString(m_hudBatch.get(), m_smallFont.get(), m_ctrlFont.get(), textBuffer, textPos, textColor);

            m_hudBatch->End();
        }
    }

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

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, DirectX::Colors::Black, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
    m_commonStates = std::make_unique<DirectX::CommonStates>(device);
    m_srvPile = std::make_unique<DescriptorPile>(
        device,
        128,
        SRV_Count);

    m_fluid.Initialize(device, m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    auto resourceUpload = ResourceUploadBatch(device);
    resourceUpload.Begin();

    // HUD
    const RenderTargetState backBufferRts(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_hudBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();
    auto const size = m_deviceResources->GetOutputSize();

    // Calculate display dimensions.
    m_displayWidth = size.right - size.left;
    m_displayHeight = size.bottom - size.top;

    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, m_displayWidth / static_cast<float>(m_displayHeight), 0.1f, 1000.0f);

    m_hudBatch->SetViewport(m_deviceResources->GetScreenViewport());

    // Begin uploading texture resources
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1080) ? L"SegoeUI_36.spritefont" : L"SegoeUI_18.spritefont",
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_Font),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            (size.bottom > 1080) ? L"XboxOneControllerLegend.spritefont" : L"XboxOneControllerLegendSmall.spritefont",
            m_srvPile->GetCpuHandle(DescriptorHeapIndex::SRV_CtrlFont),
            m_srvPile->GetGpuHandle(DescriptorHeapIndex::SRV_CtrlFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }
}
#pragma endregion
