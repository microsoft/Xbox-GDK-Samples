//--------------------------------------------------------------------------------------
// InGameChat.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UserManager.h"
#include "SampleGUI.h"
#include "TextConsole.h"
#include "UserRepeater.h"
#include "XboxLiveManager.h"
#include "GameChatManager.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    static const int MAXUSERS = 8;
    static Sample* Instance() { return s_instance; }

    void LogToConsole(const char* message);

    UserManager* GetUserManager() { return m_userManager.get(); }
    XboxLiveManager* GetLiveManager() { return m_liveManager.get(); }
    GameChatManager* GetChatManager() { return m_chatManager.get(); }

    enum class SampleState
    {
        LocalLobby,
        JoiningSession,
        JoinLobby,
        SearchingForSession,
        SearchComplete,
        ChatLobby,
        LeavingSession
    };

    SampleState GetState() { return m_state; }
    void LeaveChatSession();
    void LocalUserAdded(XUserHandle user);
    void JoinFriend(const char* handle);
    void ReturnToStart();

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void SetupUI();
    void ChangeChannel(int offset);
    void LocalLobbyUpdate();
    void SessionSearchComplete();
    void JoinLobbyUpdate();
    void ChatLobbyUpdate();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // UI
    std::shared_ptr<ATG::UIManager>             m_ui;
    std::unique_ptr<DX::TextConsoleImage>       m_console;
    std::unique_ptr<UserRepeater>               m_userList;
    std::unique_ptr<UserRepeater>               m_chatList;
    std::unique_ptr<UserRepeater>               m_playerList;

    // Managers
    std::unique_ptr<UserManager>                m_userManager;
    std::unique_ptr<XboxLiveManager>            m_liveManager;
    std::unique_ptr<GameChatManager>            m_chatManager;

    XTaskQueueRegistrationToken                 m_inviteRegistration;
    std::vector<std::string>                    m_joinableSessions;
    std::string                                 m_joinHandle;
    static Sample*                              s_instance;
    SampleState                                 m_state;

    enum Descriptors
    {
        Font,
        Background,
        Reserve,
        Count = 96,
    };
};

// Helper for output debug tracing
inline void DebugTrace(_In_z_ _Printf_format_string_ const char* format, ...)
{
#ifdef _DEBUG
    va_list args;
    va_start(args, format);

    char buff[1024] = {};
    vsprintf_s(buff, format, args);
    Sample::Instance()->LogToConsole(buff);
    va_end(args);
#else
    UNREFERENCED_PARAMETER(format);
#endif
}
