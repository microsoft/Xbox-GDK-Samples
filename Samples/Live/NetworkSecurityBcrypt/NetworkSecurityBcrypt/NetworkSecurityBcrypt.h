//--------------------------------------------------------------------------------------
// NetworkSecurityBCrypt.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "LiveResources.h"
#include "LiveInfoHUD.h"
#include "StepTimer.h"
#include "UITK.h"

#include "Debug.h"
#include "AsyncStatusWidget.h"

#include "DtlsSocket.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify, public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);
    void InitializeTaskQueue();
    void InitializeLiveResources();
    void InitializeUIEventHandlers();

    // Basic render loop
    void Tick();
    void PumpTaskQueue();

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

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

    template<typename ... Args>
    void Log(const std::string& format, Args ... args) const
    {
        auto logLine = DebugWrite(format.c_str(), args ...);

        if (m_consoleWindow)
        {
            std::vector<std::string> tokens;
            std::string intermediate;
            std::stringstream check1(logLine);

            while (getline(check1, intermediate, '\n'))
            {
                tokens.emplace_back(intermediate);
            }

            for (const std::string& str : tokens)
            {
                m_consoleWindow->AppendLineOfText(str);
            }
        }
    }

    void OnOpenConnectionButtonPressed();
    void GetExpectedIdentityString_OpenConnection();
    void OpenConnection();

    void OnAcceptConnectionsButtonPressed();
    void GetExpectedIdentityString_AcceptConnections();

private:
    // Cleanup
    void CleanupTaskQueue();

    // My Task Queue
    XTaskQueueHandle m_taskQueue = nullptr;
    XTaskQueueRegistrationToken m_taskQueueRegToken;

    XNetworkingConnectivityHint m_connectivityHint;
    HRESULT CreateLocalSocket();
    void CheckForNetworkInitialization();
    void HandleNetworkInitializationComplete();

    void LoginToXboxLive(bool silentAuth = true);
    void OnXboxLiveLoginComplete();

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void PrintLocalIdentityString();

    void OnSendMessageButtonPressed();
    void OnCloseConnectionButtonPressed();

    //void ToggleTargetDevice();

    // Networking
    std::unique_ptr<ATG::DtlsSocket>            m_socket{ nullptr };
    ATG::DtlsConnectionHandle                   m_connection{ nullptr };

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

    // cached UI elements
    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;
    std::shared_ptr<ATG::UITK::UIButton> m_openConnectionButton;
    std::shared_ptr<ATG::UITK::UIButton> m_sendMessageButton;
    std::shared_ptr<ATG::UITK::UIButton> m_closeConnectionButton;
    std::shared_ptr<ATG::UITK::UIButton> m_exitButton;

    std::unique_ptr<AsyncOpWidget> m_asyncOpWidget;

    uint16_t m_port;
    std::string m_ipAddr;
    std::string m_expectedIdentityString;

    enum Descriptors
    {
        Reserve,
        Count = 32,
    };
};
