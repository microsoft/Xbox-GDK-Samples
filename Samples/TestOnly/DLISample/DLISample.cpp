//--------------------------------------------------------------------------------------
// DLISample.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DLISample.h"
#include "GameInput.h"
#include "ATGColors.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

constexpr size_t g_MaxCommandsCount = 35;

Sample::Sample() noexcept(false) :
    m_queryRate(16.667f),
    m_queryVariance(0.f),
    m_updateRate(false),
    m_holdLS(false),
    m_holdRS(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);

    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(m_queryRate / 1000);
    srand(static_cast <unsigned> (time(0)));
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
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_renderTimer.Tick([&]()
    {
        m_deviceResources->WaitForOrigin();
        Render();
    });
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    m_timeList.emplace_back(timer.GetElapsedSeconds() * 1000);

    if (m_queryVariance > .005f)
    {
        m_updateRate = true;
    }
    else
    {
        m_queryVariance = 0.f;
        m_updateRate = false;
    }

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    if (pad.IsDPadUpPressed())
    {
        m_queryRate += .1f;
        m_updateRate = true;
    }

    if (pad.IsDPadDownPressed() && m_queryRate > 2.f)
    {
        m_queryRate -= .1f;
        m_updateRate = true;
    }

    if (pad.IsDPadRightPressed())
    {
        m_queryVariance += .01f;
        m_updateRate = true;
    }

    if (pad.IsDPadLeftPressed() && m_queryVariance > 0.f)
    {
        m_queryVariance -= .01f;
        m_updateRate = true;
    }

    if (pad.IsYPressed())
    {
        m_queryVariance = 0.f;
        m_queryRate = 33.333f;
        m_updateRate = true;
    }

    if (pad.IsXPressed())
    {
        m_queryVariance = 0.f;
        m_queryRate = 16.667f;
        m_updateRate = true;
    }

    if (pad.IsBPressed())
    {
        m_queryVariance = 0.f;
        m_queryRate = 8.333f;
        m_updateRate = true;
    }

    if (pad.IsAPressed())
    {
        m_queryVariance = 0.f;
        m_queryRate = 4.166f;
        m_updateRate = true;
    }

    if (pad.IsLeftStickPressed())
    {
        if (!m_holdLS)
        {
            m_holdLS = true;
            m_queryRate *= 2;
            m_updateRate = true;
        }
    }
    else if (m_holdLS)
    {
        m_holdLS = false;
        m_queryRate /= 2;
        m_updateRate = true;
    }

    if (pad.IsRightStickPressed())
    {
        if (!m_holdRS)
        {
            m_holdRS = true;
            m_queryRate /= 2;
            m_updateRate = true;
        }
    }
    else if (m_holdRS)
    {
        m_holdRS = false;
        m_queryRate *= 2;
        m_updateRate = true;
    }

    if (m_updateRate)
    {
        double newRate = (m_queryRate - m_queryVariance / 2) + (static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / m_queryVariance)));
        m_timer.SetTargetElapsedSeconds(newRate / 1000);
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_renderTimer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();

    auto const fullscreen = m_deviceResources->GetOutputSize();

    auto const safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(UINT(fullscreen.right - fullscreen.left), UINT(fullscreen.bottom - fullscreen.top));

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));
    wchar_t tempString[256] = {};

    m_spriteBatch->Begin(commandList);

    //Rates
    swprintf(tempString, 255, L"Current Rate: %3.3f", m_queryRate);
    m_font->DrawString(m_spriteBatch.get(), tempString, pos, ATG::Colors::White);
    pos.y += m_font->GetLineSpacing();

    swprintf(tempString, 255, L"Variance: %3.3f", m_queryVariance);
    m_font->DrawString(m_spriteBatch.get(), tempString, pos, ATG::Colors::White);
    pos.y += m_font->GetLineSpacing();

    while (m_timeList.size() > g_MaxCommandsCount)
    {
        m_timeList.pop_front();
    }

    pos.x = safeRect.right * .9f;
    pos.y = float(safeRect.top);

    auto it = m_timeList.begin();

    while (it != m_timeList.end())
    {
        swprintf(tempString, 255, L"%2.3f", *it);
        m_font->DrawString(m_spriteBatch.get(), tempString, pos, ATG::Colors::White);
        pos.y += m_font->GetLineSpacing() * .8f;
        ++it;
    }

    m_spriteBatch->End();

    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();

    // Clear the views.
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);
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
    m_renderTimer.ResetElapsedTime();
    m_gamePadButtons.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_states = std::make_unique<DirectX::CommonStates>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, Descriptors::Count);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_spriteBatch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    m_font = std::make_unique<SpriteFont>(device, upload,
        L"SegoeUI_18.spritefont",
        m_resourceDescriptors->GetCpuHandle(Descriptors::TextFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::TextFont));

    DX::ThrowIfFailed(CreateDDSTextureFromFile(device, upload, L"ATGSampleBackground.DDS", m_background.ReleaseAndGetAddressOf()));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const vp = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(vp);
}
#pragma endregion
