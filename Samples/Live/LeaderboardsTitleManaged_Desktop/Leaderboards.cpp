//--------------------------------------------------------------------------------------
// Leaderboards.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// SAMPLE NOTE:
// The sample class is split between two implementation files to allow one file
// to focus on the statistics and leaderboard calls specifically while the other
// handles rendering boilerplate and UI interactions.
//
// This is the statistics and leaderboards usage file
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

static RECT ScaleRect(const RECT &originalDisplayRect, const RECT &displayRect, const RECT &originalSubRect);

namespace {
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

    constexpr int c_maxItemsPerQuery = 30;
}

// Initiates a leaderboard query
void Sample::QueryLeaderboards(
    const std::string &leaderboardName, // Name of the leaderboard from Partner Center
    const std::string &statName,        // Stat Name the leaderboard is based off of from Partner Center
    XblSocialGroupType queryGroupType,  // Should the query use the Global or Social leaderboards
    const char **additionalColumns,     // Additional columns to include with the leaderboards, defined in Partner Center
    size_t additionalColumnsCount,      // Number of additional columns
    std::function<void(HRESULT, LeaderboardsQueryContext *)> onResultsReceived // Results handler for this sample
)
{
    if (!m_liveResources->GetLiveContext())
    {
        WriteToResult("No user is signed in.", true);
        return;
    }

    XblLeaderboardQuery query{};
    
    strcpy_s(query.scid, sizeof(query.scid), m_liveResources->GetServiceConfigId().c_str());

    query.order = XblLeaderboardSortOrder::Descending;
    query.maxItems = c_maxItemsPerQuery;

    query.statName = statName.c_str();
    query.leaderboardName = leaderboardName.c_str();
    query.queryType = XblLeaderboardQueryType::TitleManagedStatBackedGlobal;

    if (queryGroupType != XblSocialGroupType::None)
    {
        // Social queries must include a valid XUID
        // If the user has no friends or favorited people who have played this sample, only this user's results will be returned
        query.socialGroup = XblSocialGroupType::People;
        query.queryType = XblLeaderboardQueryType::TitleManagedStatBackedSocial;
        query.xboxUserId = m_liveResources->GetXuid();
    }

    if (additionalColumns && additionalColumnsCount > 0)
    {
        // Additional columns are not aggregated, but rather the "latest" value at the time the stat was changed
        // This is useful for stats which use MIN/MAX aggregations in their stat rules to include information that triggered the update
        // (e.g. MostItemsFound includes the "Environment" and "DistanceTraveled" fields to indicate
        //       where those items were found and how far they traveled).
        query.additionalColumnleaderboardNames = additionalColumns;
        query.additionalColumnleaderboardNamesCount = additionalColumnsCount;
    }

    std::unique_ptr<LeaderboardsQueryContext> ctx(new LeaderboardsQueryContext{});
    ctx->sample = this;
    ctx->onResultsReceived = onResultsReceived;
    ctx->maxItems = query.maxItems;

    std::unique_ptr<XAsyncBlock> async(new XAsyncBlock{});
    async->queue = m_mainAsyncQueue;
    async->context = ctx.get();
    async->callback = [](XAsyncBlock *async)
    {
        Sample::ProcessLeaderboardResults(async);
    };

    query.queryType = XblLeaderboardQueryType::TitleManagedStatBackedGlobal;
    auto hr = XblLeaderboardGetLeaderboardAsync(m_liveResources->GetLiveContext(), query, async.get());
    if (SUCCEEDED(hr))
    {
        ctx.release();
        async.release();
    }
    else
    {
        onResultsReceived(hr, ctx.get());
    }
}

