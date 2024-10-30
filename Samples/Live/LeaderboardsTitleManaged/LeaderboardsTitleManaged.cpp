//--------------------------------------------------------------------------------------
// LeaderboardsTitleManaged.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LeaderboardsTitleManaged.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"


extern void ExitSample() noexcept;

using namespace DirectX;
using Microsoft::WRL::ComPtr;

constexpr int c_maxItemsPerQuery = 5; // Deliberately set low in this sample to demonstrate querying multiple pages

// Initiates a leaderboard query
void Sample::QueryLeaderboards(
    const std::string& statName,        // Stat Name the leaderboard is based off of from Partner Center
    XblSocialGroupType queryGroupType  // Should the query use the Global or Social leaderboards
)
{
    if (!m_liveResources->GetLiveContext())
    {
        m_glbConsole->Clear();
        m_glbConsole->WriteLine(L"No user is signed in.");
        return;
    }

    m_logConsole->Clear();
    m_logConsole->Format(L"** Query Stat: %S / Global: %lu\n", statName.c_str(), true);

    XblLeaderboardQuery query{};
    strcpy_s(query.scid, sizeof(query.scid), m_liveResources->GetServiceConfigId().c_str());
    query.order = XblLeaderboardSortOrder::Descending;
    query.maxItems = c_maxItemsPerQuery;
    query.statName = statName.c_str();
    query.queryType = XblLeaderboardQueryType::TitleManagedStatBackedGlobal;
    // Setting skipToXboxUserId as a non-zero value will compare the given user's stored stat against the global leaderboard.
    // When a player's stored stat and their score on the global leaderboard are not the same, this can result in mismatched outputs.
    query.skipToXboxUserId = s_skipToUser ? m_liveResources->GetXuid() : 0;

    if (queryGroupType != XblSocialGroupType::None)
    {
        // Social queries must include a valid XUID
        // If the user has no friends or favorited people who have played this sample, only this user's results will be returned
        // Setting socialGroup as People or Favorites will compare the stats of the associated users. It will not use values of the global leaderboard.
        query.socialGroup = XblSocialGroupType::People;
        query.queryType = XblLeaderboardQueryType::TitleManagedStatBackedSocial;
        query.xboxUserId = m_liveResources->GetXuid();
    }

    auto* ctx = new LeaderboardsQueryContext{};
    ctx->sample = this;
    ctx->maxItems = query.maxItems;

    ctx->async.queue = m_mainAsyncQueue;
    ctx->async.context = ctx;
    ctx->async.callback = [](XAsyncBlock* async)
    {
        Sample::ProcessLeaderboardResults(async);
    };

    auto hr = XblLeaderboardGetLeaderboardAsync(m_liveResources->GetLiveContext(), query, &ctx->async);
    if (FAILED(hr))
    {
        m_logConsole->Format(DirectX::Colors::Red, L"XblLeaderboardGetLeaderboardAsync failed: 0x%08X\n", hr);
        delete ctx;
    }
}

// Displays and fetches additional leaderboard records from the service
void Sample::ProcessLeaderboardResults(XAsyncBlock* async)
{
    auto ctx = static_cast<LeaderboardsQueryContext*>(async->context);

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
            ctx->sample->m_logConsole->Format(DirectX::Colors::Red, L"XblLeaderboardGetLeaderboardResult failed: 0x%08X\n", hr);
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
        else
        {
            ctx->sample->m_logConsole->Format(DirectX::Colors::Red, L"XblLeaderboardResultGetNextResult failed: 0x%08X\n", hr);
        }
    }

    if (SUCCEEDED(hr))
    {
        ctx->sample->RenderLeaderboardsResults(ctx);

        if (ctx->result && ctx->result->hasNext)
        {
            ctx->page++;
            ZeroMemory(&async->internal[0], _countof(async->internal));
            hr = XblLeaderboardResultGetNextAsync(ctx->sample->m_liveResources->GetLiveContext(), ctx->result, ctx->maxItems, async);
            if (FAILED(hr))
            {
                ctx->sample->m_logConsole->Format(DirectX::Colors::Red, L"XblLeaderboardResultGetNextAsync failed: 0x%08X\n", hr);
            }
        }
    }
    else
    {
        delete ctx;
    }
}

