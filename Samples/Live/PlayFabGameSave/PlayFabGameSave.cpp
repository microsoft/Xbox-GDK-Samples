//--------------------------------------------------------------------------------------
// PlayFabGameSave.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PlayFabGameSave.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "StringUtil.h"


extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_mainAsyncQueue(nullptr)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN, 2);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
    m_liveResources = std::make_shared<ATG::LiveResources>();
    m_liveInfoHUD = std::make_unique<ATG::LiveInfoHUD>("");
    // Seed RNG based on time
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::seed_seq seed{ static_cast<uint32_t>(now & 0xFFFFFFFF), static_cast<uint32_t>((now >> 32) & 0xFFFFFFFF) };
    m_rng.seed(seed);

    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_mainAsyncQueue)
    );
}

Sample::~Sample()
{
    UninitializeGameSaves();

    CleanupPersistentLocalUser();

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

    InitializeUI();

    m_liveResources->SetUserChangedCallback([this](XUserHandle user)
    {
        m_liveInfoHUD->SetUser(user, m_liveResources->GetAsyncQueue());

        Log("Xbox Network: gamertag \"%s\" signed in", m_liveResources->GetGamertag().c_str());

        PlayFabSignIn();
    });

    m_liveResources->SetUserSignOutCompletedCallback([this](XUserHandle /*user*/)
    {
        m_liveInfoHUD->SetUser(nullptr, m_liveResources->GetAsyncQueue());
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


    InitializePlayFab();
    InitializeGameSaves();
}

void Sample::PlayFabSignIn()
{
    if (m_playFabResources)
    {
        m_playFabResources->Cleanup();
        m_playFabResources = nullptr;
    }

    m_playFabResources = std::make_unique<ATG::PlayFabResources>(PLAYFAB_TITLE_ID, m_liveResources->GetUser());
    m_playFabResources->LoginToPlayFab();
    Log("PlayFab(TitleID %s): gamertag \"%s\" signed in", PLAYFAB_TITLE_ID, m_liveResources->GetGamertag().c_str());

    // Enable the button after signing in to PlayFab
    m_AddUserButton->SetEnabled(true);
    SetDataButtonsEnabled(false);

    // Set focus to the Add User button
    m_uiManager.SetFocus(m_AddUserButton);
}

void Sample::InitializeUI()
{
    auto layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/sample_layout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());
    m_consoleWindow = m_uiManager.FindTypedById<UIConsoleWindow>(ID("ConsoleWindow"));

    // Find buttons
    m_AddUserButton = m_uiManager.FindTypedById<UIButton>(ID("AddUserButton"));
    m_DataSaveButton = m_uiManager.FindTypedById<UIButton>(ID("DataSaveButton"));
    m_DataLoadButton = m_uiManager.FindTypedById<UIButton>(ID("DataLoadButton"));
    m_DataDeleteButton = m_uiManager.FindTypedById<UIButton>(ID("DataDeleteButton"));
    m_SetDescriptionButton = m_uiManager.FindTypedById<UIButton>(ID("SetDescriptionButton"));
    m_UploadReleaseDeviceButton = m_uiManager.FindTypedById<UIButton>(ID("UploadReleaseDeviceButton"));
    m_UploadKeepActiveButton = m_uiManager.FindTypedById<UIButton>(ID("UploadKeepActiveButton"));

    // Disable the button until sign-in is complete
    SetButtonsEnabled(false);

    m_AddUserButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedAddUserButton();
        });
    m_DataSaveButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedDataSaveButton();
        });
    m_DataLoadButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedDataLoadButton();
        });
    m_DataDeleteButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedDataDeleteButton();
        });
    m_SetDescriptionButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedSetDescriptionButton();
        });
    m_UploadReleaseDeviceButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedUploadReleaseDeviceButton();
        });
    m_UploadKeepActiveButton->ButtonState().AddListenerWhen(UIButton::State::Pressed, [this](UIButton*)
        {
            OnPressedUploadKeepActiveButton();
        });

}

