//--------------------------------------------------------------------------------------
// PlayFabLeaderboards.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "LiveResources.h"
#include "PlayFabResources.h"
#include "LiveInfoHUD.h"
#include "StepTimer.h"
#include "UITK.h"

// PlayFab Title Id
constexpr const char* PLAYFAB_TITLE_ID = ""; // Please set this value to your own titleId from PlayFab Game Manager

// Enum for leaderboard type
enum class LeaderboardType
{
    Default,
    Friend
};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify, public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);
    void InitializeUI();

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

    // Button event handlers
    void OnPressedGetLeaderboardButton();
    void OnPressedGetEntityIDButton();
    void OnPressedGetFriendLeaderboardButton();
    void OnPressedPrevLeaderboardButton();
    void OnPressedNextLeaderboardButton();
    void OnPressedPrevRankingsButton();
    void OnPressedNextRankingsButton();

    void GetLeaderboardAsync(PFEntityHandle entityHandle, const char* leaderboardName, const uint32_t startPostion, const uint32_t leaderboardVersion);
    void GetLeaderboardAroundEntityAsync(PFEntityHandle entityHandle, const char* leaderboardName);
    void GetFriendLeaderboardForEntityAsync(PFEntityHandle entityHandle, const char* leaderboardName);
    void ToggleLeaderboard();
    void ProcessLeaderboardEntries(const PFLeaderboardsGetEntityLeaderboardResponse* result);
    // Sample methods

    void PlayFabSignIn();

#define DEBUG_BUFFER_SIZE 8192
    void Log(const char* format, ...)
    {        
        static char msgbuffer[DEBUG_BUFFER_SIZE];
        va_list args;
        va_start(args, format);
        vsprintf_s(msgbuffer, DEBUG_BUFFER_SIZE, format, args);
        va_end(args);

        std::string buffer;
        buffer += msgbuffer;
        buffer += '\n';        
        
        OutputDebugStringA(buffer.c_str());
        if (m_consoleWindow)
        {
            m_consoleWindow->AppendLineOfText(buffer);
        }
    }

private:

    // Constants
    static constexpr const char* ENTITY_TYPE = "title_player_account";
    static constexpr const char* LEADERBOARD_NAME1 = "SampleLeaderboard1";
    static constexpr const char* LEADERBOARD_NAME2 = "SampleLeaderboard2";
    static constexpr bool PREV_LEADERBOARDS = true;
    static constexpr bool FRIEND_LEADERBOARDS = false;
    static constexpr uint32_t LEADERBOARD_VERSION = 0;
    static constexpr uint32_t LEADERBOARD_NUMBER = 1;
    static constexpr uint32_t LEADERBOARD_PAGENUMBER = 0;

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void LeaderboardsUIReset();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // UITK
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_inputState;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // Xbox Live objects.
    std::shared_ptr<ATG::LiveResources>         m_liveResources;
    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;
    std::unique_ptr<ATG::PlayFabResources>      m_playFabResources;

    XTaskQueueHandle                            m_mainAsyncQueue;

    // cached UI elements
    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;
    std::shared_ptr<ATG::UITK::UIButton> m_getLeaderboardButton;
    std::shared_ptr<ATG::UITK::UIButton> m_getEntityIDButton;
    std::shared_ptr<ATG::UITK::UIButton> m_getFriendLeaderboardButton;
    std::shared_ptr<ATG::UITK::UIButton> m_prevLeaderboardButton;
    std::shared_ptr<ATG::UITK::UIButton> m_nextLeaderboardButton;
    std::shared_ptr<ATG::UITK::UIButton> m_prevRankingsButton;
    std::shared_ptr<ATG::UITK::UIButton> m_nextRankingsButton;
    std::string normalButtonStyle = "Basic_Button_Style";
    std::shared_ptr<ATG::UITK::UIElement> m_leaderboardsName;
    std::shared_ptr<ATG::UITK::UIStaticText> m_leaderboardsContentLabel;
    std::shared_ptr<ATG::UITK::UIStaticText> m_leaderboardsScoreLabel1;
    std::shared_ptr<ATG::UITK::UIStaticText> m_leaderboardsScoreLabel2;
    std::shared_ptr<ATG::UITK::UIStaticText> m_leaderboardsMetadataLabel;
    std::shared_ptr<ATG::UITK::UIStaticText> m_leaderboardsLastUpdateLabel;
    std::shared_ptr<ATG::UITK::UIStaticText> m_leaderboardsNameLabel;
    std::string m_entityId;
    std::string m_leaderboardName = LEADERBOARD_NAME1;

    uint32_t     m_leaderboardPagenumber = LEADERBOARD_PAGENUMBER;
    uint32_t     m_leaderboardNumber = LEADERBOARD_NUMBER;
    LeaderboardType m_leaderboardType = LeaderboardType::Default;
    bool    m_isResultLoaded = true;

    PFEntityKey entityKey{};

    enum Descriptors
    {
        // TODO: Put your static descriptors here
        Reserve,
        Count = 32,
    };
};
