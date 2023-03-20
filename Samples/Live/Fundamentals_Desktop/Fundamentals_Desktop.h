//--------------------------------------------------------------------------------------
// Fundamentals_Desktop.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SampleGUI.h"
#include "LiveInfoHUD.h"
#include "UIConstants.h"
#include "TextConsole.h"

class Sample;

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

    // Basic game loop
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
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    // Sample Functions
    void ConsoleWriteLine(const char* format, ...);
    void DisplayHResult(HRESULT hr, const char *strName);

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void InitializeXBL();

    void CheckForUpdates();
    void DownloadUpdates();
    void OnUpdateResult(HRESULT hr);

    void TryAddUserSilently();
    void AddUserWithUI();
    void HandleAddUserSuccess(XUserHandle user);

    void Clear();

    void PerformLicenseCheck();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // UI objects.
	std::unique_ptr<DX::TextConsoleImage>       m_console;
    std::shared_ptr<ATG::UIManager>             m_ui;

    XTaskQueueHandle                            m_asyncQueue;

    // --- Live Info HUD Start ---
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;
    // --- Live Info HUD End ---
    
    // --- Live Info HUD Start ---
    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;
    // --- Live Info HUD End ---

    //tmp for debugging
    std::string m_ExePath = "";

    XUserHandle                                 m_CurrentUserHandle;
    XStoreContextHandle                         m_xStoreContext;

    std::vector<XStorePackageUpdate>            m_updates;
};
