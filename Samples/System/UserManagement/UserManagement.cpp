//--------------------------------------------------------------------------------------
// UserManagement.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UserManagement.h"

#include "ATGColors.h"
#include "ControllerFont.h"

extern void ExitSample();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample(LPWSTR lpCmdLine) noexcept(false)
    : m_frame(0)
    , m_crossRestartTriggered(false)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);

    // Check for cross-restart triggering from command-line
    if (lpCmdLine && wcsstr(lpCmdLine, L"-crossrestart"))
    {
        m_crossRestartTriggered = true;
    }
}

Sample::~Sample()
{
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

    // Setup input management
    m_gameInputCollection = std::make_unique<ATG::GameInputCollection>();

    // Setup user management
    m_gameUserManager = std::make_unique<ATG::GameUserManager>();

    // Setup game screens
    m_gameScreenManager = std::make_unique<ATG::GameScreenManager>(this, &m_uiManager);
    if (m_crossRestartTriggered)
    {
        m_gameScreenManager->HandleCrossRestart();
        m_crossRestartTriggered = false;
    }
}

ATG::GameInputCollection* Sample::GetGameInputCollection() const
{
    return m_gameInputCollection.get();
}

ATG::GameUserManager* Sample::GetGameUserManager() const
{
    return m_gameUserManager.get();
}

ATG::GameScreenManager* Sample::GetGameScreenManager() const
{
    return m_gameScreenManager.get();
}

DX::DeviceResources* Sample::GetDeviceResources() const
{
    return m_deviceResources.get();
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
void Sample::Update(DX::StepTimer const& timer)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    // Update input. The input collection update will update all gamepad tracking and
    // button states.
    m_gameInputCollection->Update();

    // Update user management. Any user events will be fired from the user manager's internal
    // task queue.
    m_gameUserManager->Update();

    // Update game
    m_gameScreenManager->Update(timer.GetElapsedSeconds());

    // UITK
    float elapsedTime = static_cast<float>(timer.GetElapsedSeconds());
    m_uiInputState.Update(elapsedTime, *m_gamePad);
    m_uiManager.Update(elapsedTime, m_uiInputState);
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

    // Render game
    m_gameScreenManager->Render();

    // UITK
    m_uiManager.Render();

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
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
    OutputDebugStringA(u8"Suspending Application\n");

    m_deviceResources->Suspend();

    m_gameInputCollection->OnSuspend();
    m_gameUserManager->OnSuspend();
    m_gameScreenManager->OnSuspend();
}

void Sample::OnResuming()
{
    OutputDebugStringA(u8"Resuming Application\n");

    m_deviceResources->Resume();

    m_gameInputCollection->OnResume();
    m_gameUserManager->OnResume();
    m_gameScreenManager->OnResume();
    m_uiInputState.Reset();

    m_timer.ResetElapsedTime();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    // create the style renderer for the UI manager to use for rendering the UI scene styles
    auto styleRenderer = std::make_unique<ATG::UITK::UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // notify the UI manager of the current window size
    auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}
#pragma endregion
