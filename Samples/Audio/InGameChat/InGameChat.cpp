//--------------------------------------------------------------------------------------
// InGameChat.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "InGameChat.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"


extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample* Sample::s_instance = nullptr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_state(SampleState::LocalLobby)
{
    s_instance = this;

    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);

    ATG::UIConfig uiconfig;
    m_ui = std::make_shared<ATG::UIManager>(uiconfig);

    m_console = std::make_unique<DX::TextConsoleImage>();
    m_console->SetDebugOutput(true);

    m_liveManager = std::make_unique<XboxLiveManager>();
    m_chatManager = std::make_unique<GameChatManager>();
}

Sample::~Sample()
{
    XGameInviteUnregisterForEvent(m_inviteRegistration, false);

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

#ifdef _GAMING_XBOX
    m_ui->LoadLayout(L".\\Assets\\Layout.csv", L".\\Assets");
#else
    m_ui->LoadLayout(L".\\Assets\\Layout_Desktop.csv", L".\\Assets");
#endif

    SetupUI();

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_userManager = std::make_unique<UserManager>();
    m_userManager->Initialize();

    // Handle the invite accepted event
    auto hr = XGameInviteRegisterForEvent(
        nullptr,
        this,
        [](void* context, const char* inviteUri)
        {
            // ms-xbl-76b1590e://inviteHandleAccept/?invitedXuid=2814645439730606&handle=27a705f6-f080-4aa7-8bd7-ec8c51befd3d&senderXuid=2814626418925179
            std::string uri = inviteUri;

            auto pos = uri.find("handle=") + 7;
            auto end = uri.find('&', pos);

            // If the session is at the end of the string then end will return not found.
            if (end == std::string::npos)
            {
                end = uri.length() + 1;
            }

            static_cast<Sample*>(context)->JoinFriend(uri.substr(pos, end - pos).c_str());
        },
        &m_inviteRegistration
        );

    if (FAILED(hr))
    {
        DebugTrace("Failed to register for Invite event!");
    }

    // Populate each list with empty items to start
    auto users = std::vector<std::shared_ptr<UserListItem>>();

    for (auto x = 0; x < MAXUSERS; x++)
    {
        users.push_back(std::make_shared<UserListItem>());
    }

    m_userList->GenerateList(200, users, 2);
    m_chatList->GenerateList(300, users, 2);
    m_playerList->GenerateList(400, users, 2);

    // Show the lobby panel
    m_ui->FindPanel<ATG::Overlay>(200)->Show();
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

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

#ifdef _GAMING_XBOX
    m_userManager->ScanUserInput();
#endif
    m_liveManager->DoWork(elapsedTime);
    m_chatManager->ProcessStateChanges();
    m_chatManager->ProcessDataFrames();

    auto pad = m_gamePad->GetState(-2);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (pad.IsViewPressed() || kb.Escape)
    {
        ExitSample();
    }

    if (m_state == SampleState::LocalLobby)
    {
        LocalLobbyUpdate();
    }
    else if (m_state == SampleState::SearchComplete)
    {
        SessionSearchComplete();
    }
    else if (m_state == SampleState::JoinLobby)
    {
        JoinLobbyUpdate();
    }
    else if (m_state == SampleState::JoiningSession)
    {
        // Move to the chat lobby screen
        m_state = SampleState::ChatLobby;
        m_ui->FindPanel<ATG::Overlay>(300)->Show();
    }
    else if (m_state == SampleState::ChatLobby)
    {
        ChatLobbyUpdate();
    }

    m_ui->Update(elapsedTime, *m_mouse.get(), *m_keyboard.get());

    if (pad.IsConnected())
    {
        // Don't let the UX framework process 'B' button presses
        pad.buttons.b = false;

        m_ui->Update(elapsedTime, pad);
    }
#ifdef _GAMING_XBOX
    else
    {
        auto users = std::vector<std::shared_ptr<UserListItem>>();

        // Fill remaining slots with empty items
        for (auto x = 0; x < MAXUSERS; x++)
        {
            users.push_back(std::make_shared<UserListItem>(x == 0 ? true : false));
        }

        m_userList->UpdateList(users);

        m_gamePadButtons.Reset();
    }
#endif
    PIXEndEvent();
}

