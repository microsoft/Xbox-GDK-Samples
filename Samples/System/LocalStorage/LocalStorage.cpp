//--------------------------------------------------------------------------------------
// LocalStorage.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LocalStorage.h"

#include "ATGColors.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_currentState(LocalStorageLocation::PersistentLocalStorage),
    m_writeTestEnabled(false),
    m_writeTestWriteDelay(0.0f),
    m_writeTestLogDelay(0.0f),
    m_writeTestBytesRemaining(0),
    m_writeTestBytesPerSecond(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->RegisterDeviceNotify(this);

    XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_taskQueue);

    InitializePersistentLocalStorage();
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    XTaskQueueTerminate(m_taskQueue, false, nullptr, nullptr);
    while (XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, INFINITE));
    XTaskQueueCloseHandle(m_taskQueue);
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

    SetupUI();
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

    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, 0);

    Test_UpdateStressWrites(elapsedTime);

    // Push logs to UI
    {
        std::lock_guard<std::mutex> scopeLock(m_logMutex);

        for (size_t i = 0; i < m_logQueue.size(); ++i)
        {
            m_consoleWindow->AppendLineOfText(m_logQueue[i]);
        }
        m_logQueue.clear();
    }

    auto pad = m_gamePad->GetState(GamePad::c_MostRecent);
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

    m_inputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_inputState);

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
    m_inputState.Reset();
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
    width = 1280;
    height = 720;
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto size = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(size.right, size.bottom);
}

void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
    m_uiManager.GetStyleManager().ResetStyleRenderer();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

#pragma region UI
void Sample::SetupUI()
{
    auto layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_consoleWindow = layout->GetChildById(ID("Output_Console_Window_Outer_Panel"))->GetTypedChildById<UIConsoleWindow>(ID("Output_Console_Window"));
    m_storageTypeText = layout->GetTypedChildById<UIStaticText>(ID("Storage_Info_Text"));

    auto buttonStackPanel = layout->GetChildById(ID("Button_Stack_Panel"));
    for (size_t buttonIndex = 0; buttonIndex < s_numTestButtons; ++buttonIndex)
    {
        char idBuffer[17] = {};
        sprintf_s(idBuffer, 16, "Button_%llu", buttonIndex + 1);
        m_testButtons[buttonIndex] = buttonStackPanel->GetTypedChildById<UIButton>(ID(idBuffer));
        SetupButtonHandler(m_testButtons[buttonIndex], [&, buttonIndex]() { InvokeTest(buttonIndex); });
    }

    m_leftButton = layout->GetTypedChildById<UIButton>(ID("Left_Button"));
    m_leftButton->GetTypedSubElementById<UIStaticText>(ID("Button_Label"))->SetDisplayText("<");
    SetupButtonHandler(m_leftButton, [&]() { PrevStorageType(); });

    m_rightButton = layout->GetTypedChildById<UIButton>(ID("Right_Button"));
    m_rightButton->GetTypedSubElementById<UIStaticText>(ID("Button_Label"))->SetDisplayText(">");
    SetupButtonHandler(m_rightButton, [&]() { NextStorageType(); });

    // Start out with tests for PLS
    SetStorageType(LocalStorageLocation::PersistentLocalStorage);
}

void Sample::SetStorageTypeLabel(LocalStorageLocation location)
{
    switch (location)
    {
    case LocalStorageLocation::PersistentLocalStorage:
        m_storageTypeText->SetDisplayText("Storage Type: PLS");
        break;

#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)

    case LocalStorageLocation::TemporaryLocalStorage:
        m_storageTypeText->SetDisplayText("Storage Type: Temp Drive");
        break;

    case LocalStorageLocation::SystemScratch:
        m_storageTypeText->SetDisplayText("Storage Type: System Scratch");
        break;

    case LocalStorageLocation::InstalledGameData:
        m_storageTypeText->SetDisplayText("Storage Type: Installed Game Data");
        break;

#else

    case LocalStorageLocation::TempFolder:
        m_storageTypeText->SetDisplayText("Storage Type: TEMP Folder");
        break;

    case LocalStorageLocation::LocalAppData:
        m_storageTypeText->SetDisplayText("Storage Type: LocalAppData");
        break;

#endif
    };
    
}

