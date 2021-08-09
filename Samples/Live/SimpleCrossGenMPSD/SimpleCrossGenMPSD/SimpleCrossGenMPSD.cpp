//--------------------------------------------------------------------------------------
// SimpleCrossGenMPSD.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleCrossGenMPSD.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("SimpleCrossGenMPSD");
}

Sample::~Sample()
{
    m_sessionManager->CleanUp();

    CleanupTaskQueue();

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

void Sample::CleanupTaskQueue()
{
    if (m_taskQueue)
    {
        XTaskQueueCloseHandle(m_taskQueue);
        m_taskQueue = nullptr;
    }
}

void Sample::InitializeTaskQueue()
{
    assert(nullptr == m_taskQueue);

    auto hr = XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_taskQueue);
    if (FAILED(hr))
    {
        Log("Sample::InitializeTaskQueue: Task queue creation failed with HRESULT = 0x%08x", hr);
    }
    else
    {
        assert(nullptr != m_taskQueue);
    }
}

void Sample::InitializeLiveResources()
{
    m_liveResources = std::make_shared<ATG::LiveResources>(m_taskQueue);
    m_liveResources->SetErrorHandler([this](HRESULT error)
    {
        if (error == E_GAMEUSER_NO_DEFAULT_USER || error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
        {
            m_liveResources->SignInWithUI();
        }
        else // Handle other error cases
        {
            Log("LiveResources: Error HRESULT: %08x", error);
        }
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());

        if (m_sessionManager)
        {
            m_sessionManager.reset();
        }
    });

    m_liveResources->Initialize();
}

void Sample::PumpTaskQueue()
{
    while (XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 0)) {}
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
#ifdef _DEBUG
    DebugInit();
#endif

    Log("Initialize");

    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
#ifdef _GAMING_DESKTOP
    m_mouse->SetWindow(window);
#endif

    m_deviceResources->SetWindow(window, width, height);
    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();
    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_liveInfoHUD->Initialize();

    auto layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("ConsoleWindow"));
    m_createSessionButton = m_uiManager.FindTypedById<UIButton>(ID("CreateSessionButton"));
    m_startMatchmakingButton = m_uiManager.FindTypedById<UIButton>(ID("StartMatchmakingButton"));
    m_createCrossGenSessionButton = m_uiManager.FindTypedById<UIButton>(ID("CreateCrossGenSessionButton"));
    m_startMatchmakingCrossGenButton = m_uiManager.FindTypedById<UIButton>(ID("StartMatchmakingCrossGenButton"));
    m_cancelMatchmakingButton = m_uiManager.FindTypedById<UIButton>(ID("CancelMatchmakingButton"));
    m_leaveSessionButton = m_uiManager.FindTypedById<UIButton>(ID("LeaveSessionButton"));
    m_inviteFriendButton = m_uiManager.FindTypedById<UIButton>(ID("InviteFriendButton"));
    m_exitButton = m_uiManager.FindTypedById<UIButton>(ID("ExitButton"));

    m_createSessionButton->SetEnabled(false);
    m_startMatchmakingButton->SetEnabled(false);
    m_createCrossGenSessionButton->SetEnabled(false);
    m_startMatchmakingCrossGenButton->SetEnabled(false);
    m_cancelMatchmakingButton->SetEnabled(false);
    m_leaveSessionButton->SetEnabled(false);
    m_inviteFriendButton->SetEnabled(false);

    m_asyncOpWidget = std::make_unique<AsyncOpWidget>(m_uiManager, "Assets/Layouts/async-status.json");

    InitializeTaskQueue();
    InitializeLiveResources();
    CheckForNetworkInitialization();
}

void Sample::InitializeSessionManager()
{
    if (m_sessionManager == nullptr)
    {
        m_sessionManager = std::make_unique<SessionManager>();
        m_sessionManager->Init(m_liveResources->GetLiveContext(), m_liveResources->GetAsyncQueue());
        m_sessionManager->OnJoinSessionCompleted = [this](bool bSuccess)
        {
            Log("Join Session Complete: %s", bSuccess ? "Success" : "Failed");

            if (bSuccess)
            {
                DisableSessionButtons();

                m_leaveSessionButton->SetEnabled(true);
                m_inviteFriendButton->SetEnabled(true);
            }
        };

        m_sessionManager->OnPlayerJoinedSession = [this](uint64_t xuid)
        {
            Log("%llu joined the session", xuid);
        };

        m_sessionManager->OnPlayerLeftSession = [this](uint64_t xuid)
        {
            Log("%llu left the session", xuid);
        };

        m_sessionManager->RegisterForEvents();
    }
}

