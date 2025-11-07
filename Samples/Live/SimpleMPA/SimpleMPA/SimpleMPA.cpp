//--------------------------------------------------------------------------------------
// SimpleMPA.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimpleMPA.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"
#include "GuidUtil.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG;
using namespace ATG::UITK;

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("SimpleMPA");

    m_friendsManager = std::make_unique<FriendsManager>();
    m_MPAManager = std::make_unique<MPAManager>();
}

Sample::~Sample()
{
    UnregisterForInvites();

    CleanupTaskQueue();

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

void Sample::CleanupTaskQueue()
{
    Log("Sample::InitializeTaskQueue()");

    if (m_taskQueue)
    {
        XTaskQueueCloseHandle(m_taskQueue);
        m_taskQueue = nullptr;
    }
}

void Sample::InitializeTaskQueue()
{
    Log("Sample::InitializeTaskQueue()");

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
    Log("Sample::InitializeLiveResources()");

    m_liveResources = std::make_shared<ATG::LiveResources>(m_taskQueue);
    m_liveResources->SetErrorHandler([this](HRESULT error)
    {
        if (error == E_GAMEUSER_NO_DEFAULT_USER || error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
        {
            m_liveResources->SignInWithUI();
        }
        else // Handle other error cases
        {
            Log("LiveResources: Error HRESULT: 0x%08x", error);
        }
    });

    m_liveResources->SetUserChangedCallback([this](XUserHandle /*userHandle*/)
    {
        if (m_liveResources->IsUserSignedIn())
        {
            OnXboxLiveLoginComplete();
        }
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());

        m_friendsManager->RemoveUserFromSocialManager(m_liveResources->GetUser());
    });

    Log("Logging into Xbox Live");
    m_asyncOpWidget->Show("Logging into Xbox Live");
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

    Log("Sample::Initialize()");

    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);
    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();
    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_liveInfoHUD->Initialize();

    auto layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("ConsoleWindow"));
    m_setActivityButton = m_uiManager.FindTypedById<UIButton>(ID("SetActivityButton"));
    m_updateActivityButton = m_uiManager.FindTypedById<UIButton>(ID("UpdateActivityButton"));
    m_deleteActivityButton = m_uiManager.FindTypedById<UIButton>(ID("DeleteActivityButton"));
    m_getActivitiesButton = m_uiManager.FindTypedById<UIButton>(ID("GetActivitiesButton"));
    m_sendInviteButton = m_uiManager.FindTypedById<UIButton>(ID("SendInviteButton"));
    m_showInviteUIButton = m_uiManager.FindTypedById<UIButton>(ID("ShowInviteUIButton"));
    m_exitButton = m_uiManager.FindTypedById<UIButton>(ID("ExitButton"));

    m_setActivityButton->SetEnabled(false);
    m_updateActivityButton->SetEnabled(false);
    m_deleteActivityButton->SetEnabled(false);
    m_getActivitiesButton->SetEnabled(false);
    m_sendInviteButton->SetEnabled(false);
    m_showInviteUIButton->SetEnabled(false);

    m_asyncOpWidget = std::make_unique<AsyncOpWidget>(m_uiManager, "Assets/Layouts/async-status.json");

    InitializeTaskQueue();
    InitializeLiveResources();
    CheckForNetworkInitialization();
}

void Sample::CheckForNetworkInitialization()
{
    Log("Sample::CheckForNetworkInitialization()");
    m_asyncOpWidget->Show("Checking for network ready");

    m_connectivityHint = {};
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
    Log("Sample::HandleNetworkInitializationComplete()");
    m_asyncOpWidget->Hide();
    InitializeUIEventHandlers();
}