HRESULT Sample::InitializePlayFab()
{
    // PF Core & Services
    HRESULT hr = PFInitialize(nullptr);
    if (FAILED(hr))
    {
        return hr;
    }

    return S_OK;
}

HRESULT Sample::InitializeGameSaves()
{
    PFGameSaveInitArgs args{};
    args.backgroundQueue = nullptr;
    args.options = static_cast<uint64_t>(PFGameSaveInitOptions::None);
    // On PC, saveFolder can be used to choose the root path for game saves (GRTS will store saves there).
    // On Xbox consoles, this value is ignored and the platform provides the save path.

#if defined(_GAMING_DESKTOP)
    std::string saveRoot;
    {
        char cwd[MAX_PATH]{};
        if (GetCurrentDirectoryA(MAX_PATH, cwd) > 0)
        {
            saveRoot = cwd;
            saveRoot += "\\GameSaves";
            CreateDirectoryA(saveRoot.c_str(), nullptr);
        }
    }

    if (!saveRoot.empty())
    {
        args.saveFolder = saveRoot.c_str();
    }
    else
    {

        args.saveFolder = nullptr;
    }
#else
    args.saveFolder = nullptr;
#endif

    HRESULT hr = PFGameSaveFilesInitialize(&args);
    if (FAILED(hr))
    {
        Log("PFGameSaveFilesInitialize failed: 0x%08X", hr);
        return hr;
    }
    return S_OK;
}

void Sample::UninitializeGameSaves()
{
    if (m_mainAsyncQueue)
    {
        XAsyncBlock asyncBlock{};
        asyncBlock.queue = m_mainAsyncQueue;
        asyncBlock.callback = nullptr;

        HRESULT hr = PFGameSaveFilesUninitializeAsync(&asyncBlock);
        if (SUCCEEDED(hr))
        {
            while (XAsyncGetStatus(&asyncBlock, false) == E_PENDING)
            {
                XTaskQueueDispatch(m_mainAsyncQueue, XTaskQueuePort::Completion, 0);
                Sleep(10);
            }

            hr = PFGameSaveFilesUninitializeResult(&asyncBlock);
            if (SUCCEEDED(hr))
            {
                Log("PFGameSaveFilesUninitialize completed successfully");
            }
            else
            {
                Log("PFGameSaveFilesUninitialize failed: 0x%08X", hr);
            }
        }
        else
        {
            Log("PFGameSaveFilesUninitializeAsync failed to start: 0x%08X", hr);
        }
    }

    // --- PFUninitializeAsync ---
    if (m_mainAsyncQueue)
    {
        XAsyncBlock asyncBlock{};
        asyncBlock.queue = m_mainAsyncQueue;
        asyncBlock.callback = nullptr;

        HRESULT hr = PFUninitializeAsync(&asyncBlock);
        if (SUCCEEDED(hr))
        {
            while (XAsyncGetStatus(&asyncBlock, false) == E_PENDING)
            {
                XTaskQueueDispatch(m_mainAsyncQueue, XTaskQueuePort::Completion, 0);
                Sleep(10);
            }

            hr = XAsyncGetStatus(&asyncBlock, true);
            if (SUCCEEDED(hr))
            {
                Log("PFUninitializeAsync completed successfully");
            }
            else
            {
                Log("PFUninitializeAsync failed: 0x%08X", hr);
            }
        }
        else
        {
            Log("PFUninitializeAsync failed to start: 0x%08X", hr);
        }
    }
}

