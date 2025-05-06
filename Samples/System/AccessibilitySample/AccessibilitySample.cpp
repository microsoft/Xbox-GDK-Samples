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
    // Clean up Imgui DX12 Renderer
    ImGuiDx12RendererShutdown();

    // Clean up Imgui Input handler
    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext();
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

    ImGui_ImplWin32_Init(window);
    ImGuiDx12RendererInit(m_deviceResources->GetD3DDevice(),
        (int)m_deviceResources->GetBackBufferCount(),
        m_deviceResources->GetBackBufferFormat(),
        m_deviceResources->GetSRVHeap(),
        m_deviceResources->GetSRVHeapCPUHandle(),
        m_deviceResources->GetSRVHeapGPUHandle());

    imguiAcc = ImGuiAcc::GetInstance();

#ifdef _GAMING_DESKTOP
    imguiAcc->SetWindow(window);
#endif // _GAMING_DESKTOP

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
    ID3D12DescriptorHeap* descriptorHeaps[]{ m_deviceResources->GetSRVHeap() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ImGuiDx12RendererNewFrame(commandList);
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    imguiAcc->NewFrame();
    imguiAcc->Begin("Example UI", 0, 0, ImVec2(700, 300));
    imguiAcc->WindowHeader("Example UI");
    imguiAcc->Button("Button 1");
    imguiAcc->Button("Button 2");
    static char inputText[256] = "";
    imguiAcc->InputText("Input Text", inputText, IM_ARRAYSIZE(inputText), ImGui_ImplWin32_ActiveGameInputKind());
    static int sliderValue = 5;
    imguiAcc->SliderInt("Slider", &sliderValue, 0, 10);
    imguiAcc->End();

    // Set the initial coordinates for the legend window
    static bool initialPlacement = false;
    if (!initialPlacement)
    {
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 bottomLeftCorner(windowPos.x, windowPos.y + 320);

        ImGui::SetNextWindowPos(ImVec2(bottomLeftCorner.x, bottomLeftCorner.y));
        initialPlacement = true;
    }

    imguiAcc->Begin("Legend", 0, 0, ImVec2(700, 300));
    imguiAcc->WindowHeader("Legend");
    imguiAcc->Text("Switch Window: CTRL + TAB");
    imguiAcc->Text("Next Widget: TAB, DOWN ARROW");
    imguiAcc->Text("Previous Widget: SHIFT + TAB, UP ARROW");
    imguiAcc->Text("Activate Widget: ENTER");
    imguiAcc->Text("Un-focus Widget: ESC");
    imguiAcc->Text("Horizontal Scroll: LEFT ARROW, RIGHT ARROW");
    static bool enableNarration = true;
    imguiAcc->Checkbox(": Enable Narration ", &enableNarration);
    ImGui::Separator();
    ImGui::Text("");
    imguiAcc->Text("Theme control is automatically pulled from Windows settings.");
    imguiAcc->Text("Text scaling is automatically pulled from Windows settings.");
    imguiAcc->Text("Narration is automatic. Focusing on a widget will narrate that widget.");

    if(enableNarration)
    {
        imguiAcc->EnableNarration();
    }
    else
    {
        imguiAcc->DisableNarration();
    }
    imguiAcc->End();

    ImGui::Render();
    ImDrawData* drawData = ImGui::GetDrawData();
    ImGuiDx12RendererRenderDrawData(drawData, commandList);
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
    D3D12_FEATURE_DATA_SHADER_MODEL shaderModel{ D3D_SHADER_MODEL_6_0 };
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