void Sample::LocalLobbyUpdate()
{
    // Startup lobby - the primary screen
    if (m_userManager->GetUsers().size() > 0)
    {
        // [A] Join Chat Session
        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::A))
        {
            m_state = SampleState::JoiningSession;

            m_liveManager->Initialize();
            m_liveManager->CreateSession();
        }
        // [X] Find Chat Session
        else if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::X))
        {
            m_state = SampleState::SearchingForSession;

            m_ui->FindPanel<ATG::Overlay>(400)->Show();
            m_ui->FindControl<ATG::TextLabel>(400, 102)->SetText(L"Looking for sessions...");

            m_liveManager->Initialize();
            m_liveManager->FindJoinableSessions([this]()
            {
                m_state = SampleState::SearchComplete;
            });

            // Clear out the UI list
            auto users = std::vector<std::shared_ptr<UserListItem>>();

            for (auto x = 0; x < MAXUSERS; x++)
            {
                users.push_back(std::make_shared<UserListItem>());
            }

            m_playerList->UpdateList(users);
        }
    }

    if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::Y))
    {
        m_userManager->InvokeSystemAddUser();
    }

    // Fill the list with known system users, then a 'press y to join' tile, then blanks for the rest
    auto users = std::vector<std::shared_ptr<UserListItem>>();
    auto remaining = MAXUSERS;
    auto signedIn = m_userManager->GetUsers();

    for (size_t x = 0; x < signedIn.size() && remaining > 0; x++)
    {
        // List item for a local user
        users.push_back(std::make_shared<UserListItem>(signedIn.at(x)->UserHandle));
        remaining--;
    }

    if (remaining > 0)
    {
#ifdef _GAMING_XBOX
        // List item for 'Press Y to join'
        users.push_back(std::make_shared<UserListItem>(true));
        remaining--;
#endif
        // Fill remaining slots with empty items
        for (auto x = 0; x < remaining; x++)
        {
            users.push_back(std::make_shared<UserListItem>());
        }
    }

    m_userList->UpdateList(users);
}

void Sample::SessionSearchComplete()
{
    // Search for joinable sessions has completed
    m_state = SampleState::JoinLobby;

    uint32_t titleId = 0;
    auto hr = XGameGetXboxTitleId(&titleId);

    if (FAILED(hr))
    {
        DebugTrace("Unable to get Title ID!");
    }

    auto results = m_liveManager->GetJoinableSessions();
    auto users = std::vector<std::shared_ptr<UserListItem>>();

    m_joinableSessions.clear();

    // Filter the list of sessions to people playing our title with joinable sessions
    for (const auto& result : results)
    {
        bool joinableSession = result.TitleId == titleId
            && result.MembersCount < result.MaxMembersCount
            && !result.Closed
            && result.JoinRestriction != XblMultiplayerSessionRestriction::Local
            && result.Visibility == XblMultiplayerSessionVisibility::Open;

        if (joinableSession)
        {
            m_joinableSessions.push_back(result.HandleId);

            auto user = std::make_shared<UserListItem>(m_liveManager->GetGamertagForXuid(result.OwnerXuid));
            user->HideGroup();
            users.push_back(user);
        }
    }

    for (auto x = users.size(); x < MAXUSERS; x++)
    {
        users.push_back(std::make_shared<UserListItem>());
    }

    if (m_joinableSessions.size() > 0)
    {
        m_ui->FindControl<ATG::TextLabel>(400, 102)->SetText(L"Select session to join");
    }
    else
    {
        m_ui->FindControl<ATG::TextLabel>(400, 102)->SetText(L"No sessions found");
    }

    m_playerList->UpdateList(users);
}

