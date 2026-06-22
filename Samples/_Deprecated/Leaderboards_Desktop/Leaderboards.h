//--------------------------------------------------------------------------------------
// Leaderboards_Desktop.h
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

const int c_sampleUIPanel = 2000;
const int c_statsMaze = 2101;
const int c_statsCave = 2102;
const int c_statsVoid = 2103;
const int c_statsMuseum = 2104;

const int c_ldrMaze = 2201;
const int c_ldrCave = 2202;
const int c_ldrVoid = 2203;
const int c_ldrMuseum = 2204;
const int c_ldrItems = 2205;
const int c_ldrLost = 2206;

const int c_play = 2301;

const int c_modeLabel = 2004;
const int c_pageTitleLabel = 2005;
const int c_leaderboardType = 2006;

struct LeaderboardsQueryContext
{
    XAsyncBlock async;
    XblLeaderboardResult *result;
    std::vector<uint8_t> resultData;
    class Sample *sample;
    int page = 0;
    uint32_t maxItems = 25;
    std::function<void(HRESULT, LeaderboardsQueryContext *)> onResultsReceived;
};

enum SamplePage : uint32_t
{
    QueryLeaderboards = 0,
    QueryStats = 1,
    PageCount = 2
};


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

private:
    // Sample core methods
    void PlayGame();

    void QueryStatistics(const std::string & statName);

    void QueryLeaderboards(
        const std::string &leaderboardName,
        const std::string &statName,
        XblSocialGroupType queryGroupType,
        const char **additionalColumns = nullptr,
        size_t additionalColumnsCount = 0,
        std::function<void(HRESULT, LeaderboardsQueryContext *)> onResultsReceived = {});

    static void ProcessLeaderboardResults(XAsyncBlock * async);

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void SetupUI();

    void WriteToLog(std::string text, bool clearFirst = true);
    void WriteToLog(std::wstring text, bool clearFirst = true);
    void WriteToResult(std::string text, bool clearFirst = true);
    void WriteToResult(std::wstring text, bool clearFirst = true);
    void WriteTo(std::wstring text, bool clearFirst, DX::TextConsoleImage *console);

    void SetPage(SamplePage page);
    void NextPage();
    void PrevPage();
    void RenderStatisticsResult(XblUserStatisticsResult *res);
    void RenderLeaderboardsResults(HRESULT hr, LeaderboardsQueryContext * ctx);

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
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // Xbox Live objects.
    std::shared_ptr<ATG::LiveResources>         m_liveResources;

    std::unique_ptr<ATG::LiveInfoHUD>           m_liveInfoHUD;
    
    std::unique_ptr<DX::TextConsoleImage>       m_glbConsole;
    std::unique_ptr<DX::TextConsoleImage>       m_logConsole;

    XTaskQueueHandle                            m_mainAsyncQueue;

    std::unique_ptr<ATG::UIManager>             m_ui;

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
