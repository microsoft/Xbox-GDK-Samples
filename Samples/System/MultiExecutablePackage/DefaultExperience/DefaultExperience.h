//--------------------------------------------------------------------------------------
// DefaultExperience.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "UITK.h"
#include "UserManager.h"
#include "UtilityProcessManager.h"

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
    void RegisterUIEventHandlers();

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

    // UI
    void Log(const std::string text);
    void LogFailedHR(HRESULT hr, const std::string functionName = "");

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    HRESULT LoadDll();
    HRESULT RunUtilityProcess();
    void XLaunchOtherGame();

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_deviceResources->GetCommandList(); }

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

    // External resources
    HMODULE                                     m_dllModuleHandle;

    // UITK
    std::mutex                                  m_logMutex;
    std::vector<std::string>                    m_logQueue;
    ATG::UITK::UIManager                        m_uiManager;
    ATG::UITK::UIInputState                     m_inputState;
    std::shared_ptr<ATG::UITK::UIButton>        m_loadDllButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_utilityExeButton;
    std::shared_ptr<ATG::UITK::UIConsoleWindow> m_consoleWindow;
    std::shared_ptr<ATG::UITK::UIButton>        m_gameExeButton;
    std::shared_ptr<ATG::UITK::UIButton>        m_switchUserButton;
    std::shared_ptr<ATG::UITK::UIImage>         m_gamerpicImage;
    std::shared_ptr<ATG::UITK::UIStaticText>    m_gamertagText;
    std::string                                 m_currentGamerPicGamerTag;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    std::unique_ptr<UserManager>                m_userManager;
    std::unique_ptr<UITKLogManager>             m_logger;

    std::unique_ptr<UtilityProcessManager>      m_utilityProcessManager;
};