void Sample::SetupButtonHandler(std::shared_ptr<ATG::UITK::UIButton> button, std::function<void()> onClicked)
{
    button->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [&, onClicked](UIButton* button)
        {
            onClicked();

            button->GetTypedSubElementById<UIStaticText>(ID("Button_Label"))->SetStyleId(ID("Button_Label_Style"));
        });
    button->ButtonState().AddListenerWhen(UIButton::State::Focused,
        [&](UIButton* button)
        {
            button->GetTypedSubElementById<UIStaticText>(ID("Button_Label"))->SetStyleId(ID("Button_Label_Style_Dark"));
        });
    button->ButtonState().AddListenerWhen(UIButton::State::Hovered,
        [&](UIButton* button)
        {
            button->GetTypedSubElementById<UIStaticText>(ID("Button_Label"))->SetStyleId(ID("Button_Label_Style_Dark"));
        });
    button->ButtonState().AddListenerWhen(UIButton::State::Normal,
        [&](UIButton* button)
        {
            button->GetTypedSubElementById<UIStaticText>(ID("Button_Label"))->SetStyleId(ID("Button_Label_Style"));
        });
}

void Sample::SetupTestButton(size_t index, std::string buttonLabel, std::function<void()> testMethod)
{
    m_testCallbacks[index] = testMethod;
    m_testButtons[index]->SetEnabled(true);
    m_testButtons[index]->SetFocusable(true);
    m_testButtons[index]->GetTypedSubElementById<UIStaticText>(ID("Button_Label"))->SetDisplayText(buttonLabel);
}

void Sample::SetupTestButtons(LocalStorageLocation location)
{
    size_t numButtonsSetup = 0;

    // Basic buttons for each test
#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)
    SetupTestButton(numButtonsSetup++, "Get Write Stats", [&]() { Test_GetWriteStats(); });
#endif
    SetupTestButton(numButtonsSetup++, "Get Storage Info", [&, location]() { Test_GetStorageInfo(location); });
    SetupTestButton(numButtonsSetup++, "Query Free Space", [&, location]() { Test_QueryFreeSpace(location); });
    SetupTestButton(numButtonsSetup++, "Write File", [&, location]() { Test_WriteFile(location); });
    SetupTestButton(numButtonsSetup++, "Read File", [&, location]() { Test_ReadFile(location); });

    // Only add PLS info and stress tests for Xbox since PC does not enforce any write rate limiting.
#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)
    if (location == LocalStorageLocation::PersistentLocalStorage)
    {
        SetupTestButton(numButtonsSetup++, "Get PLS Info", [&]() { Test_GetPLSInfo(); });
        SetupTestButton(numButtonsSetup++, "Fill Available PLS", [&]() { Test_FillAvailablePLS(); });
    }

    if (location != LocalStorageLocation::InstalledGameData)
    {
        SetupTestButton(numButtonsSetup++, "Stress Write Stats", [&, location]() { Test_StressWriteStats(location); });
    }
#endif

    // Hide rest of buttons
    for (size_t index = numButtonsSetup; index < s_numTestButtons; ++index)
    {
        m_testCallbacks[index] = {};
        m_testButtons[index]->SetEnabled(false);
        m_testButtons[index]->SetFocusable(false);
        m_testButtons[index]->GetTypedSubElementById<UIStaticText>(ID("Button_Label"))->SetDisplayText("");
    }
}

void Sample::InvokeTest(size_t index)
{
    assert(index < s_numTestButtons);

    if (m_testCallbacks[index])
    {
        m_testCallbacks[index]();
    }
}

