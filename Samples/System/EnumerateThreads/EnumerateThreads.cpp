//--------------------------------------------------------------------------------------
// EnumerateThreads.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "EnumerateThreads.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"
#include "ThreadEnum.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

void Sample::ThreadFunc(uint32_t index)
{
    wchar_t buffer[64];
    swprintf(buffer, 64, L"Test Thread: %d", index);
    SetThreadDescription(GetCurrentThread(), buffer);

    while (!m_killThreads)
    {
        Sleep(0);
    }
}

void Sample::KillTestThreads()
{
    m_killThreads = true;
    for (auto& iter : m_testThreads)
    {
        iter->join();
        delete iter;
    }
    m_testThreads.clear();
    m_killThreads = false;
}

void Sample::CreateTestThreads(uint32_t numTotalThreads)
{
    KillTestThreads();
    for (uint32_t i = 0; i < numTotalThreads; i++)
    {
        m_testThreads.push_back(new std::thread(std::bind(&Sample::ThreadFunc, this, m_lastThreadIndex++)));
    }
}

bool Sample::EnumerateThreadsCallback(uint32_t processID, uint32_t threadID)
{
    if (processID == GetCurrentProcessId())
    {
#ifdef _GAMING_XBOX
        uint32_t processPriorityClass = NORMAL_PRIORITY_CLASS;
#else
        uint32_t processPriorityClass = GetPriorityClass(GetCurrentProcess());
#endif
        HANDLE threadHandle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, threadID);
        if (threadHandle)
        {
            wchar_t* threadDescription = nullptr;
            GetThreadDescription(threadHandle, &threadDescription);
            uint32_t priority = ConvertThreadPriorityClassToBasePriority(GetThreadPriority(threadHandle), processPriorityClass);
            m_foundThreadsFromEnumeration.push_back(FoundThreadData(threadDescription, threadID, priority));
            CloseHandle(threadHandle);
        }
    }
    return true;
}

Sample::Sample() noexcept(false) :
    m_killThreads(false),
    m_lastThreadIndex(0),
    m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
    SetThreadDescription(GetCurrentThread(), L"Main Sample Thread");
}

Sample::~Sample()
{
    KillTestThreads();
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

    m_deviceResources->SetWindow(window, width, height);

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
    if (timer.GetFrameCount() == 3)
    {
        CreateTestThreads(5);
    }

    if (timer.GetFrameCount() == 60)
    {
        ATG::EnumerateThreads(std::bind(&Sample::EnumerateThreadsCallback, this, std::placeholders::_1, std::placeholders::_2));
    }

#ifdef USING_GAMEINPUT
    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
#else
    auto pad = m_gamePad->GetState(0);
#endif
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
        }
        if (pad.IsViewPressed())
        {
            ExitSample();
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

    RECT safeRect = SimpleMath::Viewport::ComputeTitleSafeArea(1920, 1080);
    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    Clear();

    auto commandList = m_deviceResources->GetCommandList();
    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Render");

    ID3D12DescriptorHeap* pHeaps[] = { m_resourceDescriptors->Heap() };
    commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

    m_spriteBatch->Begin(commandList);

    m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), GetTextureSize(m_background.Get()), XMFLOAT2(0, 0));

    if (m_timer.GetFrameCount() > 3)
    {
        wchar_t buffer[128];
        DX::DrawControllerString(m_spriteBatch.get(), m_largeFont.get(), m_ctrlFont.get(), L"EnumerateThreads Sample", pos);
        pos.y += (m_largeFont->GetLineSpacing());

        auto headerPos(pos);
        headerPos.y += m_regularFont->GetLineSpacing();

        pos.y += m_regularFont->GetLineSpacing();
        pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));;

        std::map<uint32_t, uint32_t> noNameThreadsPerPriority;
        for (const auto& iter : m_foundThreadsFromEnumeration)
        {
            if (iter.m_name.empty())
            {
                if (noNameThreadsPerPriority.find(iter.m_priority) == noNameThreadsPerPriority.end())
                    noNameThreadsPerPriority[iter.m_priority] = 1;
                else
                    noNameThreadsPerPriority[iter.m_priority]++;
            }
        }
        for (auto& iter : m_foundThreadsFromEnumeration)
        {
            if (!iter.m_name.empty())
            {
                swprintf(buffer, 128, L"Thread %d: Priority: %d     Name: %s", iter.m_threadID, iter.m_priority, iter.m_name.c_str());
                m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
                pos.y += m_regularFont->GetLineSpacing();
            }
        }
        for (const auto& iter : noNameThreadsPerPriority)
        {
            swprintf(buffer, 128, L"%d thread%s at priority %d with no name", iter.second, iter.second > 1 ? L"s" : L"", iter.first);
            m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
            pos.y += m_regularFont->GetLineSpacing();
        }
    }

    m_spriteBatch->End();

    PIXEndEvent(commandList);

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
}

