//--------------------------------------------------------------------------------------
// PlayFabLeaderboards.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PlayFabLeaderboards.h"

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
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
    m_liveResources = std::make_shared<ATG::LiveResources>();
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("PlayFab Leaderboards");

    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_mainAsyncQueue)
    );
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    if (m_mainAsyncQueue)
    {
        XTaskQueueCloseHandle(m_mainAsyncQueue);
        m_mainAsyncQueue = nullptr;
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();
    m_keyboard = std::make_unique<Keyboard>();
    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    InitializeUI();

    // This sample does not properly handle user changes during Suspend/Constrain
    // https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xbox-game-life-cycle
    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
        {
            m_liveInfoHUD->SetUser(user, m_liveResources->GetAsyncQueue());
            
            Log("Xbox Network: gamertag \"%s\" signed in", m_liveResources->GetGamertag().c_str());

            PlayFabSignIn();
        });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
        {
            m_getLeaderboardButton->SetEnabled(false);
            m_getEntityIDButton->SetEnabled(false);
            m_getFriendLeaderboardButton->SetEnabled(false);
            m_prevLeaderboardButton->SetEnabled(false);
            m_nextLeaderboardButton->SetEnabled(false);
            m_prevRankingsButton->SetEnabled(false);
            m_nextRankingsButton->SetEnabled(false);

            m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());
        });

    m_liveResources->SetErrorHandler([this](HRESULT error)
        {
            if (error == E_GAMEUSER_NO_DEFAULT_USER || error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
            {
                m_liveResources->SignInWithUI();
            }
            else // Handle other error cases
            {

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

void Sample::PlayFabSignIn()
{
    if (m_playFabResources)
    {
        m_playFabResources->Cleanup();
        m_playFabResources = nullptr;
    }

    m_playFabResources = std::make_unique<ATG::PlayFabResources>(PLAYFAB_TITLE_ID, m_liveResources->GetUser());
    m_playFabResources->LoginToPlayFab();
    Log("PlayFab(TitleID %s): gamertag \"%s\" signed in", PLAYFAB_TITLE_ID, m_liveResources->GetGamertag().c_str());

    PFEntityHandle entityHandle = m_playFabResources->GetEntityHandle();
    const PFEntityKey* localEntityKey = nullptr;
    size_t bufferSize = 0;
    size_t bufferUsed = 0;

    HRESULT hr = PFEntityGetEntityKeySize(entityHandle, &bufferSize);
    if (FAILED(hr))
    {
        Log("Failed to get entity key size. HRESULT: %08X", hr);
        return;
    }

    std::vector<uint8_t> buffer(bufferSize);
    hr = PFEntityGetEntityKey(entityHandle, bufferSize, buffer.data(), &localEntityKey, &bufferUsed);
    if (FAILED(hr))
    {
        Log("Failed to get entity key. HRESULT: %08X", hr);
        return;
    }

    m_entityId = localEntityKey->id;
    this->entityKey.type = ENTITY_TYPE;

    m_getLeaderboardButton->SetEnabled(true);
    m_getEntityIDButton->SetEnabled(true);
    m_getFriendLeaderboardButton->SetEnabled(true);
    m_prevLeaderboardButton->SetEnabled(true);
    m_nextLeaderboardButton->SetEnabled(true);
    m_prevRankingsButton->SetEnabled(true);
    m_nextRankingsButton->SetEnabled(true);
    m_uiManager.SetFocus(m_getLeaderboardButton);
}

void Sample::InitializeUI()
{    
    auto layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());
    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("ConsoleWindow"));
    m_getLeaderboardButton = m_uiManager.FindTypedById<UIButton>(ID("GetLeaderboardButton"));
    m_getEntityIDButton = m_uiManager.FindTypedById<UIButton>(ID("GetEntityIDButton"));
    m_getFriendLeaderboardButton = m_uiManager.FindTypedById<UIButton>(ID("GetFriendLeaderboardButton"));
    m_prevLeaderboardButton = m_uiManager.FindTypedById<UIButton>(ID("PrevLeaderboardButton"));
    m_nextLeaderboardButton = m_uiManager.FindTypedById<UIButton>(ID("NextLeaderboardButton"));
    m_prevRankingsButton = m_uiManager.FindTypedById<UIButton>(ID("PrevRankingsButton"));
    m_nextRankingsButton = m_uiManager.FindTypedById<UIButton>(ID("NextRankingsButton"));
    m_leaderboardsNameLabel = layout->GetTypedChildById<UIStaticText>(ID("LeaderboardsNameLabel"));
    m_leaderboardsNameLabel->SetDisplayText("Leaderboard Name");

    m_leaderboardsContentLabel = layout->GetTypedChildById<UIStaticText>(ID("LeaderboardsContentLabel"));
    m_leaderboardsScoreLabel1 = layout->GetTypedChildById<UIStaticText>(ID("LeaderboardsScoreLabel1"));
    m_leaderboardsScoreLabel2 = layout->GetTypedChildById<UIStaticText>(ID("LeaderboardsScoreLabel2"));
    m_leaderboardsMetadataLabel = layout->GetTypedChildById<UIStaticText>(ID("LeaderboardsMetadataLabel"));
    m_leaderboardsLastUpdateLabel = layout->GetTypedChildById<UIStaticText>(ID("LeaderboardsLastUpdateLabel"));

#ifdef _GAMING_DESKTOP
    m_prevLeaderboardButton->SetFocusable(true);
    m_nextLeaderboardButton->SetFocusable(true);
    m_prevRankingsButton->SetFocusable(true);
    m_nextRankingsButton->SetFocusable(true);
    normalButtonStyle = "Basic_Button_Style";
#else
    // Make these buttons only focusable for desktop so that they don't interfere with the dpad navigation
    m_prevLeaderboardButton->SetFocusable(false);
    m_nextLeaderboardButton->SetFocusable(false);
    m_prevRankingsButton->SetFocusable(false);
    m_nextRankingsButton->SetFocusable(false);
    normalButtonStyle = "Nonfocusable_Button_Style";
#endif

    m_prevLeaderboardButton->SetStyleId(ID(normalButtonStyle));
    m_nextLeaderboardButton->SetStyleId(ID(normalButtonStyle));
    m_prevRankingsButton->SetStyleId(ID(normalButtonStyle));
    m_nextRankingsButton->SetStyleId(ID(normalButtonStyle));

    m_getLeaderboardButton->SetEnabled(false);
    m_getEntityIDButton->SetEnabled(false);
    m_prevLeaderboardButton->SetEnabled(false);
    m_getFriendLeaderboardButton->SetEnabled(false);
    m_nextLeaderboardButton->SetEnabled(false);
    m_prevRankingsButton->SetEnabled(false);
    m_nextRankingsButton->SetEnabled(false);

    m_getLeaderboardButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedGetLeaderboardButton();
        });

    m_getEntityIDButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedGetEntityIDButton();
        });

    m_getFriendLeaderboardButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedGetFriendLeaderboardButton();
        });
    m_prevLeaderboardButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedPrevLeaderboardButton();
        });

    m_nextLeaderboardButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedNextLeaderboardButton();
        });

    m_prevRankingsButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedPrevRankingsButton();
        });

    m_nextRankingsButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedNextRankingsButton();
        });
}

