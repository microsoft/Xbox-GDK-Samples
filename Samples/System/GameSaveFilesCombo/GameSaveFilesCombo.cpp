//--------------------------------------------------------------------------------------
// GameSaveFilesCombo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameSaveFilesCombo.h"
#include "ATGColors.h"
#include "FindMedia.h"
#include "XGameSaveFiles.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

namespace
{
    // The SCID for this title, used to identify it in the Xbox Live backend and access its storage there.
    const char* c_scid = "00000000-0000-0000-0000-0000600d1fb3";
}

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_userAddInProgress(false),
    m_userHandle(nullptr),
    m_gamesavePath("")
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
    m_gamerPic = { 0 };
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }

    XUserCloseHandle(m_userHandle);
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

    UIElementPtr layout = m_uiManager.LoadLayoutFromFile("Assets/Layouts/sample_layout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_loadButton = layout->GetTypedChildById<UIButton>(ID("Load_Button"));
    m_quotaButton = layout->GetTypedChildById<UIButton>(ID("Quota_Button"));
    m_generateButton = layout->GetTypedChildById<UIButton>(ID("Generate_Button"));
    m_readButton = layout->GetTypedChildById<UIButton>(ID("Read_Button"));
    m_switchUserButton = layout->GetTypedChildById<UIButton>(ID("Switch_User_Button"));
    m_consoleWindow = layout->GetTypedChildById<UIPanel>(ID("Output_Console_Window_Outer_Panel"))
        ->GetTypedChildById<UIConsoleWindow>(ID("Output_Console_Window"));
    m_userInfoPanel = layout->GetTypedChildById<UIStackPanel>(ID("Sample_Title_Panel"))->
        GetTypedChildById<UIStackPanel>(ID("User_Info_Panel"));
    m_gamertagText = m_userInfoPanel->GetTypedChildById<UIStaticText>(ID("Gamertag_Label"));
    m_gamerpicImage = m_userInfoPanel->GetTypedChildById<UIImage>(ID("Gamerpic"));

    EnableFileIOButtons(false);

    RegisterUIEventHandlers();

    DX::ThrowIfFailed(GetUserHandle(XUserAddOptions::AddDefaultUserAllowingUI));
}

// Register the event handlers to be used for UI elements.
void Sample::RegisterUIEventHandlers()
{
    if (m_loadButton)
    {
        m_loadButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                DX::ThrowIfFailed(this->GetFolderWithUIAsync());
            });
    }

    if (m_quotaButton)
    {
        m_quotaButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                DX::ThrowIfFailed(this->GetRemainingQuota());
            });
    }

    if (m_generateButton)
    {
        m_generateButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                this->GenerateFile();
            });
    }

    if (m_readButton)
    {
        m_readButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                this->ReadData();
            });
    }

    if (m_switchUserButton)
    {
        m_switchUserButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                DX::ThrowIfFailed(this->GetUserHandle(XUserAddOptions::None));
            });
    }
}

#pragma region GameSaveFiles

HRESULT Sample::GetFolderWithUIAsync()
{
    std::unique_ptr<XAsyncBlock> asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = nullptr;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> owner = std::unique_ptr<XAsyncBlock>(asyncBlock);
        Sample* sampleContext = static_cast<Sample*>(owner->context);

        size_t folderSize = 0;
        XAsyncGetResultSize(asyncBlock, &folderSize);
        char folderResult[MAX_PATH]{};
        HRESULT hr = XGameSaveFilesGetFolderWithUiResult(asyncBlock, folderSize, folderResult);

        if (SUCCEEDED(hr))
        {
            sampleContext->Log("XGameSaveFilesGetFolderWithUiResult successful");
            sampleContext->Log(folderResult);
            sampleContext->m_gamesavePath = folderResult;
            sampleContext->EnableFileIOButtons(true);
        }
        else
        {
            sampleContext->LogFailedHR("XGameSaveFilesGetFolderWithUiResult", hr);
            sampleContext->EnableFileIOButtons(false);
        }
    };

    HRESULT hr = XGameSaveFilesGetFolderWithUiAsync(m_userHandle, c_scid, asyncBlock.get());

    if (SUCCEEDED(hr))
    {
        asyncBlock.release();
    }
    else
    {
        LogFailedHR("XGameSaveFilesGetFolderWithUiAsync", hr);
    }

    return hr;
}

