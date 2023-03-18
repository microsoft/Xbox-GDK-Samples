//--------------------------------------------------------------------------------------
// SimplePLM.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SimplePLM.h"

#include "ATGColors.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_consoleIsValid(false),
    m_queue(nullptr)
{
    // 2D only rendering
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);

    m_console = std::make_unique<DX::TextConsoleImage>();
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    //
    // See Main.cpp for calls to RegisterAppStateChangeNotification and RegisterAppConstrainedChangeNotification
    //

    DX::ThrowIfFailed(
        XTaskQueueCreate(XTaskQueueDispatchMode::ThreadPool, XTaskQueueDispatchMode::Manual, &m_queue)
    );

    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    m_deviceResources->WaitForOrigin();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();

    while (XTaskQueueDispatch(m_queue, XTaskQueuePort::Completion, 0))
        { }

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const&)
{
    PIXScopedEvent(PIX_COLOR_DEFAULT, L"Update");

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            LogPLMEvent(L"Showing Account Picker");

            auto async = new XAsyncBlock;
            memset(async, 0, sizeof(XAsyncBlock));
            async->queue = m_queue;
            async->context = this;
            async->callback = [](XAsyncBlock *ab)
            {
                auto sample = reinterpret_cast<Sample*>(ab->context);

                sample->LogPLMEvent(L"Account selection complete");

                // We don't really care about the result for this sample...
                delete ab;
            };

            HRESULT hr = XUserAddAsync(XUserAddOptions::None, async);
            if (FAILED(hr))
            {
                delete async;
                throw std::exception("XUserAddAsync");
            }
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            LogPLMEvent(L"Launching into Settings");
            XLaunchUri(nullptr, "settings:");
        }
    }
    else
    {
        m_gamePadButtons.Reset();
    }
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

    auto heap = m_resourceDescriptors->Heap();
    commandList->SetDescriptorHeaps(1, &heap);

    m_console->Render(commandList);

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

    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

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
}

void Sample::OnConstrained()
{
}

void Sample::OnUnConstrained()
{
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    m_console->RestoreDevice(device, resourceUpload, rtState, L"courier_16.spritefont", L"ATGSampleBackground.DDS",
        m_resourceDescriptors->GetCpuHandle(Descriptors::ConsolasFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ConsolasFont),
        m_resourceDescriptors->GetCpuHandle(Descriptors::BackgroundImage),
        m_resourceDescriptors->GetGpuHandle(Descriptors::BackgroundImage));

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();

    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const viewport = m_deviceResources->GetScreenViewport();
    m_console->SetViewport(viewport);

    m_console->SetWindow(m_deviceResources->GetOutputSize(), true);

    // Now that the Console is valid we can flush any logs to the console.
    m_consoleIsValid = true;
    while (!m_logCache.empty())
    {
        m_console->WriteLine(m_logCache.back().c_str());
        m_logCache.pop_back();
    }
}
#pragma endregion

void Sample::ShowInstructions()
{
    m_logCache.insert(m_logCache.begin(), L"Simple PLM");

    m_logCache.insert(m_logCache.begin(), L"Launch Settings with A button");
    m_logCache.insert(m_logCache.begin(), L"Show Account Picker with X button");
}

void Sample::LogPLMEvent(const wchar_t* primaryLog, const wchar_t* secondaryData)
{
    unsigned int tid = GetCurrentThreadId();
    SYSTEMTIME curTime;
    GetSystemTime(&curTime);

    wchar_t timeAndTid[25];
    swprintf_s(timeAndTid, L"[%02d:%02d:%02d:%03d](%d)", curTime.wHour, curTime.wMinute, curTime.wSecond, curTime.wMilliseconds, tid);

    std::wstring logLine = timeAndTid;
    logLine += L" ";
    logLine += primaryLog;
    logLine += L" ";
    logLine += secondaryData;

    //Output to Debug Console.
    OutputDebugStringW(logLine.c_str());
    OutputDebugStringW(L"\n");

    //Output to screen. We must cache screen logs if a log occurs when there is no valid screen console yet.
    if (!m_consoleIsValid)
    {
        m_logCache.insert(m_logCache.begin(), logLine);
    }
    else
    {
        m_console->WriteLine(logLine.c_str());
    }
}