void Sample::OnXboxLiveLoginComplete()
{
    Log("Sample::OnXboxLiveLoginComplete()");

    RegisterForInvites();

    m_asyncOpWidget->Hide();

    m_liveInfoHUD->SetUser(m_liveResources->GetUser(), m_taskQueue);

    m_friendsManager->AddUserToSocialManager(m_liveResources->GetLiveContext(), m_liveResources->GetUser());

    m_setActivityButton->SetEnabled(true);
    m_updateActivityButton->SetEnabled(false);
    m_deleteActivityButton->SetEnabled(false);
    m_getActivitiesButton->SetEnabled(true);
    m_sendInviteButton->SetEnabled(false);
    m_showInviteUIButton->SetEnabled(false);

    m_uiManager.SetFocus(m_setActivityButton);
}

void Sample::RegisterForInvites()
{
    DEBUGLOG("SessionManager::RegisterForInvites:");

    auto InviteHandlerLambda = [](void* context, const char* inviteUri)
    {
        if (inviteUri != nullptr)
        {
            if (auto pThis = reinterpret_cast<Sample*>(context))
            {
                std::string uri = inviteUri;
                pThis->HandleInvite(uri);
            }
        }
    };

    XGameInviteRegisterForEvent(nullptr, this, InviteHandlerLambda, &m_gameInviteEventToken);
}

void Sample::UnregisterForInvites()
{
    XGameInviteUnregisterForEvent(m_gameInviteEventToken, false);
}

void Sample::InitializeUIEventHandlers()
{
    Log("Sample::InitializeUIEventHandlers()");

    m_setActivityButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnPressed_SetActivity();
    });

    m_updateActivityButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnPressed_UpdateActivity();
    });

    m_deleteActivityButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnPressed_DeleteActivity();
    });

    m_getActivitiesButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnPressed_GetActivities();
    });

    m_sendInviteButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnPressed_SendInvite();
    });

    m_showInviteUIButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
    {
        OnPressed_ShowInviteUI();
    });

    m_exitButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [](UIButton*)
    {
        ExitSample();
    });
}

void Sample::GetInputFromUser(const std::string& titleText, const std::string& descriptionText, const std::string& defaultText, std::function<void(std::string)> callback)
{
    Log("Sample::GetInputFromUser()");

    auto asyncHelper = new ATG::AsyncHelper(m_taskQueue, [callback](XAsyncBlock* async)
    {
        std::string resultString;

        uint32_t size = 0;
        uint32_t used = 0;
        std::unique_ptr<char[]> buffer;

        // Get the size of the data buffer
        HRESULT hr = XGameUiShowTextEntryResultSize(async, &size);
        if (SUCCEEDED(hr))
        {
            if (size != 0)
            {
                buffer = std::make_unique<char[]>(size);

                // Get the data
                hr = XGameUiShowTextEntryResult(
                    async,          // XAsyncBlock
                    size,           // Size of buffer
                    buffer.get(),   // Buffer
                    &used           // OUT amount of buffer filled
                );

                if (SUCCEEDED(hr))
                {
                    if (buffer[0] != 0)
                    {
                        resultString = std::string(buffer.get(), size);
                    }
                }
                else
                {
                    LogError_HRESULT("XGameUiShowTextEntryResult", hr);
                }
            }
        }
        else
        {
            LogError_HRESULT("XGameUiShowTextEntryResultSize", hr);
        }

        if (callback)
        {
            callback(resultString);
        }
    });

    // Invoke the on-screen keyboard
    HRESULT hr = XGameUiShowTextEntryAsync(
        &asyncHelper->asyncBlock,
        titleText.c_str(),
        descriptionText.c_str(),
        defaultText.c_str(),
        XGameUiTextEntryInputScope::Alphanumeric,
        255
    );

    if (FAILED(hr))
    {
        LogError_HRESULT("XGameUiShowTextEntryAsync", hr);
        delete asyncHelper;

        if (callback)
        {
            callback("");
        }
    }
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    PumpTaskQueue();

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

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

    if (m_friendsManager)
    {
        m_friendsManager->DoWork();
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
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
    auto const r = m_deviceResources->GetOutputSize();
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

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    auto const os = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 200, os.right, os.bottom);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
    auto const size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}