HRESULT Sample::GetRemainingQuota()
{
    std::thread([this]()
    {
        int64_t remainingQuota = 0;
        HRESULT hr = XGameSaveFilesGetRemainingQuota(this->m_userHandle, c_scid, &remainingQuota);

        if (SUCCEEDED(hr))
        {
            this->Log("XGameSaveFilesGetRemainingQuota successful");
            char buffer[20]{};
            sprintf_s(buffer, 20, "%I64d", remainingQuota);
            this->Log(buffer);
        }
        else
        {
            this->LogFailedHR("XGameSaveFilesGetRemainingQuota", hr);
        }

    }).detach();
    return S_OK;
}

void Sample::GenerateFile()
{
    // Define buffer for the file path
    char filePathBuffer[MAX_PATH]{};
    char containerPathBuffer[MAX_PATH]{};

    GetContainerPath(containerPathBuffer);

    sprintf_s(filePathBuffer, MAX_PATH, "%sexample.txt", containerPathBuffer);

    // Create the directory if it doesn't exist
    if (!CreateDirectoryA(containerPathBuffer, nullptr))
    {
        DWORD dwError = GetLastError();

        if (dwError != ERROR_ALREADY_EXISTS)
        {
            LogFailedDWORD("Failed to create directory (container). dword Error: ", dwError);
            return;
        }
    }
    else
    {
        Log("Directory (Container) created successfully");
    }

    // Create a file with CREATE_ALWAYS flag to overwrite a file with the same name
    // Open the file with FILE_FLAG_OVERLAPPED for asynchronous I/O whenever possible.
    HANDLE fileHandle = CreateFileA(
        filePathBuffer,                                     // File name
        GENERIC_WRITE,                                      // Desired access
        0,                                                  // Share mode (none)
        nullptr,                                            // Security attributes
        CREATE_ALWAYS,                                      // Creation disposition
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,       // Flags for async I/O
        nullptr                                             // Template file (not used)
    );

    std::time_t now = std::time(nullptr);
    std::string dataToWrite = std::to_string(now);

    OVERLAPPED ov{};
    DWORD bytesWritten = 0;

    BOOL errorFlag = WriteFile(
        fileHandle,                 // Open file handle
        dataToWrite.c_str(),        // Data to write
        (DWORD)dataToWrite.size(),
        nullptr,                    // For async I/O, this must be nullptr
        &ov                         // Overlapped structure
    );

    if (!errorFlag && GetLastError() != ERROR_IO_PENDING)
    {
        LogFailedDWORD("Failed to write file asynchronously. dword Error: ", GetLastError());
        CloseHandle(fileHandle);
        return;
    }

    // Wait for the write to complete (can be replaced with an async completion
    // mechanism like an event or I/O Completion Port).
    if (GetOverlappedResult(fileHandle, &ov, &bytesWritten, TRUE))
    {
        Log((dataToWrite + ": Data written").c_str());
    }
    else
    {
        LogFailedDWORD("GetOverlappedResult for WriteFile failed. dword Error: ", GetLastError());
    }

    // Close the file handle
    CloseHandle(fileHandle);
}

