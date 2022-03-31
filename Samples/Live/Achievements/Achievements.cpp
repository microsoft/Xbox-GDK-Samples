//--------------------------------------------------------------------------------------
// Achievements.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Achievements.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"


extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const char *ProgressStateText[] = {
        u8"Unknown",
        u8"Achieved",
        u8"Not Started",
        u8"In Progress"
    };

    const char *AchievementTypeText[] = {
        u8"Unknown",
        u8"All",
        u8"Persistent",
        u8"Challenge"
    };

    const int c_sampleUIPanel = 2000;
    const int c_getAchievementsBtn = 2101;
    const int c_getSingleAchievementBtn = 2102;
    const int c_setSingleAchievementBtn = 2103;
    const int c_getSingleAchievementBtn2 = 2104;
    const int c_setSingleAchievement2Btn25 = 2105;
    const int c_setSingleAchievement2Btn50 = 2106;
    const int c_setSingleAchievement2Btn100 = 2107;
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
    m_liveResources = std::make_shared<ATG::LiveResources>();
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("Title-managed Achievements Sample");

    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_mainAsyncQueue)
    );

    ATG::UIConfig uiconfig;
    m_ui = std::make_unique<ATG::UIManager>(uiconfig);
    m_log = std::make_unique<DX::TextConsoleImage>();
    m_display = std::make_unique<DX::TextConsoleImage>();
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

    wchar_t result[MAX_PATH];
    DX::FindMediaFile(result, MAX_PATH, L".\\Assets\\SampleUI.csv");
    m_ui->LoadLayout(result, L".\\Assets\\");

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
}

void Sample::SetupUI()
{
    using namespace ATG;
    // Get all Achievements
    m_ui->FindControl<Button>(c_sampleUIPanel, c_getAchievementsBtn)->SetCallback([this](IPanel*, IControl*)
        {
            m_log->Clear();
            m_log->WriteLine(L"** Query all achievements scenario.\n");
            m_display->Clear();

            GetAchievements();
        });

    // Get single Achievement
    m_ui->FindControl<Button>(c_sampleUIPanel, c_getSingleAchievementBtn)->SetCallback([this](IPanel*, IControl*)
        {
            m_log->Clear();
            m_log->WriteLine(L"** Query achievement scenario.\n");
            m_display->Clear();
            GetAchievement("1");
        });

    // Complete an achievement in a single go
    m_ui->FindControl<Button>(c_sampleUIPanel, c_setSingleAchievementBtn)->SetCallback([this](IPanel*, IControl*)
        {
            m_log->Clear();
            m_log->WriteLine(L"** Achievement completion scenario.\n");
            m_display->Clear();
            UpdateAchievement("1", 100);
        });

    // Get single Achievement
    m_ui->FindControl<Button>(c_sampleUIPanel, c_getSingleAchievementBtn2)->SetCallback([this](IPanel*, IControl*)
        {
            m_log->Clear();
            m_log->WriteLine(L"** Query achievement scenario.\n");
            m_display->Clear();
            GetAchievement("2");
        });

    m_ui->FindControl<Button>(c_sampleUIPanel, c_setSingleAchievement2Btn25)->SetCallback([this](IPanel*, IControl*)
        {
            m_log->Clear();
            m_log->WriteLine(L"** Achievement progress scenario.\n");
            m_display->Clear();
            UpdateAchievement("2", 25);
        });

    m_ui->FindControl<Button>(c_sampleUIPanel, c_setSingleAchievement2Btn50)->SetCallback([this](IPanel*, IControl*)
        {
            m_log->Clear();
            m_log->WriteLine(L"** Achievement progress scenario.\n");
            m_display->Clear();
            UpdateAchievement("2", 50);
        });

    m_ui->FindControl<Button>(c_sampleUIPanel, c_setSingleAchievement2Btn100)->SetCallback([this](IPanel*, IControl*)
        {
            m_log->Clear();
            m_log->WriteLine(L"** Achievement completion scenario.\n");
            m_display->Clear();
            UpdateAchievement("2", 100);
        });
}

// Pretty print some achievement information
std::wstring FormatAchievement(const XblAchievement* achievement)
{
    std::stringstream ss;
    ss << "Id:        " << achievement->id << "\n";
    ss << "Name:      " << achievement->name << "\n";
    ss << "Progress:  " << ProgressStateText[(uint32_t)achievement->progressState] << "\n";
    ss << "Type:      " << AchievementTypeText[(uint32_t)achievement->type] << "\n";
    if (achievement->progressState != XblAchievementProgressState::Achieved)
    {
        ss << "Percent:   " << achievement->progression.requirements[0].currentProgressValue << "% \n";
    }
    else
    {
        ss << "Percent:   100% \n";
    }

    return DX::Utf8ToWide(ss.str());
}

