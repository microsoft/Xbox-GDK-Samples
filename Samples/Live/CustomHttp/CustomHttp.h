//--------------------------------------------------------------------------------------
// CustomHttp.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UITK.h"
#include "Debug.h"

#pragma warning( disable : 4100 )

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
    void OnConstrained() {}
    void OnUnConstrained() {}
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

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

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // libcurl lifecycle
    void WaitForNetworkInitialization();
    void InitializeCurl();
    void CleanupCurl();

    // Proxy and certificate helpers
    void ResolveAndCacheProxy();
    void LogDebugCertificates();

    // Button handlers
    void OnHttpRequestButtonPressed();

    // Per-frame curl_multi pump
    void PumpCurlMulti();

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

    // libcurl state
    bool                                        m_curlInitialized = false;
    CURLM*                                      m_curlMulti = nullptr;
    std::vector<CURL*>                          m_activeRequests;
    std::string                                 m_proxyAddress;

    // UI elements
    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;
    std::shared_ptr<ATG::UITK::UIButton>        m_httpRequestButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_exitButton;

    enum Descriptors
    {
        Reserve,
        Count = 32,
    };

    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;
};