// Displays and fetches additional leaderboard records from the service
void Sample::ProcessLeaderboardResults(XAsyncBlock *async)
{
    std::unique_ptr<LeaderboardsQueryContext> ctxOwner(reinterpret_cast<LeaderboardsQueryContext *>(async->context));
    std::unique_ptr<XAsyncBlock> asyncOwner(async);
    auto *ctx = ctxOwner.get();
    size_t resultSize = 0;

    HRESULT hr = S_OK;

    if (ctx->page == 0)
    {
        hr = XblLeaderboardGetLeaderboardResultSize(async, &resultSize);
        if (SUCCEEDED(hr))
        {
            ctx->resultData.resize(resultSize);

            hr = XblLeaderboardGetLeaderboardResult(async, ctx->resultData.size(), ctx->resultData.data(), &ctx->result, nullptr);
        }
        else
        {
            auto xberr = XblGetErrorCondition(hr);
            UNREFERENCED_PARAMETER(xberr); // Debug Tip: Get the bucketed error reason here (e.g. Auth, Network, etc.)
        }
    }
    else
    {
        hr = XblLeaderboardResultGetNextResultSize(async, &resultSize);
        if (SUCCEEDED(hr))
        {
            ctx->resultData.resize(resultSize);
            hr = XblLeaderboardResultGetNextResult(async, resultSize, ctx->resultData.data(), &ctx->result, nullptr);
        }
    }

    if (ctx->onResultsReceived)
    {
        ctx->onResultsReceived(hr, ctx);
    }

    if (SUCCEEDED(hr) && ctx->result && ctx->result->hasNext)
    {
        ctx->page++;
        ZeroMemory(&async->internal[0], _countof(async->internal));
        XblLeaderboardResultGetNextAsync(ctx->sample->m_liveResources->GetLiveContext(), ctx->result, ctx->maxItems, async);
        ctxOwner.release();
        asyncOwner.release();
    }

}

// Queries an individual statistic for the current user
void Sample::QueryStatistics(const std::string &statName)
{
    std::unique_ptr<XAsyncBlock> async(new XAsyncBlock{});

    async->context = this;
    async->queue = m_mainAsyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        std::unique_ptr<XAsyncBlock> asyncOwner(async);
        std::vector<uint8_t> buffer;
        size_t size = 0;
        XblUserStatisticsResult *result;

        auto *sample = reinterpret_cast<Sample*>(async->context);

        if (SUCCEEDED(XblUserStatisticsGetSingleUserStatisticResultSize(async, &size)))
        {
            buffer.reserve(size);
            if (SUCCEEDED(XblUserStatisticsGetSingleUserStatisticResult(async, size, buffer.data(), &result, &size)))
            {
                sample->RenderStatisticsResult(result);
            }
        }
    };

    if (SUCCEEDED(XblUserStatisticsGetSingleUserStatisticAsync(
        m_liveResources->GetLiveContext(),
        m_liveResources->GetXuid(),
        m_liveResources->GetServiceConfigId().c_str(),
        statName.c_str(),
        async.get())))
    {
        async.release();
    }
}

const char *Sample::GenerateStringStat(uint32_t index)
{
    static const std::string stringList[] =
    {
        "", "PC", "Xbox", "Xbox 360", "Xbox One", "Xbox One S", "Xbox One X", "Xbox Series S", "Xbox Series X", "Xbox?", "Xbox!!!"
    };

    return stringList[index % _countof(stringList)].c_str();
}

double Sample::CalculateRandomScore()
{
    static std::array<double, 4> intervals{ 0.0, 3.0, 5.0, 10.0 };
    static std::array<double, 4> weights{ 0.002, 1.5, 2.5, 0.3 };
    static std::piecewise_linear_distribution<double> distanceDist(intervals.begin(), intervals.end(), weights.begin());

    return distanceDist(m_rand);
}


// Sends some stat data to the stats service
// Stat data can be strings or numbers, but only numbers can be used as leaderboards
// The score that is sent for the number is random, and may not be higher than what is already there.
// If it is lower than what is already there, the leaderboard value will still show the highest recorded stat.
void Sample::SendStats()
{
    std::vector<XblTitleManagedStatistic> statList;

    double score = CalculateRandomScore();

    statList.push_back(XblTitleManagedStatistic{ "AStringStat", XblTitleManagedStatType::String, 0, GenerateStringStat(uint32_t(score)) });
    statList.push_back(XblTitleManagedStatistic{ "ANumberStat", XblTitleManagedStatType::Number, score });

    std::unique_ptr<XAsyncBlock> async(new XAsyncBlock{});
    async->context = this;
    async->queue = m_mainAsyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        std::unique_ptr<XAsyncBlock> asyncOwner(async);
        auto sample = reinterpret_cast<Sample*>(async->context);

        if (FAILED(XAsyncGetStatus(async, false)))
        {
            sample->WriteToResult("Failed to send stats!");
        }
    };

    if (SUCCEEDED(XblTitleManagedStatsUpdateStatsAsync(m_liveResources->GetLiveContext(), statList.data(), statList.size(), async.get())))
    {
        async.release();
    }
}


