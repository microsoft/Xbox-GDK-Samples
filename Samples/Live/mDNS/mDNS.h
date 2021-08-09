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
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    // Properties
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
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // UI objects.
    std::shared_ptr<ATG::UIManager>             m_ui;

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