HRESULT Sample::AddUserToGameSaves()
{
    // Ensure we have a persistent local user handle
    if (m_localUser == nullptr)
    {
        HRESULT hrEnsure = EnsurePersistentLocalUser();
        if (FAILED(hrEnsure))
        {
            Log("No local user available for GameSave: 0x%08X", hrEnsure);
            return hrEnsure;
        }
    }

    // Suppress Code Analysis warning: m_localUser is guaranteed non-null here
    _Analysis_assume_(m_localUser != nullptr);

    SetButtonsEnabled(false);

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(this);
    asyncBlock->callback = +[](XAsyncBlock* asyncBlock)
        {
            auto async = std::unique_ptr<XAsyncBlock>(asyncBlock);
            auto* sample = static_cast<Sample*>(asyncBlock->context);

            HRESULT hr = PFGameSaveFilesAddUserWithUiResult(asyncBlock);
            if (SUCCEEDED(hr))
            {
                sample->Log("PFGameSaveFilesAddUserWithUiAsync completed!");

                sample->m_userAddedToGS = true;
                char folder[MAX_PATH] = {};

                hr = PFGameSaveFilesGetFolder(sample->m_localUser, MAX_PATH, folder, nullptr);

                if (SUCCEEDED(hr))
                {
                    sample->m_saveRoot = folder;

                    hr = PFGameSaveFilesGetRemainingQuota(sample->m_localUser, &sample->m_remainingQuota);
                    if (SUCCEEDED(hr))
                    {
                        sample->Log("PFGameSaveFilesGetRemainingQuota: %lld bytes remaining", sample->m_remainingQuota);
                        sample->RegisterActiveDeviceChangedCallback();
                        sample->DoLoad();
                    }
                    else if (hr == E_PF_GAMESAVE_DISCONNECTED_FROM_CLOUD)
                    {
                        sample->Log("PFGameSaveFilesGetRemainingQuota: Disconnected from cloud (offline mode)");
                    }
                    else
                    {
                        sample->Log("PFGameSaveFilesGetRemainingQuota FAILED: 0x%08X", hr);
                    }
                }
                else
                {
                    sample->Log("PFGameSaveFilesGetFolder FAILED");
                }
                sample->SetDataButtonsEnabled(true);
            }
            else {
                sample->Log("PFGameSaveFilesAddUserWithUiAsync FAILED: 0x%08X", hr);
                sample->m_AddUserButton->SetEnabled(true);
            }
        };

    Log("PFGameSaveFilesAddUserWithUiAsync started");

    // Start the async operation. If it fails synchronously, free the XAsyncBlock.
    HRESULT hr = PFGameSaveFilesAddUserWithUiAsync(m_localUser, PFGameSaveFilesAddUserOptions::None, asyncBlock);
    if (FAILED(hr))
    {
        Log("PFGameSaveFilesAddUserWithUiAsync failed to start: 0x%08X", hr);
        m_AddUserButton->SetEnabled(true);
        delete asyncBlock;
    }

    return hr;
}

void Sample::RegisterActiveDeviceChangedCallback()
{
    if (!m_localUser)
    {
        Log("Cannot register ActiveDeviceChanged callback: No local user");
        return;
    }

    HRESULT hr = PFGameSaveFilesSetActiveDeviceChangedCallback(
        m_mainAsyncQueue,
        +[](PFLocalUserHandle /*localUserHandle*/, PFGameSaveDescriptor* /*activeDevice*/, void* context)
        {
            Sample* sample = static_cast<Sample*>(context);

            sample->Log("-----------------------------------------------------------");
            sample->Log("Active device has changed! This device is no longer active.");
            sample->Log("Please re-add user to continue.");
            sample->Log("-----------------------------------------------------------");

            sample->ResetToAddUserState();
        },
        this
    );

    if (SUCCEEDED(hr))
    {
        Log("ActiveDeviceChanged callback registered successfully");
    }
    else
    {
        Log("Failed to register ActiveDeviceChanged callback: 0x%08X", hr);
    }
}


