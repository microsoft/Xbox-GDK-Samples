//--------------------------------------------------------------------------------------
// Fundamentals_Desktop.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Fundamentals_Desktop.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "SampleGUI.h"

#include "StringUtil.h"
#include "FindMedia.h"

extern void ExitSample();

using namespace DirectX;

namespace
{
    const int s_itemCheckForUpdates = 2001;
    const int s_itemDownloadAndInstallUpdates = 2002;
}

Sample::Sample() noexcept(false) :
    m_asyncQueue(nullptr),
    m_CurrentUserHandle(nullptr),
    m_xStoreContext(nullptr)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->RegisterDeviceNotify(this);

    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("PC Fundamentals Sample");

    ATG::UIConfig uiconfig;
    uiconfig.colorBackground = DirectX::XMFLOAT4(0, 0, 0, 1);
    uiconfig.colorFocus = DirectX::XMFLOAT4(0, 153.f / 255.f, 1.f / 255.f, 1);
    m_ui = std::make_shared<ATG::UIManager>(uiconfig);

    m_console = std::make_unique<DX::TextConsoleImage>();
    m_console->SetDebugOutput(true);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    if (m_asyncQueue)
    {
        XTaskQueueCloseHandle(m_asyncQueue);
        m_asyncQueue = nullptr;
    }
}

#pragma region Xbox Live and Login
void Sample::InitializeXBL()
{
    ConsoleWriteLine("Initializing");

    uint32_t titleId = 0;
    HRESULT hr = XGameGetXboxTitleId(&titleId);

    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
    {
        ConsoleWriteLine("FATAL ERROR: Could not retrieve title ID. Verify MicrosoftGame.config exists and that the app is installed or registered via wdapp.");

        m_ui->FindControl<ATG::Button>(c_sampleUIPanel, s_itemCheckForUpdates)->SetVisible(false);
        m_ui->FindControl<ATG::Button>(c_sampleUIPanel, s_itemDownloadAndInstallUpdates)->SetVisible(false);
    }
    else
    {
        char scidBuffer[64] = {};
        sprintf_s(scidBuffer, "00000000-0000-0000-0000-0000%08X", titleId);

        XblInitArgs xblInit = { m_asyncQueue, scidBuffer };
        hr = XblInitialize(&xblInit);
        DisplayHResult(hr, "XblInitialize");
        DX::ThrowIfFailed(hr);

        TryAddUserSilently();
    }
}

void Sample::TryAddUserSilently()
{
    ConsoleWriteLine("Logging into Xbox Live (TryAddUserSilently)");
    m_liveInfoHUD->SetUser(nullptr, m_asyncQueue);

    auto asyncBlock = new XAsyncBlock{};

    asyncBlock->context = this;
    asyncBlock->queue = m_asyncQueue;
    asyncBlock->callback = [](XAsyncBlock* asyncBlockInner)
    {
        auto pThis = reinterpret_cast<Sample*>(asyncBlockInner->context);
        XUserHandle user = nullptr;
        HRESULT hr = XUserAddResult(asyncBlockInner, &user);

        pThis->DisplayHResult(hr, "XUserAddResult");

        if (SUCCEEDED(hr))
        {
            // Call XUserGetId here to ensure all vetos (gamertag banned, etc) have passed
            uint64_t xuid = 0;

            hr = XUserGetId(user, &xuid);
            pThis->DisplayHResult(hr, "XUserGetId");
            if (FAILED(hr))
            {
                XUserCloseHandle(user);
                user = nullptr;

                pThis->AddUserWithUI();
            }
            else
            {
                pThis->HandleAddUserSuccess(user);
            }
        }
        else
        {
            pThis->AddUserWithUI();
        }

        delete asyncBlockInner;
    };

    HRESULT hr = XUserAddAsync(XUserAddOptions::AddDefaultUserSilently, asyncBlock);

    if (FAILED(hr))
    {
        delete asyncBlock;
    }

    DisplayHResult(hr, "XUserAddAsync Silently");
}