// Queries an individual statistic for the current user
void Sample::QueryStatistics(const std::string& statName)
{
    struct GetStatisticContext
    {
        XAsyncBlock async;
        Sample* sample;
    };

    HRESULT hr = S_OK;

    m_logConsole->Clear();
    m_logConsole->Format(L"** Query Stat: %S\n", statName.c_str());

    auto* ctx = new GetStatisticContext{ XAsyncBlock{}, this };

    ctx->async.context = ctx;
    ctx->async.queue = m_mainAsyncQueue;
    ctx->async.callback = [](XAsyncBlock* async)
    {
        HRESULT hr = S_OK;

        auto ctx = static_cast<GetStatisticContext*>(async->context);
        std::vector<uint8_t> buffer;
        size_t size = 0;
        XblUserStatisticsResult* result;

        hr = XblUserStatisticsGetSingleUserStatisticResultSize(async, &size);
        if (SUCCEEDED(hr))
        {
            buffer.resize(size);
            hr = XblUserStatisticsGetSingleUserStatisticResult(async, size, buffer.data(), &result, &size);
            if (SUCCEEDED(hr))
            {
                ctx->sample->RenderStatisticsResult(result);
            }
            else
            {
                ctx->sample->m_logConsole->Format(DirectX::Colors::Red, L"XblUserStatisticsGetSingleUserStatisticResult failed: 0x%08X\n", hr);
            }
        }
        else
        {
            ctx->sample->m_logConsole->Format(DirectX::Colors::Red, L"XblUserStatisticsGetSingleUserStatisticResultSize failed: 0x%08X\n", hr);
        }

        delete ctx;
    };

    hr = XblUserStatisticsGetSingleUserStatisticAsync(
        m_liveResources->GetLiveContext(),
        m_liveResources->GetXuid(),
        m_liveResources->GetServiceConfigId().c_str(),
        statName.c_str(),
        &ctx->async);

    if (FAILED(hr))
    {
        ctx->sample->m_logConsole->Format(DirectX::Colors::Red, L"XblUserStatisticsGetSingleUserStatisticAsync failed: 0x%08X\n", hr);
        delete ctx;
    }
}

const char* Sample::GenerateStringStat(uint32_t index)
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
    const char* strResult = GenerateStringStat(uint32_t(score));
    m_logConsole->Clear();
    m_logConsole->Format(
        L"NumberStat: \"%S\", NumberValue: %f; StringStat: \"%S\", StringValue: \"%S\"\n",
        "ANumberStat",
        score,
        "AStringStat",
        strResult);

    statList.push_back(XblTitleManagedStatistic{ "AStringStat", XblTitleManagedStatType::String, 0, strResult });
    statList.push_back(XblTitleManagedStatistic{ "ANumberStat", XblTitleManagedStatType::Number, score, "" });

    std::unique_ptr<XAsyncBlock> async(new XAsyncBlock{});
    async->context = this;
    async->queue = m_mainAsyncQueue;
    async->callback = [](XAsyncBlock* async)
    {
        std::unique_ptr<XAsyncBlock> asyncOwner(async);
        auto sample = reinterpret_cast<Sample*>(async->context);

        if (FAILED(XAsyncGetStatus(async, false)))
        {
            sample->m_glbConsole->Clear();
            sample->m_glbConsole->WriteLine(L"Failed to send stats!");
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
        const size_t bufferSize = 64;
        wchar_t statusBuffer[bufferSize] = L"";

        wcscat_s(statusBuffer, s_isLeaderboardGlobal ? L"< GLOBAL >" : L"< SOCIAL >");
        wcscat_s(statusBuffer, s_skipToUser ? L"< SkipToCurrentUser >" : L"");

        s_labels[c_leaderboardType]->SetText(statusBuffer);
    }
    else
    {
        s_labels[c_leaderboardType]->SetText(L"");
    }

    s_mainPanel->Cancel();
    s_mainPanel->Show();
}