void Sample::ReadData()
{
    // Define buffer for the character file path
    char filePathBuffer[MAX_PATH]{};
    char containerPathBuffer[MAX_PATH]{};

    GetContainerPath(containerPathBuffer);

    sprintf_s(filePathBuffer, MAX_PATH, "%sexample.txt", containerPathBuffer);

    // Open the file for reading with FILE_FLAG_OVERLAPPED for asynchronous I/O
    HANDLE fileHandle = CreateFileA(
        filePathBuffer,                                     // File name
        GENERIC_READ,                                       // Desired access
        0,                                                  // Share mode (none)
        nullptr,                                            // Security attributes
        OPEN_EXISTING,                                      // Only open if the file exists
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,       // Async I/O
        nullptr                                             // Template file (not used)
    );

    if (fileHandle == INVALID_HANDLE_VALUE)
    {
        LogFailedDWORD("Win32 CreateFile failed. File doesn't exist. dword Error: ", GetLastError());
    }
    else
    {
        DWORD fileSize = GetFileSize(fileHandle, nullptr);

        if (fileSize == INVALID_FILE_SIZE)
        {
            LogFailedDWORD("Failed to get file size. dword Error: ", GetLastError());
        }

        std::unique_ptr<char[]> buffer = std::make_unique<char[]>(fileSize + 1); // +1 for nullptr terminator
        OVERLAPPED ov{};
        DWORD bytesRead = 0;

        BOOL readResult = ReadFile(
            fileHandle,
            buffer.get(),
            fileSize,
            nullptr,            // For async, must be nullptr
            &ov
        );

        if (!readResult && GetLastError() != ERROR_IO_PENDING)
        {
            LogFailedDWORD("Failed to read file asynchronously. dword Error: ", GetLastError());
            CloseHandle(fileHandle);
            return;
        }

        // Wait for the read to complete
        if (GetOverlappedResult(fileHandle, &ov, &bytesRead, TRUE))
        {
            buffer[fileSize] = '\0'; // nullptr-terminate the string
            std::string_view currentData(buffer.get(), fileSize);
            Log(currentData.data());
        }
        else
        {
            LogFailedDWORD("GetOverlappedResult for WriteFile failed. dword Error: ", GetLastError());
        }
    }

    // Close the file handle
    CloseHandle(fileHandle);
}

void Sample::GetContainerPath(char* containerPathBuffer)
{
    // The folder path generated on Xbox does not have a \ at the end.
    // The folder path generated from windows has a \ at the end.
    // You should handle both cases either way.

    char lastChar = m_gamesavePath.back();

    if (lastChar == '\\')
    {
        sprintf_s(containerPathBuffer, MAX_PATH, "%sExampleContainer\\", m_gamesavePath.c_str());
    }
    else
    {
        sprintf_s(containerPathBuffer, MAX_PATH, "%s\\ExampleContainer\\", m_gamesavePath.c_str());
    }
}
#pragma endregion

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

    // Push logs to UI
    if (m_logQueue.size() > 0)
    {
        std::lock_guard<std::mutex> scopeLock(m_logMutex);

        for (size_t i = 0; i < m_logQueue.size(); ++i)
        {
            m_consoleWindow->AppendLineOfText(m_logQueue[i]);
        }

        m_logQueue.clear();
    }

    if (m_gamerPic.size != 0)
    {
        m_gamerpicImage->UseTextureData(m_gamerPic.data.get(), m_gamerPic.size);
        m_gamerPic.size = 0;
    }

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
    auto const rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto const dsvDescriptor = m_deviceResources->GetDepthStencilView();

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);
    commandList->ClearDepthStencilView(dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

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
    m_inputState.Reset();

    // On title resume, XGameSaveFilesGetFolderWithUi must be
    // called again to reacquire the XGameSave provider to ensure that the
    // title can sync and save properly.
    if (m_gamesavePath.size() == 0)
    {
        Log("Reacquiring the XGameSave provider on title resume.");
        GetFolderWithUIAsync();
    }
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
    width = 1600;
    height = 900;
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

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const size = m_deviceResources->GetOutputSize();
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

#pragma region User Management

HRESULT Sample::GetUserHandle(XUserAddOptions userAddOption)
{
    if (m_userAddInProgress)
    {
        return S_FALSE;
    }

    std::unique_ptr<XAsyncBlock> asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = nullptr;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> owner = std::unique_ptr<XAsyncBlock>(asyncBlock);
        Sample* sampleContext = static_cast<Sample*>(owner->context);

        XUserHandle newUser = nullptr;
        HRESULT hr = XUserAddResult(asyncBlock, &newUser);

        if (SUCCEEDED(hr))
        {
            if (sampleContext->m_userHandle)
            {
                XUserCloseHandle(sampleContext->m_userHandle);
            }

            sampleContext->m_userHandle = newUser;
            sampleContext->Log("Successfully obtained User Handle");

            sampleContext->UpdateUserUIData();
        }
        else
        {
            sampleContext->LogFailedHR("XUserAddResult", hr);
        }

        sampleContext->m_userAddInProgress = false;
    };

    HRESULT hr = XUserAddAsync(userAddOption, asyncBlock.get());

    if (SUCCEEDED(hr))
    {
        m_userAddInProgress = true;
        asyncBlock.release();
    }
    else
    {
        LogFailedHR("XUserAddAsync", hr);
    }

    return hr;
}