void Sample::UpdateAchievement(const std::string& achievementId, uint32_t percentComplete)
{
    struct UpdateAchievementContext
    {
        Sample *sample;
        uint32_t percentComplete;
        std::string achievementId;

        UpdateAchievementContext() : sample(nullptr), percentComplete(0) {}
    };
    auto ctx = new UpdateAchievementContext();
    ctx->sample = this;
    ctx->percentComplete = percentComplete;
    ctx->achievementId = achievementId;


    auto async = new XAsyncBlock{};
    async->queue = m_mainAsyncQueue;
    async->context = reinterpret_cast<void*>(ctx);
    async->callback = [](XAsyncBlock *async)
    {
        auto context = reinterpret_cast<UpdateAchievementContext *>(async->context);
        HRESULT result = XAsyncGetStatus(async, true);

        if (SUCCEEDED(result))
        {
            context->sample->m_log->Format(L"UpdateAchievement succeeded.  Value set to %u. \n", context->percentComplete);
        }
        else
        {
            if (result == HTTP_E_STATUS_NOT_MODIFIED)
            {
                if (context->percentComplete == 100)
                {
                    context->sample->m_log->WriteLine(L"Achievement has already been completed.");
                    context->sample->m_log->WriteLine(L"\tResponse was HTTP Code 304: Not Modified");
                }
                else
                {
                    context->sample->m_log->WriteLine(L"Achievement was previously set to a higher value.");
                    context->sample->m_log->WriteLine(L"\tResponse was HTTP Code 304: Not Modified");
                }
            }
            else
            {
                auto errorCondition = XblGetErrorCondition(result);

                context->sample->m_log->Format(L"UpdateAchievement failed with error: %u \n", (uint32_t)errorCondition);
            }
        }

        context->sample->GetAchievement(context->achievementId);
        delete context;
        delete async;
    };


    XblContextHandle context = GetXblContext();
    uint64_t xuid = 0;
    XblContextGetXboxUserId(context, &xuid);

    if (SUCCEEDED(XblAchievementsUpdateAchievementForTitleIdAsync(context, GetXuid(), GetTitleId(), GetServiceConfigId(), achievementId.c_str(), percentComplete, async)))
    {
        m_log->Format(L"UpdateAchievement dispatched for:\n\tAchievementId=%hs \n\tValue=%u \n", achievementId.c_str(), percentComplete);
    }
    else
    {
        delete async;
    }

}

void Sample::GetAchievement(const std::string& achievementId)
{
    auto async = new XAsyncBlock{};
    async->queue = m_mainAsyncQueue;
    async->context = reinterpret_cast<void*>(this);
    async->callback = [](XAsyncBlock *async)
    {
        auto _this = reinterpret_cast<Sample *>(async->context);
        XblAchievementsResultHandle result = nullptr;

        if (SUCCEEDED(XblAchievementsGetAchievementResult(async, &result)))
        {
            const XblAchievement *achievement;
            size_t count = 0;

            if (SUCCEEDED(XblAchievementsResultGetAchievements(result, &achievement, &count)))
            {
                if (count > 0)
                {
                    _this->m_display->WriteLine(FormatAchievement(achievement).c_str());
                }
            }
        }

        delete async;
    };

    if (SUCCEEDED(XblAchievementsGetAchievementAsync(GetXblContext(), GetXuid(), GetServiceConfigId(), achievementId.data(), async)))
    {
        m_log->Format(L"GetAchievement request dispatched for: \n\tAchievementId=%hs \n", achievementId.c_str());
    }
    else
    {
        delete async;
    }
}