void Sample::CheckForNetworkInitialization()
{
    Log("CheckForNetworkingInitialization() started.");
    m_asyncOpWidget->Show(u8"Checking for network ready");

    ZeroMemory(&m_connectivityHint, sizeof(m_connectivityHint));
    auto hr = XNetworkingGetConnectivityHint(&m_connectivityHint);

    if (FAILED(hr))
    {
        Log("CheckForNetworkingInitialization() failed.");
        Log("XNetworkingGetConnectivityHint() returned 0x%08x.", hr);
        return;
    }
    if (m_connectivityHint.networkInitialized)
    {
        Log("Network is already initialized.");
        HandleNetworkInitializationComplete();
    }
    else
    {
        auto callback = [](void* context, const XNetworkingConnectivityHint* connectivityHint)
        {
            if(Sample* sample = static_cast<Sample*>(context))
            {
                sample->Log("CheckForNetworkingInitialization() callback issued...");

                if (connectivityHint->networkInitialized)
                {
                    sample->Log("Network is initialized.");
                    sample->m_connectivityHint = *connectivityHint;

                    XNetworkingUnregisterConnectivityHintChanged(sample->m_taskQueueRegToken, false);

                    sample->m_taskQueueRegToken.token = 0;
                    sample->HandleNetworkInitializationComplete();
                }
                else
                {
                    sample->Log("Network is NOT initialized.");
                }
            }
        };

        m_taskQueueRegToken.token = 0;
        hr = XNetworkingRegisterConnectivityHintChanged(m_taskQueue, this, callback, &m_taskQueueRegToken);
        if (FAILED(hr))
        {
            Log("CheckForNetworkingInitialization() failed.");
            Log("XNetworkingRegisterConnectivityHintChanged() returned 0x%08x.", hr);
        }
    }
}

void Sample::HandleNetworkInitializationComplete()
{
    m_asyncOpWidget->Hide();
    LoginToXboxLive(true);
    InitializeUIEventHandlers();
}

void Sample::LoginToXboxLive(bool silentAuth)
{
    Log("LoginToXboxLive");

    m_asyncOpWidget->Show(u8"Logging into Xbox Live");

    m_liveResources->SetUserChangedCallback([this](XUserHandle /*userHandle*/)
    {
        if (m_liveResources->IsUserSignedIn())
        {
            OnXboxLiveLoginComplete();
        }
    });

    if (silentAuth)
    {
        m_liveResources->SignInSilently();
    }
    else
    {
        m_liveResources->SignInWithUI();
    }
}

void Sample::OnXboxLiveLoginComplete()
{
    m_asyncOpWidget->Hide();

    m_liveInfoHUD->SetUser(m_liveResources->GetUser(), m_taskQueue);

    m_createSessionButton->SetEnabled(true);
    m_startMatchmakingButton->SetEnabled(true);
    m_createCrossGenSessionButton->SetEnabled(true);
    m_startMatchmakingCrossGenButton->SetEnabled(true);
    m_leaveSessionButton->SetEnabled(false);
    m_inviteFriendButton->SetEnabled(false);
    m_cancelMatchmakingButton->SetEnabled(false);

    m_uiManager.SetFocus(m_createSessionButton);

    InitializeSessionManager(); 
}

void Sample::InitializeUIEventHandlers()
{
    m_createSessionButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        CreateGameSession(false);
    });

    m_startMatchmakingButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        StartMatchmaking(false);
    });

    m_createCrossGenSessionButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        CreateGameSession(true);
    });

    m_startMatchmakingCrossGenButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        StartMatchmaking(true);
    });

    m_cancelMatchmakingButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        CancelMatchmaking();
    });

    m_leaveSessionButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        LeaveSession();
    });

    m_inviteFriendButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        InviteFriend();
    });

    m_exitButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [](UIButton*)
    {
        ExitSample();
    });
}

void Sample::CreateGameSession(bool allowCrossGen)
{
    if (allowCrossGen)
    {
        m_asyncOpWidget->Show("Creating CrossGen Game Session");
        Log("Create CrossGen Game Session");
    }
    else
    {
        m_asyncOpWidget->Show("Creating Game Session");
        Log("Create Game Session");
    }

    DisableSessionButtons();

    m_sessionManager->OnCreateSessionCompleted = [this](bool bSuccess)
    {
        m_sessionManager->OnCreateSessionCompleted = nullptr;

        m_asyncOpWidget->Hide();
        Log("Create Session Complete: %s", bSuccess ? "Success" : "Failed");

        if (bSuccess)
        {
            m_leaveSessionButton->SetEnabled(true);
            m_inviteFriendButton->SetEnabled(true);
        }
        else
        {
            m_createSessionButton->SetEnabled(true);
            m_startMatchmakingButton->SetEnabled(true);
            m_createCrossGenSessionButton->SetEnabled(true);
            m_startMatchmakingCrossGenButton->SetEnabled(true);
            m_leaveSessionButton->SetEnabled(false);
            m_inviteFriendButton->SetEnabled(false);
            m_cancelMatchmakingButton->SetEnabled(false);
        }
    };

    if (allowCrossGen)
    {
        m_sessionManager->CreateSession("GameSessionCrossGen");
    }
    else
    {
        m_sessionManager->CreateSession("GameSession");
    }
}