void Sample::OnPressedGetLeaderboardButton()
{
    m_leaderboardsNameLabel->SetDisplayText(m_leaderboardName);

    if (m_isResultLoaded)
    {
        m_leaderboardType = LeaderboardType::Default;
        m_prevLeaderboardButton->SetEnabled(true);
        m_nextLeaderboardButton->SetEnabled(true);
        GetLeaderboardAsync(m_playFabResources->GetEntityHandle(), m_leaderboardName.c_str(), 1 + (m_leaderboardPagenumber * 10), LEADERBOARD_VERSION);
    }
    Log("Sample::OnPressedGetLeaderboardButton()");
}

void Sample::OnPressedGetEntityIDButton()
{
    if (m_isResultLoaded)
    {
        m_leaderboardType = LeaderboardType::Default;
        m_prevLeaderboardButton->SetEnabled(true);
        m_nextLeaderboardButton->SetEnabled(true);
        GetLeaderboardAroundEntityAsync(m_playFabResources->GetEntityHandle(), m_leaderboardName.c_str());
    }
    Log("Sample::OnPressed_getEntityIDButton()");
}

void Sample::OnPressedGetFriendLeaderboardButton()
{
    if (m_leaderboardType == LeaderboardType::Default)
    {
        m_leaderboardType = LeaderboardType::Friend;
        LeaderboardsUIReset();
    }
    if (m_isResultLoaded)
    {
        m_prevLeaderboardButton->SetEnabled(false);
        m_nextLeaderboardButton->SetEnabled(false);
        m_leaderboardsNameLabel->SetDisplayText("Friend Leaderboard");
        GetFriendLeaderboardForEntityAsync(m_playFabResources->GetEntityHandle(), m_leaderboardName.c_str());
    }
    Log("Sample::OnPressedGetFriendLeaderboardButton()");
}