HRESULT Sample::UpdateUserUIData()
{
    char gamertagBuffer[XUserGamertagComponentUniqueModernMaxBytes + 1]{};
    size_t gamertagSize = 0;
    DX::ThrowIfFailed(XUserGetGamertag(m_userHandle, XUserGamertagComponent::UniqueModern, sizeof(gamertagBuffer), gamertagBuffer, &gamertagSize));
    m_gamertagText->SetDisplayText(gamertagBuffer);

    struct AsyncBlockContext
    {
        GamerPicBytes* gamerPicBytes;
        Sample* sampleContext;
    };
    AsyncBlockContext* ctx = new AsyncBlockContext();
    ctx->gamerPicBytes = &m_gamerPic;
    ctx->sampleContext = this;

    // Setup gamerpic request
    std::unique_ptr<XAsyncBlock> asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = nullptr;
    asyncBlock->context = ctx;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> owner = std::unique_ptr<XAsyncBlock>(asyncBlock);
        AsyncBlockContext* ctx = static_cast<AsyncBlockContext*>(owner->context);

        Sample* sampleContext = ctx->sampleContext;
        // Get buffer size
        size_t bufferSize = 0;

        HRESULT hr = XUserGetGamerPictureResultSize(asyncBlock, &bufferSize);

        if ((FAILED(hr)) && (hr != HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)))
        {
            sampleContext->LogFailedHR("XUserGetGamerPictureResultSize", hr);
            delete ctx;
            DX::ThrowIfFailed(hr);
        }

        if (hr == S_OK)
        {
            size_t bufferUsed = 0;
            ctx->gamerPicBytes->size = bufferSize;
            ctx->gamerPicBytes->data.reset(new uint8_t[bufferSize]);
            DX::ThrowIfFailed(XUserGetGamerPictureResult(asyncBlock, ctx->gamerPicBytes->size, ctx->gamerPicBytes->data.get(), &bufferUsed));
        }

        delete ctx;
    };

    // Request gamerpic
    HRESULT hr = XUserGetGamerPictureAsync(m_userHandle, XUserGamerPictureSize::Medium, asyncBlock.get());

    if (SUCCEEDED(hr))
    {
        asyncBlock.release();
    }
    else
    {
        LogFailedHR("XUserGetGamerPictureAsync", hr);
        delete ctx;
    }

    return hr;
}

#pragma endregion

#pragma region Logging

void Sample::LogFailedHR(const char* functionName, HRESULT hr)
{
    char buffer[256]{};
    sprintf_s(buffer, 256, "%s failed with hr=%08X", functionName, hr);
    Log(buffer);
}

void Sample::LogFailedDWORD(const char* functionName, DWORD dword)
{
    char buffer[256]{};
    sprintf_s(buffer, 256, "%s failed with dword=%08X", functionName, dword);
    Log(buffer);
}

void Sample::EnableFileIOButtons(bool enable)
{
    m_generateButton->SetEnabled(enable);
    m_readButton->SetEnabled(enable);
}

void Sample::Log(const char* text)
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);

    if (m_consoleWindow)
    {
        m_logQueue.push_back(text);
    }

    OutputDebugStringA(text);
    OutputDebugStringA("\n");
}

#pragma endregion