void Sample::NextStorageType()
{
#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)

    switch (m_currentState)
    {
    case LocalStorageLocation::PersistentLocalStorage:
        SetStorageType(LocalStorageLocation::TemporaryLocalStorage);
        break;
    case LocalStorageLocation::TemporaryLocalStorage:
        SetStorageType(LocalStorageLocation::SystemScratch);
        break;
    case LocalStorageLocation::SystemScratch:
        SetStorageType(LocalStorageLocation::InstalledGameData);
        break;
    case LocalStorageLocation::InstalledGameData:
        SetStorageType(LocalStorageLocation::PersistentLocalStorage);
        break;
    };

#else

    switch (m_currentState)
    {
    case LocalStorageLocation::PersistentLocalStorage:
        SetStorageType(LocalStorageLocation::TempFolder);
        break;
    case LocalStorageLocation::TempFolder:
        SetStorageType(LocalStorageLocation::LocalAppData);
        break;
    case LocalStorageLocation::LocalAppData:
        SetStorageType(LocalStorageLocation::PersistentLocalStorage);
        break;
    };

#endif
}

void Sample::PrevStorageType()
{
#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)

    switch (m_currentState)
    {
    case LocalStorageLocation::PersistentLocalStorage:
        SetStorageType(LocalStorageLocation::InstalledGameData);
        break;
    case LocalStorageLocation::TemporaryLocalStorage:
        SetStorageType(LocalStorageLocation::PersistentLocalStorage);
        break;
    case LocalStorageLocation::SystemScratch:
        SetStorageType(LocalStorageLocation::TemporaryLocalStorage);
        break;
    case LocalStorageLocation::InstalledGameData:
        SetStorageType(LocalStorageLocation::SystemScratch);
        break;
    };

#else

    switch (m_currentState)
    {
    case LocalStorageLocation::PersistentLocalStorage:
        SetStorageType(LocalStorageLocation::LocalAppData);
        break;
    case LocalStorageLocation::TempFolder:
        SetStorageType(LocalStorageLocation::PersistentLocalStorage);
        break;
    case LocalStorageLocation::LocalAppData:
        SetStorageType(LocalStorageLocation::TempFolder);
        break;
    };

#endif
}

void Sample::SetStorageType(LocalStorageLocation location)
{
    m_currentState = location;
    SetStorageTypeLabel(location);
    SetupTestButtons(location);
}

void Sample::Log(const char* log)
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);

    OutputDebugStringA(log);
    OutputDebugStringA("\n");

    if (m_consoleWindow)
    {
        // m_consoleWindow->AppendLineOfText(text) is not thread-safe with rendering, so queue and push on update
        m_logQueue.push_back(log);
    }
}
#pragma endregion

#pragma region Local Storage Examples
void Sample::InitializePersistentLocalStorage()
{
    size_t pathSize = 0;
    DX::ThrowIfFailed(XPersistentLocalStorageGetPathSize(&pathSize));
    m_plsPath.resize(pathSize);
    DX::ThrowIfFailed(XPersistentLocalStorageGetPath(pathSize, m_plsPath.data(), &pathSize));

    // Remove the terminating null from the string since std::string provides one automatically.
    m_plsPath.resize(m_plsPath.size() - 1);

    // Ensure the path ends in a slash
    if (m_plsPath.size() > 0 && m_plsPath[m_plsPath.size() - 1] != '\\')
    {
        m_plsPath += "\\";
    }
}

std::string Sample::GetLocalStoragePath(LocalStorageLocation location)
{
    switch (location)
    {
    case LocalStorageLocation::PersistentLocalStorage:
        // See InitializePersistentLocalStorage() for how this path is acquired
        return m_plsPath;

#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)

    case LocalStorageLocation::TemporaryLocalStorage:
        return "T:\\";

    case LocalStorageLocation::SystemScratch:
        return "D:\\";

    case LocalStorageLocation::InstalledGameData:
        return "G:\\";

#else

    case LocalStorageLocation::TempFolder:
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-gettemppatha
        // GetTempPath returns the path ending in a backslash (\), but doesn't write the null terminator.
        char path[MAX_PATH + 2] = {};
        DWORD result = GetTempPathA(MAX_PATH + 2, path);
        if (result == 0)
        {
            throw std::runtime_error("Failed to get temp path.");
        }
        return path;
    }

    case LocalStorageLocation::LocalAppData:
    {
        // https://docs.microsoft.com/en-us/windows/win32/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath
        // SHGetKnownFolderPath retrieves the full path of a known folder from the OS.
        PWSTR path = nullptr;
        DX::ThrowIfFailed(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path));
        std::string outPath = DX::WideToUtf8(path);
        CoTaskMemFree(path);
        if (outPath.size() > 0 && outPath[outPath.size() - 1] != '\\')
        {
            outPath += "\\";
        }
        return outPath;
    }