void Sample::ToggleLeaderboard()
{
    if (m_leaderboardNumber == 1)
    {
        m_leaderboardName = LEADERBOARD_NAME2;
        m_leaderboardNumber = 2;
    }
    else
    {
        m_leaderboardName = LEADERBOARD_NAME1;
        m_leaderboardNumber = 1;
    }
    m_leaderboardsNameLabel->SetDisplayText(m_leaderboardName);
}

void Sample::OnPressedPrevLeaderboardButton()
{
    ToggleLeaderboard();

    if (m_isResultLoaded)
    {
        GetLeaderboardAsync(m_playFabResources->GetEntityHandle(), m_leaderboardName.c_str(), 1 + (m_leaderboardPagenumber * 10), LEADERBOARD_VERSION);
    }
    Log("Sample::OnPressedPrevLeaderboardButton()");
}

void Sample::OnPressedNextLeaderboardButton()
{
    ToggleLeaderboard();

    if (m_isResultLoaded)
    {
        GetLeaderboardAsync(m_playFabResources->GetEntityHandle(), m_leaderboardName.c_str(), 1 + (m_leaderboardPagenumber * 10), LEADERBOARD_VERSION);
    }
    Log("Sample::OnPressedNextLeaderboardButton()");
}

void Sample::OnPressedPrevRankingsButton()
{
    if (m_leaderboardPagenumber > 0) {
        m_leaderboardPagenumber--;
        if (m_leaderboardType == LeaderboardType::Default) {
            OnPressedGetLeaderboardButton();
        }
        else {
            OnPressedGetFriendLeaderboardButton();
        }
    }
    Log("Sample::OnPressedPrevRankingsButton()");
}

void Sample::OnPressedNextRankingsButton()
{
    m_leaderboardPagenumber++;
    if (m_leaderboardType == LeaderboardType::Default) {
        OnPressedGetLeaderboardButton();
    }
    else {
        OnPressedGetFriendLeaderboardButton();
    }
    Log("Sample::OnPressedNextRankingsButton()");
}

