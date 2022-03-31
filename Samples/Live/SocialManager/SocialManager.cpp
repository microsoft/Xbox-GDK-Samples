//--------------------------------------------------------------------------------------
// SocialManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SocialManager.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"


extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const int c_sampleUIPanel = 200;
    const int c_groupLabel = 201;

    const int c_unselected1 = 300;
    const int c_selected1 = 310;

    const int c_listSize = 15;

    std::string friendListTypeStrings[] =
    {
        "All Friends",
        "All Online Friends",
        "All Online In-Title Friends",
        "All Favorites",
        "Custom"
    };
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
    m_liveResources = std::make_shared<ATG::LiveResources>();
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("SocialManager Sample");

    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_asyncQueue)
    );

    ATG::UIConfig uiconfig;
    m_ui = std::make_unique<ATG::UIManager>(uiconfig);
    m_console = std::make_unique<DX::TextConsoleImage>();
}

Sample::~Sample()
{
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

    wchar_t result[MAX_PATH];
    DX::FindMediaFile(result, MAX_PATH, L".\\Assets\\SampleUI.csv");
    m_ui->LoadLayout(result, L".\\Assets\\");
    SetupUI();

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
    {
        m_liveInfoHUD->SetUser(user, m_liveResources->GetAsyncQueue());
        m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Show();
        AddUserToSocialManager(user);
        RefreshUserList();
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle user)
    {
        m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());
        m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Close();
        RemoveUserFromSocialManager(user);
    });

    m_liveResources->SetErrorHandler([this](HRESULT error)
    {
        if (error == E_GAMEUSER_NO_DEFAULT_USER || error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
        {
            m_liveResources->SignInWithUI();
        }
        else if (error == E_GAMEUSER_NO_PACKAGE_IDENTITY)
        {
            m_console->WriteLine(L"No package identity found, ensure you're running from the Start Menu\r\n  after registering the app with wdapp.");
        }
        else // Handle other error cases
        {
            m_console->Format(L"Xbox Live Error: 0x%08X\n", error);
        }
    });

    // Before we can make an Xbox Live call we need to ensure that the Game OS has intialized the network stack
    // For sample purposes we block user interaction with the sample.  A game should wait for the network to be
    // initialized before the main menu appears.  For samples, we will wait at the end of initialization.
    while (!m_liveResources->IsNetworkAvailable())
    {
        SwitchToThread();
    }

    m_liveResources->Initialize();
    m_liveInfoHUD->Initialize();
}

#pragma region UI Methods
void Sample::SetupUI()
{
    m_selectedFriendList = friendListType::allFriends;

    // Setup user list repeater
    auto loc = POINT{ 150, 200 };
    auto pos = SIZE{ 610, 40 };

    m_userRepeater = std::make_unique<UserRepeater>(m_ui, loc, pos, 8000, m_liveResources->GetTitleId());
    m_userRepeater->SetSelectedCallback([this](std::shared_ptr<UserListItem> item)
    {
        if(!item)
            return;

        auto async = new XAsyncBlock();
        async->context = this;
        async->queue = m_asyncQueue;
        async->callback = [](XAsyncBlock* async)
        {
            auto sample = reinterpret_cast<Sample*>(async->context);
            HRESULT hr = XGameUiShowPlayerProfileCardResult(async);
            if(FAILED(hr))
            {
                sample->m_console->Format(L"Could not complete showing player profile card: 0x%08X\n", static_cast<unsigned int>(hr));
            }
        };

        m_console->Format(L"Opening profile card for %ws\n", item->GetName().c_str());

        // Invoke player profile card
        HRESULT hr = XGameUiShowPlayerProfileCardAsync(
            async,
            m_liveResources->GetUser(),
            item->GetXuid()
        );
        if(FAILED(hr))
        {
            m_console->WriteLine(L"Could not show player profile card.");
        }
    });

    // Populate each list with empty items to start
    auto users = std::vector<std::shared_ptr<UserListItem>>(c_listSize);
    m_userRepeater->GenerateList(200, users, 2);

    SetActivePage(0);
}

void Sample::SetActivePage(int page)
{
    for(int i = 0; i < 4; i++)
    {
        m_ui->FindControl<ATG::Image>(c_sampleUIPanel, static_cast<unsigned int>(c_unselected1 + i))->SetVisible(!(page==i));
        m_ui->FindControl<ATG::Image>(c_sampleUIPanel, static_cast<unsigned int>(c_selected1 + i))->SetVisible(page==i);
    }

}
#pragma endregion