#endif

    default:
        return {};
    };
}

std::string Sample::LocalStorageLocationToString(LocalStorageLocation location)
{
    switch (location)
    {
    case LocalStorageLocation::PersistentLocalStorage:
        return "PLS";

#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)

    case LocalStorageLocation::TemporaryLocalStorage:
        return "Temp Drive";

    case LocalStorageLocation::SystemScratch:
        return "System Scratch";

    case LocalStorageLocation::InstalledGameData:
        return "Installed Game Data";

#else

    case LocalStorageLocation::TempFolder:
        return "TEMP Folder";

    case LocalStorageLocation::LocalAppData:
        return "LocalAppData";

#endif

    default:
        return "";
    };
}

bool Sample::WriteFile(const char* filePath, const std::vector<uint8_t>& data, size_t numDuplicates, bool append, bool log)
{
    std::vector<char> buf(MAX_PATH + 256, 0);
    if (log)
    {
        sprintf_s(buf.data(), buf.size(), "Writing to [%s] with data size = %llu (%.2fMB)", filePath, data.size() * numDuplicates,
            static_cast<float>(static_cast<double>(data.size()) * static_cast<double>(numDuplicates) / (1024.0 * 1024.0)));
        Log(buf.data());
    }

    // First try to open an existing file
    DWORD disposition = append ? static_cast<DWORD>(OPEN_EXISTING) : static_cast<DWORD>(TRUNCATE_EXISTING);
    DWORD desiredAccess = append ? static_cast<DWORD>(FILE_APPEND_DATA) : static_cast<DWORD>(GENERIC_WRITE);
    HANDLE fileHandle = CreateFileA(
        filePath,               /* lpFileName*/
        desiredAccess,          /* dwDesiredAccess */
        0,                      /* dwShareMode */
        NULL,                   /* lpSecurityAttributes */
        disposition,            /* dwCreationDisposition */
        FILE_ATTRIBUTE_NORMAL,  /* dwFlagsAndAttributes */
        NULL/* hTemplateFile */
    );

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        // Create new file
        fileHandle = CreateFileA(
            filePath,               /* lpFileName*/
            GENERIC_WRITE,          /* dwDesiredAccess */
            0,                      /* dwShareMode */
            NULL,                   /* lpSecurityAttributes */
            CREATE_NEW,             /* dwCreationDisposition */
            FILE_ATTRIBUTE_NORMAL,  /* dwFlagsAndAttributes */
            NULL/* hTemplateFile */
        );
    }

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        sprintf_s(buf.data(), buf.size(), "  Failed to create file. Last Error: %d", GetLastError());
        Log(buf.data());
        return false;
    }

    for (size_t index = 0; index < numDuplicates; ++index)
    {
        const size_t maxBytesPerCall = 2 * 1024 * 1024;
        size_t bytesWrittenOverall = 0;
        while (bytesWrittenOverall < data.size())
        {
            const size_t bytesRemaining = data.size() - bytesWrittenOverall;
            const DWORD bytesToWrite = bytesRemaining <= maxBytesPerCall ? static_cast<DWORD>(bytesRemaining) : static_cast<DWORD>(maxBytesPerCall);
            DWORD bytesWrittenThisCall = 0;
            BOOL result = ::WriteFile(
                fileHandle,
                data.data() + bytesWrittenOverall,
                bytesToWrite,
                &bytesWrittenThisCall,
                NULL
            );

            bytesWrittenOverall += bytesWrittenThisCall;

            if (!result)
            {
                CloseHandle(fileHandle);
                sprintf_s(buf.data(), buf.size(), "  Failed to write data to file. Last Error: %d", GetLastError());
                Log(buf.data());
                return false;
            }
        }
    }

    if (log)
    {
        Log("  Finished!");
    }
    CloseHandle(fileHandle);
    return true;
}