// Display leaderboard entries
void Sample::ProcessLeaderboardEntries(const PFLeaderboardsGetEntityLeaderboardResponse* result)
{
    std::string leaderboardText;
    std::string leaderboardTextScore1;
    std::string leaderboardTextScore2;
    std::string leaderboardTextMetadata;
    std::string leaderboardTextLastUpdate;

    for (size_t i = 0; i < result->rankingsCount; ++i)
    {
        const PFLeaderboardsEntityLeaderboardEntry* entry = result->rankings[i];
        leaderboardText += std::to_string(entry->rank) + ". " + std::string(entry->entity->id) + "\n";
        leaderboardTextScore1 += ":   " + std::string(entry->scores[0]) + "\n";
        leaderboardTextScore2 += std::string(entry->scores[1]) + "\n";
        leaderboardTextMetadata += entry->metadata + std::string("\n");
        std::time_t lastUpdatedTime = entry->lastUpdated;
        std::tm tm;
        localtime_s(&tm, &lastUpdatedTime);
        char dateBuffer[100];
        std::strftime(dateBuffer, sizeof(dateBuffer), "%Y-%m-%d %H:%M:%S", &tm);
        leaderboardTextLastUpdate += std::string(dateBuffer) + "\n";
    }

    if (m_leaderboardsContentLabel)
    {
        m_leaderboardsContentLabel->SetDisplayText(leaderboardText.c_str());
        m_leaderboardsScoreLabel1->SetDisplayText(leaderboardTextScore1.c_str());
        m_leaderboardsScoreLabel2->SetDisplayText(leaderboardTextScore2.c_str());
        m_leaderboardsMetadataLabel->SetDisplayText(leaderboardTextMetadata.c_str());
        m_leaderboardsLastUpdateLabel->SetDisplayText(leaderboardTextLastUpdate.c_str());
    }

    m_isResultLoaded = true;
}

void Sample::GetLeaderboardAsync(PFEntityHandle entityHandle, const char* leaderboardName, const uint32_t startPotion, const uint32_t leaderboardVersion)
{
    PFLeaderboardsGetEntityLeaderboardRequest request{};
    request.leaderboardName = leaderboardName;
    request.startingPosition = &startPotion; // start point on leaderboard
    request.pageSize = 10; // max entry number
    request.version = &leaderboardVersion;
    m_isResultLoaded = false;

    // create XAsyncBlock
    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(this);
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        auto async = std::unique_ptr<XAsyncBlock>(asyncBlock);
        auto* sample = static_cast<Sample*>(asyncBlock->context);

        size_t bufferSize;
        HRESULT hr = PFLeaderboardsGetLeaderboardGetResultSize(asyncBlock, &bufferSize);
        if (SUCCEEDED(hr))
        {
            std::vector<uint8_t> buffer(bufferSize);
            PFLeaderboardsGetEntityLeaderboardResponse* result;
            hr = PFLeaderboardsGetLeaderboardGetResult(asyncBlock, buffer.size(), buffer.data(), &result, nullptr);
            if (SUCCEEDED(hr))
            {
                sample->ProcessLeaderboardEntries(result);
            }
            else
            {
                sample->Log("Failed to get leaderboard result. HRESULT: %08X", hr);
            }
        }
        else
        {
            sample->Log("Failed to get leaderboard result size. HRESULT: %08X", hr);
        }
        sample->m_isResultLoaded = true;
    };

    // get leaderboard at Async
    HRESULT hr = PFLeaderboardsGetLeaderboardAsync(
        entityHandle,
        &request,
        asyncBlock);

    if (FAILED(hr))
    {
        Log("Failed to start leaderboard request. HRESULT: %08X", hr);
        delete asyncBlock;
    }

}

