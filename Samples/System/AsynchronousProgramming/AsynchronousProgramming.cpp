//--------------------------------------------------------------------------------------
// AsynchronousProgramming.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AsynchronousProgramming.h"

#include "ATGColors.h"
#include "FindMedia.h"


extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG;
using namespace ATG::UITK;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_lastFocusIndex(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);

    m_examples = std::make_unique<XAsyncExamples>(this);
}

Sample::~Sample()
{
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

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    // Perform UITK initialization
    InitializeUI();

    // Uncomment to set the main thread as time-sensitive
    // After uncommenting, asserts will fire in the AdvancedUsage test and the Synchronous
    // Task Queue testing as these tests use APIs that can be slow on occasion.
    // The other tests use APIs that are considered safe for time-sensitive threads.
    //XThreadSetTimeSensitive(true);
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

    // Push logs to UI
    for (size_t i = 0; i < m_logQueue.size(); ++i)
    {
        m_consoleWindow->AppendLineOfText(m_logQueue[i]);
    }
    m_logQueue.clear();

    // Update our UI input state and managed layout
    m_uiInputState.Update(elapsedTime, *m_gamePad, *m_keyboard, *m_mouse);
    m_uiManager.Update(elapsedTime, m_uiInputState);

    m_examples->Update(elapsedTime);

    // Check if the "view" button was pressed and exit the sample if so
    auto buttons = m_uiInputState.GetGamePadButtons(0);
    if (buttons.view == GamePad::ButtonStateTracker::PRESSED)
    {
        ExitSample();
    }

    // Also exit if the Sample user presses the [ESC] key
    auto keys = m_uiInputState.GetKeyboardKeys();
    if (keys.IsKeyPressed(DirectX::Keyboard::Keys::Escape))
    {
        ExitSample();
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

    // Render the UI Scene
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
    m_uiInputState.Reset();
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

    // Create the style renderer for the UI manager to use for rendering the UI scene styles
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Notify the UI manager of the current window size
    auto os = m_deviceResources->GetOutputSize();
    m_uiManager.SetWindowSize(os.right, os.bottom);
}

void Sample::OnDeviceLost()
{
    m_uiManager.GetStyleManager().ResetStyleRenderer();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

void Sample::InitializeUI()
{
    auto root = m_uiManager.GetRootElement()->AddChildFromLayout("Assets/Layouts/Layout.json");

    m_consoleWindow = root->GetChildById(ID("Output_Console_Window_Outer_Panel"))->GetTypedChildById<UIConsoleWindow>(ID("Output_Console_Window"));

    auto buttonStackPanel = root->GetChildById(ID("Button_Stack_Panel"));
    m_buttons[0] = buttonStackPanel->GetTypedChildById<UIButton>(ID("Test_Button_1"));
    m_buttons[1] = buttonStackPanel->GetTypedChildById<UIButton>(ID("Test_Button_2"));
    m_buttons[2] = buttonStackPanel->GetTypedChildById<UIButton>(ID("Test_Button_3"));
    m_buttons[3] = buttonStackPanel->GetTypedChildById<UIButton>(ID("Test_Button_4"));
    m_buttons[4] = buttonStackPanel->GetTypedChildById<UIButton>(ID("Test_Button_5"));
    m_buttons[5] = buttonStackPanel->GetTypedChildById<UIButton>(ID("Test_Button_6"));
    m_buttons[6] = buttonStackPanel->GetTypedChildById<UIButton>(ID("Test_Button_7"));
    m_buttons[7] = buttonStackPanel->GetTypedChildById<UIButton>(ID("Test_Button_8"));
    m_description = buttonStackPanel->GetTypedChildById<UIStaticText>(ID("Test_Description_Label"));

    // Custom GDK Style APIs
    SetupButtonHandler(0, "Custom GDK-Style API", [&]() {m_examples->StartTest_CustomGDKStyleAPIs(); });

    // Synchronous Task Queue
    SetupButtonHandler(1, "Synchronous TaskQueue", [&]() {m_examples->StartTest_SynchronousTaskQueue(); });

    // Manual & Message Loop
    SetupButtonHandler(2, "Manual & Message Loop", [&]() {m_examples->StartTest_ManualAndMessageLoop(); });

    // Serialized Async Task Queue
    SetupButtonHandler(3, "SerializedAsync TaskQueue", [&]() {m_examples->StartTest_SerializedAsync(); });

    // ParallelFor
    SetupButtonHandler(4, "Custom ParallelFor", [&]() {m_examples->StartTest_ParallelFor(); });

    // Canceling
    SetupButtonHandler(5, "Canceling Requests", [&]() {m_examples->StartTest_Canceling(); });

    // Advanced Usage
    SetupButtonHandler(6, "Advanced Usage", [&]() {m_examples->StartTest_AdvancedUsage(); });

    // Overhead Calculations
    SetupButtonHandler(7, "Overhead Calculations", [&]() {m_examples->StartTest_OverheadCalculations(); });
}

void Sample::Log(const char* text)
{
    std::lock_guard<std::mutex> scopeLock(m_logMutex);

    if (m_consoleWindow)
    {
        // m_consoleWindow->AppendLineOfText(text) is not thread-safe, so queue and push on update
        m_logQueue.push_back(text);
    }

    OutputDebugStringA(text);
    OutputDebugStringA(u8"\n");
}

void Sample::SetupButtonHandler(int index, std::string displayText, std::function<void()> onClicked)
{
    m_buttons[index]->GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetDisplayText(displayText);
    m_buttons[index]->ButtonState().AddListenerWhen(UIButton::State::Pressed,
        [&, index, onClicked](UIButton* /*button*/)
        {
            onClicked();

            m_buttons[index]->GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetStyleId(ID("Button_Label_Style"));
        });
    m_buttons[index]->ButtonState().AddListenerWhen(UIButton::State::Focused,
        [&, index](UIButton* /*button*/)
        {
            m_lastFocusIndex = index;
            UpdateDescription(m_lastFocusIndex);

            m_buttons[index]->GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetStyleId(ID("Button_Label_Style_Dark"));
        });
    m_buttons[index]->ButtonState().AddListenerWhen(UIButton::State::Hovered,
        [&, index](UIButton* /*button*/)
        {
            UpdateDescription(index);

            m_buttons[index]->GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetStyleId(ID("Button_Label_Style_Dark"));
        });
    m_buttons[index]->ButtonState().AddListenerWhen(UIButton::State::Normal,
        [&, index](UIButton* /*button*/)
        {
            UpdateDescription(m_lastFocusIndex);

            m_buttons[index]->GetTypedSubElementById<UIStaticText>(ID("Test_Button_Label"))->SetStyleId(ID("Button_Label_Style"));
        });
}

void Sample::UpdateDescription(int index)
{
    switch (index)
    {
    case 0:
        m_description->SetDisplayText(u8"Demonstrates how to create your\nown async functions that have\nthe same calling pattern as GDK\nAPI functions.");
        break;
    case 1:
        m_description->SetDisplayText(u8"Demonstrates how to use a\nsynchronous task queue which\nruns the submitted callbacks\nsynchronously. The same code\ncan run synchronously or \nasynchronously based soled on\nthe task queue.");
        break;
    case 2:
        m_description->SetDisplayText(u8"Demonstrates how to use a\nfully manual task queue and also\nhow to integrate the usage\ninto the Window message loop.");
        break;
    case 3:
        m_description->SetDisplayText(u8"Demonstrates how to use a\nSerializeAsync task queue. These\ntask queues process callbacks in\nthe order that they are queued\nwith no overlapping.");
        break;
    case 4:
        m_description->SetDisplayText(u8"Demonstrates how XAsync can\nbe used to create a ParallelFor\nimplementation with a manual\nport in a task queue.");
        break;
    case 5:
        m_description->SetDisplayText(u8"Demonstrates how to cancel\nasynchronous requests when\nusing an asynchronous provider\nimplementation.");
        break;
    case 6:
        m_description->SetDisplayText(u8"Demonstrates less common use-\ncases: composite queues,\nduplicating task queue handles,\nand using waiters and delayed\ndispatching");
        break;
    case 7:
        m_description->SetDisplayText(u8"Calculates some average\noverhead timings for different\noperations. This helps understand\nthe overhead between different\nusages.");
        break;
    default:
        break;
    }
}
