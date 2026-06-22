//--------------------------------------------------------------------------------------
// LeaderboardsSetup.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// SAMPLE NOTE:
// The sample class is split between two implementation files to allow one file
// to focus on the statistics and leaderboard calls specifically while the other
// handles rendering boilerplate and UI interactions.
//
// This is the boilerplate and UI file
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Leaderboards.h"
#include "StringUtil.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "DirectXHelpers.h"

extern void ExitSample();

using namespace DirectX;
using namespace DX;

using Microsoft::WRL::ComPtr;

struct UIPage
{
    UIPage() : buttons() {}
    std::map<int, ATG::Button *> buttons;

    void SetActive(bool enable)
    {
        for (auto &item : buttons)
        {
            auto button = item.second;
            button->SetEnabled(enable);
            button->SetVisible(enable);
        }
    }
};

namespace {
    uint32_t s_activePage = 0;

    std::vector<UIPage> s_uiPages = { UIPage(), UIPage() };
    std::map<int, ATG::TextLabel *> s_labels = {};
    ATG::IPanel *s_mainPanel = nullptr;

    bool s_isLeaderboardGlobal = true;

    const int c_pageTitleText = 0;
    const int c_pageDescText = 1;

    const wchar_t *s_labelsText[][2]
    {
        { L"Query Leaderboards", L"Query current leaderboards for the given stat."},
        { L"Query Stats", L"Query a specific stat for the current user." }
    };
}

void Sample::SetPage(SamplePage page)
{
    assert(page < SamplePage::PageCount);

    for (uint32_t i = 0; i < (uint32_t)SamplePage::PageCount; i++)
    {
        s_uiPages[i].SetActive((uint32_t)page == i);
    }
    s_activePage = page;

    s_labels[c_pageTitleText]->SetText(s_labelsText[page][c_pageTitleText]);
    s_labels[c_pageDescText]->SetText(s_labelsText[page][c_pageDescText]);

    if (s_activePage == SamplePage::QueryLeaderboards)
    {
        s_labels[c_leaderboardType]->SetText(s_isLeaderboardGlobal ? L"< GLOBAL >" : L"< SOCIAL >");
    }
    else
    {
        s_labels[c_leaderboardType]->SetText(L"");
    }

    s_mainPanel->Cancel();
    s_mainPanel->Show();
}

void Sample::NextPage()
{
    s_activePage = (++s_activePage) % SamplePage::PageCount;
    SetPage((SamplePage)s_activePage);
}

void Sample::PrevPage()
{
    --s_activePage;
    if (s_activePage > SamplePage::PageCount) { s_activePage = SamplePage::PageCount - 1; }
    SetPage((SamplePage)s_activePage);
}

static void FormatLeaderboardHeader(std::stringstream &ss, const XblLeaderboardResult *res)
{
    ss << std::setw(8) << "Rank"; ss << std::setw(0) << " |";
    ss << std::setw(16) << "Gamer Tag";

    for (size_t i = 0; i < res->columnsCount; i++)
    {
        ss << std::setw(0) << " |";
        ss << std::setw(i == 0 ? 32 : 18) << res->columns[i].statName;
    }
    ss << std::endl;
}

static void FormatLeaderboardRows(std::stringstream &ss, const XblLeaderboardResult *res)
{
    for (size_t i = 0; i < res->rowsCount; i++)
    {
        ss << std::setw(8) << res->rows[i].rank; ss << std::setw(0) << " |";
        ss << std::setw(16) << res->rows[i].gamertag;
        for (size_t j = 0; j < res->rows[i].columnValuesCount; j++)
        {
            ss << std::setw(0) << " |";
            ss << std::setw(j == 0 ? 32 : 18) << res->rows[i].columnValues[j];
        }

        if (i + 1 < res->rowsCount) { ss << std::endl; }
    }
}

static void FormatStatisticsResult(std::stringstream &ss, const XblUserStatisticsResult *res)
{
    for (uint32_t i = 0; i < res->serviceConfigStatisticsCount; i++)
    {
        for (uint32_t j = 0; j < res->serviceConfigStatistics[i].statisticsCount; j++)
        {
            ss << std::setw(20) << res->serviceConfigStatistics[i].statistics[j].statisticName;
            ss << std::setw(12) << res->serviceConfigStatistics[i].statistics[j].statisticType;
            ss << std::setw(12) << res->serviceConfigStatistics[i].statistics[j].value;
            ss << std::endl;
        }
    }
}

void Sample::RenderStatisticsResult(XblUserStatisticsResult *res)
{
    std::stringstream ss;
    FormatStatisticsResult(ss, res);
    WriteToResult(ss.str().c_str());
}