void Sample::GetLeaderboardAroundEntityAsync(PFEntityHandle entityHandle, const char* leaderboardName)
{
    // create request
    PFLeaderboardsGetLeaderboardAroundEntityRequest request{};
    request.leaderboardName = leaderboardName;
    request.maxSurroundingEntries = (10 - 1);
    entityKey.id = "PlayerEntityId1";
    request.entity = &entityKey;

    m_isResultLoaded = false;

    // create XAsyncBlock
    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(this);
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
        {
            auto async = std::unique_ptr<XAsyncBlock>(asyncBlock);
            auto* sample = static_cast<Sample*>(asyncBlock->context);

            size_t bufferSize;
            HRESULT hr = PFLeaderboardsGetLeaderboardAroundEntityGetResultSize(asyncBlock, &bufferSize);
            if (SUCCEEDED(hr))
            {
                std::vector<uint8_t> buffer(bufferSize);
                PFLeaderboardsGetEntityLeaderboardResponse* result;
                hr = PFLeaderboardsGetLeaderboardAroundEntityGetResult(asyncBlock, buffer.size(), buffer.data(), &result, nullptr);
                if (SUCCEEDED(hr))
                {
                    sample->m_leaderboardPagenumber = static_cast<uint32_t>(result->rankings[0]->rank / 10);
                    sample->ProcessLeaderboardEntries(result);
                }
                else
                {
                    sample->Log("Failed to get leaderboard result. HRESULT: %08X", hr);
                }
            }
            else
            {
                sample->Log("Failed to get leaderboard result size. HRESULT: %08X", hr);
            }
            sample->m_isResultLoaded = true;
        };

    // get leaderboard around entity at Async
    HRESULT hr = PFLeaderboardsGetLeaderboardAroundEntityAsync(
        entityHandle,
        &request,
        asyncBlock);

    if (FAILED(hr))
    {
        Log("Failed to start leaderboard request. HRESULT: %08X", hr);
        delete asyncBlock;
    }
}

void Sample::GetFriendLeaderboardForEntityAsync(PFEntityHandle entityHandle, const char* leaderboardName)
{
    // create request
    PFLeaderboardsGetFriendLeaderboardForEntityRequest request{};
    request.leaderboardName = leaderboardName;
    entityKey.id = m_entityId.c_str();
    request.entity = &entityKey;
    PFExternalFriendSources externalFriendSources = PFExternalFriendSources::All;
    request.externalFriendSources = &externalFriendSources;
    request.xboxToken = m_playFabResources->GetXboxToken();

    m_isResultLoaded = false;

    // create XAsyncBlock
    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(this);
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
        {
            auto async = std::unique_ptr<XAsyncBlock>(asyncBlock);
            auto* sample = static_cast<Sample*>(asyncBlock->context);

            size_t bufferSize;
            HRESULT hr = PFLeaderboardsGetFriendLeaderboardForEntityGetResultSize(asyncBlock, &bufferSize);
            if (SUCCEEDED(hr))
            {
                std::vector<uint8_t> buffer(bufferSize);
                PFLeaderboardsGetEntityLeaderboardResponse* result;
                hr = PFLeaderboardsGetFriendLeaderboardForEntityGetResult(asyncBlock, buffer.size(), buffer.data(), &result, nullptr);
                if (SUCCEEDED(hr))
                {
                    if (result->rankingsCount == 0)
                    {
                        sample->Log("No friend rankings found.");
                    }
                    else
                    {
                        sample->ProcessLeaderboardEntries(result);
                    }
                }
                else
                {
                    sample->Log("Failed to get friend leaderboard result. HRESULT: %08X", hr);
                }
            }
            else
            {
                sample->Log("Failed to get friend leaderboard result size. HRESULT: %08X", hr);
            }
            sample->m_isResultLoaded = true;
        };

    // get friend leaderboard at Async
    HRESULT hr = PFLeaderboardsGetFriendLeaderboardForEntityAsync(
        entityHandle,
        &request,
        asyncBlock);

    if (FAILED(hr))
    {
        Log("Failed to start friend leaderboard request. HRESULT: %08X", hr);
        delete asyncBlock;
    }
}

