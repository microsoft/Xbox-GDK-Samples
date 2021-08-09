//--------------------------------------------------------------------------------------
// SimpleWebSockets.h
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

#include "httpClient/httpClient.h"

enum class WebSocketStatus
{
    Uninitialized,
    Connected,
    Closed,
    Error
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
            m_consoleWindow->AppendLineOfText(logLine);
        }
    }

    XTaskQueueHandle m_taskQueue = nullptr;

private:
    XNetworkingConnectivityHint m_connectivityHint;
    void CheckForNetworkInitialization();
    void HandleNetworkInitializationComplete();

    void CleanupTaskQueue();
    XTaskQueueRegistrationToken m_taskQueueRegToken;

    void LoginToXboxLive(bool silentAuth = true);
    void OnXboxLiveLoginComplete();

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
    std::shared_ptr<ATG::UITK::UIButton> m_connectWebSocketButton;
    std::shared_ptr<ATG::UITK::UIButton> m_closeWebSocketButton;
    std::shared_ptr<ATG::UITK::UIButton> m_sendMessageButton;
    std::shared_ptr<ATG::UITK::UIButton> m_sendBinaryMessageButton;
    std::shared_ptr<ATG::UITK::UIButton> m_exitButton;
    std::unique_ptr<AsyncOpWidget> m_asyncOpWidget;

    enum Descriptors
    {
        Reserve,
        Count = 32,
    };

public:
    // WebSocket sample logic
    HCWebsocketHandle m_webSocket = nullptr;
    WebSocketStatus m_lastSocketStatus = WebSocketStatus::Uninitialized;

    void CleanUpWebSocket();

    void OnConnectButtonPressed();
    void OnWebSocketConnected();

    void OnSendMessageButtonPressed();
    void SendMessageToWebSocket(const char* buffer, uint32_t length);

    uint32_t m_testNum = 0;
    void OnSendBinaryMessageButtonPressed();
    void SendBinaryMessageToWebSocket(const char* buffer, uint32_t length);

    void OnCloseButtonPressed();
    void OnWebSocketClosed();
};

void message_received(HCWebsocketHandle websocket, const char* incomingBodyString, void* functionContext);
void binary_message_received(HCWebsocketHandle websocket, const uint8_t* payloadBytes, uint32_t payloadSize, void* functionContext);
void websocket_closed(HCWebsocketHandle websocket, HCWebSocketCloseStatus closeStatus, void* functionContext);

struct MessageReceivedContext
{
    Sample* samplePtr = nullptr;
    std::string messageBodyString;
};

struct BinaryMessageReceivedContext
{
    Sample* samplePtr = nullptr;
    std::vector<uint8_t> payload;
};