void Sample::RenderLeaderboardsResults(HRESULT hr, LeaderboardsQueryContext *ctx)
{
    if (SUCCEEDED(hr))
    {
        std::stringstream ss;
        if (ctx->page == 0)
        {
            FormatLeaderboardHeader(ss, ctx->result);
        }

        FormatLeaderboardRows(ss, ctx->result);
        WriteToResult(ss.str(), ctx->page == 0);
    }
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_mainAsyncQueue(nullptr)
{
    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_mainAsyncQueue)
    );

    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_liveResources = std::make_shared<ATG::LiveResources>(m_mainAsyncQueue);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("Leaderboards");
    m_glbConsole = std::make_unique<DX::TextConsoleImage>();
    m_logConsole = std::make_unique<DX::TextConsoleImage>();

    ATG::UIConfig uiconfig;
    m_ui = std::make_unique<ATG::UIManager>(uiconfig);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    if (m_mainAsyncQueue)
    {
        XTaskQueueCloseHandle(m_mainAsyncQueue);
        m_mainAsyncQueue = nullptr;
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
        {
            m_liveInfoHUD->SetUser(user, m_liveResources->GetAsyncQueue());
            m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Show();
        });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
        {
            m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());
            m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Close();
        });

    m_liveResources->SetErrorHandler([this](HRESULT error)
        {
            if (error == E_GAMEUSER_NO_DEFAULT_USER || error == E_GAMEUSER_RESOLVE_USER_ISSUE_REQUIRED)
            {
                m_liveResources->SignInWithUI();
            }
            else // Handle other error cases
            {

            }
        });

    // Before we can make an Xbox Live call we need to ensure that the Game OS has intialized the network stack
    // For sample purposes we block user interaction with the sample.  A game should wait for the network to be
    // initialized before the main menu appears.  For samples, we will wait at the end of initialization.
    while (!m_liveResources->IsNetworkAvailable())
    {
        SwitchToThread();
    }

    m_liveResources->Initialize();
    m_liveInfoHUD->Initialize();

    SetupUI();

    XErrorSetOptions(XErrorOptions::DebugBreakOnError, XErrorOptions::OutputDebugStringOnError);

    XErrorSetCallback([](HRESULT hr, const char *msg, void *context) -> bool
        {
            UNREFERENCED_PARAMETER(context);
            char buf[256];
            sprintf_s(buf, 512, "HRESULT: 0x%08X | %s", static_cast<unsigned int>(hr), msg);
            OutputDebugStringA(buf);
            return true;
        }, this);
}

