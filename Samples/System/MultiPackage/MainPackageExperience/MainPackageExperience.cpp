//--------------------------------------------------------------------------------------
// MainPackageExperience.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MainPackageExperience.h"

#include "ATGColors.h"
#include "FindMedia.h"
#include "XLauncher.h"
#include "XGameProtocol.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

// Handle the custom URI when this package is launched from a custom protocol.
void CALLBACK CustomProtocolCallback(void* context, const char* protocolUri)
{
    Sample* pThis = reinterpret_cast<Sample*>(context);
    std::string protocolDescription = "Launched with protocol: ";
    protocolDescription.append(protocolUri);
    pThis->Log(protocolDescription);
}

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    XGameProtocolUnregisterForActivation(m_protocolRegistrationToken, false);
    XTaskQueueTerminate(m_taskQueue, false, nullptr, nullptr);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Work, INFINITE);
    XTaskQueueDispatch(m_taskQueue, XTaskQueuePort::Completion, INFINITE);
    XTaskQueueCloseHandle(m_taskQueue);

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
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
    SetupUI();

    m_logger = std::make_unique<SimpleLogManager>();
    m_userManager = std::make_unique<UserManager>(m_logger.get());
    m_userManager->LoadUserHandle(XUserAddOptions::AddDefaultUserAllowingUI);

    DX::ThrowIfFailed(XTaskQueueCreate(
        XTaskQueueDispatchMode::ThreadPool,
        XTaskQueueDispatchMode::ThreadPool,
        &m_taskQueue));

    HRESULT result = XGameProtocolRegisterForActivation(m_taskQueue, this, CustomProtocolCallback, &m_protocolRegistrationToken);
    if (result == S_OK)
    {
        Log("Successfully registered a callback for protocol activation.");
    }
    else
    {
        LogFailedHR(result,"XGameProtocolRegisterForActivation");
    }

    RegisterUIEventHandlers();
}

// Register the event handlers to be used for UI elements.
void Sample::RegisterUIEventHandlers()
{
    if (m_customProtocolButton)
    {
        m_customProtocolButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                this->CustomProtocolLaunch();
            });
    }
    if (m_msXblLaunchButton)
    {
        m_msXblLaunchButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                this->XBOXLiveProtocolLaunch();
            });
    }
    if (m_uninstalledLaunch)
    {
        m_uninstalledLaunch->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                this->XBOXLiveProtocolPDPLaunch();
            });
    }
    if (m_switchUserButton)
    {
        m_switchUserButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                HRESULT hr = m_userManager->LoadUserHandle(XUserAddOptions::AllowGuests);
                if (hr != S_OK)
                {
                    LogFailedHR(hr, "LoadUserHandle");
                }
            });
    }
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
    std::string userManagerGamerPicGamerTag = m_userManager->GetGamerPicGamerTag();

    // UPDATE GAMER PIC HERE
    if (m_currentGamerPicGamerTag != userManagerGamerPicGamerTag)
    {
        auto [data, size] = m_userManager->GetGamerPicData();
        m_gamerpicImage->UseTextureData(data, size);
        m_currentGamerPicGamerTag = userManagerGamerPicGamerTag;
        m_gamertagText->SetDisplayText(m_currentGamerPicGamerTag);
    }

    // Push logs to UI
    if (m_logger->QueueSize() > 0)
    {
        std::vector<std::string> logQueue = m_logger->FlushLogQueue();
        for (size_t i = 0; i < logQueue.size(); ++i)
        {
            m_consoleWindow->AppendLineOfText(logQueue[i]);
        }
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

void Sample::CustomProtocolLaunch()
{
    XLaunchUri(m_userManager->GetUserHandle(), "launchalternatepackage://Parameter1=xyz");
}

void Sample::XBOXLiveProtocolLaunch()
{
    XLaunchUri(m_userManager->GetUserHandle(), "ms-xbl-675747EA://");
}

void Sample::XBOXLiveProtocolPDPLaunch()
{
    // Currently using the PlayFabStore sample as its in the store. If you have already downloaded it, the PDP page will not
    // be opened.
    XLaunchUri(m_userManager->GetUserHandle(), "ms-xbl-62a18058://");
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

    auto const size = m_deviceResources->GetOutputSize();
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, size.right, size.bottom);
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

void Sample::Log(const std::string& text)
{
    m_logger->Log(text);
}

void Sample::LogFailedHR(HRESULT hr, const std::string& functionName)
{
    m_logger->LogFailedHR(hr, functionName);
}

void Sample::SetupUI()
{
    auto layout = m_uiManager.LoadLayoutFromFile("Assets/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());
    m_customProtocolButton = layout->GetTypedChildById<UIButton>(ID("Custom_Launch_Button"));
    m_msXblLaunchButton = layout->GetTypedChildById<UIButton>(ID("XBL_Launch_Other_Package_Button"));
    m_uninstalledLaunch = layout->GetTypedChildById<UIButton>(ID("XBL_Launch_PDP_Button"));
    m_switchUserButton = layout->GetTypedChildById<UIButton>(ID("Switch_User_Button"));
    m_consoleWindow = layout->GetTypedChildById<UIPanel>(ID("Output_Console_Window_Outer_Panel"))
        ->GetTypedChildById<UIConsoleWindow>(ID("Output_Console_Window"));
    m_gamerpicImage = layout->GetTypedChildById<UIStackPanel>(ID("User_Info_Panel"))
        ->GetTypedChildById<UIImage>(ID("Gamerpic"));
    m_gamertagText = layout->GetTypedChildById<UIStackPanel>(ID("User_Info_Panel"))
        ->GetTypedChildById<UIStaticText>(ID("Gamertag_Label"));
}
