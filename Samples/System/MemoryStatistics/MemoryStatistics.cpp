//--------------------------------------------------------------------------------------
// MemoryStatistics.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MemoryStatistics.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "FindMedia.h"

#include <XError.h>

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;

using Microsoft::WRL::ComPtr;

Sample::Sample() noexcept(false) :
    m_frame(0),
    m_frameXMemStats{},
    m_preRunXMemStats{},
    m_temporaryTextBuffer{},
    m_temporaryTextTime(0.f),
    m_gamepadPresent(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>();
    m_deviceResources->SetClearColor(ATG::Colors::Background);
    m_deviceResources->RegisterDeviceNotify(this);
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

    m_deviceResources->SetWindow(window, width, height);

    m_deviceResources->CreateDeviceResources();  	
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    m_teapots.reserve(c_MaxTeapots);
    CreateNewTeapot();

    // Grab a snapshot of the memory usage post-initialization so it can be compared to memory usage in any given frame
    XMemGetWorkingSetStatistics(XMEM_WORKING_SET_ALL, &m_preRunXMemStats);
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %I64u", m_frame);

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

    if (m_temporaryTextTime > 0.f)
    {
        m_temporaryTextTime -= elapsedTime;
        if (m_temporaryTextTime <= 0.f)
        {
            m_temporaryTextTime = 0.f;
            *m_temporaryTextBuffer = 0;
        }
    }

    // Query information about the memory usage
    XMemGetWorkingSetStatistics(XMEM_WORKING_SET_TITLE,       &m_frameXMemStats[XMemQueries::Title]);
    XMemGetWorkingSetStatistics(XMEM_WORKING_SET_TOOLS,       &m_frameXMemStats[XMemQueries::Tools]);
    XMemGetWorkingSetStatistics(XMEM_WORKING_SET_STACK,       &m_frameXMemStats[XMemQueries::Stack]);
    XMemGetWorkingSetStatistics(XMEM_WORKING_SET_KERNEL_POOL, &m_frameXMemStats[XMemQueries::Kernel]);
    XMemGetWorkingSetStatistics(XMEM_WORKING_SET_ALL,         &m_frameXMemStats[XMemQueries::All]);

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamepadPresent = true;

        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_teapots.size() < c_MaxTeapots)
                CreateNewTeapot();
        }

        if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::PRESSED)
        {
            if (m_teapots.size() > 1)
                DestroyTeapot();
        }

        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::PRESSED)
        {
            PercentageStats();
        }
    }
    else
    {
        m_gamepadPresent = false;
        m_gamePadButtons.Reset();
    }

    auto kb = m_keyboard->GetState();
    m_keyboardButtons.Update(kb);

    if (kb.Escape)
    {
        ExitSample();
    }

    if (kb.Right)
    {
        if (m_teapots.size() < c_MaxTeapots)
            CreateNewTeapot();
    }
    else if (kb.Left)
    {
        if (m_teapots.size() > 1)
            DestroyTeapot();
    }
    else if (kb.P)
    {
        PercentageStats();
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

    for (auto & teapot : m_teapots)
    {
        auto world = Matrix::CreateRotationY(teapot.m_lifeFrameCount++ / 100.f);

        m_effect->SetMatrices(world * teapot.m_location, m_view, m_projection);
        m_effect->Apply(commandList);

        teapot.m_teapot->Draw(commandList);
    }

    auto const rect = m_deviceResources->GetOutputSize();
    auto const safeRect = Viewport::ComputeTitleSafeArea(UINT(rect.right), UINT(rect.bottom));

    XMFLOAT2 pos(float(safeRect.left), float(safeRect.top));

    m_batch->Begin(commandList);

    if (*m_temporaryTextBuffer)
    {
        m_font->DrawString(m_batch.get(), m_temporaryTextBuffer, pos, ATG::Colors::Blue);
    }

    wchar_t buffer[2048] = {};
    swprintf_s(buffer,
        L"XMemGetWorkingSetStatistics:\n"
        L"GameLimit: \n"
        L"GameUsed:\n"
        L"ToolsLimit:\n"
        L"ToolsUsed:\n"

        L"StackUsed:\n"

        L"GpuOptimalBandwidthLimit:\n"
        L"GpuOptimalBandwidthUsed:\n"
        L"ToolsGpuOptimalBandwidthLimit:\n"
        L"ToolsGpuOptimalBandwidthUsed:\n"
        L"KernelPoolUsed:\n"
        L"KernelPoolLimit:\n");

    pos.y = float(safeRect.bottom) - XMVectorGetY(m_font->MeasureString(buffer)) - m_font->GetLineSpacing() * 1.5f;

    m_font->DrawString(m_batch.get(), buffer, pos, ATG::Colors::Green);

    pos.x += XMVectorGetX(m_font->MeasureString(buffer)) + 30;


    const wchar_t* typeString[] = { L"TITLE", L"TOOLS", L"STACK", L"KERNEL_POOL", L"ALL" };

    for (uint8_t xmemType = 0; xmemType <= XMemQueries::All; xmemType++)
    {
        float GameLimit = m_frameXMemStats[xmemType].GameLimit / (1024.f * 1024.f);
        float GameUsed = m_frameXMemStats[xmemType].GameUsed / (1024.f * 1024.f);
        float ToolsLimit = m_frameXMemStats[xmemType].ToolsLimit / (1024.f * 1024.f);
        float ToolsUsed = m_frameXMemStats[xmemType].ToolsUsed / (1024.f * 1024.f);
        float StackUsed = m_frameXMemStats[xmemType].StackUsed / (1024.f * 1024.f);

        float GpuOptimalBandwidthLimit = m_frameXMemStats[xmemType].GpuOptimalBandwidthLimit / (1024.f * 1024.f);
        float GpuOptimalBandwidthUsed = m_frameXMemStats[xmemType].GpuOptimalBandwidthUsed / (1024.f * 1024.f);
        float ToolsGpuOptimalBandwidthLimit = m_frameXMemStats[xmemType].ToolsGpuOptimalBandwidthLimit / (1024.f * 1024.f);
        float ToolsGpuOptimalBandwidthUsed = m_frameXMemStats[xmemType].ToolsGpuOptimalBandwidthUsed / (1024.f * 1024.f);
        float KernelPoolUsed = m_frameXMemStats[xmemType].KernelPoolUsed / (1024.f * 1024.f);
#if _GXDK_VER >= 0x4A610D49 /* GDK Edition 200602 */
        float KernelPoolLimit = m_frameXMemStats[xmemType].KernelPoolLimit / (1024.f * 1024.f);
#else
        float KernelPoolLimit = 0;
#endif

        swprintf_s(buffer,
            L"%s\n"
            L"%.3f (MiB)\n"
            L"%.3f (MiB)\n"
            L"%.3f (MiB)\n"
            L"%.3f (MiB)\n"
            L"%.3f (MiB)\n"

            L"%.3f (MiB)\n"
            L"%.3f (MiB)\n"
            L"%.3f (MiB)\n"
            L"%.3f (MiB)\n"
            L"%.3f (MiB)\n"
            L"%.3f (MiB)\n",
            typeString[xmemType],
            GameLimit,
            GameUsed,
            ToolsLimit,
            ToolsUsed,
            StackUsed,
            GpuOptimalBandwidthLimit,
            GpuOptimalBandwidthUsed,
            ToolsGpuOptimalBandwidthLimit,
            ToolsGpuOptimalBandwidthUsed,
            KernelPoolUsed,
            KernelPoolLimit);

        m_font->DrawString(m_batch.get(), buffer, pos, ATG::Colors::Green);
        pos.x += XMVectorGetX(m_font->MeasureString(buffer)) + 30;
    }

    pos.x = float(safeRect.left);
    pos.y = float(safeRect.bottom) - m_font->GetLineSpacing();

    if (m_gamepadPresent)
    {
        DX::DrawControllerString(m_batch.get(), m_font.get(), m_ctrlFont.get(), L"Use [DPad] to add/remove teapots, and the [Y] button for percentages", pos, ATG::Colors::OffWhite);
    }
    else
    {
        m_font->DrawString(m_batch.get(), L"Use Right key to add teapots, Left key to remove teapots, and the P key for percentages", pos, ATG::Colors::OffWhite);
    }

    m_batch->End();

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

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    m_resourceDescriptors = std::make_unique<DescriptorHeap>(device, Descriptors::Count);

    for (auto & teapot : m_teapots)
    {
        teapot.m_teapot = DirectX::GeometricPrimitive::CreateTeapot();
    }

    RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    ResourceUploadBatch upload(device);
    upload.Begin();

    {
        EffectPipelineStateDescription pd(
            &GeometricPrimitive::VertexType::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullCounterClockwise,
            rtState);

        m_effect = std::make_unique<BasicEffect>(device, EffectFlags::Lighting, pd);
        m_effect->EnableDefaultLighting();
        
        SpriteBatchPipelineStateDescription spritepd(
            rtState);

        m_batch = std::make_unique<SpriteBatch>(device, upload, spritepd);
    }

    wchar_t strFilePath[MAX_PATH] = {};
    DX::FindMediaFile(strFilePath, MAX_PATH, L"SegoeUI_18.spritefont");
    m_font = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::Font),
        m_resourceDescriptors->GetGpuHandle(Descriptors::Font));

    DX::FindMediaFile(strFilePath, MAX_PATH, L"XboxOneControllerLegendSmall.spritefont");
    m_ctrlFont = std::make_unique<SpriteFont>(device, upload,
        strFilePath,
        m_resourceDescriptors->GetCpuHandle(Descriptors::ControllerFont),
        m_resourceDescriptors->GetGpuHandle(Descriptors::ControllerFont));

    auto finish = upload.End(m_deviceResources->GetCommandQueue());
    finish.wait();

    m_deviceResources->WaitForGpu();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    auto const vp = m_deviceResources->GetScreenViewport();
    m_batch->SetViewport(vp);

    m_at = SimpleMath::Vector3(0.f, 0.f, -0.1f);
    m_eye = SimpleMath::Vector3(0.0f, 0.0f, 6.0f);
    m_view = SimpleMath::Matrix::CreateLookAt(m_eye, m_at, SimpleMath::Vector3::UnitY);

    auto const size = m_deviceResources->GetOutputSize();
    const float aspectRatio = float(size.right) / float(size.bottom);
    float fovAngleY = 70.0f * XM_PI / 180.0f;

    // This is a simple example of change that can be made when the app is in
    // portrait or snapped view.
    if (aspectRatio < 1.0f)
    {
        fovAngleY *= 2.0f;
    }

    m_projection = SimpleMath::Matrix::CreatePerspectiveFieldOfView(
        fovAngleY,
        aspectRatio,
        0.01f,
        100.0f
    );
}