void Sample::JoinLobbyUpdate()
{
    // Selecting a joinable session
    // Pressing [A] to select will be handled by the button click handler
    if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::X))
    {
        // Reload the list of sessions
        m_state = SampleState::SearchingForSession;

        m_ui->FindControl<ATG::TextLabel>(400, 102)->SetText(L"Looking for sessions...");

        m_liveManager->FindJoinableSessions([this]()
        {
            m_state = SampleState::SearchComplete;
        });

        // Clear out the UI list
        auto users = std::vector<std::shared_ptr<UserListItem>>();
        for (auto x = 0; x < MAXUSERS; x++)
        {
            users.push_back(std::make_shared<UserListItem>());
        }

        m_playerList->UpdateList(users);
    }
    else if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::B))
    {
        // Exit this screen
        m_state = SampleState::LocalLobby;
        m_ui->FindPanel<ATG::Overlay>(200)->Show();
    }
}

void Sample::ChatLobbyUpdate()
{
    // Chat lobby
    // Press [X] to send a text message
    if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::X))
    {
        uint64_t xuid = 0;
        XUserGetId(m_userManager->GetUsers().at(0)->UserHandle, &xuid);

        auto async = new XAsyncBlock{};

        async->context = m_chatManager->GetChatUserByXboxUserId(xuid);
        async->callback = [](XAsyncBlock* async)
        {
            uint32_t size = 0;
            uint32_t used = 0;
            std::unique_ptr<char[]> buffer;

            // Get the size of the data buffer
            HRESULT hr = XGameUiShowTextEntryResultSize(
                async,              // XAsyncBlock
                &size               // OUT size of buffer
                );

            if (SUCCEEDED(hr))
            {
                buffer = std::make_unique<char[]>(size);

                // Get the data
                hr = XGameUiShowTextEntryResult(
                    async,          // XAsyncBlock
                    size,           // Size of buffer
                    buffer.get(),   // Buffer
                    &used           // OUT amount of buffer filled
                    );
            }

            if (SUCCEEDED(hr))
            {
                auto chatuser = reinterpret_cast<xbox::services::game_chat_2::chat_user*>(async->context);

                chatuser->local()->send_chat_text(DX::Utf8ToWide(buffer.get()).c_str());
                chatuser->local()->synthesize_text_to_speech(DX::Utf8ToWide(buffer.get()).c_str());
            }

            delete async;
        };

        auto hr = XGameUiShowTextEntryAsync(
            async,
            "Text Message",
            "Send a text message to all users",
            "Enter a message",
            XGameUiTextEntryInputScope::Alphanumeric,
            255
            );

        if (FAILED(hr))
        {
            DebugTrace("Unable to show virtual keyboard!");
        }
    }
    else if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::Y))
    {
        // Show Invite UI
        m_liveManager->InviteFriends(m_userManager->GetUsers().at(0)->UserHandle);
    }
    else if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::U))
    {
        // Increase channel number
        ChangeChannel(1);
    }
    else if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::D))
    {
        // Decrease channel number
        ChangeChannel(-1);
    }
    else if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyPressed(Keyboard::Keys::B))
    {
        // Leave the chat session
        m_state = SampleState::LeavingSession;
        LeaveChatSession();
    }

    // User list is gathered from game chat
    auto users = std::vector<std::shared_ptr<UserListItem>>();
    auto players = m_chatManager->GetChatUsersXuids();

    for (const auto& xuid : players)
    {
        users.push_back(std::make_shared<UserListItem>(m_chatManager->GetChatUserByXboxUserId(xuid)));
    }

    for (auto x = users.size(); x < MAXUSERS; x++)
    {
        users.push_back(std::make_shared<UserListItem>());
    }

    m_chatList->UpdateList(users);
}

