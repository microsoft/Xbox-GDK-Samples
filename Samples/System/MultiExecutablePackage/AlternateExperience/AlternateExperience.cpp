//--------------------------------------------------------------------------------------
// AlternateExperience.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AlternateExperience.h"

#include "ATGColors.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_dllModuleHandle(nullptr)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
    // Make sure to free the dll when the program ends, or switches to an other process.
    if (m_dllModuleHandle != nullptr)
    {
        FreeLibrary(m_dllModuleHandle);
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

    auto layout = m_uiManager.LoadLayoutFromFile("Assets/AlternateExperience/AlternateUILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    m_loadDllButton = layout->GetTypedChildById<UIButton>(ID("Load_DLL_Button"));
    m_utilityExeButton = layout->GetTypedChildById<UIButton>(ID("Run_Utility_Button"));
    m_gameExeButton = layout->GetTypedChildById<UIButton>(ID("XLaunch_Other_Button"));
    m_switchUserButton = layout->GetTypedChildById<UIButton>(ID("Switch_User_Button"));
    m_consoleWindow = layout->GetTypedChildById<UIPanel>(ID("Output_Console_Window_Outer_Panel"))
        ->GetTypedChildById<UIConsoleWindow>(ID("Output_Console_Window"));
    m_gamerpicImage = layout->GetTypedChildById<UIStackPanel>(ID("User_Info_Panel"))
        ->GetTypedChildById<UIImage>(ID("Gamerpic"));
    m_gamertagText = layout->GetTypedChildById<UIStackPanel>(ID("User_Info_Panel"))
        ->GetTypedChildById<UIStaticText>(ID("Gamertag_Label"));

    m_logger = std::make_unique<UITKLogManager>();
    m_userManager = std::make_unique<UserManager>(m_logger.get());
    m_userManager->LoadUserHandle(XUserAddOptions::AddDefaultUserAllowingUI);
    m_utilityProcessManager = std::make_unique<UtilityProcessManager>(m_logger.get());

    RegisterUIEventHandlers();
}


// Register the event handlers to be used for UI elements.
void Sample::RegisterUIEventHandlers()
{
    if (m_loadDllButton)
    {
        m_loadDllButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                DX::ThrowIfFailed(this->LoadDll());
            });
    }
    if (m_utilityExeButton)
    {
        m_utilityExeButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                DX::ThrowIfFailed(this->RunUtilityProcess());
            });
    }
    if (m_gameExeButton)
    {
        m_gameExeButton->ButtonState().AddListenerWhen(UIButton::State::Pressed,
            [this](UIButton*)
            {
                XLaunchOtherGame();
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

    Render();

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    // Receive the updated information from the external utilty process.
    m_utilityProcessManager->UpdateToolProcess();

    std::string userManagerGamerPicGamerTag = m_userManager->GetGamerPicGamerTag();
    if (std::strcmp(m_currentGamerPicGamerTag.c_str(), userManagerGamerPicGamerTag.c_str()) != 0)
    {

        // UPDATE GAMER PIC HERE
        auto [data, size] = m_userManager->GetGamerPicData();
        m_gamerpicImage->UseTextureData(data, size);

        m_currentGamerPicGamerTag = userManagerGamerPicGamerTag;
        m_gamertagText->SetDisplayText(m_currentGamerPicGamerTag);
    }

    // Push logs to UI
    if (m_logger->QueueSize() > 0)
    {
        std::lock_guard<std::mutex> scopeLock(m_logMutex);
        std::vector<std::string> logQueue = m_logger->GetLogQueue();
        for (size_t i = 0; i < logQueue.size(); ++i)
        {
            m_consoleWindow->AppendLineOfText(logQueue[i]);
        }
        m_logger->ClearLogQueue();
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

#pragma region Input Handling

// Loads a DLL during runtime. Also calls a function from the dll once the dll is loaded.
HRESULT Sample::LoadDll()
{
    std::string dll = "ComboDLL.dll";

    m_dllModuleHandle = GetModuleHandleA(dll.c_str());

    // If the dll hasn't been loaded yet, load it.
    if (m_dllModuleHandle == nullptr)
    {
        m_dllModuleHandle = LoadLibraryA(dll.c_str());
        if (m_dllModuleHandle == nullptr)
        {
            m_logger->Log("Failed to load " + dll);
            return E_FAIL;
        }
        m_logger->Log("Loaded " + dll);
    }

    // The function name is a C style name. This is the name that will appear in the DLL's export table
    // after it is built. This is specified within the Dll's header file through the {extern "C"} modifier.
    std::string functionName = "GetDllInfo";

    // Create and obtain a function_pointer to the function defined in our DLL.
    // Both XBOX and Desktop DLLs in this sample define this function with the same signature for simplicity.
    using FunctionPtr = std::add_pointer<void(const char*, size_t, char*)>::type;

    // There is a compiler warning that appears when converting FARPROC_, which represents a function ptr
    // with no args, to FunctionPtr. This is can be avoided by doing the following casts.
    FunctionPtr getDllInfoPtr =
        reinterpret_cast<FunctionPtr>(
            reinterpret_cast<void*>
            (GetProcAddress(m_dllModuleHandle, functionName.c_str())));

    if (getDllInfoPtr == nullptr)
    {
        m_logger->Log("Failed to find " + functionName + " from " + dll + " export table!");
        return E_FAIL;
    }

    // Call the example function with a string argument
    std::string executableName = "AlternateExperience.exe";
    const int resultSize = 256;
    char result[resultSize] = {};
    getDllInfoPtr(executableName.c_str(), resultSize, result);

    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"Called %s:: %s", dll.c_str(), functionName.c_str());
    m_logger->Log(std::string(buffer));
    sprintf_s(buffer, 256, u8"-- Result: %s", result);
    m_logger->Log(std::string(buffer));
    return S_OK;
}

HRESULT Sample::RunUtilityProcess()
{
    m_utilityProcessManager->SpawnCPUTool();
    return S_OK;
}

void Sample::XLaunchOtherGame()
{
    //The debugger will disconnect here as the current process is ending and a new one is spawned
    XLaunchNewGame(u8"DefaultExperience.exe", u8"-crossrestart", nullptr);
}
#pragma endregion

void Sample::LogFailedHR(HRESULT hr, const std::string functionName)
{
    char buffer[256] = {};
    sprintf_s(buffer, 256, u8"%s Failed with hr=%08X", functionName.c_str(), hr);
    m_logger->Log(buffer);
}

void Sample::Log(const std::string text)
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);

    if (m_consoleWindow)
    {
        m_logQueue.push_back(text);
    }
    OutputDebugStringA(text.c_str());
    OutputDebugStringA(u8"\n");
}