void Sample::DrawCenteredString(const wchar_t* str, float left, float right, float y)
{
    auto stringWidth = XMVectorGetX(m_regularFont->MeasureString(str));

    float xCoord = left + ((right - left) / 2);
    xCoord -= (stringWidth / 2);
    m_regularFont->DrawString(m_spriteBatch.get(), str, XMFLOAT2(xCoord, y));
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
    wchar_t strFilePath[MAX_PATH] = {};

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    ResourceUploadBatch resourceUpload(device);

    resourceUpload.Begin();

    DX::FindMediaFile(strFilePath, MAX_PATH, L"ATGSampleBackground.dds");
    DX::ThrowIfFailed(
        CreateDDSTextureFromFile(device, resourceUpload,
            strFilePath,
            m_background.ReleaseAndGetAddressOf()));

    CreateShaderResourceView(device, m_background.Get(), m_resourceDescriptors->GetCpuHandle(Descriptors::Background));

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    {
        SpriteBatchPipelineStateDescription pd(rtState);
        m_spriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);
    }

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
        m_regularFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::RegularFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::RegularFont));
    }

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_24.spritefont");
        m_largeFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::LargeFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::LargeFont));
    }

    {
        DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneController.spritefont");
        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            strFilePath,
            m_resourceDescriptors->GetCpuHandle(Descriptors::CtrlFont),
            m_resourceDescriptors->GetGpuHandle(Descriptors::CtrlFont));
    }

    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());

    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto viewport = m_deviceResources->GetScreenViewport();
    m_spriteBatch->SetViewport(viewport);
}

void Sample::OnDeviceLost()
{
    m_regularFont.reset();
    m_largeFont.reset();
    m_ctrlFont.reset();
    m_spriteBatch.reset();
    m_resourceDescriptors.reset();

    m_background.Reset();

    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}

uint32_t Sample::ConvertThreadPriorityClassToBasePriority(int32_t priority, uint32_t priorityClass)
{
    if (priority == THREAD_PRIORITY_IDLE)
    {
        if (priorityClass == REALTIME_PRIORITY_CLASS)
            return 16;
        return 1;
    }
    if (priority == THREAD_PRIORITY_TIME_CRITICAL)
    {
        if (priorityClass == REALTIME_PRIORITY_CLASS)
            return 31;
        return 15;
    }
    switch (priorityClass)
    {
    case IDLE_PRIORITY_CLASS:
        return priority + 4u;
    case BELOW_NORMAL_PRIORITY_CLASS:
        return priority + 6u;
    case NORMAL_PRIORITY_CLASS:
        return priority + 8u;
    case ABOVE_NORMAL_PRIORITY_CLASS:
        return priority + 10u;
    case HIGH_PRIORITY_CLASS:
        return priority + 13u;
    case  REALTIME_PRIORITY_CLASS:
        return priority + 24u;
    }
    return 0;
}
#pragma endregion
