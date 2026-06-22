//--------------------------------------------------------------------------------------
// Achievements.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "LiveResources.h"
#include "LiveInfoHUD.h"
#include "StepTimer.h"
#include "SampleGUI.h"
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

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Update the progress towards an achievement
    void UpdateAchievement(const std::string& achievementId, uint32_t percentComplete);
    // Get a specific achievement by id
    void GetAchievement(const std::string& achievementId);
    // Get all achievements
    void GetAchievements();

    void SetupUI();

    XblContextHandle GetXblContext() { return m_liveResources->GetLiveContext(); }
    uint64_t GetXuid() { return m_liveResources->GetXuid(); }
    XUserHandle GetUser() { return m_liveResources->GetUser(); }
    uint32_t GetTitleId() { return m_liveResources->GetTitleId(); }
    const char * GetServiceConfigId() { return m_liveResources->GetServiceConfigId().data(); }

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
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // Xbox Live objects.
    std::shared_ptr<ATG::LiveResources>         m_liveResources;

    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;

    XTaskQueueHandle                            m_mainAsyncQueue;

    // UI Objects
    std::unique_ptr<ATG::UIManager>             m_ui;
    std::unique_ptr<DX::TextConsoleImage>       m_log;
    std::unique_ptr<DX::TextConsoleImage>       m_display;

    enum Descriptors
    {
        Font,
        ConsoleFont,
        Background,
        ConsoleBackground,
        Reserve,
        Count = 32,
    };
};
