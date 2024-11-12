//--------------------------------------------------------------------------------------
// AccessibilitySample.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "AccessibilitySample.h"

#include "ATGColors.h"
#include "FindMedia.h"

using namespace DirectX::DX12;

extern void ExitSample() noexcept;


using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_D32_FLOAT, 2);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
#ifdef _GAMING_XBOX
    imgui_acc_gdk::CleanUp();
#else
    imgui_acc_win32::CleanUp();
#endif
    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window, int width, int height)
{
    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();

    // IMGUI initialization
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Disable ImGui ini file
    io.IniFilename = nullptr;

#ifdef _GAMING_XBOX
    // Setup Platform/Renderer backends

    ImGui_ImplWin32_Init(window);

    // No command queue for GDK. Xbox manages its own command queue.
    ImGui_ImplGdkDX12_Init(m_deviceResources->GetD3DDevice(),
               (int)m_deviceResources->GetBackBufferCount(),
               (int)m_deviceResources->GetBackBufferFormat(),
               m_deviceResources->GetSRVHeap(),
               m_deviceResources->GetSRVHeapCPUHandle(),
               m_deviceResources->GetSRVHeapGPUHandle());

    // Grab the instance of the accessibility module
    imgAccGdk = imgui_acc_gdk::GetInstance();

    // Initialize the accessibility module.
    imgAccGdk->Initialize();
#else
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(window);

    // Create the SRV descriptor heap for ImGui
    ImGui_ImplDX12_Init(m_deviceResources->GetD3DDevice(),
        (int)m_deviceResources->GetBackBufferCount(),
        m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetSRVHeap(),
        m_deviceResources->GetSRVHeapCPUHandle(),
        m_deviceResources->GetSRVHeapGPUHandle());

    // Initialize our accessibility module.
    imgAccWin32 = imgui_acc_win32::GetInstance();
    imgAccWin32->Initialize(window);
#endif

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
}   

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

#ifdef _GAMING_XBOX
    m_deviceResources->WaitForOrigin();
#endif

    m_timer.Tick([&](){});

    Render();

    PIXEndEvent();
    m_frame++;
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

    // Clear the CSV and RTV
    Clear();

    // Attach SRV descriptor heap to the command list
    auto commandList = m_deviceResources->GetCommandList();
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_deviceResources->GetSRVHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");
#ifdef _GAMING_XBOX

    ImGui_ImplGdkDX12_NewFrame(commandList);
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Accessibility checks for new frame
    imgAccGdk->NewFrame();

    // ExampleUI Window
    imgAccGdk->Begin("Example UI", 0, 0, ImVec2(700, 300));
    imgAccGdk->WindowHeader("Example UI");
    imgAccGdk->Button("Button 1");
    imgAccGdk->Button("Button 2");
    static char inputText[256] = "";
    imgAccGdk->InputText("Input Text", inputText, IM_ARRAYSIZE(inputText), ImGui_ImplWin32_ActiveGameInputKind());
    static int sliderValue = 5;
    imgAccGdk->SliderInt("Slider", &sliderValue, 0, 10);
    imgAccGdk->End();

    // Set the initial coordinates for the legend window
    static bool initialPlacement = false;
    if (!initialPlacement)
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 bottomLeftCorner(windowPos.x, windowPos.y + 320);

        ImGui::SetNextWindowPos(ImVec2(bottomLeftCorner.x, bottomLeftCorner.y));
        initialPlacement = true;
    }

    // Legend Window
    imgAccGdk->Begin("Legend", 0, 0, ImVec2(700, 300));
    imgAccGdk->WindowHeader("Legend");
    imgAccGdk->Text("Switch Window: X + LEFT SHOULDER, X + RIGHT SHOULDER");
    imgAccGdk->Text("Next Widget: D-PAD DOWN");
    imgAccGdk->Text("Previous Widget: D-PAD UP");
    imgAccGdk->Text("Activate Widget: A");
    imgAccGdk->Text("Text Input: Y");
    imgAccGdk->Text("Un-focus Widget: B");
    imgAccGdk->Text("Horizontal Scroll: THUMBSTICK");
    static bool enableNarration = true;
    imgAccGdk->Checkbox(": Enable Narration ", &enableNarration);
    ImGui::Separator();
    ImGui::Text("");
    imgAccGdk->Text("Theme control is automatically pulled from Windows settings.");
    imgAccGdk->Text("Text scaling is automatically pulled from Windows settings.");
    imgAccGdk->Text("Narration is automatic. Focusing on a widget will narrate that widget.");

    if (enableNarration)
    {
        imgAccGdk->EnableNarration();
    }
    else
    {
        imgAccGdk->DisableNarration();
    }

    imgAccGdk->End();

    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplGdkDX12_RenderDrawData(drawData, commandList);

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();

#else

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Accessibility checks for new frame
    imgAccWin32->NewFrame();

    // ExampleUI Window
    imgAccWin32->Begin("Example UI", 0, 0, ImVec2(700,300));
    imgAccWin32->WindowHeader("Example UI");
    imgAccWin32->Button("Button 1");
    imgAccWin32->Button("Button 2");
    static char inputText[256] = "";
    imgAccWin32->InputText("Input Text", inputText, IM_ARRAYSIZE(inputText));
    static int sliderValue = 5;
    imgAccWin32->SliderInt("Slider", &sliderValue, 0, 10);
    imgAccWin32->End();

    // Set the initial coordinates for the legend window
    static bool initialPlacement = false;
    if (!initialPlacement)
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 bottomLeftCorner(windowPos.x, windowPos.y + 320);

        ImGui::SetNextWindowPos(ImVec2(bottomLeftCorner.x, bottomLeftCorner.y));
        initialPlacement = true;
    }

    // Legend Window
    imgAccWin32->Begin("Legend", 0, 0, ImVec2(700, 300));
    imgAccWin32->WindowHeader("Legend");
    imgAccWin32->Text("Switch Window: CTRL + TAB");
    imgAccWin32->Text("Next Widget: TAB, DOWN ARROW");
    imgAccWin32->Text("Previous Widget: SHIFT + TAB, UP ARROW");
    imgAccWin32->Text("Activate Widget: ENTER");
    imgAccWin32->Text("Un-focus Widget: ESC");
    imgAccWin32->Text("Horizontal Scroll: LEFT ARROW, RIGHT ARROW");
    static bool enableNarration = true;
    imgAccWin32->Checkbox(": Enable Narration ", &enableNarration);
    ImGui::Separator();
    ImGui::Text("");
    imgAccWin32->Text("Theme control is automatically pulled from Windows settings.");
    imgAccWin32->Text("Text scaling is automatically pulled from Windows settings.");
    imgAccWin32->Text("Narration is automatic. Focusing on a widget will narrate that widget.");

    if (enableNarration)
    {
        imgAccWin32->EnableNarration();
    }
    else
    {
        imgAccWin32->DisableNarration();
    }

    imgAccWin32->End();

    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    ImGui_ImplDX12_RenderDrawData(drawData, commandList);

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
#endif

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
#ifdef _DEBUG
        OutputDebugStringA("ERROR: Shader Model 6.0 is not supported!\n");
#endif
        throw std::runtime_error("Shader Model 6.0 is not supported!");
    }
#endif

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
}


void Sample::OnDeviceLost()
{
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();
}
#pragma endregion

