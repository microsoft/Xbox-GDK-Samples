//--------------------------------------------------------------------------------------
// SocialManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include <atomic>
#include <map>

#include "DeviceResources.h"
#include "LiveResources.h"
#include "LiveInfoHUD.h"
#include "StepTimer.h"
#include "SampleGUI.h"
#include "TextConsole.h"
#include "UserRepeater.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
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

private:
    void SetupUI();
    void SetActivePage(int page);
    void RefreshUserList();

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

    void LoadGamerPics(uint64_t* xuids, size_t count);

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
    XTaskQueueHandle                            m_asyncQueue;

    std::mutex                                  m_socialManagerLock;
    std::vector<XblSocialManagerUserGroup*>     m_userSocialGroups;
    XblSocialManagerUserPtrArray                m_userList;

    // UI Objects
    std::shared_ptr<ATG::UIManager>             m_ui;
    std::unique_ptr<DX::TextConsoleImage>       m_console;
    std::unique_ptr<UserRepeater>               m_userRepeater;

    std::map<uint64_t, std::pair<uint8_t*, size_t>>     m_profilePicCache;
    size_t                                              m_profilesToLoadCount;
    std::atomic<size_t>                                 m_profilesLoadedCount;

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

struct PictureData
{
    uint64_t        xuid;
    HCCallHandle   httpHandle;
    std::map<uint64_t, std::pair<uint8_t*, size_t>>* profileCache;
    std::atomic<size_t>* profilesLoadedCount;

    PictureData(uint64_t x, HCCallHandle handle, std::map<uint64_t, std::pair<uint8_t*, size_t>>* cache, std::atomic<size_t>* loadCount) :
        xuid(x),
        httpHandle(handle),
        profileCache(cache),
        profilesLoadedCount(loadCount)
    {
    }
};
