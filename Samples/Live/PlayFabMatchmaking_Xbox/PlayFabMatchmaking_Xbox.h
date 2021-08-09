//--------------------------------------------------------------------------------------
// PlayFabMatchmaking_Xbox.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UITK.h"
#include "LiveInfoHUD.h"
#include "LiveResources.h"
#include "ILoggingInterface.h"
#include "PlayFabMatchmakingManager.h"
#include "AsyncStatusWidget.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : 
    public ATG::UITK::D3DResourcesProvider, public ILoggingInterface
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
    void InitializeUIEventHandlers();
    void InitializeTaskQueue();
    void InitializeXboxLive();

    // Cleanup
    void Cleanup();
    void CleanupTaskQueue();
    void CleanupXboxLive();

    // Basic render loop
    void Tick();
    void PumpTaskQueue();

    // Messages
    void OnSuspending();
    void OnResuming();

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

private:
    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-security"
    #endif

    template<typename ... Args>
    void Log(const std::string& format, Args ... args) const 
    {
        auto expectedSize = snprintf(nullptr, size_t(0), format.c_str(), args ...) + 1; // Extra space for '\0'
        if (expectedSize <= 0) { throw std::runtime_error("Error during formatting."); }
        if (expectedSize > 1023) { throw std::runtime_error("Excessively long log line."); }

        char buf[1024] = {};
        snprintf(buf, size_t(1024), format.c_str(), args ...);
        auto logLine = std::string(buf); 

        AppendLineOfText(logLine);
    }

    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif

    void AppendLineOfText(const std::string& lineOfText) const override
    {
        if (m_consoleWindow)
        {
            m_consoleWindow->AppendLineOfText(lineOfText);
        }
    }

private:
    // My Task Queue
    XTaskQueueHandle m_theTaskQueue;
    XTaskQueueRegistrationToken m_taskQueueRegToken;

private:
    // step: wait for network readiness
    XNetworkingConnectivityHint m_connectivityHint;
    void CheckForNetworkInitialization();
    void HandleNetworkInitializationComplete();

    // step: log into XboxLive
    std::shared_ptr<ATG::LiveResources> m_liveResources;
    void LoginToXboxLive(bool silentAuth = true);
    void HandleXboxLiveLoginComplete();

    // step: get an Xbox user token
    std::string m_userToken;
    void RequestLiveUserToken();
    void HandleLiveUserTokenRequestComplete();

    // step: log into PlayFab
    std::string m_playFabId;
    std::string m_entityId;
    std::string m_entityType;
    std::string m_entityToken;
    void LoginToPlayFab();
    void HandlePlayFabLoginComplete();

    // step: get the latency to the region data center
    int m_westUsRegionLatency;
    int m_eastUsRegionLatency;
    bool HasRegionLatencies() { return m_westUsRegionLatency != -1 && m_eastUsRegionLatency != -1; }
    void GetLatencyToRegions();
    int PingServerUrl(const char* serverUrl, int serverPort);
    void HandleRegionLatencyComplete();

    // step: find a match using region-rule-based matchmaking
    void DoPlayFabMatchMake(bool simple);
    void HandlePlayFabMatchMakeComplete();

    // step: cancel the matchmaking?
    void CancelMatchmaking();
    void HandleMatchingCancelled();

private:
    // cached UI elements
    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;
    std::shared_ptr<ATG::UITK::UIStaticText> m_matchLevelText;
    std::shared_ptr<ATG::UITK::UISlider> m_matchLevelSlider;
    std::shared_ptr<ATG::UITK::UIButton> m_findSimpleMatchButton;
    std::shared_ptr<ATG::UITK::UIButton> m_getRegionLatenciesButton;
    std::shared_ptr<ATG::UITK::UIButton> m_findRegionMatchButton;
    std::shared_ptr<ATG::UITK::UIButton> m_cancelMatchmakingButton;
    std::shared_ptr<ATG::UITK::UIButton> m_exitButton;

    std::unique_ptr<AsyncOpWidget> m_asyncOpWidget;
    
    std::unique_ptr<ATG::LiveInfoHUD> m_liveInfoHUD;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

private:
    std::unique_ptr<PlayFabMatchmakingManager>  m_matchmakingManager;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // UITK
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_inputState;

    // Input devices.
    DirectX::GamePad::State                     m_previousGamePadState;
    std::unique_ptr<DirectX::GamePad>           m_gamePad;

    // DirectXTK objects.
    enum Descriptors
    {
        General,
        Reserve,
        Count = 32,
    };

    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;
};