bool Sample::ReadFile(const char* filePath, std::vector<uint8_t>& outData)
{
    std::vector<char> buf(MAX_PATH + 256, 0);
    sprintf_s(buf.data(), buf.size(), "Reading file [%s]", filePath);
    Log(buf.data());

    HANDLE fileHandle = CreateFileA(
        filePath,               /* lpFileName*/
        GENERIC_READ,           /* dwDesiredAccess */
        0,                      /* dwShareMode */
        NULL,                   /* lpSecurityAttributes */
        OPEN_EXISTING,          /* dwCreationDisposition */
        FILE_ATTRIBUTE_NORMAL,  /* dwFlagsAndAttributes */
        NULL                    /* hTemplateFile */
    );

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        sprintf_s(buf.data(), buf.size(), "  Failed to create file. Last Error: %d", GetLastError());
        Log(buf.data());
        return false;
    }

    // Get file size
    LARGE_INTEGER fileSize = {};
    if (!GetFileSizeEx(fileHandle, &fileSize))
    {
        sprintf_s(buf.data(), buf.size(), "  Failed to get file size. Last Error: %d", GetLastError());
        Log(buf.data());
        CloseHandle(fileHandle);
        return false;
    }

    // Read data into output buffer
    outData.resize(static_cast<size_t>(fileSize.QuadPart));
    const size_t maxBytesToRead = 64 * 1024;
    size_t overallBytesRead = 0;
    while (overallBytesRead < static_cast<size_t>(fileSize.QuadPart))
    {
        const size_t bytesRemaining = fileSize.QuadPart - overallBytesRead;
        const DWORD bytesToRead = bytesRemaining <= maxBytesToRead ? static_cast<DWORD>(bytesRemaining) : static_cast<DWORD>(maxBytesToRead);
        DWORD bytesReadThisCall = 0;
        BOOL result = ::ReadFile(
            fileHandle,
            outData.data() + overallBytesRead,
            bytesToRead,
            &bytesReadThisCall,
            NULL
        );

        if (!result)
        {
            sprintf_s(buf.data(), buf.size(), "  Failed to read from file. Last Error: %d", GetLastError());
            Log(buf.data());
            CloseHandle(fileHandle);
            return false;
        }

        overallBytesRead += bytesReadThisCall;
    }

    Log("  Finished!");
    CloseHandle(fileHandle);
    return true;
}

void Sample::Test_GetStorageInfo(LocalStorageLocation location)
{
    char buf[256] = {};

    sprintf_s(buf, 256, "%s Info:", LocalStorageLocationToString(location).c_str());
    Log(buf);
    sprintf_s(buf, 256, "  Path: %s", GetLocalStoragePath(location).c_str());
    Log(buf);

    // Count files/folders in path
    size_t numFiles = 0;
    size_t numFolders = 0;
    std::string searchString = GetLocalStoragePath(location) + "*";
    WIN32_FIND_DATAA fileData = {};
    HANDLE searchHandle = FindFirstFileA(searchString.c_str(), &fileData);
    if (searchHandle == INVALID_HANDLE_VALUE)
    {
        sprintf_s(buf, 256, "  FindFirstFileA failed. Last Error = %d", GetLastError());
        Log(buf);
        return;
    }
    do 
    {
        if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            ++numFolders;
        }
        else
        {
            ++numFiles;
        }

        
    } while (FindNextFileA(searchHandle, &fileData));
    FindClose(searchHandle);
    sprintf_s(buf, 256, "  Files at Path: %llu", numFiles);
    Log(buf);
    sprintf_s(buf, 256, "  Folders at Path: %llu", numFolders);
    Log(buf);
}