void Sample::AddUserWithUI()
{
    ConsoleWriteLine("Logging into Xbox Live (with UI)");
    m_liveInfoHUD->SetUser(nullptr, m_asyncQueue);

    auto asyncBlock = new XAsyncBlock{};
    asyncBlock->context = this;
    asyncBlock->queue = m_asyncQueue;
    asyncBlock->callback = [](XAsyncBlock* asyncBlockInner)
    {
        auto pThis = reinterpret_cast<Sample*>(asyncBlockInner->context);
        XUserHandle user = nullptr;
        HRESULT hr = XUserAddResult(asyncBlockInner, &user);
        pThis->DisplayHResult(hr, "XUserAddResult");

        if (SUCCEEDED(hr))
        {
            // Call XUserGetId here to ensure all vetos (gamertag banned, etc) have passed
            uint64_t xuid = 0;
            hr = XUserGetId(user, &xuid);

            if (FAILED(hr))
            {
                // If XUserAddResult fails, then call XUserResolveIssueWithUiAsync
                auto resolveAsyncBlock = new XAsyncBlock{};
                resolveAsyncBlock->queue = pThis->m_asyncQueue;
                resolveAsyncBlock->context = pThis;
                resolveAsyncBlock->callback = [](XAsyncBlock* asyncBlockInner)
                {
                    auto pThis = reinterpret_cast<Sample*>(asyncBlockInner->context);
                    HRESULT hr = XAsyncGetStatus(asyncBlockInner, false);

                    XUserHandle user = nullptr;
                    if (FAILED(hr))
                    {
                        XUserCloseHandle(user);
                        user = nullptr;
                        pThis->DisplayHResult(hr, "XUserResolveIssueWithUiAsync");
                    }
                    else
                    {
                        pThis->HandleAddUserSuccess(user);
                    }

                    delete asyncBlockInner;
                };

                hr = XUserResolveIssueWithUiAsync(user, "https://www.xboxlive.com", asyncBlockInner);

                if (FAILED(hr))
                {
                    delete asyncBlockInner;
                }
            }
            else
            {
                delete asyncBlockInner;
                pThis->HandleAddUserSuccess(user);
            }
        }

        delete asyncBlockInner;
    };

    HRESULT hr = XUserAddAsync(XUserAddOptions::None, asyncBlock);

    if (FAILED(hr))
    {
        delete asyncBlock;
    }

    DisplayHResult(hr, "XUserAddAsync with UI");
    assert(SUCCEEDED(hr));
}

void Sample::HandleAddUserSuccess(XUserHandle user)
{
    m_CurrentUserHandle = user;

    // Create store context
    // This is required for license check and checking for updates
    // See readme for more details
    HRESULT hr = XStoreCreateContext(user, &m_xStoreContext);
    DisplayHResult(hr, "XStoreCreateContext");
    if (SUCCEEDED(hr))
    {
        PerformLicenseCheck();
    }
    else
    {
        ConsoleWriteLine("\tExtended Error creating XStoreContext: 0x%08X", static_cast<unsigned int>(hr));
    }

    m_liveInfoHUD->SetUser(m_CurrentUserHandle, m_asyncQueue);
}
#pragma endregion

#pragma region Updates and Licensing
void Sample::PerformLicenseCheck()
{
    auto async = new XAsyncBlock{};
    async->context = this;
    async->queue = m_asyncQueue;
    async->callback = [](XAsyncBlock *async)
    {
        auto pThis = reinterpret_cast<Sample*>(async->context);

        bool isLicensed = false;

        XStoreGameLicense license;
        HRESULT hr = XStoreQueryGameLicenseResult(async, &license);

        if (SUCCEEDED(hr))
        {
            if (license.isActive)
            {
                if (!license.isTrial)
                {
                    pThis->ConsoleWriteLine("Active full license");
                    isLicensed = true;
                }
                else
                {
                    if (!license.isTrialOwnedByThisUser)
                    {
                        pThis->ConsoleWriteLine("No license: trial not owned by current account");

                    }
                    else if (license.trialTimeRemainingInSeconds <= 0)
                    {
                        pThis->ConsoleWriteLine("No license: trial expired");
                    }
                    else
                    {
                        pThis->ConsoleWriteLine("Active trial license");
                        isLicensed = true;
                    }
                }
            }
            else
            {
                pThis->ConsoleWriteLine("Inactive license");
            }
        }
        else
        {
            pThis->ConsoleWriteLine("XStoreQueryGameLicenseResult failed");
        }

        if (isLicensed)
        {
            pThis->CheckForUpdates();
        }

        delete async;
    };

    HRESULT hr = XStoreQueryGameLicenseAsync(m_xStoreContext, async);
    DisplayHResult(hr, "XStoreQueryGameLicenseAsync");

    if (FAILED(hr))
    {
        delete async;
    }
}