void Sample::SetupUI()
{
    using namespace ATG;
    m_ui->LoadLayout(L".\\Assets\\SampleUI.csv", L".\\Assets\\");

    s_mainPanel = m_ui->FindPanel<IPanel>(c_sampleUIPanel);

    s_labels.emplace(c_pageTitleText, m_ui->FindControl<TextLabel>(c_sampleUIPanel, c_pageTitleLabel));
    s_labels.emplace(c_pageDescText, m_ui->FindControl<TextLabel>(c_sampleUIPanel, c_modeLabel));
    s_labels.emplace(c_leaderboardType, m_ui->FindControl<TextLabel>(c_sampleUIPanel, c_leaderboardType));

    auto statsMaze = m_ui->FindControl<Button>(c_sampleUIPanel, c_statsMaze);
    statsMaze->SetCallback([this](IPanel*, IControl*)
        {
            WriteToLog(L"** Query Stat: AreaExplored.Environment.Maze\n");
            QueryStatistics("AreaExplored.Environment.Maze");
        });

    auto statsCave = m_ui->FindControl<Button>(c_sampleUIPanel, c_statsCave);
    statsCave->SetCallback([this](IPanel*, IControl*)
        {
            WriteToLog(L"** Query Stat: AreaExplored.Environment.Cave\n");
            QueryStatistics("AreaExplored.Environment.Cave");
        });

    auto statsVoid = m_ui->FindControl<Button>(c_sampleUIPanel, c_statsVoid);
    statsVoid->SetCallback([this](IPanel*, IControl*)
        {
            WriteToLog(L"** Query Stat: AreaExplored.Environment.Void\n");
            QueryStatistics("AreaExplored.Environment.Void");
        });

    auto statsMuseum = m_ui->FindControl<Button>(c_sampleUIPanel, c_statsMuseum);
    statsMuseum->SetCallback([this](IPanel*, IControl*)
        {
            WriteToLog(L"** Query Stat: AreaExplored.Environment.Museum\n");
            QueryStatistics("AreaExplored.Environment.Museum");
        });

    auto & statsPage = s_uiPages[SamplePage::QueryStats];
    statsPage.buttons.emplace(c_statsMaze, statsMaze);
    statsPage.buttons.emplace(c_statsCave, statsCave);
    statsPage.buttons.emplace(c_statsVoid, statsVoid);
    statsPage.buttons.emplace(c_statsMuseum, statsMuseum);

    // Leaderboards

    auto ldrMaze = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrMaze);
    ldrMaze->SetCallback([this](IPanel*, IControl*)
        {
            auto render = [this](HRESULT hr, LeaderboardsQueryContext *ctx) { RenderLeaderboardsResults(hr, ctx); };
            WriteToLog(L"** Query leaderboards for MostTraveled in Maze environment.\n");
            auto lbType = s_isLeaderboardGlobal ? XblSocialGroupType::None : XblSocialGroupType::People;
            QueryLeaderboards("MostTraveledMaze", "AreaExplored.Environment.Maze", lbType, nullptr, 0, render);
        });

    auto ldrCave = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrCave);
    ldrCave->SetCallback([this](IPanel*, IControl*)
        {
            auto render = [this](HRESULT hr, LeaderboardsQueryContext *ctx) { RenderLeaderboardsResults(hr, ctx); };
            WriteToLog(L"** Query leaderboards for MostTraveled in Cave environment.\n");
            auto lbType = s_isLeaderboardGlobal ? XblSocialGroupType::None : XblSocialGroupType::People;
            QueryLeaderboards("MostTraveledCave", "AreaExplored.Environment.Cave", lbType, nullptr, 0, render);
        });

    auto ldrVoid = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrVoid);
    ldrVoid->SetCallback([this](IPanel*, IControl*)
        {
            auto render = [this](HRESULT hr, LeaderboardsQueryContext *ctx) { RenderLeaderboardsResults(hr, ctx); };
            WriteToLog(L"** Query leaderboards for MostTraveled in Void environment.\n");
            auto lbType = s_isLeaderboardGlobal ? XblSocialGroupType::None : XblSocialGroupType::People;
            QueryLeaderboards("MostTraveledVoid", "AreaExplored.Environment.Void", lbType, nullptr, 0, render);
        });

    auto ldrMuseum = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrMuseum);
    ldrMuseum->SetCallback([this](IPanel*, IControl*)
        {
            auto render = [this](HRESULT hr, LeaderboardsQueryContext *ctx) { RenderLeaderboardsResults(hr, ctx); };
            WriteToLog(L"** Query leaderboards for MostTraveled in Museum environment.\n");
            auto lbType = s_isLeaderboardGlobal ? XblSocialGroupType::None : XblSocialGroupType::People;
            QueryLeaderboards("MostTraveledMuseum", "AreaExplored.Environment.Museum", lbType, nullptr, 0, render);
        });

    auto ldrItems = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrItems);
    ldrItems->SetCallback([this](IPanel*, IControl*)
        {
            auto render = [this](HRESULT hr, LeaderboardsQueryContext *ctx) { RenderLeaderboardsResults(hr, ctx); };
            WriteToLog(L"** Query leaderboards for MostFoundItems.\n");
            auto lbType = s_isLeaderboardGlobal ? XblSocialGroupType::None : XblSocialGroupType::People;
            const char *addColumns[] = { "Environment", "DistanceTraveled" };
            QueryLeaderboards("MostItemsFound", "AreaExploredMostFoundItems", lbType, addColumns, _countof(addColumns), render);
        });

    auto ldrLost = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrLost);
    ldrLost->SetCallback([this](IPanel*, IControl*)
        {
            auto render = [this](HRESULT hr, LeaderboardsQueryContext *ctx) { RenderLeaderboardsResults(hr, ctx); };
            WriteToLog(L"** Query leaderboards for LeastTimesLost.\n");
            auto lbType = s_isLeaderboardGlobal ? XblSocialGroupType::None : XblSocialGroupType::People;
            const char *addColumns[] = { "Environment", "DistanceTraveled" };
            QueryLeaderboards("LeastTimesLost", "AreaExploredLeastTimesLost", lbType, addColumns, _countof(addColumns), render);
        });

    auto & ldrPage = s_uiPages[SamplePage::QueryLeaderboards];
    ldrPage.buttons.emplace(c_ldrMaze, ldrMaze);
    ldrPage.buttons.emplace(c_ldrCave, ldrCave);
    ldrPage.buttons.emplace(c_ldrVoid, ldrVoid);
    ldrPage.buttons.emplace(c_ldrMuseum, ldrMuseum);
    ldrPage.buttons.emplace(c_ldrItems, ldrItems);
    ldrPage.buttons.emplace(c_ldrLost, ldrLost);


    SetPage(SamplePage::QueryLeaderboards);
}

void Sample::WriteToLog(std::string text, bool clearFirst)
{
    WriteToLog(DX::Utf8ToWide(text), clearFirst);
}

void Sample::WriteToLog(std::wstring text, bool clearFirst)
{
    WriteTo(text, clearFirst, m_logConsole.get());
}

void Sample::WriteToResult(std::string text, bool clearFirst)
{
    WriteToResult(DX::Utf8ToWide(text), clearFirst);
}