void Sample::StartMatchmaking(bool allowCrossGen)
{
    if (allowCrossGen)
    {
        m_asyncOpWidget->Show("Matchmaking (CrossGen)");
        Log("Starting CrossGen Matchmaking");
    }
    else
    {
        m_asyncOpWidget->Show("Matchmaking");
        Log("Starting Matchmaking");
    }

    DisableSessionButtons();

    m_sessionManager->OnCreateSessionCompleted = [this, allowCrossGen](bool bSuccess)
    {
        m_sessionManager->OnCreateSessionCompleted = nullptr;

        m_sessionManager->OnMatchmakingChanged = [this](MatchmakingState newState)
        {
            HandleMatchmakingChanged(newState);
        };

        Log("Create Lobby Session Complete: %s", bSuccess ? "Success" : "Failed");

        if (bSuccess)
        {
            m_cancelMatchmakingButton->SetEnabled(true);
            m_sessionManager->StartMatchmaking(allowCrossGen);
        }
        else
        {
            CancelMatchmaking();

            m_createSessionButton->SetEnabled(true);
            m_startMatchmakingButton->SetEnabled(true);
            m_createCrossGenSessionButton->SetEnabled(true);
            m_startMatchmakingCrossGenButton->SetEnabled(true);
            m_leaveSessionButton->SetEnabled(false);
            m_inviteFriendButton->SetEnabled(false);
            m_cancelMatchmakingButton->SetEnabled(false);
        }
    };

    if (allowCrossGen)
    {
        m_sessionManager->CreateSession("LobbySessionCrossGen");
    }
    else
    {
        m_sessionManager->CreateSession("LobbySession");
    }
}

void Sample::HandleMatchmakingChanged(MatchmakingState newState)
{
    switch (newState)
    {
    case MatchmakingState::Unknown:
    {
        Log("New Matchmaking State: Unknown");
        break;
    }
    case MatchmakingState::None:
    {
        Log("New Matchmaking State: None");
        break;
    }
    case MatchmakingState::Searching:
    {
        Log("New Matchmaking State: Searching");
        break;
    }
    case MatchmakingState::Expired:
    {
        Log("New Matchmaking State: Expired");
        break;
    }
    case MatchmakingState::Found:
    {
        Log("New Matchmaking State: Found");
        break;
    }
    case MatchmakingState::Canceled:
    {
        Log("New Matchmaking State: Canceled");
        break;
    }
    case MatchmakingState::Failed:
    {
        Log("New Matchmaking State: Failed");
        break;
    }
    }
}

void Sample::CancelMatchmaking()
{
    m_cancelMatchmakingButton->SetEnabled(false);
    m_asyncOpWidget->Show("Canceling Matchmaking");

    m_sessionManager->OnCancelMatchmakingCompleted = [this](bool bSuccess)
    {
        m_sessionManager->OnCancelMatchmakingCompleted = nullptr;

        m_asyncOpWidget->Hide();

        Log("Cancel Matchmaking Complete: %s", bSuccess ? "Success" : "Failed");

        if (bSuccess)
        {
            LeaveSession();
        }
    };

    m_sessionManager->CancelMatchmaking();
}

void Sample::LeaveSession()
{
    m_asyncOpWidget->Show(u8"Leaving Session");
    m_leaveSessionButton->SetEnabled(false);
    m_inviteFriendButton->SetEnabled(false);
    Log("Leave Session");

    m_sessionManager->OnLeaveSessionCompleted = [this](bool bSuccess)
    {
        m_asyncOpWidget->Hide();
        Log("Leave Session Complete: %s", bSuccess ? "Success" : "Failed");

        if (bSuccess)
        {
            m_createSessionButton->SetEnabled(true);
            m_startMatchmakingButton->SetEnabled(true);
            m_createCrossGenSessionButton->SetEnabled(true);
            m_startMatchmakingCrossGenButton->SetEnabled(true);
            m_leaveSessionButton->SetEnabled(false);
            m_inviteFriendButton->SetEnabled(false);
            m_cancelMatchmakingButton->SetEnabled(false);
        }

        m_sessionManager->OnLeaveSessionCompleted = nullptr;
    };

    m_sessionManager->LeaveSession();
}

void Sample::InviteFriend()
{
    Log("Show Invite UI");

    m_sessionManager->ShowPlatformInviteUI(m_liveResources->GetUser());
}

void Sample::DisableSessionButtons()
{
    m_createSessionButton->SetEnabled(false);
    m_startMatchmakingButton->SetEnabled(false);
    m_createCrossGenSessionButton->SetEnabled(false);
    m_startMatchmakingCrossGenButton->SetEnabled(false);
    m_inviteFriendButton->SetEnabled(false);
    m_cancelMatchmakingButton->SetEnabled(false);
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

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
        {
            if (!m_liveResources->IsUserSignedIn())
            {
                m_liveResources->SignInSilently();
            }
            else
            {
                m_liveResources->SignInWithUI();
            }
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Tab))
    {
        if (!m_liveResources->IsUserSignedIn())
        {
            m_liveResources->SignInSilently();
        }
        else
        {
            m_liveResources->SignInWithUI();
        }
    }

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);
    m_asyncOpWidget->Update(elapsedTime);

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

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);

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
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_liveResources->Refresh();
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

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // TODO: Initialize windows-size dependent objects here.
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
    auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}

void Sample::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
    m_uiManager.GetStyleManager().ResetStyleRenderer();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

#pragma endregion