HRESULT Sample::EnsurePersistentLocalUser()
{
    // If a local user handle already exists, recreate it for the current XUser
    CleanupPersistentLocalUser();

    XUserHandle xuser = (m_liveResources) ? m_liveResources->GetUser() : nullptr;
    if (!xuser)
    {
        return E_FAIL;
    }

    // Build endpoint from title id
    std::string endpoint = "https://";
    endpoint += PLAYFAB_TITLE_ID;
    endpoint += ".playfabapi.com";

    HRESULT hr = PFServiceConfigCreateHandle(endpoint.c_str(), PLAYFAB_TITLE_ID, &m_localServiceConfig);
    if (FAILED(hr))
    {
        m_localServiceConfig = nullptr;
        return hr;
    }

    hr = PFLocalUserCreateHandleWithXboxUser(m_localServiceConfig, xuser, nullptr, &m_localUser);
    if (FAILED(hr))
    {
        if (m_localServiceConfig)
        {
            PFServiceConfigCloseHandle(m_localServiceConfig);
            m_localServiceConfig = nullptr;
        }
        m_localUser = nullptr;
        return hr;
    }

    return S_OK;
}

void Sample::CleanupPersistentLocalUser()
{
    if (m_localUser)
    {
        PFLocalUserCloseHandle(m_localUser);
        m_localUser = nullptr;
    }
    if (m_localServiceConfig)
    {
        PFServiceConfigCloseHandle(m_localServiceConfig);
        m_localServiceConfig = nullptr;
    }
}

void Sample::ResetToAddUserState()
{
    m_userAddedToGS = false;
    m_saveRoot.clear();
    m_AddUserButton->SetEnabled(true);
    SetDataButtonsEnabled(false);
    m_uiManager.SetFocus(m_AddUserButton);
}

void Sample::DoLoad()
{
    if (m_saveRoot.empty())
    {
        Log("Load failed: No save root yet");
        return;
    }
    // NOTE:
    //  Save files should not be written directly under the save root.
    //  All files in the root share a single atomic unit, which increases the chance
    //  of merge conflicts when additional files are added later.
    //
    //  Instead, place frequently updated data under a subfolder so that each folder
    //  (Save1, settings, etc.) can map to its own atomic unit.
    //
    //  Example structure:
    //      SaveRoot/
    //          Save1/
    //              savegame.dat   <- independent atomic unit
    //
    std::string path = m_saveRoot + "\\settings\\savegame.dat";
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
    {
        Log("Load failed: file not found");
        return;
    }
    uint8_t header[4]{};
    f.read(reinterpret_cast<char*>(header), 4);
    if (f.gcount() == 4)
    {
        // Log first 4 bytes read (in hex)
        Log("Loaded game data. Header: %02X %02X %02X %02X", header[0], header[1], header[2], header[3]);
    }
    else
    {
        Log("Load failed: file too small to read header");
    }
}

void Sample::DoSave()
{
    if (!m_userAddedToGS || m_saveRoot.empty())
    {
        return;
    }

    const size_t fileSize = (size_t)(1 * 512 * 1024); // 512k
    std::vector<uint8_t> buf(fileSize);
    // Fill with random bytes
    std::uniform_int_distribution<int> dist(0, 255);
    for (size_t i = 0; i < fileSize; ++i)
    {
        buf[i] = static_cast<uint8_t>(dist(m_rng));
    }

    std::string folder = m_saveRoot + "\\settings";
    std::string path = folder + "\\savegame.dat";

    CreateDirectoryA(folder.c_str(), nullptr);

    std::ofstream f(path, std::ios::binary);
    if (f.is_open())
    {
        f.write(reinterpret_cast<const char*>(buf.data()), (std::streamsize)buf.size());
        // Log first 4 bytes of what we saved (in hex)
        Log("Saved game data. Header: %02X %02X %02X %02X", buf[0], buf[1], buf[2], buf[3]);
    }
    else
    {
        Log("Save failed: could not open file for writing");
        return;
    }
}