void Sample::CheckForUpdates()
{
    // This implements the simplest way to check for updates:
    // 1. Check for update
    // 2. Download and install update in one operation
    // This will shut down the title without warning when download is complete
    // See readme for more details and alternatives
    ConsoleWriteLine("Checking for Updates...");

    auto asyncBlock = new XAsyncBlock{};
    asyncBlock->context = this;
    asyncBlock->queue = m_asyncQueue;
    asyncBlock->callback = [](XAsyncBlock* asyncBlockInner)
    {
        auto pThis = reinterpret_cast<Sample*>(asyncBlockInner->context);

        UINT32 numUpdates = 0;
        HRESULT hrCount = XStoreQueryGameAndDlcPackageUpdatesResultCount(asyncBlockInner, &numUpdates);
        pThis->DisplayHResult(hrCount, "XStoreQueryGameAndDlcPackageUpdatesResultCount");

        if (SUCCEEDED(hrCount))
        {
            if (numUpdates > 0)
            {
                std::vector<XStorePackageUpdate> packages(numUpdates);

                HRESULT hr = XStoreQueryGameAndDlcPackageUpdatesResult(asyncBlockInner, numUpdates, packages.data());
                pThis->DisplayHResult(hr, "XStoreQueryGameAndDlcPackageUpdatesResult");

                if (SUCCEEDED(hr))
                {
                    pThis->m_updates.clear();

                    for (auto &package : packages)
                    {
                        // The mandatory flag is set in Partner Center for a new package submission
                        // For PC it does not mean anything in terms of blocking title launch like it does on console
                        pThis->ConsoleWriteLine("%s: %s", package.isMandatory ? "MANDATORY" : "OPTIONAL", package.packageIdentifier);
                        pThis->m_updates.push_back(package);
                    }

                    if (pThis->m_updates.size() > 0)
                    {
                        pThis->ConsoleWriteLine("%d updates are available", pThis->m_updates.size());
                    }
                }
                else
                {
                    pThis->ConsoleWriteLine("XStoreQueryGameAndDlcPackageUpdatesResult failed");
                }
            }
            else
            {
                pThis->ConsoleWriteLine("No updates are available");
            }
        }
        else
        {
            pThis->ConsoleWriteLine("XStoreQueryGameAndDlcPackageUpdatesResultCount failed");
        }

        pThis->m_ui->FindControl<ATG::Button>(c_sampleUIPanel, s_itemDownloadAndInstallUpdates)->SetVisible(numUpdates > 0);

        delete asyncBlockInner;
    };

    if (FAILED(XStoreQueryGameAndDlcPackageUpdatesAsync(m_xStoreContext, asyncBlock)))
    {
        delete asyncBlock;
    }
}

void Sample::DownloadUpdates()
{
    std::vector<const char*> packageIds;

    for (XStorePackageUpdate package : m_updates)
    {
        packageIds.push_back(package.packageIdentifier);
    }

    if (!packageIds.empty())
    {
        auto asyncBlock = new XAsyncBlock{};
        asyncBlock->context = this;
        asyncBlock->queue = m_asyncQueue;
        asyncBlock->callback = [](XAsyncBlock* asyncBlockInner)
        {
            auto pThis = reinterpret_cast<Sample*>(asyncBlockInner->context);
            HRESULT hr = XStoreDownloadAndInstallPackageUpdatesResult(asyncBlockInner);
            pThis->DisplayHResult(hr, "XStoreDownloadAndInstallPackageUpdatesResult");
            pThis->OnUpdateResult(hr);
            delete asyncBlockInner;
        };

        // This is the most straightforward method
        // You can separate this out by calling XStoreDownloadPackageUpdatesAsync first and monitor the download portion
        // before calling XStoreDownloadAndInstallPackageUpdatesAsync, which will always shut down the game to apply the update
        HRESULT hr = XStoreDownloadAndInstallPackageUpdatesAsync(m_xStoreContext, packageIds.data(), packageIds.size(), asyncBlock);
        DisplayHResult(hr, "XStoreDownloadAndInstallPackageUpdatesAsync");

    }
}
#pragma endregion