static void FormatLeaderboardHeader(std::stringstream& ss, const XblLeaderboardResult* res)
{
    // render header columns
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

static void FormatLeaderboardRows(std::stringstream& ss, const XblLeaderboardResult* res)
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

void Sample::RenderStatisticsResult(XblUserStatisticsResult* res)
{
    std::stringstream ss;

    // Format stats header
    ss << std::setw(20) << "Stat Name" << " |";
    ss << std::setw(12) << "Stat Type" << " |";
    ss << std::setw(20) << "Stat Value";
    ss << std::endl;
    ss << "--------------------------------------------------------";
    ss << std::endl;

    // Format stats result
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

    m_glbConsole->Clear();
    m_glbConsole->WriteLine(DX::Utf8ToWide(ss.str()).c_str());
}

void Sample::RenderLeaderboardsResults(LeaderboardsQueryContext* ctx)
{
    std::stringstream ss;

    if (ctx->page == 0)
    {
        // render header columns
        FormatLeaderboardHeader(ss, ctx->result);
    }

    // render all rows
    FormatLeaderboardRows(ss, ctx->result);

    if (ctx->page == 0)
    {
        m_glbConsole->Clear();
    }

    m_glbConsole->WriteLine(DX::Utf8ToWide(ss.str()).c_str());
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
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveResources = std::make_shared<ATG::LiveResources>(m_mainAsyncQueue);
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("Title-managed Leaderboards");
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
    m_mouse->SetWindow(window);

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
            m_logConsole->Format(DirectX::Colors::Red, L"Xbox Live Error: 0x%08X\n", error);
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

    XErrorSetCallback([](HRESULT hr, const char* msg, void* context) -> bool
        {
            UNREFERENCED_PARAMETER(context);
            char buf[512];
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

    // Stats

    auto statsString = m_ui->FindControl<Button>(c_sampleUIPanel, c_statsString);
    statsString->SetCallback([this](IPanel*, IControl*)
        {
            QueryStatistics("AStringStat");
        });

    auto statsNumber = m_ui->FindControl<Button>(c_sampleUIPanel, c_statsNumber);
    statsNumber->SetCallback([this](IPanel*, IControl*)
        {
            QueryStatistics("ANumberStat");
        });

    auto& statsPage = s_uiPages[SamplePage::QueryStats];
    statsPage.buttons.emplace(c_statsString, statsString);
    statsPage.buttons.emplace(c_statsNumber, statsNumber);

    // Leaderboards

    auto ldrString = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrString);
    ldrString->SetCallback([this](IPanel*, IControl*)
        {
            m_logConsole->Clear();
            m_glbConsole->Clear();
            m_glbConsole->WriteLine(L"String stats cannot be queried as leaderboards.");
        });

    auto ldrNumber = m_ui->FindControl<Button>(c_sampleUIPanel, c_ldrNumber);
    ldrNumber->SetCallback([this](IPanel*, IControl*)
        {
            auto lbType = s_isLeaderboardGlobal ? XblSocialGroupType::None : XblSocialGroupType::People;
            QueryLeaderboards("ANumberStat", lbType);
        });

    auto& ldrPage = s_uiPages[SamplePage::QueryLeaderboards];
    ldrPage.buttons.emplace(c_ldrString, ldrString);
    ldrPage.buttons.emplace(c_ldrNumber, ldrNumber);


    SetPage(SamplePage::QueryLeaderboards);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    m_mouse->EndOfInputFrame();

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
    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    if (pad.IsViewPressed() || kb.Escape)
    {
        ExitSample();
    }

    if (m_gamePadButtons.menu == GamePad::ButtonStateTracker::PRESSED || m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Tab))
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

    if (m_gamePadButtons.x == GamePad::ButtonStateTracker::RELEASED || m_keyboardButtons.IsKeyReleased(Keyboard::Keys::Space))
    {
        updateUIKeyboardState = false;
        SendStats();
    }

    if ((m_gamePadButtons.y == GamePad::ButtonStateTracker::RELEASED || m_keyboardButtons.IsKeyReleased(Keyboard::Keys::F1)) && s_activePage == SamplePage::QueryLeaderboards)
    {
        s_isLeaderboardGlobal = !s_isLeaderboardGlobal;
        SetPage((SamplePage)s_activePage);
    }

    if ((m_gamePadButtons.b == GamePad::ButtonStateTracker::RELEASED || m_keyboardButtons.IsKeyReleased(Keyboard::Keys::F2)) && s_activePage == SamplePage::QueryLeaderboards)
    {
        s_skipToUser = !s_skipToUser;
        SetPage((SamplePage)s_activePage);
    }

    if (m_gamePadButtons.rightShoulder == GamePad::ButtonStateTracker::RELEASED || m_keyboardButtons.IsKeyReleased(Keyboard::Keys::F3))
    {
        // next page
        s_activePage = (++s_activePage) % SamplePage::PageCount;
        SetPage((SamplePage)s_activePage);
    }

    if (m_gamePadButtons.leftShoulder == GamePad::ButtonStateTracker::RELEASED)
    {
        // previous page
        --s_activePage;
        if (s_activePage > SamplePage::PageCount) { s_activePage = SamplePage::PageCount - 1; }
        SetPage((SamplePage)s_activePage);
    }

    m_ui->Update(elapsedTime, pad);
    if (!kb.IsKeyDown(Keyboard::Space) && updateUIKeyboardState) // Space normally also selects buttons, but for this sample, it should just send events
    {
        m_ui->Update(elapsedTime, *m_mouse, *m_keyboard);
    }
    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

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
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
																  
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
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

void Sample::OnWindowMoved()
{
    auto const r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnWindowSizeChanged(int width, int height)
{
    if (!m_deviceResources->WindowSizeChanged(width, height))
        return;

    CreateWindowSizeDependentResources();
}

// Properties
void Sample::GetDefaultSize(int& width, int& height) const noexcept
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

#ifdef _GAMING_DESKTOP
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { D3D_SHADER_MODEL_6_0 };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))
        || (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0))
    {
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
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

// Scales a rectangle based on a ratio of a "layout" rect to a "current" rect (for resolution independent UI scaling)
static RECT ScaleRect(const RECT& originalDisplayRect, const RECT& displayRect, const RECT& originalSubRect)
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

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    static const RECT LAYOUT_SIZE = { 0, 0, 1920, 1080 };
    static const RECT LOG_RECT = { 525, 200, 1880, 275 };
    static const RECT RESULT_RECT = { 525, 325, 1880, 950 };

    const RECT currentScreenRect = m_deviceResources->GetOutputSize();
    auto const viewport = m_deviceResources->GetScreenViewport();

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
#pragma endregion