void Sample::ChangeChannel(int offset)
{
    auto localUsers = m_userManager->GetUsers();
    for (const auto& localUser : localUsers)
    {
        uint64_t xuid = 0;
        XUserGetId(localUser->UserHandle, &xuid);

        auto channel = m_chatManager->GetChannelForUser(xuid);
        auto newchan = channel + offset;

        // Wrap-around between 1 and GameChatManager::c_numberOfChannels
        if (newchan > GameChatManager::c_numberOfChannels) newchan = 1;
        if (newchan < 1) newchan = GameChatManager::c_numberOfChannels;

        auto hr = XblMultiplayerManagerLobbySessionSetLocalMemberProperties(
            localUser->UserHandle,
            "channel",
            std::to_string(newchan).c_str(),
            nullptr
            );

        if (FAILED(hr))
        {
            DebugTrace("Failed to set user property!");
        }

        m_chatManager->ChangeChannelForUser(
            xuid,
            static_cast<uint8_t>(newchan)
            );
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

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    if (m_state == SampleState::LocalLobby ||
        m_state == SampleState::JoiningSession ||
        m_state == SampleState::ChatLobby)
    {
        m_console->Render(commandList);
    }

    m_ui->Render(commandList);

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

void Sample::LogToConsole(const char* message)
{
    m_console->WriteLine(DX::Utf8ToWide(message).c_str());
}

void Sample::LeaveChatSession()
{
    auto users = m_userManager->GetUsers();
    for (const auto& user : users)
    {
        uint64_t xuid = 0;
        XUserGetId(user->UserHandle, &xuid);

        m_chatManager->RemoveLocalUser(xuid);
    }

    m_chatManager->ProcessStateChanges();
    m_chatManager->ProcessDataFrames();

    m_liveManager->LeaveSession();
}

void Sample::LocalUserAdded(XUserHandle user)
{
    DebugTrace("New local user added: %lu", user);
    m_liveManager->AddLocalUser(user);
}

void Sample::JoinFriend(const char* handle)
{
    if (m_state == SampleState::LocalLobby ||
        m_state == SampleState::JoinLobby)
    {
        // Just join
        m_state = SampleState::JoiningSession;

        m_liveManager->Initialize();
        m_liveManager->JoinSession(handle);
    }
    else
    {
        m_joinHandle = handle;
        m_state = SampleState::LeavingSession;

        LeaveChatSession();
    }
}

void Sample::ReturnToStart()
{
    m_state = SampleState::LocalLobby;
    m_ui->FindPanel<ATG::Overlay>(200)->Show();
}

void Sample::SetupUI()
{
    // Setup user list repeaters
#ifdef _GAMING_XBOX
    auto loc = POINT{ 169, 249 };
    auto pos = SIZE{ 610, 76 };
#else
    auto loc = POINT{ 80, 149 };
    auto pos = SIZE{ 610, 76 };
#endif

    // UserList is the 'local lobby' user list
    m_userList = std::make_unique<UserRepeater>(m_ui, loc, pos, 8000);
    m_userList->SetReadOnly(true);

    // ChatList is the active session user list
    m_chatList = std::make_unique<UserRepeater>(m_ui, loc, pos, 9000);
    m_chatList->SetSelectedCallback([](unsigned index)
    {
        // Mute the selected user
        auto players = Sample::Instance()->GetChatManager()->GetChatUsersXuids();

        if (players.size() > index)
        {
            Sample::Instance()->GetChatManager()->ToggleChatUserMuteState(players.at(index));
        }
    });

    // PlayerList is for joining friends
    m_playerList = std::make_unique<UserRepeater>(m_ui, loc, pos, 7000);
    m_playerList->SetSelectedCallback([this](unsigned index)
    {
        m_state = SampleState::JoiningSession;
        m_liveManager->JoinSession(m_joinableSessions[index].c_str());
    });
}

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
    m_ui->Reset();
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

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count,
        Descriptors::Reserve
        );

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_console->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        L"Assets\\courier_16.spritefont",
        L"Assets\\ATGSampleBackground.DDS",
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Background)
    );

    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    m_userList->LoadImages(device, resourceUpload, *m_resourceDescriptors);
    m_chatList->LoadImages(device, resourceUpload, *m_resourceDescriptors);
    m_playerList->LoadImages(device, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();

    m_ui->SetWindow(m_deviceResources->GetOutputSize());

    RECT console = {};

#ifdef _GAMING_XBOX
    console.top = 247;
    console.left = 795;
    console.bottom = console.top + 641;
    console.right = console.left + 925;
#else
    console.top = 117;
    console.left = 600;
    console.bottom = console.top + 510;
    console.right = console.left + 640;
#endif

    m_console->SetWindow(console, false);
    m_console->SetViewport(viewport);

    m_console->WriteLine(L"GameChat2 Sample ready");
}

void Sample::OnDeviceLost()
{
    m_console->ReleaseDevice();
    m_resourceDescriptors.reset();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