#pragma region Utility
void Sample::DisplayHResult(HRESULT hr, const char* strName)
{
	if (SUCCEEDED(hr))
	{
		ConsoleWriteLine("%s Result: Succeeded", strName);
	}
	else
	{
        ConsoleWriteLine("%s Result: Failed (0x%08X)", strName, static_cast<unsigned int>(hr));
	}
}

void Sample::OnUpdateResult(HRESULT hr)
{
    // This is called when the update is completed
    // Useful if download and update operations are separated and a notification of pending restart is to be displayed
    // But for XStoreDownloadAndInstallPackageUpdatesAsync the title will terminate at this point
    const size_t numPackages = m_updates.size();
    
    ConsoleWriteLine("Update with %zu packages: %s (RESULT: 0x%08X)", numPackages, SUCCEEDED(hr) ? "Succeeded" : "Failed", static_cast<unsigned int>(hr));

    if (SUCCEEDED(hr))
    {
        ConsoleWriteLine("SUCCESS: Update operation was a success.");
    }
    else
    {
        ConsoleWriteLine("FATAL ERROR: Updates could not be installed.");
    }
}

void Sample::ConsoleWriteLine(const char* format, ...)
{
    if (m_console)
    {
        va_list argList;
        va_start(argList, format);

        char buf[512] = {};
        vsnprintf(buf, sizeof(buf), format, argList);
        m_console->WriteLine(DX::Utf8ToWide(buf).c_str());

        va_end(argList);
    }
}
#pragma endregion

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    // NOTE: When running the app from the Start Menu (required for
    //	Store API's to work) the Current Working Directory will be
    //	returned as C:\Windows\system32 unless you overwrite it.
    //	The sample relies on the font and image files in the .exe's
    //	directory and so we do the following to set the working
    //	directory to what we want.
    char dir[1024];
    GetModuleFileNameA(NULL, dir, 1024);

    m_ExePath = dir;
    m_ExePath = m_ExePath.substr(0, m_ExePath.find_last_of("\\"));
    SetCurrentDirectoryA(m_ExePath.c_str());
    
    m_gamePad = std::make_unique<GamePad>();

    m_keyboard = std::make_unique<Keyboard>();

    m_mouse = std::make_unique<Mouse>();
    m_mouse->SetWindow(window);

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    //  Custom sample code
    XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_asyncQueue);

    m_ui->LoadLayout(L".\\Assets\\SampleUI.csv", L".\\Assets");

    m_liveInfoHUD->Initialize(width, height);

    m_ui->FindPanel<ATG::IPanel>(c_sampleUIPanel)->Show();

    // UI callbacks
    m_ui->FindControl<ATG::Button>(c_sampleUIPanel, s_itemDownloadAndInstallUpdates)->SetCallback(
        [this](ATG::IPanel*, ATG::IControl*)
        {
            DownloadUpdates();
        });

    m_ui->FindControl<ATG::Button>(c_sampleUIPanel, s_itemCheckForUpdates)->SetCallback(
        [this](ATG::IPanel*, ATG::IControl*)
        {
            CheckForUpdates();
        });

    InitializeXBL();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }
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

    m_ui->Update(static_cast<float>(timer.GetElapsedSeconds()), *m_mouse, *m_keyboard);

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    while (XTaskQueueDispatch(m_asyncQueue, XTaskQueuePort::Completion, 0)) {}

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
    m_console->Render(commandList);
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

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        50, // LiveInfoHUD needs 4 (3 fonts and 1 texture)
        0
        );

    auto fonthandle = m_resourceDescriptors->Allocate();
    auto bghandle = m_resourceDescriptors->Allocate();

    m_console->RestoreDevice(
        device,
        resourceUpload,
        rtState,
        L"courier_16.spritefont",
        L"Assets\\ATGSampleBackground.DDS",
        m_resourceDescriptors->GetCpuHandle(fonthandle),
        m_resourceDescriptors->GetGpuHandle(fonthandle),
        m_resourceDescriptors->GetCpuHandle(bghandle),
        m_resourceDescriptors->GetGpuHandle(bghandle)
    );

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    m_ui->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();

    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());

    static const RECT screenDisplay = { 960, 150, 1880, 825 };

    m_console->SetWindow(screenDisplay, false);
    m_console->SetViewport(viewport);

    m_ui->SetWindow(m_deviceResources->GetOutputSize());
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_ui->ReleaseDevice();
    m_liveInfoHUD->ReleaseDevice();
    m_console->ReleaseDevice();
    m_resourceDescriptors.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