void Sample::DoDelete()
{
    if (m_saveRoot.empty())
    {
        return;
    }

    std::string folder = m_saveRoot + "\\settings";
    std::string path = folder + "\\savegame.dat";

    DWORD attrs = GetFileAttributesA(path.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES)
    {
        if (DeleteFileA(path.c_str()))
        {
            Log("Deleted local save game file");
        }
        else
        {
            Log("Failed to delete local save game file");
        }
    }
    else
    {
        Log("Local save game file not found");
    }
}

void Sample::DoUpload(bool keepDeviceActive)
{
    if (!m_userAddedToGS)
    {
        SetButtonsEnabled(true);
        return;
    }

    m_keepDeviceActiveOnUpload = keepDeviceActive;

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(this);
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        auto async = std::unique_ptr<XAsyncBlock>(asyncBlock);
        auto* sample = static_cast<Sample*>(asyncBlock->context);

        HRESULT hr = PFGameSaveFilesUploadWithUiResult(asyncBlock);
        if (SUCCEEDED(hr))
        {
            sample->Log("PFGameSaveFilesUploadWithUiAsync completed");
            if (!sample->m_keepDeviceActiveOnUpload)
            {
                sample->Log("Device released - User must be re-added to continue operations");

                // Reset state after ReleaseDeviceAsActive - requires re-adding user
                sample->UninitializeGameSaves();
                sample->CleanupPersistentLocalUser();

                hr = sample->InitializePlayFab();
                if (FAILED(hr))
                {
                    sample->Log("InitializePlayFab after ReleaseDeviceAsActive FAILED: 0x%08X", hr);
                    return;
                }

                hr = sample->InitializeGameSaves();
                if (FAILED(hr))
                {
                    sample->Log("InitializeGameSaves after ReleaseDeviceAsActive FAILED: 0x%08X", hr);
                    return;
                }

                hr = sample->EnsurePersistentLocalUser();
                if (FAILED(hr))
                {
                    sample->Log("EnsurePersistentLocalUser after ReleaseDeviceAsActive FAILED: 0x%08X", hr);
                    return;
                }

                sample->ResetToAddUserState();
            }
            else
            {
                sample->SetDataButtonsEnabled(true);
                sample->Log("Device kept active - User can continue operations without re-adding");
            }
        }
        else
        {
            sample->Log("PFGameSaveFilesUploadWithUiAsync failed: 0x%08X", hr);
        }
    };

    auto opt = keepDeviceActive ? PFGameSaveFilesUploadOption::KeepDeviceActive : PFGameSaveFilesUploadOption::ReleaseDeviceAsActive;
    Log(keepDeviceActive ? "PFGameSaveFilesUploadWithUiAsync started (keep active)" : "PFGameSaveFilesUploadWithUiAsync started (release device)");

    HRESULT hr = PFGameSaveFilesUploadWithUiAsync(m_localUser, opt, asyncBlock);
    if (FAILED(hr))
    {
        Log("PFGameSaveFilesUploadWithUiAsync start failed: 0x%08X", hr);
        SetButtonsEnabled(true);
        delete asyncBlock;
        return;
    }
}

