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
    m_userHandle(nullptr)
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

    XTaskQueueTerminate(m_taskQueue, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Work, INFINITE);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, INFINITE);
    XTaskQueueCloseHandle(m_taskQueue);

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
    m_switchUserButton = layout->GetTypedChildById<UIButton>(ID("Switch_User_Button"));
    m_consoleWindow = layout->GetTypedChildById<UIPanel>(ID("Output_Console_Window_Outer_Panel"))
        ->GetTypedChildById<UIConsoleWindow>(ID("Output_Console_Window"));
    m_userInfoPanel = layout->GetTypedChildById<UIStackPanel>(ID("Sample_Title_Panel"))->
        GetTypedChildById<UIStackPanel>(ID("User_Info_Panel"));
    m_gamertagText = m_userInfoPanel->GetTypedChildById<UIStaticText>(ID("Gamertag_Label"));
    m_gamerpicImage = m_userInfoPanel->GetTypedChildById<UIImage>(ID("Gamerpic"));

    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::ThreadPool,
        &m_taskQueue));

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
    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        Sample* pThis = static_cast<Sample*>(asyncBlock->context);
        size_t folderSize = 0;
        XAsyncGetResultSize(asyncBlock, &folderSize);
        char* folderResult = new char[folderSize];
        HRESULT hr = XGameSaveFilesGetFolderWithUiResult(asyncBlock, folderSize, folderResult);

        if (SUCCEEDED(hr))
        {
            pThis->Log("XGameSaveFilesGetFolderWithUiResult successful");
            pThis->Log(folderResult);
        }
        else
        {
            pThis->LogFailedHR("XGameSaveFilesGetFolderWithUiResult", hr);
        }

        delete [] folderResult;
        delete asyncBlock;
    };

    HRESULT hr = XGameSaveFilesGetFolderWithUiAsync(m_userHandle, c_scid, asyncBlock);
    return hr;
}

HRESULT Sample::GetRemainingQuota()
{
    XTaskQueueCallback* getremainingQuota = [](void* context, bool)
    {
        Sample* pThis = reinterpret_cast<Sample*>(context);
        int64_t remainingQuota = 0;
        HRESULT hr = XGameSaveFilesGetRemainingQuota(pThis->m_userHandle, c_scid, &remainingQuota);

        if (SUCCEEDED(hr))
        {
            pThis->Log("XGameSaveFilesGetRemainingQuota successful");
            char buffer[20] = {};
            sprintf_s(buffer, 20, u8"%I64d", remainingQuota);
            pThis->Log(buffer);
        }
        else
        {
            pThis->LogFailedHR("XGameSaveFilesGetRemainingQuota", hr);
        }
    };

    // As XGameSaveFilesGetRemainingQuota may cause the thread to hang, so we call it asynchrounously by putting it into
    // an XTaskQueueCallback that we add to our TaskQueue.
    HRESULT hr = XTaskQueueSubmitCallback(m_taskQueue, XTaskQueuePort::Work, this, getremainingQuota);

    if (SUCCEEDED(hr))
    {
        Log("Submitted XGameSaveFilesGetRemainingQuota to TaskQueue successfully");
    }
    else
    {
        LogFailedHR("XTaskQueueSubmitCallback", hr);
    }

    return hr;
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

    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        Sample* pThis = static_cast<Sample*>(asyncBlock->context);

        XUserHandle newUser = nullptr;
        HRESULT hr = XUserAddResult(asyncBlock, &newUser);

        if (SUCCEEDED(hr))
        {
            if (pThis->m_userHandle)
            {
                XUserCloseHandle(pThis->m_userHandle);
            }
            pThis->m_userHandle = newUser;
            pThis->Log("Successfully obtained User Handle");

            pThis->UpdateUserUIData();
        }
        else
        {
            pThis->LogFailedHR("XUserAddResult", hr);
        }

        pThis->m_userAddInProgress = false;
        delete asyncBlock;
    };

    HRESULT hr = XUserAddAsync(userAddOption, asyncBlock);
    if (SUCCEEDED(hr))
    {
        m_userAddInProgress = true;
    }
    else
    {
        LogFailedHR("XUserAddAsync", hr);
        delete asyncBlock;
    }

    return hr;
}

HRESULT Sample::UpdateUserUIData()
{
    char gamertagBuffer[XUserGamertagComponentUniqueModernMaxBytes + 1] = {};
    size_t gamertagSize = 0;
    DX::ThrowIfFailed(XUserGetGamertag(m_userHandle, XUserGamertagComponent::UniqueModern, sizeof(gamertagBuffer), gamertagBuffer, &gamertagSize));
    m_gamertagText->SetDisplayText(gamertagBuffer);

    struct AsyncBlockContext
    {
        GamerPicBytes* gamerPicBytes;
        Sample* pThis;
    };
    AsyncBlockContext* ctx = new AsyncBlockContext();
    ctx->gamerPicBytes = &m_gamerPic;
    ctx->pThis = this;

    // Setup gamerpic request
    XAsyncBlock* asyncBlock = new XAsyncBlock{};
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = ctx;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        AsyncBlockContext* ctx = static_cast<AsyncBlockContext*>(asyncBlock->context);
        Sample* pThis = ctx->pThis;
        // Get buffer size
        size_t bufferSize = 0;
        DX::ThrowIfFailed(XUserGetGamerPictureResultSize(asyncBlock, &bufferSize));

        size_t bufferUsed = 0;

        ctx->gamerPicBytes->size = bufferSize;
        ctx->gamerPicBytes->data.reset(new uint8_t[bufferSize]);

        DX::ThrowIfFailed(XUserGetGamerPictureResult(asyncBlock, ctx->gamerPicBytes->size, ctx->gamerPicBytes->data.get(), &bufferUsed));

        // Set in UI
        pThis->Log("Got the gamer pic.");
        delete ctx;
        delete asyncBlock;
    };

    // Request gamerpic
    HRESULT hr = XUserGetGamerPictureAsync(m_userHandle, XUserGamerPictureSize::Medium, asyncBlock);
    if (FAILED(hr))
    {
        LogFailedHR("XUserGetGamerPictureAsync", hr);
    }
    return hr;
}

#pragma endregion

#pragma region Logging

void Sample::LogFailedHR(const char* functionName, HRESULT hr)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"%s failed with hr=%08X", functionName, hr);
    Log(buffer);
}

void Sample::Log(const char* text)
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);

    if (m_consoleWindow)
    {
        m_logQueue.push_back(text);
    }

    OutputDebugStringA(text);
    OutputDebugStringA(u8"\n");
}

#pragma endregion

