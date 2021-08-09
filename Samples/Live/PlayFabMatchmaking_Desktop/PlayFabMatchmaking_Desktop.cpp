//--------------------------------------------------------------------------------------
// PlayFabMatchmaking_Desktop.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "ATGColors.h"
#include "PlayFabMatchmaking_Desktop.h"
#include "playfab\PlayFabClientApi.h"
#include "playfab\PlayFabMultiplayerApi.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG;
using namespace ATG::UITK;
using namespace PlayFab;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_theTaskQueue(nullptr),
    m_westUsRegionLatency(-1),
    m_eastUsRegionLatency(-1),
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);

    m_matchmakingManager = std::make_unique<PlayFabMatchmakingManager>(*this);
}

Sample::~Sample()
{
    Cleanup();
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

void Sample::Cleanup()
{
    CleanupXboxLive();
    CleanupTaskQueue();
}

void Sample::CleanupTaskQueue()
{
    if (m_theTaskQueue)
    {
        XTaskQueueCloseHandle(m_theTaskQueue);
        m_theTaskQueue = nullptr;
    }
}

void Sample::CleanupXboxLive()
{

}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_liveInfoHUD = std::make_unique<LiveInfoHUD>("PlayFab Matchmaking Desktop");

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    auto layout = m_uiManager.LoadLayoutFromFile("Assets/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());
    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("Console_Window"));
    m_matchLevelText = m_uiManager.FindTypedById<UIStaticText>(ID("Level_Label"));
    m_matchLevelSlider = m_uiManager.FindTypedById<UISlider>(ID("Set_Match_Level_Slider"));
    m_findSimpleMatchButton = m_uiManager.FindTypedById<UIButton>(ID("Find_Simple_Match_Button"));
    m_getRegionLatenciesButton = m_uiManager.FindTypedById<UIButton>(ID("Get_Region_Latencies_Button"));
    m_findRegionMatchButton = m_uiManager.FindTypedById<UIButton>(ID("Find_Region_Match_Button"));
    m_cancelMatchmakingButton = m_uiManager.FindTypedById<UIButton>(ID("Cancel_Matchmaking_Button"));
    m_exitButton = m_uiManager.FindTypedById<UIButton>(ID("Exit_Button"));

    m_matchLevelSlider->SetEnabled(true);
    m_findSimpleMatchButton->SetEnabled(false);
    m_getRegionLatenciesButton->SetEnabled(false);
    m_findRegionMatchButton->SetEnabled(false);
    m_cancelMatchmakingButton->SetEnabled(false);

    m_asyncOpWidget = std::make_unique<AsyncOpWidget>(m_uiManager, "Assets/Layouts/async-status.json");

    InitializeUIEventHandlers();
    InitializeTaskQueue();

    Log("Initialize(0x%08x) complete.", window);

    CheckForNetworkInitialization();
}

void Sample::InitializeUIEventHandlers()
{
    m_matchLevelSlider->CurrentValueState().AddListener(
        [this](UISlider*)
        {
            char displayTextBuffer[32] = {};
            sprintf_s<32>(
                displayTextBuffer,
                "Matched Level: %d",
                static_cast<int>(m_matchLevelSlider->GetCurrentValue()));
            m_matchLevelText->SetDisplayText(displayTextBuffer);
        });

    m_findSimpleMatchButton->ButtonState().AddListenerWhen(
        UIButton::State::Pressed,
        [this](UIButton*)
        {
            DoPlayFabMatchMake(true);
        });

    m_getRegionLatenciesButton->ButtonState().AddListenerWhen(
        UIButton::State::Pressed,
        [this](UIButton*)
        {
            GetLatencyToRegions();
        });

    m_findRegionMatchButton->ButtonState().AddListenerWhen(
        UIButton::State::Pressed,
        [this](UIButton*)
        {
            DoPlayFabMatchMake(false);
        });

    m_cancelMatchmakingButton->ButtonState().AddListenerWhen(
        UIButton::State::Pressed,
        [this](UIButton*)
        {
            CancelMatchmaking();
        });

    m_exitButton->ButtonState().AddListenerWhen(
        UIButton::State::Pressed,
        [](UIButton*)
        {
            ExitSample();
        });
}

void Sample::InitializeTaskQueue()
{
    assert(nullptr == m_theTaskQueue);

    auto hr = XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::Manual,
        &m_theTaskQueue);

    if (FAILED(hr))
    {
        Log("Task queue creation failed with HRESULT = 0x%08x", hr);
    }
    else
    {
        assert(nullptr != m_theTaskQueue);
    }
}

void Sample::InitializeXboxLive()
{
    Log("InitializeXboxLive() started.");
    m_liveResources = std::make_shared<LiveResources>(m_theTaskQueue);
    LoginToXboxLive(false);
}

void Sample::PumpTaskQueue()
{
    while (XTaskQueueDispatch(m_theTaskQueue, XTaskQueuePort::Completion, 0)) {}
    PlayFabClientAPI::Update();
    PlayFabMultiplayerAPI::Update();
    m_matchmakingManager->Update();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    PumpTaskQueue();

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
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);
    m_asyncOpWidget->Update(elapsedTime);
    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        if (pad.IsViewPressed())
        {
            ExitSample();
        }
    }
    m_previousGamePadState = pad;

    auto kb = m_keyboard->GetState();
    if (kb.Escape)
    {
        ExitSample();
    }
    m_previousKeyboardState = kb;

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

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    m_uiManager.Render();

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);
    m_liveInfoHUD->Render(commandList);

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
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
void Sample::OnActivated()
{
}

void Sample::OnDeactivated()
{
}

void Sample::OnSuspending()
{
}

void Sample::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_inputState.Reset();
}

void Sample::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count,
        Descriptors::Reserve);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
    m_liveInfoHUD->Initialize(size.right, size.bottom);
    m_liveInfoHUD->SetWindowSize(static_cast<float>(size.right), static_cast<float>(size.bottom));
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_uiManager.GetStyleManager().ResetStyleRenderer();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