void Sample::LeaderboardsUIReset()
{
    m_leaderboardPagenumber = 0;
    m_leaderboardsContentLabel->SetDisplayText("");
    m_leaderboardsScoreLabel1->SetDisplayText("");
    m_leaderboardsScoreLabel2->SetDisplayText("");
    m_leaderboardsMetadataLabel->SetDisplayText("");
    m_leaderboardsLastUpdateLabel->SetDisplayText("");
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

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

    #pragma message( __FILE__  ": TODO in Update" )
    // TODO: Add your sample logic here.
    elapsedTime;

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
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

        // Preview leaderboard by LT
        if (m_prevLeaderboardButton->IsEnabled())
        {
            switch (m_gamePadButtons.leftTrigger)
            {
            case GamePad::ButtonStateTracker::PRESSED:
                OnPressedPrevLeaderboardButton();
                break;
            case GamePad::ButtonStateTracker::HELD:
                m_prevLeaderboardButton->SetStyleId(ID("Pressed_Button_Style"));
                break;
            case GamePad::ButtonStateTracker::RELEASED:
                m_prevLeaderboardButton->SetStyleId(ID(normalButtonStyle));
                break;
            case GamePad::ButtonStateTracker::UP:
                break;
            default:
                break;
            }
        }

        // Friend leaderboard by RT
        if (m_nextLeaderboardButton->IsEnabled())
        {
            switch (m_gamePadButtons.rightTrigger)
            {
            case GamePad::ButtonStateTracker::PRESSED:
                OnPressedNextLeaderboardButton();
                break;
            case GamePad::ButtonStateTracker::HELD:
                m_nextLeaderboardButton->SetStyleId(ID("Pressed_Button_Style"));
                break;
            case GamePad::ButtonStateTracker::RELEASED:
                m_nextLeaderboardButton->SetStyleId(ID(normalButtonStyle));
                break;
            case GamePad::ButtonStateTracker::UP:
                break;
            default:
                break;
            }
        }

        // Previous ranking by LB
        if (m_prevRankingsButton->IsEnabled())
        {
            switch (m_gamePadButtons.leftShoulder)
            {
            case GamePad::ButtonStateTracker::PRESSED:
                OnPressedPrevRankingsButton();
                break;
            case GamePad::ButtonStateTracker::HELD:
                m_prevRankingsButton->SetStyleId(ID("Pressed_Button_Style"));
                break;
            case GamePad::ButtonStateTracker::RELEASED:
                m_prevRankingsButton->SetStyleId(ID(normalButtonStyle));
                break;
            case GamePad::ButtonStateTracker::UP:
                break;
            default:
                break;
            }
        }

        // Next ranking by RB
        if (m_nextRankingsButton->IsEnabled())
        {
            switch (m_gamePadButtons.rightShoulder)
            {
            case GamePad::ButtonStateTracker::PRESSED:
                OnPressedNextRankingsButton();
                break;
            case GamePad::ButtonStateTracker::HELD:
                m_nextRankingsButton->SetStyleId(ID("Pressed_Button_Style"));
                break;
            case GamePad::ButtonStateTracker::RELEASED:
                m_nextRankingsButton->SetStyleId(ID(normalButtonStyle));
                break;
            case GamePad::ButtonStateTracker::UP:
                break;
            default:
                break;
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

    auto mouse = m_mouse->GetState();
    mouse;

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);

    // Process any completed tasks
    while (XTaskQueueDispatch(m_mainAsyncQueue, XTaskQueuePort::Completion, 0))
    {
        SwitchToThread();
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
    m_uiManager.Render(); 

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);

    #pragma message( __FILE__  ": TODO in Render" )
    // TODO: Add your rendering code here.
    commandList;

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

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
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

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    #pragma message( __FILE__  ": TODO in CreateDeviceDependentResources" )
    // TODO: Initialize device dependent objects here (independent of window size).
    device;

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    #pragma message( __FILE__  ": TODO in CreateWindowSizeDependentResources" )
    // TODO: Initialize windows-size dependent objects here.

    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
    auto const size = m_deviceResources->GetOutputSize(); 
    m_uiManager.SetWindowSize(size.right, size.bottom);

}

void Sample::OnDeviceLost()
{
#pragma message( __FILE__  ": TODO in OnDeviceLost" )
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