void Sample::OnDeviceLost()
{   
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

#pragma region Sample Logic
void Sample::OnPressed_SetActivity()
{
    Log("Sample::OnPressed_SetActivity()");

    GetInputFromUser("Example Connection String", "Enter an example connection string to set on the activity.\n Example: \"ABCD1234\"", "", [this](const std::string connectionString)
    {
        if (connectionString.empty() == false)
        {
            m_activityGuid = DX::GuidUtil::NewGuid();
            m_connectionString = connectionString;
            m_currentPlayers = 1;

            m_MPAManager->SetActivity(
                m_liveResources->GetLiveContext(),               //Xbox live context
                m_liveResources->GetXuid(),                      //local user xuid
                m_connectionString.c_str(),                      //our fake connection string
                XblMultiplayerActivityJoinRestriction::Followed, //only friends can join this activity
                m_maxPlayers,                                    //max number of players for this activity
                m_currentPlayers,                                //current number of players for this activity
                m_activityGuid.c_str(),                          //unique identifier for the activity
                m_allowCrossPlatformJoin,                        //allow cross platform joining
                [this](bool bSuccess)
                {
                    if (bSuccess)
                    {
                        Log("Set activity with \"%s\" as the connection string!", m_connectionString.c_str());

                        m_updateActivityButton->SetEnabled(true);
                        m_deleteActivityButton->SetEnabled(true);
                        m_sendInviteButton->SetEnabled(true);
                        m_showInviteUIButton->SetEnabled(true);
                    }
                    else
                    {
                        Log("Failed to set the activity...");
                    }
                });
        }
        else
        {
            Log("Please enter a value for the example connection string");
        }
    });
}

void Sample::OnPressed_UpdateActivity()
{
    Log("Sample::OnPressed_UpdateActivity()");

    m_currentPlayers += 1;
    if (m_currentPlayers > m_maxPlayers)
    {
        m_currentPlayers = 1;
    }

    m_MPAManager->SetActivity(
        m_liveResources->GetLiveContext(),               //Xbox live context
        m_liveResources->GetXuid(),                      //local user xuid
        m_connectionString.c_str(),                      //our fake connection string
        XblMultiplayerActivityJoinRestriction::Followed, //only friends can join this activity
        m_maxPlayers,                                    //max number of players for this activity
        m_currentPlayers,                                //current number of players for this activity
        m_activityGuid.c_str(),                          //unique identifier for the activity
        m_allowCrossPlatformJoin,                        //allow cross platform joining
        [this](bool bSuccess)
        {
            if (bSuccess)
            {
                Log("Updated the activity to show %d of %d players in the activity!", m_currentPlayers, m_maxPlayers);

                m_updateActivityButton->SetEnabled(true);
                m_deleteActivityButton->SetEnabled(true);
                m_sendInviteButton->SetEnabled(true);
                m_showInviteUIButton->SetEnabled(true);
            }
            else
            {
                Log("Failed to update the activity...");
            }
        });
}

void Sample::OnPressed_DeleteActivity()
{
    Log("Sample::OnPressed_DeleteActivity()");

    m_MPAManager->DeleteActivity(m_liveResources->GetLiveContext(), m_liveResources->GetXuid(), [this](bool bSuccess)
    {
        if (bSuccess)
        {
            Log("Activity successfully deleted!", m_currentPlayers, m_maxPlayers);

            m_updateActivityButton->SetEnabled(false);
            m_deleteActivityButton->SetEnabled(false);
            m_sendInviteButton->SetEnabled(false);
            m_showInviteUIButton->SetEnabled(false);
        }
        else
        {
            Log("Failed to delete the activity...");
        }
    });
}

void Sample::OnPressed_GetActivities()
{
    Log("Sample::OnPressed_GetActivities()");

    //get a list of all of our friends
    std::vector<uint64_t> friends;
    m_friendsManager->GetFriends(friends);

    if (friends.empty())
    {
        Log("Friends manager returned an empty list of friends...");
        return;
    }

    m_MPAManager->GetActivities(
        m_liveResources->GetLiveContext(),
        friends,
        [this](std::vector<std::shared_ptr<UserActivity>>& userActivities)
        {
            Log("Retrieved %d friend activities...", (uint32_t)userActivities.size());

            for (const auto& userActivity : userActivities)
            {
                if (userActivity)
                {
                    Log("UserActivity:");
                    Log("    xuid: %llu", userActivity->xuid);
                    Log("    connectionString: %s", userActivity->connectionString.c_str());
                    Log("    maxPlayers: %llu", userActivity->maxPlayers);
                    Log("    currentPlayers: %llu", userActivity->currentPlayers);
                }
            }
        });
}

void Sample::OnPressed_SendInvite()
{
    Log("Sample::OnPressed_SendInvite()");

    //get a list of all of our friends
    std::vector<uint64_t> friends;
    m_friendsManager->GetFriends(friends);

    if (friends.empty())
    {
        Log("Friends manager returned an empty list of friends...");
        return;
    }

    //let the user pick which friends they want to invite
    auto asyncHelper = new ATG::AsyncHelper(m_taskQueue, [this](XAsyncBlock* async)
    {
        HRESULT hr = XAsyncGetStatus(async, false);
        if (SUCCEEDED(hr))
        {
            uint32_t numPickedFriends = 0;
            hr = XGameUiShowPlayerPickerResultCount(async, &numPickedFriends);
            if (SUCCEEDED(hr) && numPickedFriends > 0)
            {
                std::vector<uint64_t> pickedFriends;
                pickedFriends.resize(numPickedFriends);

                uint32_t resultPlayersUsed = 0;
                hr = XGameUiShowPlayerPickerResult(async, numPickedFriends, pickedFriends.data(), &resultPlayersUsed);
                if (SUCCEEDED(hr))
                {
                    if (pickedFriends.empty() == false)
                    {
                        //send the invite(s)
                        m_MPAManager->SendInvites(
                            m_liveResources->GetLiveContext(),
                            pickedFriends,
                            m_connectionString.c_str(),
                            m_allowCrossPlatformJoin,
                            [this](bool bSuccess)
                            {
                                if (bSuccess)
                                {
                                    Log("Invite(s) sent!");
                                }
                                else
                                {
                                    Log("Failed to send invite(s)...");
                                }
                            });
                    }
                    else
                    {
                        Log("You must pick at least one friend to send an invite...");
                    }
                }
                else
                {
                    LogError_HRESULT("XGameUiShowPlayerPickerResult", hr);
                }
            }
            else
            {
                LogError_HRESULT("XGameUiShowPlayerPickerResultCount", hr);
            }
        }
        else
        {
            LogError_HRESULT("XblMultiplayerActivitySetActivityAsync", hr);
        }
    });

    HRESULT hr = XGameUiShowPlayerPickerAsync(&asyncHelper->asyncBlock, m_liveResources->GetUser(), "Please, select players to invite:", (uint32_t)friends.size(), friends.data(), 0, nullptr, 1, m_maxPlayers);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblMultiplayerActivitySetActivityAsync", hr);
        delete asyncHelper;
    }
}

void Sample::OnPressed_ShowInviteUI()
{
    m_MPAManager->ShowInviteUI(m_liveResources->GetUser(), [this](bool bSuccess)
    {
        if (bSuccess)
        {
            Log("Successfully opened invite UI");
        }
        else
        {
            Log("Failed to show invite UI");
        }
    });
}

void Sample::HandleInvite(const std::string& uri)
{
    std::string str = "connectionString=";
    auto startPos = uri.find(str) + str.length();
    auto endPos = uri.find('&', startPos);

    // If the connectionString is at the end of the string then end will return not found.
    if (endPos == std::string::npos)
    {
        endPos = uri.length() + 1;
    }

    std::string handle = uri.substr(startPos, endPos - startPos);

    Log("Accepted an invite with connection string: %s", handle.c_str());
}
#pragma endregion
