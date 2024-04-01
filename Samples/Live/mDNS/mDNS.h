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
#include "FindMedia.h"
#include "TextConsole.h"

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
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
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

namespace
{
    const int c_sampleUIPanel = 2000;

    const int c_twistElementStart = 3001;

    const int c_previousButton = 4901;
    const int c_previousImage = 4911;
    const int c_nextButton = 4903;
    const int c_nextImage = 4913;

    const int c_item00 = 4000;
    const int c_item01 = 4100;
    const int c_item02 = 4200;
    const int c_item10 = 4300;
    const int c_item11 = 4400;
    const int c_item12 = 4500;
    const int c_item20 = 4600;
    const int c_item21 = 4700;
    const int c_item22 = 4800;

    const int c_imageOffset = 1;
    const int c_textOffest = 2;
    const int c_itemOffset = 100;
    const int c_rowOffset = 300;

    const int c_productTitle = 5000;
    const int c_productDescription = 5001;
    const int c_priceLabel = 5002;
    const int c_includedIn = 5005;
    const int c_bundleName = 5006;
    const int c_actionResult = 5007;
    const int c_posterImage = 5010;

    const int c_popupPanel = 6000;
    const int c_popupLabel = 6001;
    const int c_popupButton = 6002;

    const int c_pageSize = 9;
}