void Sample::OnDeviceLost()
{
    for (auto & teapot : m_teapots)
    {
        teapot.m_teapot.reset();
    }
    m_effect.reset();
    m_batch.reset();
    m_font.reset();
    m_ctrlFont.reset();
    m_resourceDescriptors.reset();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion

void Sample::CreateNewTeapot()
{
    // Grab a snapshot of memory usage before a teapot is allocated
    XMEM_WORKING_SET_STATISTICS before, after;
    XMemGetWorkingSetStatistics(XMEM_WORKING_SET_ALL, &before);

    TeapotData teapot;
    teapot.m_lifeFrameCount = 0;
    teapot.m_location = Matrix::CreateTranslation(FloatRand(-4.f, 4.f), FloatRand(-4.f, 4.f), FloatRand(-8.f, -4.f));

    m_teapots.push_back(teapot);
    m_teapots.back().m_teapot = DirectX::GeometricPrimitive::CreateTeapot();

    // Now that a teapot has been created, grab another capture of resource usage
    XMemGetWorkingSetStatistics(XMEM_WORKING_SET_ALL, &after);

    // Comparing resource usage before and after a teapot was created will allow
    //  us to see exactly how much was needed for this particular action. Display
    //  this data to the screen since it can be interesting.
    size_t titleDiff = after.GameUsed - before.GameUsed;
    size_t titleGpuDiff = after.GpuOptimalBandwidthUsed - before.GpuOptimalBandwidthUsed;
    size_t toolsDiff = after.ToolsUsed - before.ToolsUsed;
    size_t toolsGpuDiff = after.ToolsGpuOptimalBandwidthUsed - before.ToolsGpuOptimalBandwidthUsed;

    swprintf_s(m_temporaryTextBuffer, L"Memory used by teapot creation:\n"
        L"GameUsed: %zu bytes\n"
        L"GpuOptimalBandwidthUsed: %zu bytes\n"
        L"ToolsUsed: %zu bytes\n"
        L"ToolsGpuOptimalBandwidthUsed: %zu bytes\n\n",
        titleDiff,
        titleGpuDiff,
        toolsDiff,
        toolsGpuDiff);

    m_temporaryTextTime = 4.f;
}

void Sample::DestroyTeapot()
{
    m_teapots.pop_back();
}

float Sample::FloatRand(float lowerBound, float upperBound)
{
    if (lowerBound == upperBound)
        return lowerBound;

    std::uniform_real_distribution<float> dist(lowerBound, upperBound);

    return dist(m_randomEngine);
}

void Sample::PercentageStats()
{
    //  Calculate current frame's memory usage as a percentage of
    //  the memory that was needed for initialization. If this is greater than 100% it means
    //  more resources are being allocated at runtime. This is a good way to see if too many
    //  resources are being allocated during gameplay. If any of those allocations can be
    //  anticipated and made during initialization, this could improve performance during any
    //  portion of a game that the user is actively engaged in.

    float gameUsedPercent = 100 * float(m_frameXMemStats[XMemQueries::All].GameUsed) / float(m_preRunXMemStats.GameUsed);
    float gameUsedGpuPercent = 100 * float(m_frameXMemStats[XMemQueries::All].GpuOptimalBandwidthUsed) / float(m_preRunXMemStats.GpuOptimalBandwidthUsed);
    float stackUsedPercent = 100 * float(m_frameXMemStats[XMemQueries::All].StackUsed) / float(m_preRunXMemStats.StackUsed);
    float kernelUsedPercent = 100 * float(m_frameXMemStats[XMemQueries::All].KernelPoolUsed) / float(m_preRunXMemStats.KernelPoolUsed);
    swprintf_s(m_temporaryTextBuffer, L"Percentage of initial memory in use\n"
        L"GameUsed: %.2f%%\n"
        L"GpuOptimalBandwidthUsed: %.2f%%\n"
        L"StackUsed: %.2f%%\n"
        L"KernelPoolUsed: %.2f%%\n\n",
        gameUsedPercent,
        gameUsedGpuPercent,
        stackUsedPercent,
        kernelUsedPercent);

    m_temporaryTextTime = 4.f;
}