void Sample::WriteToResult(std::wstring text, bool clearFirst)
{
    WriteTo(text, clearFirst, m_glbConsole.get());
}

void Sample::WriteTo(std::wstring text, bool clearFirst, DX::TextConsoleImage * console)
{
    if (clearFirst) { console->Clear(); }
    console->WriteLine(text.c_str());
}

// Scales a rectangle based on a ratio of a "layout" rect to a "current" rect (for resolution independent UI scaling)
static RECT ScaleRect(const RECT &originalDisplayRect, const RECT &displayRect, const RECT &originalSubRect)
{
    RECT outScaledSubRect;
    const float widthScale = ((float)displayRect.right - (float)displayRect.left) / ((float)originalDisplayRect.right - (float)originalDisplayRect.left);
    const float heightScale = ((float)displayRect.bottom - (float)displayRect.top) / ((float)originalDisplayRect.bottom - (float)originalDisplayRect.top);

    outScaledSubRect.top = (LONG)((float)originalSubRect.top * heightScale);
    outScaledSubRect.left = (LONG)((float)originalSubRect.left * widthScale);
    outScaledSubRect.bottom = (LONG)((float)originalSubRect.bottom * heightScale);
    outScaledSubRect.right = (LONG)((float)originalSubRect.right * widthScale);

    return outScaledSubRect;
}


#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_timer.Tick([&]()
        {
            Update(m_timer);
        });

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::RELEASED)
        {
            PlayGame();
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::RELEASED && s_activePage == SamplePage::QueryLeaderboards)
        {
            s_isLeaderboardGlobal = !s_isLeaderboardGlobal;
            SetPage((SamplePage)s_activePage);
        }

        if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::RELEASED)
        {
            NextPage();
        }

        if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::RELEASED)
        {
            PrevPage();
        }

        if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED)
        {
            if (!m_liveResources->IsUserSignedIn())
            {
                m_liveResources->SignInSilently();
            }
            else
            {
                m_liveResources->SignInWithUI();
            }
        }

        m_ui->Update(elapsedTime, pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Tab))
    {
        if (!m_liveResources->IsUserSignedIn())
        {
            m_liveResources->SignInSilently();
        }
        else
        {
            m_liveResources->SignInWithUI();
        }
    }

#ifdef _GAMING_DESKTOP
    m_ui->Update(elapsedTime, *m_mouse, *m_keyboard);
#endif

    // Process any completed tasks
    while (XTaskQueueDispatch(m_mainAsyncQueue, XTaskQueuePort::Completion, 0))
    {
    }

    PIXEndEvent();
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());
    m_liveInfoHUD->Render(commandList);

    m_logConsole->Render(commandList);
    m_glbConsole->Render(commandList);
    m_ui->Render(commandList);

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto viewport = m_deviceResources->GetScreenViewport();
    auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnActivated()
{
}

void Sample::OnDeactivated()
{
}

void Sample::OnSuspending()
{
    m_deviceResources->Suspend();
}

void Sample::OnResuming()
{
    m_deviceResources->Resume();
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_liveResources->Refresh();
    m_ui->Reset();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const
{
    width = 1920;
    height = 1080;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    wchar_t font[260];
    wchar_t background[260];

    DX::FindMediaFile(font, 260, L"courier_16.spritefont");
    DX::FindMediaFile(background, 260, L"ATGConsoleBlack.DDS");

    m_logConsole->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        font,
        background,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ConsoleFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ConsoleFont),
        m_resourceDescriptors->GetCpuHandle(Descriptors::ConsoleBackground),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ConsoleBackground)
    );

    m_glbConsole->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        font,
        background,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ConsoleFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ConsoleFont),
        m_resourceDescriptors->GetCpuHandle(Descriptors::ConsoleBackground),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ConsoleBackground)
    );

    m_logConsole->SetDebugOutput(true);
    m_glbConsole->SetDebugOutput(true);

    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    static const RECT LAYOUT_SIZE = { 0, 0, 1920, 1080 };
    static const RECT LOG_RECT = { 525, 200, 1880, 275 };
    static const RECT RESULT_RECT = { 525, 325, 1880, 950 };

    RECT currentScreenRect = m_deviceResources->GetOutputSize();
    auto viewport = m_deviceResources->GetScreenViewport();

    m_liveInfoHUD->SetViewport(viewport);

    RECT scaledGlb = ScaleRect(LAYOUT_SIZE, currentScreenRect, LOG_RECT);
    m_logConsole->SetWindow(scaledGlb, false);
    m_logConsole->SetViewport(viewport);

    scaledGlb = ScaleRect(LAYOUT_SIZE, currentScreenRect, RESULT_RECT);
    m_glbConsole->SetWindow(scaledGlb, false);
    m_glbConsole->SetViewport(viewport);

    m_ui->SetWindow(currentScreenRect);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