void Sample::DoSetSaveDescription()
{
    // Require a valid local save root to proceed (user added and folder retrieved)
    if (!m_userAddedToGS || m_saveRoot.empty())
    {
        Log("SetSaveDescription failed: No local save root yet");
        SetButtonsEnabled(true);
        return;
    }

    // Create a simple short description; include a timestamp so it changes between calls
    wchar_t wbuf[128]{};
    SYSTEMTIME st{};
    GetLocalTime(&st);
    swprintf_s(wbuf, L"Saved %04u-%02u-%02u %02u:%02u:%02u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    // Convert to UTF-8 for API
    char utf8[256]{};
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, utf8, static_cast<int>(sizeof(utf8)), nullptr, nullptr);

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_mainAsyncQueue;
    asyncBlock->context = reinterpret_cast<void*>(this);
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
        {
            auto async = std::unique_ptr<XAsyncBlock>(asyncBlock);
            auto* sample = static_cast<Sample*>(asyncBlock->context);

            HRESULT hr = PFGameSaveFilesSetSaveDescriptionResult(asyncBlock);

            if (SUCCEEDED(hr))
            {
                sample->Log("PFGameSaveFilesSetSaveDescriptionAsync completed");
            }
            else
            {
                sample->Log("PFGameSaveFilesSetSaveDescriptionAsync failed: 0x%08X", hr);
            }
            sample->SetButtonsEnabled(true);
        };

    Log("PFGameSaveFilesSetSaveDescriptionAsync started");

    HRESULT hr = PFGameSaveFilesSetSaveDescriptionAsync(m_localUser, utf8, asyncBlock);
    if (FAILED(hr))
    {
        Log("PFGameSaveFilesSetSaveDescriptionAsync start failed: 0x%08X", hr);
        SetButtonsEnabled(true);
        delete asyncBlock;
    }
}

void Sample::SetDataButtonsEnabled(bool enabled)
{
    m_DataSaveButton->SetEnabled(enabled);
    m_DataLoadButton->SetEnabled(enabled);
    m_DataDeleteButton->SetEnabled(enabled);
    m_SetDescriptionButton->SetEnabled(enabled);
    m_UploadReleaseDeviceButton->SetEnabled(enabled);
    m_UploadKeepActiveButton->SetEnabled(enabled);
}

void Sample::SetButtonsEnabled(bool enabled)
{
    m_AddUserButton->SetEnabled(enabled);
    SetDataButtonsEnabled(enabled);
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

void Sample::OnPressedAddUserButton()
{
    AddUserToGameSaves();
}

void Sample::OnPressedDataSaveButton()
{
    DoSave();
}

void Sample::OnPressedDataLoadButton()
{
    DoLoad();
}

void Sample::OnPressedDataDeleteButton()
{
    DoDelete();

}
void Sample::OnPressedSetDescriptionButton()
{
    SetButtonsEnabled(false);
    DoSetSaveDescription();
}

void Sample::OnPressedUploadReleaseDeviceButton()
{
    SetButtonsEnabled(false);
    DoUpload(false);
}

void Sample::OnPressedUploadKeepActiveButton()
{
    SetButtonsEnabled(false);
    DoUpload(true);
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
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

    auto mouse = m_mouse->GetState();
    mouse;

    m_liveInfoHUD->Update(m_deviceResources->GetCommandQueue());

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);

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
    m_uiManager.Render();

    ID3D12DescriptorHeap* heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_liveInfoHUD->Render(commandList);

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
    const auto rtvDescriptor = m_deviceResources->GetRenderTargetView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    const auto viewport = m_deviceResources->GetScreenViewport();
    const auto scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXEndEvent(commandList);
}
#pragma endregion

#pragma region Message Handlers
// Message handlers
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
    m_inputState.Reset();
}

void Sample::OnWindowMoved()
{
    const auto r = m_deviceResources->GetOutputSize();
    m_deviceResources->WindowSizeChanged(r.right, r.bottom);
}

void Sample::OnDisplayChange()
{
    m_deviceResources->UpdateColorSpace();
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
    width = 1280;
    height = 720;
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

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    m_resourceDescriptors = std::make_unique<DirectX::DescriptorPile>(device,
        Descriptors::Count,
        Descriptors::Reserve
        );

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    m_liveInfoHUD->RestoreDevice(device, rtState, resourceUpload, *m_resourceDescriptors);

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    auto const os = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 200, os.right, os.bottom);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    m_liveInfoHUD->SetViewport(m_deviceResources->GetScreenViewport());
    const auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);

}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_liveInfoHUD->ReleaseDevice();
    m_resourceDescriptors.reset();
    m_uiManager.GetStyleManager().ResetStyleRenderer();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
