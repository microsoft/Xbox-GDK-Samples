//--------------------------------------------------------------------------------------
// FutexCombo.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FutexCombo.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"

extern void ExitSample() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_displayMode(DisplayMode::e_mixedMode)
    , m_nextCycleTimeSecs(DBL_MAX)
    , m_frame(0)
{
    // Renders only 2D, so no need for a depth buffer.
    m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_UNKNOWN);
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
}

Sample::~Sample()
{
    FutexTest::StopFutexTest();
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

void Sample::SetMutexTestResults(MutexType mutexType, ContentionLevel contentionLevel, uint32_t numThreads, bool threadsLocked, uint64_t workerWorkDone, uint64_t backgroundWorkDone, uint64_t* workerWorkDonePerThread, uint64_t* backgroundWorkDonePerThread)
{
    std::scoped_lock lock(m_resultsMutex);

    std::vector<uint64_t> workDonePerThread;
    workDonePerThread.resize(numThreads);
    for (uint32_t i = 0; i < numThreads; i++)
    {
        workDonePerThread[i] = workerWorkDonePerThread[i];
    }
    m_workerWorkDone[threadsLocked ? 0 : 1][std::pair(mutexType, contentionLevel)] = std::pair(workerWorkDone, workDonePerThread);

    for (uint32_t i = 0; i < numThreads; i++)
    {
        workDonePerThread[i] = backgroundWorkDonePerThread[i];
    }
    m_backgroundWorkDone[threadsLocked ? 0 : 1][std::pair(mutexType, contentionLevel)] = std::pair(backgroundWorkDone, workDonePerThread);
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

void Sample::CycleScreen()
{
    switch (m_displayMode)
    {
    case DisplayMode::e_lockedThreads:
        m_displayMode = DisplayMode::e_floatingThreads;
        break;
    case DisplayMode::e_floatingThreads:
        m_displayMode = DisplayMode::e_mixedMode;
        break;
    case DisplayMode::e_mixedMode:
        m_displayMode = DisplayMode::e_lockedThreads;
        break;
    }
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    if (timer.GetFrameCount() == 3)
    {
        m_nextCycleTimeSecs = timer.GetTotalSeconds() + 5;
        FutexTest::StartFutexTest(this);
    }

#ifdef _GAMING_XBOX
    auto pad = m_gamePad->GetState(GamePad::c_MergedInput);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            CycleScreen();
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
#else
    if (timer.GetTotalSeconds() >= m_nextCycleTimeSecs)
    {
        CycleScreen();
        m_nextCycleTimeSecs = timer.GetTotalSeconds() + 5;
    }
#endif
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

    auto gridTopLeft(pos);

    m_spriteBatch->Begin(commandList);

    //m_spriteBatch->Draw(m_resourceDescriptors->GetGpuHandle(Descriptors::Background), GetTextureSize(m_background.Get()), XMFLOAT2(0, 0));

    if (m_timer.GetFrameCount() > 3)
    {
#ifdef _GAMING_XBOX
        switch (m_displayMode)
        {
        case DisplayMode::e_lockedThreads:
            DX::DrawControllerString(m_spriteBatch.get(), m_largeFont.get(), m_ctrlFont.get(), L"Optimized Futex Sample - Work Done: Threads locked to cores   Press [A] to switch screens", pos);
            break;
        case DisplayMode::e_floatingThreads:
            DX::DrawControllerString(m_spriteBatch.get(), m_largeFont.get(), m_ctrlFont.get(), L"Optimized Futex Sample - Work Done: Threads freely floating   Press [A] to switch screens", pos);
            break;
        case DisplayMode::e_mixedMode:
            DX::DrawControllerString(m_spriteBatch.get(), m_largeFont.get(), m_ctrlFont.get(), L"Optimized Futex Sample - Total Work: Thread affinity mixed mode   Press [A] to switch screens", pos);
            break;
        }
#else
        switch (m_displayMode)
        {
        case DisplayMode::e_lockedThreads:
            m_largeFont->DrawString(m_spriteBatch.get(), L"Optimized Futex Sample - Work Done: Threads locked to cores", pos);
            break;
        case DisplayMode::e_floatingThreads:
            m_largeFont->DrawString(m_spriteBatch.get(), L"Optimized Futex Sample - Work Done: Threads freely floating", pos);
            break;
        case DisplayMode::e_mixedMode:
            m_largeFont->DrawString(m_spriteBatch.get(), L"Optimized Futex Sample - Total Work: Thread affinity mixed mode", pos);
            break;
        }
#endif
        pos.y += (m_largeFont->GetLineSpacing());

        auto headerPos(pos);
        headerPos.y += m_regularFont->GetLineSpacing();
        DisplayLineHeader(MutexType::e_futex, ContentionLevel::e_highContention, headerPos);
        DisplayLineHeader(MutexType::e_slowtex, ContentionLevel::e_highContention, headerPos);
        DisplayLineHeader(MutexType::e_nulltex, ContentionLevel::e_highContention, headerPos);

        DisplayLineHeader(MutexType::e_futex, ContentionLevel::e_mediumContention, headerPos);
        DisplayLineHeader(MutexType::e_slowtex, ContentionLevel::e_mediumContention, headerPos);
        DisplayLineHeader(MutexType::e_nulltex, ContentionLevel::e_mediumContention, headerPos);

        DisplayLineHeader(MutexType::e_futex, ContentionLevel::e_lowContention, headerPos);
        DisplayLineHeader(MutexType::e_slowtex, ContentionLevel::e_lowContention, headerPos);
        DisplayLineHeader(MutexType::e_nulltex, ContentionLevel::e_lowContention, headerPos);

        if (m_displayMode == DisplayMode::e_mixedMode)
        {
            float gridXBreak[] = { 700,1000,1300 };
            DrawCenteredString(L"Threads locked to cores", gridXBreak[0], gridXBreak[1], pos.y);
            DrawCenteredString(L"Threads freely floating", gridXBreak[1], gridXBreak[2], pos.y);

            pos.y += m_regularFont->GetLineSpacing();
            pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));
            gridTopLeft = pos;

            DisplayMixedResults(MutexType::e_futex, ContentionLevel::e_highContention, pos, gridXBreak);
            DisplayMixedResults(MutexType::e_slowtex, ContentionLevel::e_highContention, pos, gridXBreak);
            DisplayMixedResults(MutexType::e_nulltex, ContentionLevel::e_highContention, pos, gridXBreak);

            DisplayMixedResults(MutexType::e_futex, ContentionLevel::e_mediumContention, pos, gridXBreak);
            DisplayMixedResults(MutexType::e_slowtex, ContentionLevel::e_mediumContention, pos, gridXBreak);
            DisplayMixedResults(MutexType::e_nulltex, ContentionLevel::e_mediumContention, pos, gridXBreak);

            DisplayMixedResults(MutexType::e_futex, ContentionLevel::e_lowContention, pos, gridXBreak);
            DisplayMixedResults(MutexType::e_slowtex, ContentionLevel::e_lowContention, pos, gridXBreak);
            DisplayMixedResults(MutexType::e_nulltex, ContentionLevel::e_lowContention, pos, gridXBreak);

            m_gridBatchEffect->Apply(commandList);
            m_gridBatch->Begin(commandList);
            DrawGrid(gridTopLeft, gridXBreak, 3);
            m_gridBatch->End();
        }
        else
        {
            float gridXBreak[] = { 700,900,1100,1300,1500,1700 };
            DrawCenteredString(L"Thread 1", gridXBreak[0], gridXBreak[1], pos.y);
            DrawCenteredString(L"Thread 2", gridXBreak[1], gridXBreak[2], pos.y);
            DrawCenteredString(L"Thread 3", gridXBreak[2], gridXBreak[3], pos.y);
            DrawCenteredString(L"Thread 4", gridXBreak[3], gridXBreak[4], pos.y);
            DrawCenteredString(L"Total Work", gridXBreak[4], gridXBreak[5], pos.y);

            pos.y += m_regularFont->GetLineSpacing();
            pos.x += XMVectorGetX(m_regularFont->MeasureString(L"XXX"));
            gridTopLeft = pos;

            uint32_t threadLockIndex = m_displayMode == DisplayMode::e_lockedThreads ? 0u : 1u;
            DisplaySingleResults(MutexType::e_futex, ContentionLevel::e_highContention, pos, gridXBreak, threadLockIndex);
            DisplaySingleResults(MutexType::e_slowtex, ContentionLevel::e_highContention, pos, gridXBreak, threadLockIndex);
            DisplaySingleResults(MutexType::e_nulltex, ContentionLevel::e_highContention, pos, gridXBreak, threadLockIndex);

            DisplaySingleResults(MutexType::e_futex, ContentionLevel::e_mediumContention, pos, gridXBreak, threadLockIndex);
            DisplaySingleResults(MutexType::e_slowtex, ContentionLevel::e_mediumContention, pos, gridXBreak, threadLockIndex);
            DisplaySingleResults(MutexType::e_nulltex, ContentionLevel::e_mediumContention, pos, gridXBreak, threadLockIndex);

            DisplaySingleResults(MutexType::e_futex, ContentionLevel::e_lowContention, pos, gridXBreak, threadLockIndex);
            DisplaySingleResults(MutexType::e_slowtex, ContentionLevel::e_lowContention, pos, gridXBreak, threadLockIndex);
            DisplaySingleResults(MutexType::e_nulltex, ContentionLevel::e_lowContention, pos, gridXBreak, threadLockIndex);

            m_gridBatchEffect->Apply(commandList);
            m_gridBatch->Begin(commandList);
            DrawGrid(gridTopLeft, gridXBreak, 6);
            m_gridBatch->End();
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

void Sample::DrawGrid(XMFLOAT2& pos, float* xCoords, uint32_t numGridLines)
{
    constexpr float xAdjust(1920.0 / 2);
    constexpr float yAdjust(1080.0 / 2);
    constexpr float xScale(1.0 / xAdjust);
    constexpr float yScale(-1.0 / yAdjust);
    float yDelta = m_regularFont->GetLineSpacing() * 3;
    for (uint32_t i = 0; i < 9; i++)
    {
        VertexPositionColor startPoint(SimpleMath::Vector3((pos.x - xAdjust) * xScale, (pos.y - yAdjust) * yScale, 0), Colors::Green);
        VertexPositionColor endPoint(SimpleMath::Vector3((xCoords[numGridLines - 1] - xAdjust) * xScale, (pos.y - yAdjust) * yScale, 0), Colors::Green);
        m_gridBatch->DrawLine(startPoint, endPoint);
        for (uint32_t j = 0; j < numGridLines; j++)
        {
            startPoint = VertexPositionColor(SimpleMath::Vector3((xCoords[j] - xAdjust) * xScale, (pos.y - yAdjust) * yScale, 0), Colors::Green);
            endPoint = VertexPositionColor(SimpleMath::Vector3((xCoords[j] - xAdjust) * xScale, ((pos.y + yDelta) - yAdjust) * yScale, 0), Colors::Green);
            m_gridBatch->DrawLine(startPoint, endPoint);
        }
        pos.y += yDelta;
    }
    {
        VertexPositionColor startPoint(SimpleMath::Vector3((pos.x - xAdjust) * xScale, (pos.y - yAdjust) * yScale, 0), Colors::Green);
        VertexPositionColor endPoint(SimpleMath::Vector3((xCoords[numGridLines - 1] - xAdjust) * xScale, (pos.y - yAdjust) * yScale, 0), Colors::Green);
        m_gridBatch->DrawLine(startPoint, endPoint);
    }
}

void Sample::DisplayLineHeader(MutexType mutexType, ContentionLevel contentionLevel, DirectX::XMFLOAT2& pos)
{
    static const wchar_t* mutexName[] = {
        L"Futex",
        L"Slowtex",
        L"Nulltex",
    };
    static const wchar_t* contentionName[] = {
        L"High Contention",
        L"Medium Contention",
        L"Low Contention",
    };
    wchar_t buffer[128];
    std::scoped_lock lock(m_resultsMutex);
    auto indent = XMVectorGetX(m_regularFont->MeasureString(L"XXX"));

    swprintf(buffer, 128, L"%s-%s", mutexName[static_cast<uint32_t> (mutexType)], contentionName[static_cast<uint32_t> (contentionLevel)]);
    m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
    pos.y += m_regularFont->GetLineSpacing();
    pos.x += indent;
    swprintf(buffer, 128, L"Foreground Work Done:  ");
    m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
    pos.y += m_regularFont->GetLineSpacing();
    swprintf(buffer, 128, L"Background Work Done: ");
    m_regularFont->DrawString(m_spriteBatch.get(), buffer, pos);
    pos.x -= indent;
    pos.y += m_regularFont->GetLineSpacing();
}

void Sample::DisplaySingleResults(MutexType mutexType, ContentionLevel contentionLevel, DirectX::XMFLOAT2& pos, float* xCoords, uint32_t threadLockIndex)
{
    float dataYPos = pos.y + m_regularFont->GetLineSpacing();
    wchar_t buffer[128];
    bool stillCalculating(false);
    std::scoped_lock lock(m_resultsMutex);
    if ((m_workerWorkDone[threadLockIndex].find(std::pair(mutexType, contentionLevel)) == m_workerWorkDone[threadLockIndex].end()) ||
        (m_backgroundWorkDone[threadLockIndex].find(std::pair(mutexType, contentionLevel)) == m_backgroundWorkDone[threadLockIndex].end()))
    {
        stillCalculating = true;
    }

    if (stillCalculating)
    {
        for (uint32_t i = 0; i < 5; i++)
        {
            DrawCenteredString(L"Calculating", xCoords[i], xCoords[i + 1], dataYPos);
        }
        dataYPos += m_regularFont->GetLineSpacing();
        for (uint32_t i = 0; i < 5; i++)
        {
            DrawCenteredString(L"Calculating", xCoords[i], xCoords[i + 1], dataYPos);
        }
    }
    else
    {
        {
            uint32_t curColumn = 0;
            for (auto& iter : m_workerWorkDone[threadLockIndex][std::pair(mutexType, contentionLevel)].second)
            {
                swprintf(buffer, 128, L"%llu", iter);
                DrawCenteredString(buffer, xCoords[curColumn], xCoords[curColumn + 1], dataYPos);
                curColumn++;
            }
            {
                swprintf(buffer, 128, L"%llu", m_workerWorkDone[threadLockIndex][std::pair(mutexType, contentionLevel)].first);
                DrawCenteredString(buffer, xCoords[curColumn], xCoords[curColumn + 1], dataYPos);
            }
        }

        dataYPos += m_regularFont->GetLineSpacing();
        {
            uint32_t curColumn = 0;
            for (auto& iter : m_backgroundWorkDone[threadLockIndex][std::pair(mutexType, contentionLevel)].second)
            {
                swprintf(buffer, 128, L"%llu", iter);
                DrawCenteredString(buffer, xCoords[curColumn], xCoords[curColumn + 1], dataYPos);
                curColumn++;
            }
            {
                swprintf(buffer, 128, L"%llu", m_backgroundWorkDone[threadLockIndex][std::pair(mutexType, contentionLevel)].first);
                DrawCenteredString(buffer, xCoords[curColumn], xCoords[curColumn + 1], dataYPos);
            }
        }
    }
    pos.y += m_regularFont->GetLineSpacing() * 3;
}

void Sample::DisplayMixedResults(MutexType mutexType, ContentionLevel contentionLevel, DirectX::XMFLOAT2& pos, float* xCoords)
{
    float dataYPos = pos.y + m_regularFont->GetLineSpacing();
    wchar_t buffer[128];
    bool stillCalculating(false);
    std::scoped_lock lock(m_resultsMutex);
    if ((m_workerWorkDone[0].find(std::pair(mutexType, contentionLevel)) == m_workerWorkDone[0].end()) ||
        (m_workerWorkDone[1].find(std::pair(mutexType, contentionLevel)) == m_workerWorkDone[1].end()) ||
        (m_backgroundWorkDone[0].find(std::pair(mutexType, contentionLevel)) == m_backgroundWorkDone[0].end()) ||
        (m_backgroundWorkDone[1].find(std::pair(mutexType, contentionLevel)) == m_backgroundWorkDone[1].end()))
    {
        stillCalculating = true;
    }

    if (stillCalculating)
    {
        for (uint32_t i = 0; i < 2; i++)
        {
            DrawCenteredString(L"Calculating", xCoords[i], xCoords[i + 1], dataYPos);
        }
        dataYPos += m_regularFont->GetLineSpacing();
        for (uint32_t i = 0; i < 2; i++)
        {
            DrawCenteredString(L"Calculating", xCoords[i], xCoords[i + 1], dataYPos);
        }
    }
    else
    {
        swprintf(buffer, 128, L"%llu", m_workerWorkDone[0][std::pair(mutexType, contentionLevel)].first);
        DrawCenteredString(buffer, xCoords[0], xCoords[1], dataYPos);
        swprintf(buffer, 128, L"%llu", m_workerWorkDone[1][std::pair(mutexType, contentionLevel)].first);
        DrawCenteredString(buffer, xCoords[1], xCoords[2], dataYPos);

        dataYPos += m_regularFont->GetLineSpacing();

        swprintf(buffer, 128, L"%llu", m_backgroundWorkDone[0][std::pair(mutexType, contentionLevel)].first);
        DrawCenteredString(buffer, xCoords[0], xCoords[1], dataYPos);
        swprintf(buffer, 128, L"%llu", m_backgroundWorkDone[1][std::pair(mutexType, contentionLevel)].first);
        DrawCenteredString(buffer, xCoords[1], xCoords[2], dataYPos);
    }
    pos.y += m_regularFont->GetLineSpacing() * 3;
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

    m_gridBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

    {
        EffectPipelineStateDescription pd(&VertexPositionColor::InputLayout, CommonStates::Opaque, CommonStates::DepthNone, CommonStates::CullNone, rtState, D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
        m_gridBatchEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, pd);
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
#pragma endregion