void Sample::Test_QueryFreeSpace(LocalStorageLocation location)
{
    char buf[256] = {};

    sprintf_s(buf, 256, "Querying free space for %s", LocalStorageLocationToString(location).c_str());
    Log(buf);

    // Querying free space is done using https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-getdiskfreespaceexa
    ULARGE_INTEGER freeBytesAvailableToCaller = {};
    ULARGE_INTEGER totalNumberOfBytes = {};
    ULARGE_INTEGER totalNumberOfFreeBytes = {};
    BOOL result = GetDiskFreeSpaceExA(GetLocalStoragePath(location).c_str(), &freeBytesAvailableToCaller, &totalNumberOfBytes, &totalNumberOfFreeBytes);
    if (result)
    {
        sprintf_s(buf, 256, "  Total Number of Bytes: %llu (%.2fMB)", totalNumberOfBytes.QuadPart,
            static_cast<float>(static_cast<double>(totalNumberOfBytes.QuadPart) / (1024.0 * 1024.0)));
        Log(buf);
        sprintf_s(buf, 256, "  Total Number of Free Bytes: %llu (%.2fMB)", totalNumberOfFreeBytes.QuadPart,
            static_cast<float>(static_cast<double>(totalNumberOfFreeBytes.QuadPart) / (1024.0 * 1024.0)));
        Log(buf);
        sprintf_s(buf, 256, "  Free Bytes Available to Caller: %llu (%.2fMB)", freeBytesAvailableToCaller.QuadPart,
            static_cast<float>(static_cast<double>(freeBytesAvailableToCaller.QuadPart) / (1024.0 * 1024.0)));
        Log(buf);
    }
    else
    {
        sprintf_s(buf, 256, "  Failed to get disk free space with last error = %d", GetLastError());
        Log(buf);
    }
}