void Sample::RefreshUserList()
{
    const auto& group = m_userSocialGroups[m_selectedFriendList];
    std::string label = friendListTypeStrings[m_selectedFriendList];

    m_console->Format(L"Displaying %ws\n", DX::Utf8ToWide(label).c_str());
    m_ui->FindControl<ATG::TextLabel>(c_sampleUIPanel, c_groupLabel)->SetText(DX::Utf8ToWide(label).c_str());

    auto users = std::vector<std::shared_ptr<UserListItem>>();
    size_t count;

    HRESULT hr = XblSocialManagerUserGroupGetUsers(group, &m_userList, &count);
    if(FAILED(hr))
    {
        m_console->Format(L" Error refreshing user list: 0x%08X\n", hr);
    }

    m_console->Format(L"Group contains %d user(s)\n", count);

    for (size_t x = 0; x < c_listSize; x++)
    {
        if (x < count)
        {
            users.push_back(std::make_shared<UserListItem>(m_userList[x]));
        }
        else
        {
            users.push_back(nullptr);
        }
    }

    m_userRepeater->UpdateList(users);
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

    bool refresh = false;
    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    if (m_gamePadButtons.view == GamePad::ButtonStateTracker::PRESSED || kb.Escape)
    {
        ExitSample();
    }

    if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Tab))
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
    if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyReleased(Keyboard::Keys::F5))
    {
        refresh = true;
    }
    if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Right))
    {
        int newVal = ((int)m_selectedFriendList) + 1;

        if (newVal > 3)
        {
            newVal = 0;
        }

        m_selectedFriendList = (friendListType)newVal;
        refresh = true;
        SetActivePage(newVal);
    }
    if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Left))
    {
        int newVal = ((int)m_selectedFriendList) - 1;

        if (newVal < 0)
        {
            newVal = 3;
        }

        m_selectedFriendList = (friendListType)newVal;
        refresh = true;
        SetActivePage(newVal);
    }

    if (refresh)
    {
        std::lock_guard<std::mutex> guard(m_socialManagerLock);
        RefreshUserList();
    }

    if (m_liveResources != nullptr)
    {
        std::lock_guard<std::mutex> guard(m_socialManagerLock);
        UpdateSocialManager();
    }

    // Process any completed tasks
    while (XTaskQueueDispatch(m_asyncQueue, XTaskQueuePort::Completion, 0))
    {
    }

    m_ui->Update(elapsedTime, pad);
    m_ui->Update(elapsedTime, *m_mouse, *m_keyboard);
    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

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
    m_console->Render(commandList);
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
    width = 1840;
    height = 1035;
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

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    wchar_t font[260];
    wchar_t background[260];

    DX::FindMediaFile(font, 260, L"courier_16.spritefont");
    DX::FindMediaFile(background, 260, L"ATGSampleBackground.DDS");

    m_console->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        font,
        background,
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Background)
    );

    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);
    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

void ScaleRect(const RECT &originalDisplayRect, const RECT &displayRect, const RECT &originalSubRect, RECT &scaledSubRect)
{
    const float widthScale = ((float)displayRect.right - (float)displayRect.left) / ((float)originalDisplayRect.right - (float)originalDisplayRect.left);
    const float heightScale = ((float)displayRect.bottom - (float)displayRect.top) / ((float)originalDisplayRect.bottom - (float)originalDisplayRect.top);

    scaledSubRect.top = (LONG)((float)originalSubRect.top * heightScale);
    scaledSubRect.left = (LONG)((float)originalSubRect.left * widthScale);
    scaledSubRect.bottom = (LONG)((float)originalSubRect.bottom * heightScale);
    scaledSubRect.right = (LONG)((float)originalSubRect.right * widthScale);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT fullscreen = m_deviceResources->GetOutputSize();
    auto viewport = m_deviceResources->GetScreenViewport();

     // Scaled for 1920x1080
    static const RECT originalScale = { 0, 0, 1920, 1080 };
    static const RECT console = { 795, 200, console.left + 925, console.top + 657 };
    RECT scaledDisplay;
    ScaleRect(originalScale, fullscreen, console, scaledDisplay);

    m_ui->SetWindow(fullscreen);

    m_console->SetWindow(scaledDisplay, false);
    m_console->SetViewport(viewport);

    m_liveInfoHUD->SetViewport(viewport);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
    m_ui->ReleaseDevice();
    m_console->ReleaseDevice();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
