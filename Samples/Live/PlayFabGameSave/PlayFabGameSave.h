//--------------------------------------------------------------------------------------
// PlayFabGameSave.h
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

#include <playfab/core/PFLocalUser_Xbox.h>
#include <playfab/core/PFServiceConfig.h>
#include <playfab/core/PFCore.h>
#include <playfab/services/PFServices.h>
#include <playfab/gamesave/PFGameSaveFiles.h>

#include <random>

// PlayFab Title Id
constexpr const char* PLAYFAB_TITLE_ID = ""; // Please set this value to your own titleId from PlayFab Game Manager


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
#ifdef _GAMING_XBOX
    void OnConstrained() {}
    void OnUnConstrained() {}
#endif
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
#ifdef _GAMING_XBOX
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }
#endif

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

#define DEBUG_BUFFER_SIZE 8192
    void Log(const char* format, ...)
    {
        char msgbuffer[DEBUG_BUFFER_SIZE];
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

    void Update(DX::StepTimer const& timer);
    void Render();
    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // UI initialization
    void InitializeUI();

    // Button event handlers
    void OnPressedAddUserButton();
    void OnPressedDataSaveButton();
    void OnPressedDataLoadButton();
    void OnPressedDataDeleteButton();
    void OnPressedSetDescriptionButton();
    void OnPressedUploadReleaseDeviceButton();
    void OnPressedUploadKeepActiveButton();

    // PlayFab and Game Save helpers
    void PlayFabSignIn();
    HRESULT InitializePlayFab();
    HRESULT InitializeGameSaves();
    void UninitializeGameSaves();
    HRESULT AddUserToGameSaves();
    HRESULT EnsurePersistentLocalUser();
    void CleanupPersistentLocalUser();
    void RegisterActiveDeviceChangedCallback();

    // Game Save operations
    void DoSave();
    void DoLoad();
    void DoDelete();
    void DoUpload(bool keepDeviceActive);
    void DoSetSaveDescription();

    // UI state management
    void SetButtonsEnabled(bool enabled);
    void SetDataButtonsEnabled(bool enabled);
    void ResetToAddUserState();

    // Device resources
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // DirectXTK objects
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    enum Descriptors
    {
        Reserve,
        Count = 32,
    };

    // Input devices
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // Xbox Live objects
    std::shared_ptr<ATG::LiveResources>         m_liveResources;
    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;

    // PlayFab objects
    std::unique_ptr<ATG::PlayFabResources>      m_playFabResources;
    PFLocalUserHandle                           m_localUser{ nullptr };
    PFServiceConfigHandle                       m_localServiceConfig{ nullptr };

    // Async queue
    XTaskQueueHandle                            m_mainAsyncQueue;
    XTaskQueueRegistrationToken                 m_activeDeviceChangedToken{};

    // Game Save state
    std::string                                 m_saveRoot;
    int64_t                                     m_remainingQuota{ 0 };
    bool                                        m_userAddedToGS{ false };
    bool                                        m_keepDeviceActiveOnUpload{ false };

    // Random number generator
    std::mt19937                                m_rng;

    // UITK
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_inputState;

    // UI elements
    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;
    std::shared_ptr<ATG::UITK::UIButton>        m_AddUserButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_DataSaveButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_DataLoadButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_DataDeleteButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_SetDescriptionButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_UploadReleaseDeviceButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_UploadKeepActiveButton;
};