#pragma region UI Related Methods
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
    uint32_t length = 0;
    ss << std::setw(8) << "Rank" << " |";
    ss << std::setw(16) << "Gamer Tag";
    length += (8 + 2 + 16);


    for (size_t i = 0; i < res->columnsCount; i++)
    {
        ss << " |";
        ss << std::setw(i == 0 ? 32 : 18) << res->columns[i].statName;
        length += (i == 0 ? 32 : 18) + 2;
    }
    ss << std::endl;

    std::string border;
    border.insert(0, length, '-');
    ss << border << std::endl;
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
            ss << std::setw(20) << res->serviceConfigStatistics[i].statistics[j].statisticName << " |";
            ss << std::setw(12) << res->serviceConfigStatistics[i].statistics[j].statisticType << " |";
            ss << std::setw(20) << res->serviceConfigStatistics[i].statistics[j].value;
            ss << std::endl;
        }
    }
}

static void FormatStatisticHeader(std::stringstream& ss)
{
    ss << std::setw(20) << "Stat Name" << " |";
    ss << std::setw(12) << "Stat Type" << " |";
    ss << std::setw(20) << "Stat Value";
    ss << std::endl;
    ss << "--------------------------------------------------------";
    ss << std::endl;
}

void Sample::RenderStatisticsResult(XblUserStatisticsResult *res)
{
    std::stringstream ss;
    FormatStatisticHeader(ss);
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
#pragma endregion

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
void Sample::Initialize(HWND window, int width, int height)
{
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
#ifdef _GAMING_DESKTOP
    m_mouse->SetWindow(window);
#endif

    m_deviceResources->SetWindow(window, width, height);

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

    auto statsString = m_ui->FindControl<Button>(c_sampleUIPanel, c_statsString);
    statsString->SetCallback([this](IPanel*, IControl*)
        {
            WriteToLog(L"** Query Stat: AStringStat\n");
            QueryStatistics("AStringStat");
        });

    auto statsNumber = m_ui->FindControl<Button>(c_sampleUIPanel, c_statsNumber);
    statsNumber->SetCallback([this](IPanel*, IControl*)
        {
            WriteToLog(L"** Query Stat: ANumberStat\n");
            QueryStatistics("ANumberStat");
        });

    auto & statsPage = s_uiPages[SamplePage::QueryStats];
    statsPage.buttons.emplace(c_statsString, statsString);
    statsPage.buttons.emplace(c_statsNumber, statsNumber);

    // Leaderboards

    auto ldrString = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrString);
    ldrString->SetCallback([this](IPanel*, IControl*)
        {
            WriteToLog(L"** Query leaderboards: AStringStat\n");
            WriteToResult("String stats cannot be queried as leaderboards.");
        });

    auto ldrNumber = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrNumber);
    ldrNumber->SetCallback([this](IPanel*, IControl*)
        {
            auto render = [this](HRESULT hr, LeaderboardsQueryContext *ctx) { RenderLeaderboardsResults(hr, ctx); };
            WriteToLog(L"** Query leaderboards: ANumberStat\n");
            auto lbType = s_isLeaderboardGlobal ? XblSocialGroupType::None : XblSocialGroupType::People;
            QueryLeaderboards("ANumberStat", "ANumberStat", lbType, nullptr, 0, render);
        });

    auto & ldrPage = s_uiPages[SamplePage::QueryLeaderboards];
    ldrPage.buttons.emplace(c_ldrString, ldrString);
    ldrPage.buttons.emplace(c_ldrNumber, ldrNumber);


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
    bool updateUIKeyboardState = true;

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
            SendStats();
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

    if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Space))
    {
        updateUIKeyboardState = false;
        SendStats();
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::F1))
    {
        NextPage();
    }
    else if (m_keyboardButtons.IsKeyReleased(Keyboard::Keys::F2) && s_activePage == SamplePage::QueryLeaderboards)
    {
        s_isLeaderboardGlobal = !s_isLeaderboardGlobal;
        SetPage((SamplePage)s_activePage);
    }

    if (!kb.IsKeyDown(Keyboard::Space) && updateUIKeyboardState) // Space normally also selects buttons, but for this sample, it should just send events
    {
        m_ui->Update(elapsedTime, *m_mouse, *m_keyboard);
    }

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
}

void Sample::OnResuming()
{
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_keyboardButtons.Reset();
    m_liveResources->Refresh();
    m_ui->Reset();
}

void Sample::OnWindowMoved()
{
    auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
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
    m_glbConsole->ReleaseDevice();
    m_logConsole->ReleaseDevice();
    m_ui->ReleaseDevice();
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
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
#pragma endregion
