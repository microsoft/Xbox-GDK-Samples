//--------------------------------------------------------------------------------------
// FrontPanelGame.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FrontPanelGame.h"

#include "ATGColors.h"

#include "FrontPanel\CPUShapes.h"

extern void ExitSample();

using namespace DirectX;
using namespace ATG;
using ButtonState = FrontPanelInput::ButtonStateTracker::ButtonState;
using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_score(0)
    , m_alive(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);

    m_available = XFrontPanelIsSupported();
    if (m_available)
    {
        // Initialize the FrontPanelDisplay object
        m_frontPanelDisplay = std::make_unique<FrontPanelDisplay>();

        // Initialize the FrontPanelInput object
        m_frontPanelInput = std::make_unique<FrontPanelInput>();
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

    // Using fixed frame rate with 60fps target frame rate
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60.0);

    // Don't do anything if there is no front panel
    if (m_available)
    {
        m_font = RasterFont(L"Assets\\LucidaConsole12.rasterfont");

        InitializeGame(false);
    }
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
void Sample::Update(DX::StepTimer const& /*timer*/)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    // Don't do anything if there is no front panel
    if (m_available)
    {
        UpdateGame();

        RenderToFrontPanel();
    }

    auto pad = m_gamePad->GetState(0);
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

    auto output = m_deviceResources->GetOutputSize();

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_batch->Begin(commandList);
    m_batch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), XMUINT2(1920, 1080), output);
    m_batch->End();

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

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);

    // No clears needed as sample only draws a sprite background

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
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

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        SpriteBatchPipelineStateDescription pd(
            rtState,
            &CommonStates::AlphaBlend);

        m_batch = std::make_unique<SpriteBatch>(device, upload, pd);
    }

    DX::ThrowIfFailed(
        CreateWICTextureFromFile(device, upload,
            m_available ? L"FrontPanelPresent.png" : L"NoFrontPanel.png",
            m_background.ReleaseAndGetAddressOf())
    );

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);
}
#pragma endregion

void Sample::InitializeGame(bool alive)
{
    m_alive = alive;

    m_score = 0;

    m_gameBoard = std::make_shared<GameBoard>(*m_frontPanelDisplay.get());

    if (m_alive)
    {
        m_snake = std::make_shared<Snake>(
            m_gameBoard,
            0, 1,
            5, 5);

        m_gameBoard->SpawnFood();
    }
}

void Sample::RenderToFrontPanel() const
{
    m_frontPanelDisplay->Clear();

    BufferDesc fpDesc = m_frontPanelDisplay->GetBufferDescriptor();
    m_font.DrawStringFmt(fpDesc, 0, 0, L"\nSnake\n\nScore: %u", m_score);

    int ts = static_cast<int>(m_timer.GetTotalSeconds());

    CPUShapes shapes = CPUShapes(m_frontPanelDisplay->GetDisplayWidth(), m_frontPanelDisplay->GetDisplayHeight(), m_frontPanelDisplay->GetBuffer());
    shapes.RenderRect(0 + ts % 11, 0, 2, 2);

    m_gameBoard->Render();

    m_frontPanelDisplay->Present();
}

void Sample::UpdateGame()
{
    RespondToInput();

    // Each frame is 1/60th of a second
    static unsigned frames = 0;

    // advancing the simulation every 8 frames seems to the right amount
    // given the size of the dpad control etc.
    if (m_alive && (frames > 8)) 
    {
        switch (m_snake->Move())
        {
        case SnakeMoveResult::Move:
            break;

        case SnakeMoveResult::Eat:
            m_gameBoard->SpawnFood();
            m_score++;
            break;

        case SnakeMoveResult::Fail:
            m_alive = false;
            break;
        }

        frames = 0;
    }
    else if (!m_alive && (frames > 20))
    {
        // Blink the light around the start button
        auto state = m_frontPanelInput->GetState();
        XFrontPanelLight lights = state.lights.rawLights;

        if (state.lights.light1)
        {
            lights &= ~XFrontPanelLight::Light1;
        }
        else
        {
            lights |= XFrontPanelLight::Light1;
        }
        m_frontPanelInput->SetLightStates(lights);

        frames = 0;
    }

    ++frames;
}

void Sample::RespondToInput()
{
    auto fpInput = m_frontPanelInput->GetState();
    m_frontPanelInputButtons.Update(fpInput);

    if(m_snake && m_frontPanelInputButtons.dpadUp == ButtonState::PRESSED)
    {
        m_snake->SetDirection(0, -1);
    }
    else if (m_snake && m_frontPanelInputButtons.dpadDown == ButtonState::PRESSED)
    {
        m_snake->SetDirection(0, 1);
    }
    else if(m_snake && m_frontPanelInputButtons.dpadLeft == ButtonState::PRESSED)
    {
        m_snake->SetDirection(-1, 0);
    }
    else if(m_snake && m_frontPanelInputButtons.dpadRight == ButtonState::PRESSED)
    {
        m_snake->SetDirection(1, 0);
    }
    else if(m_frontPanelInputButtons.button1 == ButtonState::PRESSED)
    {
        InitializeGame(true);
    }

    // Use the select button to take a screen capture
    if (m_frontPanelInputButtons.buttonSelect == ButtonState::PRESSED)
    {
        m_frontPanelDisplay->SaveDDSToFile(L"D:\\FrontPanelDisplay.dds");
    }

    if (m_frontPanelInputButtons.buttonsChanged)
    {
        XFrontPanelLight lights = XFrontPanelLight::None;

        if(m_frontPanelInputButtons.button1 & ButtonState::HELD)
        {
            lights |= XFrontPanelLight::Light1;
        }
        if (m_frontPanelInputButtons.button2 & ButtonState::HELD)
        {
            lights |= XFrontPanelLight::Light2;
        }
        if (m_frontPanelInputButtons.button3 & ButtonState::HELD)
        {
            lights |= XFrontPanelLight::Light3;
        }
        if (m_frontPanelInputButtons.button4 & ButtonState::HELD)
        {
            lights |= XFrontPanelLight::Light4;
        }
        if (m_frontPanelInputButtons.button5 & ButtonState::HELD)
        {
            lights |= XFrontPanelLight::Light5;
        }

        m_frontPanelInput->SetLightStates(lights);
    }
}