void Sample::Test_WriteFile(LocalStorageLocation location)
{
    Test_StopStressWrites();

    const size_t bytesToWrite = 1024 * 1024;

#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)
    // PLS needs to confirm there is space available first
    if (location == LocalStorageLocation::PersistentLocalStorage)
    {
        XPersistentLocalStorageSpaceInfo spaceInfo;
        DX::ThrowIfFailed(XPersistentLocalStorageGetSpaceInfo(&spaceInfo));

        if (spaceInfo.availableFreeBytes < bytesToWrite)
        {
            if (bytesToWrite < spaceInfo.totalFreeBytes)
            {
                Log("Not enough available space to write file to PLS. Requesting PLS allocation increase.");

                XAsyncBlock* asyncBlock = new XAsyncBlock{};
                asyncBlock->queue = m_taskQueue;
                asyncBlock->context = this;
                asyncBlock->callback = [](XAsyncBlock* asyncBlock)
                {
                    Sample* pThis = static_cast<Sample*>(asyncBlock->context);

                    if (SUCCEEDED(XPersistentLocalStoragePromptUserForSpaceResult(asyncBlock)))
                    {
                        pThis->Test_WriteFile(LocalStorageLocation::PersistentLocalStorage);
                    }
                    else
                    {
                        pThis->Log("Failed to grow PLS.");
                    }

                    delete asyncBlock;
                };
                
                DX::ThrowIfFailed(XPersistentLocalStoragePromptUserForSpaceAsync(bytesToWrite, asyncBlock));
            }
            else
            {
                Log("Not enough space remaining in PLS to write file");

                return;
            }
        }
    }
#endif

    char buf[256] = {};

    sprintf_s(buf, 256, "Testing writing a file to %s", LocalStorageLocationToString(location).c_str());
    Log(buf);

    // Get a guid for the filename
    GUID guid = {};
    DX::ThrowIfFailed(CoCreateGuid(&guid));
    char guidString[64] = {};
    sprintf_s(guidString, "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

    std::string writePath = GetLocalStoragePath(location) + "testfile_" + guidString + ".bin";
    std::vector<uint8_t> dataToWrite(bytesToWrite, 0);

    WriteFile(writePath.c_str(), dataToWrite);
}

void Sample::Test_ReadFile(LocalStorageLocation location)
{
    char buf[256] = {};

    sprintf_s(buf, 256, "Testing reading a file from %s", LocalStorageLocationToString(location).c_str());
    Log(buf);

    // Find a test file
    std::string searchString = GetLocalStoragePath(location) + "testfile_*.bin";
    WIN32_FIND_DATAA foundFileData = {};
    HANDLE searchHandle = FindFirstFileA(searchString.c_str(), &foundFileData);
    if (searchHandle != INVALID_HANDLE_VALUE)
    {
        std::string filePath = GetLocalStoragePath(location) + foundFileData.cFileName;
        std::vector<uint8_t> fileData;
        bool result = ReadFile(filePath.c_str(), fileData);
        if (result)
        {
            sprintf_s(buf, 256, "  Read %llu bytes (%.2fMB) successfully", fileData.size(),
                static_cast<float>(static_cast<double>(fileData.size()) / (1024.0 * 1024.0)));
            Log(buf);
        }

        FindClose(searchHandle);
    }
    else
    {
        Log("No test files found to read. Test writing a file with \"Write File\" first.");
    }

#if defined(_GAMING_XBOX_SCARLETT) || defined(_GAMING_XBOX)
    // If testing Game Data read, also read from a file we know will exist
    if (location == LocalStorageLocation::InstalledGameData)
    {
        std::string readPath = GetLocalStoragePath(location) + "MicrosoftGame.config";
        std::vector<uint8_t> fileData;
        bool result = ReadFile(readPath.c_str(), fileData);
        if (result)
        {
            sprintf_s(buf, 256, "  Read %llu bytes (%.2fMB) successfully", fileData.size(),
                static_cast<float>(static_cast<double>(fileData.size()) / (1024.0 * 1024.0)));
            Log(buf);
        }
    }
#endif
}

void Sample::Test_GetWriteStats()
{
    // While this can be called on PC, the write stats are only tracked on Xbox. On PC,
    // the value will always return 0 for written/elapsed and very high numbers for interval/budget.

    Log("Getting Package Write Stats");

    XPackageWriteStats writeStats = {};
    DX::ThrowIfFailed(XPackageGetWriteStats(&writeStats));

    // elapsed and interval are in milliseconds
    char buf[256] = {};
    sprintf_s(buf, 256, "  Seconds Elapsed in Current Interval: %.2f",
        static_cast<float>(static_cast<double>(writeStats.elapsed) / 1000.0));
    Log(buf);
    sprintf_s(buf, 256, "  Total Seconds in Current Interval: %.2f",
        static_cast<float>(static_cast<double>(writeStats.interval) / 1000.0));
    Log(buf);
    sprintf_s(buf, 256, "  Bytes Written in Current Interval: %llu (%.2fMB)", writeStats.bytesWritten,
        static_cast<float>(static_cast<double>(writeStats.bytesWritten) / (1024.0 * 1024.0)));
    Log(buf);
    sprintf_s(buf, 256, "  Total Bytes Allowed in Current Interval: %llu (%.2fMB)", writeStats.budget,
        static_cast<float>(static_cast<double>(writeStats.budget) / (1024.0 * 1024.0)));
    Log(buf);
}

void Sample::Test_StressWriteStats(LocalStorageLocation location)
{
    // Write limit for XR-113 is 1GB/5min (about 3.41MB/s).
    Log("Starting a stress writer to write 2GB");
    size_t bytesPerSecond = (1024ull * 1024ull * 1024ull) / 300ull;
    size_t bytesToWrite = 2ull * 1024ull * 1024ull * 1024ull;
    Test_StartStressWrites(location, bytesToWrite, bytesPerSecond);
}

void Sample::Test_GetPLSInfo()
{
    // PLS space is not specifically provisioned or managed for PC users.
    // Therefore, the settings in MicrosoftGame.config are ignored on PC and instead the
    // entire drive is available. Reported space information here for PC is based on the installed
    // directory. It's better on PC to simply query free space for drives you use.

    Log("Getting PLS Space Info");

    XPersistentLocalStorageSpaceInfo spaceInfo = {};
    DX::ThrowIfFailed(XPersistentLocalStorageGetSpaceInfo(&spaceInfo));

    char buf[256] = {};
    sprintf_s(buf, 256, "  Available Free Bytes: %llu (%.2fMB)", spaceInfo.availableFreeBytes,
        static_cast<float>(static_cast<double>(spaceInfo.availableFreeBytes) / (1024.0 * 1024.0)));
    Log(buf);
    sprintf_s(buf, 256, "  Total Free Bytes: %llu (%.2fMB)", spaceInfo.totalFreeBytes,
        static_cast<float>(static_cast<double>(spaceInfo.totalFreeBytes) / (1024.0 * 1024.0)));
    Log(buf);
    sprintf_s(buf, 256, "  Used Bytes: %llu (%.2fMB)", spaceInfo.usedBytes,
        static_cast<float>(static_cast<double>(spaceInfo.usedBytes) / (1024.0 * 1024.0)));
    Log(buf);
    sprintf_s(buf, 256, "  Total Bytes: %llu (%.2fMB)", spaceInfo.totalBytes,
        static_cast<float>(static_cast<double>(spaceInfo.totalBytes) / (1024.0 * 1024.0)));
    Log(buf);
}

void Sample::Test_FillAvailablePLS()
{
    XPersistentLocalStorageSpaceInfo spaceInfo = {};
    DX::ThrowIfFailed(XPersistentLocalStorageGetSpaceInfo(&spaceInfo));

    // Write limit for XR-113 is 1GB/5min (about 3.41MB/s).
    Log("Starting a stress writer to fill up the remaining available space in PLS.");
    size_t bytesPerSecond = (1024ull * 1024ull * 1024ull) / 300ull;
    Test_StartStressWrites(LocalStorageLocation::PersistentLocalStorage, spaceInfo.availableFreeBytes, bytesPerSecond);
}

// Write limit for XR-113 is 1GB/5min (about 3.41MB/s).
// Bursting is allowed in the interval instead of a write rate limiting.
// First 1GB is not tracked to an interval in a title by design.
void Sample::Test_StartStressWrites(LocalStorageLocation location, size_t bytesToWrite, size_t bytesPerSecond)
{
    Test_StopStressWrites();

    char buf[256] = {};
    sprintf_s(buf, 256, "Beginning stress writing to %s", LocalStorageLocationToString(location).c_str());
    Log(buf);

    m_writeTestEnabled = true;
    m_writeTestBytesRemaining = bytesToWrite;
    m_writeTestBytesPerSecond = bytesPerSecond;
    m_writeTestLogDelay = 5.0f;
    m_writeTestWriteDelay = 0.0f;
    m_writeTestPath = GetLocalStoragePath(location);
}

void Sample::Test_StopStressWrites()
{
    if (!m_writeTestEnabled)
    {
        return;
    }

    Log("Stopping current stress writer");
    m_writeTestEnabled = false;
}

void Sample::Test_UpdateStressWrites(float deltaSeconds)
{
    if (!m_writeTestEnabled)
    {
        return;
    }

    bool doWrite = false;
    bool doLog = false;

    // Check if a write should be performed
    m_writeTestWriteDelay -= deltaSeconds;
    if (m_writeTestWriteDelay <= 0.0f)
    {
        doWrite = true;
        m_writeTestWriteDelay += 1.0f;
    }

    // Check if a log should be performed
    m_writeTestLogDelay -= deltaSeconds;
    if (m_writeTestLogDelay <= 0.0f)
    {
        doLog = true;
        m_writeTestLogDelay += 5.0f;
    }

    if (doWrite)
    {
        std::string filePath = m_writeTestPath + "StressWriteFile.bin";
        const size_t bytesToWrite = m_writeTestBytesPerSecond <= m_writeTestBytesRemaining ? m_writeTestBytesPerSecond : m_writeTestBytesRemaining;
        std::vector<uint8_t> dataToWrite(bytesToWrite, 0);

        if (WriteFile(filePath.c_str(), dataToWrite, 1, true, false))
        {
            m_writeTestBytesRemaining -= bytesToWrite;
        }
        else
        {
            Log("Failed to write more data to the stress write file.");
            Test_StopStressWrites();
            Test_GetWriteStats();
            return;
        }
    }

    if (doLog)
    {
        char buf[256] = {};
        sprintf_s(buf, 256, "Bytes remaining to write: %llu (%.2fMB)", m_writeTestBytesRemaining,
            static_cast<float>(static_cast<double>(m_writeTestBytesRemaining) / (1024.0 * 1024.0)));
        Log(buf);
        Test_GetWriteStats();
    }

    if (m_writeTestBytesRemaining == 0)
    {
        Log("Stress writer finished.");
        Test_StopStressWrites();
        Test_GetWriteStats();
    }
}
#pragma endregion
