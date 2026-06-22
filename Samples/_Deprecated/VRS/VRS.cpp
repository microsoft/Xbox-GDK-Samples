//--------------------------------------------------------------------------------------
// VRS.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "VRS.h"

#include "ATGColors.h"
#include "FindMedia.h"


#pragma warning(disable : 5038)
#pragma warning(disable : 4061)


extern void ExitSample() noexcept;

using namespace DirectX;
using namespace SimpleMath;

using Microsoft::WRL::ComPtr;
using DirectX::Colors::Black;
using DirectX::Colors::White;
using DirectX::Colors::PeachPuff;

namespace
{
    inline void UpdateTimingResult(float& avg, float value)
    {
        if (avg == 0.0f)
        {
            avg = value;
        }
        else if (value > 0.0)
        {
            avg = lerp(value, avg, 0.925f);
        }
    }
}

Sample::Sample() noexcept(false)
    : m_frame(0)
    , m_position(-56.8390427f, 12.7207355f, -16.0951595f)
    , m_pitch(-0.0675464123f)
    , m_yaw(-1.82958961f)
    , m_sunSpeed(SunSpeed::Slow)
    , m_sunHeight(0.1f)
    , m_sobelTolerance(0.2f)
    , m_solelToleranceHalfRes(0.2f)
    , m_visualise(Terrain::Visualise::VRS)
    , m_renderTechnique(Terrain::RenderTechnqiue::Normal)
    , m_shadingRateCalc(Terrain::ShadingRateCalculation::Wave64_HalfRes)
    , m_avgTimingResults{}
    , m_initializedAvgTimings(false)
{
    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_R10G10B10A2_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        2);
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

    m_terrain.Initialize(m_deviceResources, m_graphicsMemory);
    m_gpuTimer.Initialize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetCommandQueue(), Terrain::GPUPasses::Count);
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
    float elapsedTime = float(timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
              ExitSample();
        }
        m_yaw += pad.thumbSticks.rightX * elapsedTime;
        m_pitch += pad.thumbSticks.rightY * elapsedTime;
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
    // Limit to avoid looking directly up or down
    constexpr float c_limit = XM_PI / 2.0f - 0.01f;
    m_pitch = std::max(-c_limit, m_pitch);
    m_pitch = std::min(+c_limit, m_pitch);

    if (m_yaw > XM_PI)
    {
        m_yaw -= XM_PI * 2.f;
    }
    else if (m_yaw < -XM_PI)
    {
        m_yaw += XM_PI * 2.f;
    }

    XMVECTOR pos = XMLoadFloat3(&m_position);
    XMVECTOR camForwardVector = XMVectorSet(
        sinf(m_yaw),
        m_pitch,
        cosf(m_yaw),
        0);

    XMMATRIX camera = XMMatrixLookToLH(pos, camForwardVector, g_XMIdentityR1);

    if (pad.IsConnected())
    {
        if (!pad.IsLeftStickPressed())
        {
            float speed = 10.0f * elapsedTime;

            XMMATRIX invCamera = XMMatrixTranspose(camera);
            float tempF = pad.thumbSticks.leftX * speed * 1.5f;     pos = XMVectorAdd(pos, XMVectorMultiply(invCamera.r[0], XMVectorSet(tempF, tempF, tempF, 1.0)));
            tempF = pad.thumbSticks.leftY * speed * 1.5f;           pos = XMVectorAdd(pos, XMVectorMultiply(invCamera.r[2], XMVectorSet(tempF, tempF, tempF, 1.0)));

            float y = pad.IsDPadDownPressed() ? -speed : 0.0f;
            y += pad.IsDPadUpPressed() ? speed : 0.0f;
            XMVECTOR yOffset = XMVectorSet(0.0f, y, 0.0f, 0.0f);
            m_position = XMVectorAdd(pos, yOffset);
        }
        if (pad.IsLeftShoulderPressed())
        {
            m_sunHeight -= elapsedTime * 0.33f;
            m_sunHeight = std::max(m_sunHeight, 0.1f);
        }
        if (pad.IsRightShoulderPressed())
        {
            m_sunHeight += elapsedTime * 0.33f;
            m_sunHeight = std::min(m_sunHeight, 1.0f);
        }        
        if (m_gamePadButtons.dpadLeft == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            UINT var = UINT(m_visualise) - 1;
            m_visualise = Terrain::Visualise::Enum(var >= Terrain::Visualise::Count ? Terrain::Visualise::Count - 1 : var);
        }
        if (m_gamePadButtons.dpadRight == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            UINT var = UINT(m_visualise) + 1;
            m_visualise = Terrain::Visualise::Enum(var >= Terrain::Visualise::Count ? 0 : var);
        }
        if (m_gamePadButtons.y == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            m_sunSpeed = SunSpeed::Enum((UINT(m_sunSpeed) + 1) % SunSpeed::Count);
        }
        if (m_gamePadButtons.x== GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            UINT var = UINT(m_renderTechnique) + 1;
            m_renderTechnique = Terrain::RenderTechnqiue::Enum(var >= Terrain::RenderTechnqiue::Count ? 0 : var);
        }
        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::ButtonState::PRESSED)
        {
            UINT var = UINT(m_shadingRateCalc) + 1;
            m_shadingRateCalc = Terrain::ShadingRateCalculation::Enum(var >= Terrain::ShadingRateCalculation::Count ? 0 : var);
        }
        static float sobelSpeed = 0.001f;

        float &sobelTolerance = (m_shadingRateCalc & 1) ? m_solelToleranceHalfRes : m_sobelTolerance;
        sobelTolerance -= pad.triggers.right * sobelSpeed;
        sobelTolerance += pad.triggers.left * sobelSpeed;
        sobelTolerance = std::min(sobelTolerance, 1.0f);
        sobelTolerance = std::max(sobelTolerance, 0.0f);     
    }
    m_viewProj = XMMatrixMultiply(camera, m_proj);
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    if (!m_initializedAvgTimings)
    {
        ZeroMemory(m_avgTimingResults, sizeof(m_avgTimingResults));
        m_initializedAvgTimings = true;
    }
    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    auto commandList = m_deviceResources->GetCommandList();
    auto const viewport = m_deviceResources->GetScreenViewport();
    auto const scissorRect = m_deviceResources->GetScissorRect();
    commandList->RSSetViewports(1, &viewport);
    commandList->RSSetScissorRects(1, &scissorRect);

    PIXBeginEvent(commandList, PIX_COLOR_DEFAULT, L"Terrain Render");
    m_gpuTimer.BeginFrame();

    static float sunSpeed[SunSpeed::Count] = { 0.0f, 1.0f, 10.0f };
    m_terrain.Render(
        m_viewProj,
        m_position,
        m_deviceResources,
        m_graphicsMemory,
        float(m_timer.GetElapsedSeconds()) * sunSpeed[m_sunSpeed],
        m_gpuTimer,
        m_visualise,
        m_renderTechnique,
        (m_shadingRateCalc & 1) ? m_solelToleranceHalfRes : m_sobelTolerance,
        m_shadingRateCalc,
        m_sunHeight);
    {
        float timingResults[Terrain::GPUPasses::Count];
        m_gpuTimer.EndFrame(commandList, timingResults);

        for (UINT i = 0; i < Terrain::GPUPasses::Count; ++i)
        {
            UpdateTimingResult(m_avgTimingResults[i], timingResults[i]);
        }
    }
    PIXEndEvent(commandList);

    // Switch to back buffer
    auto rtvDescriptor = m_deviceResources->GetRenderTargetView();
    auto dsvDescriptor = m_deviceResources->GetDepthStencilView();
    commandList->OMSetRenderTargets(1, &rtvDescriptor, FALSE, &dsvDescriptor);

    // Render the text    
    float totalTimeNoVRS = m_avgTimingResults[Terrain::GPUPasses::NoVRSTerrain] + m_avgTimingResults[Terrain::GPUPasses::NoVRSSky]; // + avgTimingResults[Terrain::GPUPasses::NoVRSClear]
    float totalTimeVRSNoOverheads = m_avgTimingResults[Terrain::GPUPasses::VRSTerrain] + m_avgTimingResults[Terrain::GPUPasses::VRSSky]; // + avgTimingResults[Terrain::GPUPasses::VRSClear]
    float overheads = m_avgTimingResults[Terrain::GPUPasses::VRSCalcShadingRate] + m_avgTimingResults[Terrain::GPUPasses::VRSSetShadingRate];
    float totalTimeVRS = totalTimeVRSNoOverheads + overheads;

    ID3D12DescriptorHeap* heaps[] = { m_uiDescriptorHeap->Heap() };
    commandList->SetDescriptorHeaps(_countof(heaps), heaps);

    auto viewportUI = m_deviceResources->GetScreenViewport();
    m_fontBatch->SetViewport(viewportUI);
    m_fontBatch->Begin(commandList);

    wchar_t strText[2048] = {};
    const float fontScale = 0.75f;
    Vector2 fontPos(50.0f, 40.0f);
    Vector2 fontPosTab(60.0f, 40.0f);

    const XMVECTORF32 &colour = PeachPuff;

    swprintf_s(strText, L"%d x %d Resolution, 2048x2048 Heightmap, Raymarched Soft Shadows, 27 Tap Raymarched AO", static_cast<int>(viewportUI.Width), static_cast<int>(viewportUI.Height));
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    m_textFont->DrawString(m_fontBatch.get(), L"Shading rate calculated from last frame, no motion vectors", fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;

    switch (m_visualise)
    {
    default:
        assert(0);
    case Terrain::Visualise::NoVRS:                 swprintf_s(strText, L"<DPAD L/R> Display: Ground Truth");                          break;
    case Terrain::Visualise::VRS:                   swprintf_s(strText, L"<DPAD L/R> Display: VRS Signal");                            break;
    case Terrain::Visualise::ShadingRate:           swprintf_s(strText, L"<DPAD L/R> Display: Shading Rate");                          break;
    case Terrain::Visualise::ShadingRateOverlay:    swprintf_s(strText, L"<DPAD L/R> Display: VRS Signal with Shading Rate Overlay");  break;
    case Terrain::Visualise::Diff:                  swprintf_s(strText, L"<DPAD L/R> Display: Absolute Difference");                   break;
    }
    float &sobelTolerance = (UINT(m_shadingRateCalc) & 1) ? m_solelToleranceHalfRes : m_sobelTolerance;

    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"<Triggers> Sobel Tolerance: %1.3f", sobelTolerance);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, m_renderTechnique == Terrain::RenderTechnqiue::Normal ? L"<X> Normal" : L"<X> AO Only");
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;

    switch (m_shadingRateCalc)
    {
    default:
        assert(0);
    case Terrain::ShadingRateCalculation::Wave64_FullRes:        swprintf_s(strText, L"<B> Wave64 Full Res Shading rate calculation");       break;
    case Terrain::ShadingRateCalculation::Wave64_HalfRes:        swprintf_s(strText, L"<B> Wave64 Half Res Shading rate calculation");       break;
    case Terrain::ShadingRateCalculation::Wave32_FullRes:        swprintf_s(strText, L"<B> Wave32 Full Res Shading rate calculation");       break;
    case Terrain::ShadingRateCalculation::Wave32_HalfRes:        swprintf_s(strText, L"<B> Wave32 Half Res Shading rate calculation");       break;        
    }
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"<Y><Shoulders> Control Sun");
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    fontPos.y += 40; fontPosTab.y += 40;

    m_textFont->DrawString(m_fontBatch.get(), "NoVRS / VRS", fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"Terrain Render: %3.2fms/%3.2fms", m_avgTimingResults[Terrain::GPUPasses::NoVRSTerrain], m_avgTimingResults[Terrain::GPUPasses::VRSTerrain]);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"Sky Render: %3.2fms/%3.2fms", m_avgTimingResults[Terrain::GPUPasses::NoVRSSky], m_avgTimingResults[Terrain::GPUPasses::VRSSky]);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    fontPos.y += 40; fontPosTab.y += 40;

    m_textFont->DrawString(m_fontBatch.get(), "VRS Overheads", fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"Calculate Shading Rate: %3.3fms", m_avgTimingResults[Terrain::GPUPasses::VRSCalcShadingRate]);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"RSSetShadingRateImage: %3.3fms", m_avgTimingResults[Terrain::GPUPasses::VRSSetShadingRate]);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"Total: %3.2fms/%3.2fms", totalTimeNoVRS, totalTimeVRS);
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;

    if ((m_visualise == Terrain::Visualise::ShadingRate) || (m_visualise == Terrain::Visualise::ShadingRateOverlay))
    {
        fontPos.y += 240; fontPosTab.y += 240;
        m_textFont->DrawString(m_fontBatch.get(), L"Shading Rate Key:", fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
        m_textFont->DrawString(m_fontBatch.get(), L"Red: 1x1, full rate", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
        m_textFont->DrawString(m_fontBatch.get(), L"Cyan: 2x1, half rate, more detail vertically", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
        m_textFont->DrawString(m_fontBatch.get(), L"Yellow: 1x2, half rate, more detail horizontally", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
        m_textFont->DrawString(m_fontBatch.get(), L"No tint: 2x2, quarter rate", fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    }
    fontPos.y = 2160 - (5 * 50); fontPosTab.y = fontPos.y;
    m_textFont->DrawString(m_fontBatch.get(), "Saving from VRS", fontPos, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"Terrain Render: %3.2fms (%3.1f%% reduction)", m_avgTimingResults[Terrain::GPUPasses::NoVRSTerrain] - m_avgTimingResults[Terrain::GPUPasses::VRSTerrain], 100.0f * (1.0f - m_avgTimingResults[Terrain::GPUPasses::VRSTerrain] / m_avgTimingResults[Terrain::GPUPasses::NoVRSTerrain]));
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"Sky Render: %3.2fms (%3.1f%% reduction)", m_avgTimingResults[Terrain::GPUPasses::NoVRSSky] - m_avgTimingResults[Terrain::GPUPasses::VRSSky], 100.0f * (1.0f - m_avgTimingResults[Terrain::GPUPasses::VRSSky] / m_avgTimingResults[Terrain::GPUPasses::NoVRSSky]));
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    swprintf_s(strText, L"Total saving %3.2fms - overheads %3.2fms = %3.2fms (%3.1f%% reduction)", totalTimeNoVRS - totalTimeVRSNoOverheads, overheads, totalTimeNoVRS - totalTimeVRS, 100.0f * (1.0f - totalTimeVRS / totalTimeNoVRS));
    m_textFont->DrawString(m_fontBatch.get(), strText, fontPosTab, colour, 0.0f, g_XMZero, fontScale); fontPos.y += 40; fontPosTab.y += 40;
    
    m_fontBatch->End();

    // Show the new frame.
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    m_deviceResources->Present();
    m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());
    PIXEndEvent();
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

    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    // Create descriptor heap for resources
    m_uiDescriptorHeap = std::make_unique<DescriptorHeap>(device, static_cast<UINT>(UIDescriptors::Count));

    // Init fonts
    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();
    {
        SpriteBatchPipelineStateDescription pd(rtState, &CommonStates::AlphaBlend);
        m_fontBatch = std::make_unique<SpriteBatch>(device, resourceUpload, pd);

        auto cpuDescHandleText = m_uiDescriptorHeap->GetCpuHandle(static_cast<int>(UIDescriptors::TextFont));
        auto gpuDescHandleText = m_uiDescriptorHeap->GetGpuHandle(static_cast<int>(UIDescriptors::TextFont));
        m_textFont = std::make_unique<SpriteFont>(device, resourceUpload, L"Courier_36.spritefont", cpuDescHandleText, gpuDescHandleText);
    }
    auto uploadResourcesFinished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    uploadResourcesFinished.wait();
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Initialize the projection matrix.
    auto const size = m_deviceResources->GetOutputSize();

    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, float(size.right) / float(size.bottom), 0.1f, 1000.0f);
}

void Sample::OnDeviceLost()
{
    // DX12Timer and Terrain class needs a ReleaseDevice() method.
    m_textFont.reset();
    m_fontBatch.reset();
    m_uiDescriptorHeap.reset();
    m_graphicsMemory.reset();
}

void Sample::OnDeviceRestored()
{
    CreateDeviceDependentResources();

    CreateWindowSizeDependentResources();
}
#pragma endregion