void Sample::GetAchievements()
{
    // Local struct storing state that will be passed in the async block's context
    struct PagingContext
    {
        Sample *sample;
        int iteration;
        size_t total;
    };

    auto context = new PagingContext();
    context->sample = this;
    context->iteration = 0;
    context->total = 0;

    auto async = new XAsyncBlock{};
    async->queue = m_mainAsyncQueue;
    async->context = reinterpret_cast<void*>(context);
    async->callback = [](XAsyncBlock *async)
    {
        bool hasNext = false;
        auto *ctx = reinterpret_cast<PagingContext*>(async->context);
        auto *log = ctx->sample->m_log.get();
        auto *display = ctx->sample->m_display.get();

        XblAchievementsResultHandle result;
        HRESULT hr;

        if (ctx->iteration == 0)
        {
            hr = XblAchievementsGetAchievementsForTitleIdResult(async, &result);	// Initial results getter
        }
        else
        {
            hr = XblAchievementsResultGetNextResult(async, &result);				// Subsequent results getter
        }

        if (SUCCEEDED(hr))
        {
            const XblAchievement *achievement; // We'll be given a pointer to the array managed by the handle
            size_t count = 0;                  // And a count

            if (SUCCEEDED(XblAchievementsResultGetAchievements(result, &achievement, &count)))
            {
                // Note: Copy the achievements out of this array if you need them outside this block!
                ctx->total += count;
                for (size_t i = 0; i < count; i++)
                {
                    display->WriteLine(FormatAchievement(&achievement[i]).c_str());
                }
                log->Format(L"Page contained %u achievement(s). \n", count);
            }

            if (SUCCEEDED(XblAchievementsResultHasNext(result, &hasNext)))
            {
                if (hasNext)
                {
                    ctx->iteration++;
                    XblAchievementsResultGetNextAsync(result, 1, async);
                    log->WriteLine(L"Getting next page of achievements...");
                }
                else
                {
                    log->Format(L"Completed. Retrieved %zu achievement(s).", ctx->total);
                }
            }
        }

        // Reuse the async block unless there's no more results to get
        if (!hasNext)
        {
            delete ctx;
            delete async;
        }
    };

    // ** Note: Deliberately forcing this to 1 achievement to demonstrate multiple calls to get the achievements. Use larger batches in your code!
    if (SUCCEEDED(XblAchievementsGetAchievementsForTitleIdAsync(
        GetXblContext(),
        GetXuid(),
        GetTitleId(),
        XblAchievementType::All,					// Achievement types
        false,										// Unlocked only?
        XblAchievementOrderBy::DefaultOrder,		// No sort order
        0,											// Skip count
        1,											// Take count **
        async)))
    {
        m_log->WriteLine(L"Getting first page of achievements.");
    }
    else
    {
        delete context;
        delete async;
    }
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

    m_ui->Update(elapsedTime, *m_mouse, *m_keyboard);

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    // Process any completed tasks
    while (XTaskQueueDispatch(m_mainAsyncQueue, XTaskQueuePort::Completion, 0))
    {
        SwitchToThread();
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
    m_log->Render(commandList);
    m_display->Render(commandList);
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
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
void Sample::GetDefaultSize(int& width, int& height) const noexcept
{
    width = 1840;
    height = 1035;
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
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    wchar_t font[260];
    wchar_t background[260];

    DX::FindMediaFile(font, 260, L"courier_16.spritefont");
    DX::FindMediaFile(background, 260, L"ATGSampleBackground.DDS");

    m_log->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        font,
        background,
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetCpuHandle(Descriptors::Background),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Background)
    );

    m_display->RestoreDevice(
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

    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

void ScaleRect(const RECT &originalDisplayRect, const RECT &displayRect, const RECT &originalSubRect, RECT &scaledSubRect)
{
    const float widthScale = ((float)displayRect.right - (float)displayRect.left) / ((float)originalDisplayRect.right - (float)originalDisplayRect.left);
    const float heightScale = ((float)displayRect.bottom - (float)displayRect.top) / ((float)originalDisplayRect.bottom - (float)originalDisplayRect.top);

    scaledSubRect.top = (LONG)((float)originalSubRect.top * heightScale);
    scaledSubRect.left = (LONG)((float)originalSubRect.left * widthScale);
    scaledSubRect.bottom = (LONG)((float)originalSubRect.bottom * heightScale);
    scaledSubRect.right = (LONG)((float)originalSubRect.right * widthScale);
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    RECT fullscreen = m_deviceResources->GetOutputSize();
    auto viewport = m_deviceResources->GetScreenViewport();

    m_liveInfoHUD->SetViewport(viewport);

    // Scaled for 1920x1080
    static const RECT originalScale = { 0, 0, 1920, 1080 };
    static const RECT screenDisplay = { 960, 200, 1780, 450 };
    RECT scaledDisplay;
    ScaleRect(originalScale, fullscreen, screenDisplay, scaledDisplay);

    m_log->SetWindow(scaledDisplay, false);
    m_log->SetViewport(viewport);

    // Scaled for 1920x1080
    static const RECT screenDisplay2 = { 960, 500, 1780, 950 };
    ScaleRect(originalScale, fullscreen, screenDisplay2, scaledDisplay);

    m_display->SetWindow(scaledDisplay, false);
    m_display->SetViewport(viewport);

    m_ui->SetWindow(fullscreen);
}

void Sample::OnDeviceLost()
{
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
