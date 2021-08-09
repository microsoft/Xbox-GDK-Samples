//--------------------------------------------------------------------------------------
// mDNS.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SampleGUI.h"
#include "UIConstants.h"
#include "SampleGUI.h"
#include "FindMedia.h"
#include "TextConsole.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

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
    void GetDefaultSize( int& width, int& height ) const;

    void SetDNSServiceInstance(PDNS_SERVICE_INSTANCE pInstance);
    void SetWaitEvent()
    {
        SetEvent(m_waitEvent);
    }

    bool Resolve(PWSTR targetToResolve);
    std::unique_ptr<DX::TextConsoleImage>       m_console;
private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

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
    std::shared_ptr<ATG::UIManager>             m_ui;

    //tmp for debugging
    std::string m_ExePath = "";

    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // Sample functions
    void RegisterDNS();
    void DeRegisterDNS();
    bool BrowseDNS();
    DNS_STATUS StopBrowseDNS();

    HANDLE m_waitEvent;

    // Handle for our registered DNS service
    PDNS_SERVICE_INSTANCE m_dnsServiceInstance;
    DNS_SERVICE_CANCEL m_browseCancel; // Handle for canceling current dns browse
};
