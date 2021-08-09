//--------------------------------------------------------------------------------------
// Social.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "LiveResources.h"
#include "LiveInfoHUD.h"
#include "SampleGUI.h"
#include "StepTimer.h"
#include "TextConsole.h"
#include "UserRepeater.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

private:
    void SetupUI();
    void SetActivePage(int page);
    void RefreshUserList();

    bool HandleInput(DX::StepTimer const& timer);
    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // SocialManager
    void AddUserToSocialManager(_In_ XUserHandle user);
    void RemoveUserFromSocialManager(_In_ XUserHandle user);
    void DisplayUsersAffected(XblSocialManagerUser* const* users);
    uint64_t GetXuidForXUserHandle(XUserHandle user);

    void CreateSocialGroupFromList(
        _In_ XUserHandle,
        _In_ std::vector<uint64_t>& xuidList);

    void CreateSocialGroupFromFilters(
        _In_ XUserHandle user,
        _In_ XblPresenceFilter extraDetailLevel,
        _In_ XblRelationshipFilter filter);

    void DestroySocialGroup(_In_ XblSocialManagerUserGroup*);

    void UpdateSocialManager();

    enum friendListType
    {
        allFriends,
        allOnlineFriends,
        allOnlineInTitleFriends,
        allFavorites,
        custom
    };

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // Xbox Live objects.
    std::shared_ptr<ATG::LiveResources>         m_liveResources;
    XTaskQueueHandle                            m_asyncQueue;

    std::mutex                                  m_socialManagerLock;
    std::vector<XblSocialManagerUserGroup*>     m_userSocialGroups;
    XblSocialManagerUserPtrArray                m_userList;

    // UI Objects
    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;
    std::shared_ptr<ATG::UIManager>             m_ui;
    std::unique_ptr<DX::TextConsoleImage>       m_console;
    std::unique_ptr<UserRepeater>               m_userRepeater;

    enum Descriptors
    {
        Font,
        ConsoleFont,
        Background,
        ConsoleBackground,
        Reserve,
        Count = 32,
    };

    // UI State
    friendListType                              m_selectedFriendList;
};
