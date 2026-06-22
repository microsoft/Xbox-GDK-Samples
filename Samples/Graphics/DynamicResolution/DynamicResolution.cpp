//--------------------------------------------------------------------------------------
// DynamicResolution.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "DynamicResolution.h"

#include "ATGColors.h"
#include "ControllerFont.h"
#include "Menu.h"
#include "ResolutionSet.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace DirectX::SimpleMath;
using Microsoft::WRL::ComPtr;

#if _DEBUG
#pragma message("Note: Additional CPU overhead on the Debug build configuration may make dynamic resolution less effective, especially on Xbox One console modes. Run on Profile or Release for best results.")
#endif

#pragma warning(disable : 4061)

namespace
{
    // Fixed offsets into descriptor heaps
    enum SRVDescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_Count
    };

    // Barebones definition of scene objects
    struct ObjectDefinition
    {
        size_t              modelIndex;
        Matrix              world;
    };

    // Will store frequency of QueryPerformanceCounter
    static double g_perfFreq = 0;

    // Camera constants
    constexpr float c_defaultCameraAngle = -0.93f;
    constexpr float c_defaultCameraElevation = 2.97f;
    constexpr float c_defaultCameraDistance = 3.3f;

    // Assest paths
    const wchar_t* c_modelPaths[] =
    {
        L"scanner.sdkmesh",
        L"occcity.sdkmesh",
        L"column.sdkmesh",
    };

    // Barebones definition of a scene
    const ObjectDefinition c_sceneDefinition[] =
    {
        { 0, XMMatrixIdentity() },
        { 0, XMMatrixRotationY(XM_2PI * (1.0f / 6.0f)) },
        { 0, XMMatrixRotationY(XM_2PI * (2.0f / 6.0f)) },
        { 0, XMMatrixRotationY(XM_2PI * (3.0f / 6.0f)) },
        { 0, XMMatrixRotationY(XM_2PI * (4.0f / 6.0f)) },
        { 0, XMMatrixRotationY(XM_2PI * (5.0f / 6.0f)) },
        { 1, XMMatrixIdentity() },
        { 2, XMMatrixIdentity() },
    };

    // Determine how many resolutions are possible in a particular dimension.
    uint32_t CalculateResolutionSteps(uint32_t minRez, uint32_t maxRez, uint32_t increment)
    {
        assert(minRez <= maxRez);

        if (increment == 0 || minRez == maxRez)
        {
            return 1;
        }
        else
        {
            uint32_t steps = 1 + static_cast<uint32_t>(ceil((maxRez - minRez) / static_cast<float>(increment)));

            if (maxRez - steps * increment != minRez)
            {
                // This means the steps don't get us evenly to the lowest resolution, so there will be clamping.
                // When increasing the resolution afterward, different steps will be used, except for the start and end values.
                // E.g. with minRez = 5, maxRez = 10, increment = 2, decreasing will use steps at 10, 8, 6, 5.
                // Increasing will use 5, 7, 9, 10. So we need to double the number of intermediate steps only.
                steps = steps * 2 - 2;
            }

            return steps;
        }
    }
}

Sample::Sample() noexcept(false)
    : m_deviceType(XSystemDeviceType::Unknown)
    , m_frame(0)
    , m_cameraAngle(c_defaultCameraAngle)
    , m_cameraElevation(c_defaultCameraElevation)
    , m_cameraDistance(c_defaultCameraDistance)
    , m_useDynamicResolution(true)
    , m_moveCamera(true)
    , m_showPanel(true)
    , m_frameWidth(1920)
    , m_frameHeight(1080)
    , m_frameViewportDynamic{}
    , m_frameViewportIdeal{}
    , m_desiredGpuFrameTimeInMs(0.0f)
    , m_desiredCpuFrameTimeInMs(0.0f)
    , m_cpuEnforceTimeThread(nullptr)
    , m_isExiting(false)
    , m_startCpuFrameSignal(true)
    , m_endCpuFrameSignal(false)
    , m_frameRate(60.0f)
    , m_tearLocation(0.0f)
{
    LARGE_INTEGER freq;
    std::ignore = QueryPerformanceFrequency(&freq);
    g_perfFreq = (double)freq.QuadPart;

    m_deviceResources = std::make_unique<DX::DeviceResources>(
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_D32_FLOAT,
        c_numBackBuffers,
        DX::DeviceResources::c_ReverseDepth);
    m_deviceResources->SetClearColor(ATG::ColorsLinear::Background);
}

Sample::~Sample()
{
    m_isExiting = true;

    if (m_cpuEnforceTimeThread)
    {
        m_cpuEnforceTimeThread->join();
        m_cpuEnforceTimeThread = nullptr;
    }

    if (m_deviceResources)
    {
        m_deviceResources->WaitForGpu();
    }
}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_deviceResources->SetWindow(window);

    m_deviceResources->CreateDeviceResources();
    CreateDeviceDependentResources();

    m_deviceResources->CreateWindowSizeDependentResources();
    CreateWindowSizeDependentResources();

    InitializeUI();
}

bool Sample::CheckDeviceType()
{
    m_deviceType = XSystemGetDeviceType();
    if (m_deviceType == XSystemDeviceType::XboxOneX || m_deviceType == XSystemDeviceType::XboxOneXDevkit)
    {
        OutputDebugStringA("** UNSUPPORTED: This sample renders to multiple display planes, which isn't supported on Xbox One X. "
            "Please change the devkit's console mode, or run on a different device.\n");
        return false;
    }
    return true;
}

void Sample::InitializeUI()
{
    m_uiManager.GetRootElement()->AddChildFromLayout("Assets/ui_layout.json");

    // Cache these for faster access
    m_sidePanel = m_uiManager.FindTypedById<UIPanel>(ID("info_panel"));
    m_dynamicRezText = m_uiManager.FindTypedById<UIStaticText>(ID("dynamic_rez_value"));
    m_widthText = m_uiManager.FindTypedById<UIStaticText>(ID("width_value"));
    m_heightText = m_uiManager.FindTypedById<UIStaticText>(ID("height_value"));
    m_presetText = m_uiManager.FindTypedById<UIStaticText>(ID("preset_label"));
    m_fpsText = m_uiManager.FindTypedById<UIStaticText>(ID("fps_num"));
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

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& timer)
{
    using ButtonState = DirectX::GamePad::ButtonStateTracker::ButtonState;

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Update");

    float elapsedTime = float(timer.GetElapsedSeconds());

    m_uiInputState.Update(elapsedTime, *m_gamePad);
    m_uiManager.Update(elapsedTime, m_uiInputState);

    const auto& pad = m_uiInputState.GetGamePadState(0);
    const auto& buttons = m_uiInputState.GetGamePadButtons(0);
    if (pad.IsConnected())
    {
        if (buttons.back == ButtonState::PRESSED)
        {
            ExitSample();
            m_isExiting = true;
        }

        if (buttons.y == ButtonState::PRESSED)
        {
            m_useDynamicResolution = !m_useDynamicResolution;
        }

        if (buttons.x == ButtonState::PRESSED)
        {
            m_moveCamera = !m_moveCamera;
        }

        if (buttons.a == ButtonState::PRESSED)
        {
            m_showPanel = !m_showPanel;
            m_sidePanel->SetVisible(m_showPanel);
            m_menu.m_processInput = m_showPanel;
        }

        m_menu.Update(buttons);
    }

    if (m_moveCamera)
    {
        // Constant rotation
        m_cameraAngle += 1.0f * elapsedTime;
    }

    XMVECTOR lookFrom = XMVectorSet(
        sinf(m_cameraAngle) * m_cameraDistance,
        m_cameraElevation,
        cosf(m_cameraAngle) * m_cameraDistance,
        0);

    m_view = XMMatrixLookAtLH(lookFrom, g_XMZero, g_XMIdentityR1);

    // Update the scene.
    for (auto& obj : m_scene)
    {
        Model::UpdateEffectMatrices(obj.effects, obj.world, m_view, m_proj);
    }

    // Measure current time
    LARGE_INTEGER currentTime;
    std::ignore = QueryPerformanceCounter(&currentTime);
    float currentTimeInMs = 1000.0f * (float)(currentTime.QuadPart / (DOUBLE)g_perfFreq);

    // Set artificial processor loads
    auto cpuLoad = m_menu.GetItemValue< float >(UI_PARAM_CPU_LOAD);
    auto gpuLoad = m_menu.GetItemValue< float >(UI_PARAM_GPU_LOAD);
    auto spikeProcessor = static_cast<SpikeProcessor>(m_menu.GetItemValue< uint32_t >(UI_PARAM_SPIKE_PROCESSOR));
    auto spikeAmplitudeInMs = m_menu.GetItemValue< float >(UI_PARAM_SPIKE_AMPLITUDE);
    auto spikePeriodInSec = m_menu.GetItemValue< float >(UI_PARAM_SPIKE_PERIOD);
    auto spikeThicknessInSec = m_menu.GetItemValue< float >(UI_PARAM_SPIKE_DURATION);

    auto mean = (SpikeProcessor::Cpu == spikeProcessor) ? cpuLoad : gpuLoad;
    Spike< SpikeShape::Square > spike =
    {
        mean,                          // float m_mean;
        spikeAmplitudeInMs,            // float m_amplitude;
        spikeThicknessInSec * 1000.f,  // float m_thickness;
        spikePeriodInSec * 1000.f,     // float m_period;
    };

    // Set artificial CPU frame load
    m_desiredCpuFrameTimeInMs = (SpikeProcessor::Cpu == spikeProcessor) ? spike(currentTimeInMs) : cpuLoad;

    // Set artificial GPU frame load
    float desiredGpuFrameTimeAtFullResInMs = (SpikeProcessor::Gpu == spikeProcessor) ? spike(currentTimeInMs) : gpuLoad;
    m_desiredGpuFrameTimeInMs = desiredGpuFrameTimeAtFullResInMs * EstimateGpuPercentTimeRelativeToIdeal(m_frameWidth, m_frameHeight);

    PIXEndEvent();
}


// Forces the CPU to wait until a predetermined time has elapsed.
void Sample::EnforceCpuTime()
{
    SetThreadDescription(GetCurrentThread(), L"EnforceCpuTime");

    while (!m_isExiting)
    {
        if (m_startCpuFrameSignal)
        {
            PIXBeginEvent(0, L"CPU artificial workload");

            assert(!m_endCpuFrameSignal);

            m_startCpuFrameSignal = false;
            LARGE_INTEGER lStartTime;
            std::ignore = QueryPerformanceCounter(&lStartTime);

            LARGE_INTEGER endTime;
            endTime.QuadPart = lStartTime.QuadPart + (LONGLONG)(m_desiredCpuFrameTimeInMs / 1000.0 * g_perfFreq);
            LARGE_INTEGER currentTime;
            do
            {
                std::ignore = QueryPerformanceCounter(&currentTime);
            } while (currentTime.QuadPart < endTime.QuadPart && !m_isExiting);

            m_endCpuFrameSignal = true;

            PIXEndEvent();
        }
    }

    m_endCpuFrameSignal = true;
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    // Don't try to render anything before the first Update or during exit.
    if (m_timer.GetFrameCount() == 0 || m_isExiting)
    {
        return;
    }

    // Prepare the command list to render a new frame.
    m_deviceResources->Prepare();
    auto commandList = m_deviceResources->GetCommandList();

    m_profiler->BeginFrame(commandList);
    m_profiler->Start(commandList);

    DetermineResolution();

    RecordFrameStatistics();

    // The first frame rendered doesn't generate stats, so don't bother trying to get them
    if (m_timer.GetFrameCount() > 1)
    {
        m_frameTokens.push_back(m_deviceResources->GetFrameToken());
    }


    // Set descriptor heaps
    ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap(), m_commonStates->Heap() };
    commandList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);

    {
        ScopedPixEvent Workload(commandList, 0, L"Get GPU start time");

        commandList->SetComputeRootSignature(m_getStartTimeRS.Get());
        commandList->SetComputeRootUnorderedAccessView(0, m_gpuTimeResource->GetGPUVirtualAddress());
        commandList->SetPipelineState(m_getStartTimePSO.Get());

        auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_gpuTimeResource.Get());
        commandList->ResourceBarrier(1, &barrier);
        commandList->Dispatch(1, 1, 1);
        commandList->ResourceBarrier(1, &barrier);
    }

    {
        ScopedPixEvent Render(commandList, PIX_COLOR_DEFAULT, L"Render");

        // Switch to the render and depth targets for the current resolution
        SetAndClearTargets(&m_frameViewportDynamic, &m_currentRezData.rtvDescriptor, &m_currentRezData.dsvDescriptor, ATG::ColorsLinear::Background);

        {
            ScopedPixEvent Scene(commandList, PIX_COLOR_DEFAULT, L"Scene");

            // Draw the scene.
            for (auto& obj : m_scene)
            {
                auto it = obj.effects.cbegin();
                obj.model->DrawOpaque(commandList, it);
            }
        }

        {
            ScopedPixEvent Copy(commandList, PIX_COLOR_DEFAULT, L"Copy to swap chain");

            // Copy from the dynamically-sized render target to the upper-left corner of the swap chain.
            // In a real game, the last full-screen render pass would likely write directly to the swap chain,
            // but this sample is simple enough that it can render everything in one pass.
            D3D12_RESOURCE_BARRIER barriers[2] =
            {
                CD3DX12_RESOURCE_BARRIER::Transition(m_deviceResources->GetRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST),
                CD3DX12_RESOURCE_BARRIER::Transition(m_currentRezData.renderTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE)
            };
            commandList->ResourceBarrier(_countof(barriers), barriers);

            CD3DX12_TEXTURE_COPY_LOCATION dest(m_deviceResources->GetRenderTarget(), 0);
            CD3DX12_TEXTURE_COPY_LOCATION src(m_currentRezData.renderTarget.Get(), 0);

            commandList->CopyTextureRegion(&dest, 0, 0, 0, &src, nullptr);

            barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
            barriers[0].Transition.StateAfter = barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
            commandList->ResourceBarrier(_countof(barriers), barriers);
        }

        RenderUI();
    }

    {
        ScopedPixEvent Workload(commandList, 0, L"GPU artificial workload");

        uint64_t clockRate = 0;
        switch (m_deviceType)
        {
        case XSystemDeviceType::XboxOne:
            clockRate = 853;
            break;
        case XSystemDeviceType::XboxOneS:
            clockRate = 914;
            break;
        case XSystemDeviceType::XboxOneX:
        case XSystemDeviceType::XboxOneXDevkit:
            clockRate = 1172;
            break;
        case XSystemDeviceType::XboxScarlettLockhart:
            clockRate = 1565;
            break;
        case XSystemDeviceType::XboxScarlettAnaconda:
        case XSystemDeviceType::XboxScarlettDevkit:
            clockRate = 1825;
            break;
        case XSystemDeviceType::Pc:
        case XSystemDeviceType::Unknown:
        default:
            assert(false);
        }

        auto desiredFrameTimeInTicks = static_cast<uint64_t>(m_desiredGpuFrameTimeInMs * 1000.0f) * clockRate;

        commandList->SetComputeRootSignature(m_gpuEnforceTimeRS.Get());
        commandList->SetComputeRoot32BitConstants(0, 2, &desiredFrameTimeInTicks, 0);
        commandList->SetComputeRootUnorderedAccessView(1, m_gpuTimeResource->GetGPUVirtualAddress());
        commandList->SetPipelineState(m_gpuEnforceTimePSO.Get());

        auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(m_gpuTimeResource.Get());
        commandList->ResourceBarrier(1, &barrier);
        commandList->Dispatch(1, 1, 1);
        commandList->ResourceBarrier(1, &barrier);
    }

    m_profiler->Stop(commandList);
    m_profiler->EndFrame(commandList);

    // Kickoff the GPU work, to avoid forcing a full frame of latency
    // A real title would naturally have multiple kickoffs, so it wouldn't need an artificial one
    commandList->KickoffX();

    uint32_t presentationThreshold = static_cast<uint32_t>(m_menu.GetItemValue< INT >(UI_PARAM_PRESENTATION_THRESHOLD));
    m_deviceResources->SetPresentationThreshold(static_cast<float>(presentationThreshold));

    // Store frame data for use by dynamic resolution algorithm
    {
        m_frameDataMap.emplace(std::make_pair(m_deviceResources->GetFrameToken(),
            FrameData{
                presentationThreshold,
                m_frameWidth,
                m_frameHeight
            }));
    }

    // Show the new frame.
    {
        while (!m_endCpuFrameSignal)
        {
            _mm_pause();
        }
        m_endCpuFrameSignal = false;

        PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");

        m_deviceResources->Present(m_frameWidth, m_frameHeight);

        m_graphicsMemory->Commit(m_deviceResources->GetCommandQueue());

        m_startCpuFrameSignal = true;

        PIXEndEvent();
    }
}

void Sample::SetAndClearTargets(
    const D3D12_VIEWPORT* viewport,
    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvDescriptor,
    const D3D12_CPU_DESCRIPTOR_HANDLE* dsvDescriptor,
    const float* colorRGBA)
{
    auto commandList = m_deviceResources->GetCommandList();

    ScopedPixEvent Clear(commandList, PIX_COLOR_DEFAULT, L"SetAndClearTargets");

    // Set the viewport and scissor rect.
    commandList->RSSetViewports(1, viewport);
    const D3D12_RECT scissorRect =
    {
        0,                          // LONG    left;
        0,                          // LONG    top;
        (LONG)viewport->Width,      // LONG    right;
        (LONG)viewport->Height,     // LONG    bottom;
    };
    commandList->RSSetScissorRects(1, &scissorRect);

    // Set and clear the views.
    commandList->OMSetRenderTargets(1, rtvDescriptor, false, dsvDescriptor);
    commandList->ClearRenderTargetView(*rtvDescriptor, colorRGBA, 0, nullptr);
    if (dsvDescriptor)
    {
        commandList->ClearDepthStencilView(*dsvDescriptor, D3D12_CLEAR_FLAG_DEPTH, 0.0f, 0, 0, nullptr);
    }
}

void Sample::RenderUI()
{
    auto commandList = m_deviceResources->GetCommandList();

    ScopedPixEvent RenderUI(commandList, PIX_COLOR_DEFAULT, L"Render UI");

    auto const rtvDescriptor = m_deviceResources->GetUIRenderTargetView();

    // Clear UI plane alpha to 0.0f, so it doesn't overwrite background plane
    SetAndClearTargets(&m_frameViewportIdeal, &rtvDescriptor, nullptr, DirectX::Colors::Transparent);

    // Render predefined UI (background panels, etc.)
    {
        const std::string c_on = "ON";
        const std::string c_off = "OFF";
        m_dynamicRezText->SetDisplayText(m_useDynamicResolution ? c_on : c_off);
        m_widthText->SetDisplayText(std::to_string(m_frameWidth));
        m_heightText->SetDisplayText(std::to_string(m_frameHeight));

        m_presetText->SetDisplayText(m_menu.GetPresetText());

        // Framerate indicator (lags by a few frames)
        // Average the last few frames to see changes more easily
        constexpr uint32_t numGhostFrames = 5;
        static Ring< float, numGhostFrames > s_prevFrameRate;
        s_prevFrameRate.push_back(m_frameRate);

        float frameRate = 0.0f;
        for (auto it = s_prevFrameRate.cbegin(); it != s_prevFrameRate.cend(); ++it)
        {
            frameRate += *it;
        }
        frameRate /= s_prevFrameRate.size();

        int roundedFrameRate = static_cast<int>(frameRate + 0.5f);
        m_fpsText->SetDisplayText(std::to_string(roundedFrameRate));

        m_uiManager.Render();
    }


    // Render tear indicator (lags by a few frames)
    {
        m_triEffect->Apply(commandList);
        m_primBatch->Begin(commandList);

        float indicatorWidth = 2.0f * 190.0f / c_frameWidthMaximum;
        float indicatorHeight = 2.0f * 2.0f / c_frameHeightMaximum;
        float tearIndicatorY = 2.0f * (1.0f - m_tearLocation) - 1.0f;
        Vector4 tearIndicatorColor(0.8588235294f, 0.2862745098f, 0.1098039215f, 1.0f);

        Vector3 upperLeft(1.0f - indicatorWidth, tearIndicatorY + indicatorHeight / 2.0f, 0.0f);
        Vector3 lowerLeft(1.0f - indicatorWidth, tearIndicatorY - indicatorHeight / 2.0f, 0.0f);
        Vector3 lowerRight(1.0f, tearIndicatorY - indicatorHeight / 2.0f, 0.0f);
        Vector3 upperRight(1.0f, tearIndicatorY + indicatorHeight / 2.0f, 0.0f);

        VertexPositionColor qvul(upperLeft, tearIndicatorColor);
        VertexPositionColor qvll(lowerLeft, tearIndicatorColor);
        VertexPositionColor qvlr(lowerRight, tearIndicatorColor);
        VertexPositionColor qvur(upperRight, tearIndicatorColor);

        m_primBatch->DrawQuad(qvul, qvll, qvlr, qvur);

        // Background for text
        float backgroundHeight = 2.0f * 39.0f / c_frameHeightMaximum;
        Vector4 backgroundColor(0.0980392156f, 0.0980392156f, 0.0980392156f, 0.95f);

        qvul.position.x = (qvll.position.x += 2.0f / c_frameWidthMaximum);
        qvul.position.y = qvur.position.y = lowerLeft.y;
        qvll.position.y = qvlr.position.y = qvul.position.y - backgroundHeight;
        qvul.color = qvur.color = qvll.color = qvlr.color = backgroundColor;

        m_primBatch->DrawQuad(qvul, qvll, qvlr, qvur);

        m_primBatch->End();

        // Text label
        m_uiSpriteBatch->Begin(commandList);    // This remains active for the rest of the text UI

        int indicatorWidthPixels = static_cast<int>(m_frameViewportIdeal.Width * indicatorWidth / 2.0f);
        XMFLOAT2 textPos = XMFLOAT2(m_frameViewportIdeal.Width - indicatorWidthPixels + 10.0f, m_tearLocation * m_frameViewportIdeal.Height);
        m_smallFont->DrawString(m_uiSpriteBatch.get(), L"Tear Indicator", textPos, tearIndicatorColor, 0.0f, { 0.0f, 0.0f }, 1.0f);
    }

    if (m_showPanel)
    {
        m_menu.Render(m_uiSpriteBatch.get(), m_smallFont.get(), m_ctrlFont.get(), { 92.f, 660.f });
    }

    m_uiSpriteBatch->End();

    // Draw timelines
    if (m_showPanel)
    {
        D3D12_VIEWPORT timelineViewport =
        {
            50.0f,                          // float TopLeftX;
            170.0f,                         // float TopLeftY;
            485.0f,                         // float Width;
            120.0f,                         // float Height;
            0.0f,                           // float MinDepth;
            1.0f,                           // float MaxDepth;
        };
        for (uint32_t i = 0; i < _countof(m_timeline); ++i)
        {
            m_timeline[i].Render(commandList, m_primBatch.get(), m_lineEffect.get(), m_uiSpriteBatch.get(), m_smallFont.get(), &m_frameViewportIdeal, &timelineViewport);

            timelineViewport.TopLeftY += 144.0f;
        }
    }
}

// Queries the frame statistics needed for the dynamic resolution algorithm
bool Sample::GetFrameStatistics(D3D12XBOX_FRAME_PIPELINE_TOKEN frameToken, FrameStatistics& stats)
{
    constexpr uint32_t expectedStatCount = 6;
    D3D12XBOX_FRAME_STATISTICS rawStats[expectedStatCount];

    uint32_t statCount = expectedStatCount;
    HRESULT hr = m_deviceResources->GetD3DDevice()->GetFrameStatisticsX(
        frameToken,
        D3D12XBOX_FRAME_STATISTICS_TYPE_RENDER | D3D12XBOX_FRAME_STATISTICS_TYPE_PRESENT |
        D3D12XBOX_FRAME_STATISTICS_TYPE_DISPLAY,
        &statCount,
        rawStats);

    if (hr == S_FALSE || statCount < expectedStatCount)
    {
        // Results not available yet
        return false;
    }

    DX::ThrowIfFailed(hr);

    assert(statCount == expectedStatCount);

    // Grab the stats for display plane 0 only. Plane 1 is the UI and doesn't use dynamic resolution.
    for (const auto& rawStat : rawStats)
    {
        switch (rawStat.Type)
        {
        case D3D12XBOX_FRAME_STATISTICS_TYPE_RENDER:
            if (rawStat.Render.PlaneIndex == 0)
                stats.render = rawStat.Render;
            break;
        case D3D12XBOX_FRAME_STATISTICS_TYPE_PRESENT:
            if (rawStat.Present.PlaneIndex == 0)
                stats.present = rawStat.Present;
            break;
        case D3D12XBOX_FRAME_STATISTICS_TYPE_DISPLAY:
            if (rawStat.Display.PlaneIndex == 0)
                stats.display = rawStat.Display;
            break;
        case D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_EVENT:
        case D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_INTERVAL:
        case D3D12XBOX_FRAME_STATISTICS_TYPE_INPUT:
        case D3D12XBOX_FRAME_STATISTICS_TYPE_NONE:
 #if defined(_GAMING_XBOX_SCARLETT) && (_GXDK_VER >= 0x633610AF /* GDK Edition 240600 */)
        case D3D12XBOX_FRAME_STATISTICS_TYPE_POWERSCALING:
#endif
        default:
            assert(false);
            break;
        }
    }

    // Additional frame data tracked separately
    auto dataIter = m_frameDataMap.find(frameToken);
    assert(dataIter != m_frameDataMap.end());

    stats.data = dataIter->second;

    return true;
}


// Keeps a record of the frame statistics for graphing purposes.
// This routine is NOT part of the dynamic resolution algorithm, just part of the tracking.
void Sample::RecordFrameStatistics()
{
    uint64_t timePreviousFrameFlip = 0;

    // Check all unprocessed frames, stop if we find an unfinished one
    while (m_frameTokens.size() > 0)
    {
        FrameStatistics stats{};

        if (!GetFrameStatistics(m_frameTokens.front(), stats))
        {
            // Results not available yet
            break;
        }

        if (!m_frameStatistics.empty())
        {
            timePreviousFrameFlip = m_frameStatistics.back().display.FlipTime;
        }

        m_frameStatistics.push_back(stats);

        for (uint32_t timelineIndex = 0; timelineIndex < _countof(m_timeline); ++timelineIndex)
        {
            m_timeline[timelineIndex].Update(&m_frameStatistics.back());
        }

        // This is safe since DetermineResolution runs first
        m_frameDataMap.erase(m_frameTokens.front());
        m_frameTokens.pop_front();
    }

    if (timePreviousFrameFlip > 0)    // At least two frames have been completed
    {
        FrameStatistics latestFrameStats = m_frameStatistics.back();
        m_tearLocation = latestFrameStats.display.PercentScanned / 100.0f;

        uint64_t timeFrameFlip = latestFrameStats.display.FlipTime;
        m_frameRate = 1.0f / (float)((timeFrameFlip - timePreviousFrameFlip) / g_perfFreq);
    }

}


// Based on recent performance, decide what resolution to render at this frame.
//
// This routine is the core dynamic resolution algorithm.
//
// It makes three determinations:
//
// 1) Is the title likely to drop frames soon at the current resolution?
// 2) If so, is the reason CPU load or GPU load?
// 3) If not, could the title sustain a higher resolution?
void Sample::DetermineResolution()
{
    if (m_useDynamicResolution)
    {
        auto marginDropResolutionInMs = m_menu.GetItemValue< float >(UI_PARAM_DROP_RESOLUTION_MARGIN);
        auto marginRaiseResolutionInMs = m_menu.GetItemValue< float >(UI_PARAM_RAISE_RESOLUTION_MARGIN);
        auto raiseResolutionWaitInFrames = 1000.0f * m_menu.GetItemValue< float >(UI_PARAM_RESOLUTION_HYSTERESIS);

        // Set frame dimensions to maintain frame rate
        // This is part of the dynamic resolution implementation

        // The last token in the queue is the most recent. Reverse iterate to find the most recent frame with statistics available.
        for (auto iter = m_frameTokens.crbegin(); iter != m_frameTokens.crend(); iter++)
        {
            // We also retrieve these in RecordFrameStatistics. But that copy is just for tracking purposes,
            // and isn't part of the algorithm. The copy we retrieve here is for determining resolution, and it
            // is part of the algorithm.
            FrameStatistics frameStats{};
            if(GetFrameStatistics(*iter, frameStats))
            {
                static uint64_t s_prevFrameVsync = 0;
                static uint64_t s_prevFrameComplete = 0;

                if (s_prevFrameVsync > 0) // We've seen at least two frames
                {
                    // Compute GPU-to-Flip latency (from when GPU reaches Present to when the frame becomes the active front buffer)
                    static float s_prevLatencyFromGpuFinishedToVsyncInMs = 33.33f; // Start at a good value
                    uint64_t timeFrameComplete = frameStats.present.GPUProcessTime;
                    constexpr uint32_t c_syncInterval = 1;
                    uint64_t scheduledTimeVSync = s_prevFrameVsync + (uint64_t)((c_syncInterval / c_vsyncsPerSecond) * g_perfFreq);
                    float latencyFromGpuFinishedToVsyncInMs = 1000.0f * (float)((scheduledTimeVSync - timeFrameComplete) / g_perfFreq);

                    // Predict GPU-to-Flip latency for the frame which is now starting (if this latency is < 0, we expect to drop this frame)
                    float changeInLatencyFromGpuFinishedToVsyncInMs = latencyFromGpuFinishedToVsyncInMs - s_prevLatencyFromGpuFinishedToVsyncInMs;
                    float predictedLatencyFromGpuFinishedToVsyncInMs = latencyFromGpuFinishedToVsyncInMs + c_maxFramesCpuToFlipLatency * changeInLatencyFromGpuFinishedToVsyncInMs;
                    s_prevLatencyFromGpuFinishedToVsyncInMs = latencyFromGpuFinishedToVsyncInMs;

                    // Compute GPU frame time (to decide whether to raise resolution)
                    uint64_t gpuFrameTime = frameStats.render.GPUBusyDuration;
                    float gpuFrameTimeInMs = 1000.0f * (float)(gpuFrameTime / g_perfFreq);
                    uint32_t nextHigherWidth = std::min(c_frameWidthMaximum, m_frameWidth + c_frameWidthIncrement);
                    uint32_t nextHigherHeight = std::min(c_frameHeightMaximum, m_frameHeight + c_frameHeightIncrement);
                    float predictedGpuFrameTimeInMs = gpuFrameTimeInMs
                        * EstimateGpuPercentTimeRelativeToIdeal(nextHigherWidth, nextHigherHeight) / EstimateGpuPercentTimeRelativeToIdeal(m_frameWidth, m_frameHeight);
                    float idealGpuFrameTimeInMs = 1000.0f * (float)(c_syncInterval / c_vsyncsPerSecond);

                    // Compute CPU-to-GPU latency (to determine whether we are CPU bound)
                    uint64_t timePresentCalled = frameStats.present.PresentCallTime;
                    float latencyCpuFinishedToGpuFinishedInMs = 1000.0f * (float)((timeFrameComplete - timePresentCalled) / g_perfFreq);
                    float latencyCpuFinishedToGpuFinishedInFrames = latencyCpuFinishedToGpuFinishedInMs / gpuFrameTimeInMs;

                    // What is the max amount by which CPU runs ahead if we are CPU bound?
                    // Overestimate is okay --- there are generally two cases:
                    //  1: CPU bound: CPU will run ahead by 0-N ms, where N is the biggest "bubble" of GPU time
                    //  2: GPU bound: CPU will run ahead by 1-2 frames (for swap chain count of 2)
                    constexpr float c_cpuToGpuLatencyThresholdInFrames = 0.5f;

                    // Make our decisions
                    bool considerDropResolution = (predictedLatencyFromGpuFinishedToVsyncInMs < marginDropResolutionInMs);
                    bool considerRaiseResolution = (predictedGpuFrameTimeInMs < idealGpuFrameTimeInMs - marginRaiseResolutionInMs);
                    bool isCpuBound = (latencyCpuFinishedToGpuFinishedInFrames <= c_cpuToGpuLatencyThresholdInFrames);

                    static float s_raiseResolutionWaitTimeInMs = 0.0f;

                    // We are GPU bound --- go to lower resolution
                    // We should wait until the camera moves (comment "&& m_moveCamera" out to see the resolution changes).
                    if (considerDropResolution && !isCpuBound && m_moveCamera)
                    {
                        // By how much should we reduce resolution to hit target GPU time?
                        float gpuPercentTimeTargetReduction = idealGpuFrameTimeInMs / gpuFrameTimeInMs;
                        uint32_t frameWidth = frameStats.data.width;
                        uint32_t frameHeight = frameStats.data.height;
                        ReduceGpuToPercentTime(gpuPercentTimeTargetReduction, frameWidth, frameHeight);
                        m_frameWidth = std::min(m_frameWidth, frameWidth);
                        m_frameHeight = std::min(m_frameHeight, frameHeight);

                        s_raiseResolutionWaitTimeInMs = 0.0f;
                    }
                    // We have GPU time to spare --- go to higher resolution
                    else if (considerRaiseResolution)
                    {
                        // Avoid oscillating back and forth between resolutions by using a enforced wait time
                        float gpuFrameIncrementInMs = 1000.0f * (float)((timeFrameComplete - s_prevFrameComplete) / g_perfFreq);
                        s_raiseResolutionWaitTimeInMs += gpuFrameIncrementInMs;

                        // We should wait until the camera moves (comment "&& m_moveCamera" out to see the resolution changes).
                        if (s_raiseResolutionWaitTimeInMs >= raiseResolutionWaitInFrames && m_moveCamera)
                        {
                            m_frameWidth = std::min(c_frameWidthMaximum, m_frameWidth + c_frameWidthIncrement);
                            m_frameHeight = std::min(c_frameHeightMaximum, m_frameHeight + c_frameHeightIncrement);

                            s_raiseResolutionWaitTimeInMs = 0.0f;
                        }
                    }
                    else
                    {
                        s_raiseResolutionWaitTimeInMs = 0.0f;
                    }
                }

                s_prevFrameVsync = frameStats.display.VSyncTime;
                s_prevFrameComplete = frameStats.present.GPUProcessTime;

                break;
            }
        }
    }
    else    // Not using dynamic resolution
    {
        m_frameWidth = c_frameWidthMaximum;
        m_frameHeight = c_frameHeightMaximum;
    }

    m_frameViewportDynamic.Width = (float)m_frameWidth;
    m_frameViewportDynamic.Height = (float)m_frameHeight;

    m_currentRezData = m_resolutionSet.GetOrCreateResourcesForResolution(m_frameWidth, m_frameHeight);

    m_deviceResources->SetSceneRenderSize(static_cast<long>(m_frameWidth), static_cast<long>(m_frameHeight));
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
    m_uiInputState.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    auto device = m_deviceResources->GetD3DDevice();

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
    m_profiler = std::make_unique<DX::GPUTimer>(device, m_deviceResources->GetCommandQueue());

    // State objects
    m_commonStates = std::make_unique<DirectX::CommonStates>(device);

    // Create heap
    m_srvPile = std::make_unique<DescriptorPile>(device,
        128,
        SRVDescriptorHeapIndex::SRV_Count);

    // Load models from disk.
    m_models.resize(_countof(c_modelPaths));
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        m_models[i] = Model::CreateFromSDKMESH(device, c_modelPaths[i]);
    }

    ResourceUploadBatch resourceUpload(device);
    resourceUpload.Begin();

    // Optimize meshes for rendering
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        m_models[i]->LoadStaticBuffers(device, resourceUpload);
    }

    // Upload textures to GPU.
    m_textureFactory = std::make_unique<EffectTextureFactory>(device, resourceUpload, m_srvPile->Heap());

    auto texOffsets = std::vector<size_t>(m_models.size());
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        size_t _;
        m_srvPile->AllocateRange(m_models[i]->textureNames.size(), texOffsets[i], _);

        m_models[i]->LoadTextures(*m_textureFactory, int(texOffsets[i]));
    }

    // Instantiate objects from basic scene definition.
    auto effectFactory = EffectFactory(m_srvPile->Heap(), m_commonStates->Heap());
    const RenderTargetState rtState(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());

    auto objectPSD = EffectPipelineStateDescription(
        nullptr,
        CommonStates::Opaque,
        CommonStates::DepthReverseZ,
        CommonStates::CullCounterClockwise,
        rtState,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

    m_scene.resize(_countof(c_sceneDefinition));
    for (size_t i = 0; i < m_scene.size(); i++)
    {
        size_t index = c_sceneDefinition[i].modelIndex;

        assert(index < m_models.size());
        auto& model = *m_models[index];

        m_scene[i].world = c_sceneDefinition[i].world;
        m_scene[i].model = &model;
        m_scene[i].effects = model.CreateEffects(effectFactory, objectPSD, objectPSD, int(texOffsets[index]));

        std::for_each(
            m_scene[i].effects.begin(),
            m_scene[i].effects.end(),
            [&](std::shared_ptr<IEffect>& e)
            {
                static_cast<BasicEffect*>(e.get())->SetEmissiveColor(XMVectorSet(1.0f, 1.0f, 1.0f, 1.0f));
            });
    }

    // UI
    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, 1920, 1080);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));

    const RenderTargetState backBufferRts(m_deviceResources->GetBackBufferFormat(), m_deviceResources->GetDepthBufferFormat());
    auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
    m_uiSpriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

    auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
    finished.wait();

    m_primBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

    EffectPipelineStateDescription effectPSD(
        &VertexPositionColor::InputLayout,
        CommonStates::Opaque,
        CommonStates::DepthReverseZ,
        CommonStates::CullNone,
        backBufferRts);
    m_triEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, effectPSD);

    effectPSD.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    m_lineEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, effectPSD);

    CreateWorkers();

    CreateTimelines();

    // Disable warning about barrier validation, without disabling barrier validation
#ifdef _GAMING_XBOX_SCARLETT
    device->SetDebugErrorFilterX(0x6BCA2E89, D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_BREAKS | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_OUTPUT);
#else
    device->SetDebugErrorFilterX(0xDA62126E, D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_BREAKS | D3D12XBOX_DEBUG_FILTER_FLAG_DISABLE_OUTPUT);
#endif
}

void Sample::CreateWorkers()
{
    auto device = m_deviceResources->GetD3DDevice();

    // Create worker compute shaders to enforce GPU frame time
    {
        const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        const D3D12_RESOURCE_DESC bufDesc = CD3DX12_RESOURCE_DESC::Buffer(2 * sizeof(uint64_t), D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_gpuTimeResource.ReleaseAndGetAddressOf())));
        m_gpuTimeResource->SetName(L"Enforce GPU Time Resource");

        // Shader to read frame start time
        auto computeShaderBlob = DX::ReadData(L"GetStartTime.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_getStartTimeRS.ReleaseAndGetAddressOf())));
        m_getStartTimeRS->SetName(L"Get GPU Time RS");

        D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
        descComputePSO.pRootSignature = m_getStartTimeRS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_getStartTimePSO.ReleaseAndGetAddressOf())));
        m_getStartTimePSO->SetName(L"Enforce GPU Time PSO");

        // Shader to wait until specified time
        computeShaderBlob = DX::ReadData(L"EnforceGpuTime.cso");

        DX::ThrowIfFailed(
            device->CreateRootSignature(0, computeShaderBlob.data(), computeShaderBlob.size(),
                IID_GRAPHICS_PPV_ARGS(m_gpuEnforceTimeRS.ReleaseAndGetAddressOf())));
        m_gpuEnforceTimeRS->SetName(L"Enforce GPU Time RS");

        // Create compute pipeline state
        descComputePSO = {};
        descComputePSO.pRootSignature = m_gpuEnforceTimeRS.Get();
        descComputePSO.CS.pShaderBytecode = computeShaderBlob.data();
        descComputePSO.CS.BytecodeLength = computeShaderBlob.size();

        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_gpuEnforceTimePSO.ReleaseAndGetAddressOf())));
        m_gpuEnforceTimePSO->SetName(L"Enforce GPU Time PSO");
    }

    // Create worker thread to enforce CPU frame time
    {
        m_startCpuFrameSignal = true; // Need to seed this once before the first Present
        m_endCpuFrameSignal = false;

        m_cpuEnforceTimeThread = new std::thread(&Sample::EnforceCpuTime, this);
        auto cpuThreadHandle = m_cpuEnforceTimeThread->native_handle();
        (void)SetThreadAffinityMask(cpuThreadHandle, 0x4);   // Run on its own core
        (void)SetThreadPriority(cpuThreadHandle, THREAD_PRIORITY_TIME_CRITICAL);  // Don't yield
    }
}

void Sample::CreateTimelines()
{
    for (uint32_t i = 0; i < TIMELINE_COUNT; ++i)
    {
        m_timeline[i].m_name = c_timelineNames[i];
    }
    m_timeline[TIMELINE_PIXEL_COUNT].m_maxY = 100.0f;
    m_timeline[TIMELINE_PIXEL_COUNT].m_minY = 0.0f;
    m_timeline[TIMELINE_PIXEL_COUNT].m_tickInterval = 10.0f;
    m_timeline[TIMELINE_PIXEL_COUNT].NewDataPoint = [](const FrameStatistics* frameStats)->Timeline::DataPoint
    {
        Timeline::DataPoint dataPoint = {};
        dataPoint.m_timeInMs = 1000.0f * (float)((frameStats->display.FlipTime) / g_perfFreq);
        dataPoint.m_value = 100.0f * (frameStats->data.width * frameStats->data.height) / (float)(c_frameWidthMaximum * c_frameHeightMaximum);

        return dataPoint;
    };
    m_timeline[TIMELINE_FRAME_TIME_FLIP].m_maxY = 50.0f;
    m_timeline[TIMELINE_FRAME_TIME_FLIP].m_minY = 0.0f;
    m_timeline[TIMELINE_FRAME_TIME_FLIP].m_tickInterval = 16.66f;
    m_timeline[TIMELINE_FRAME_TIME_FLIP].NewDataPoint = [](const FrameStatistics* frameStats)->Timeline::DataPoint
    {
        static FrameStatistics s_stats[2] = {};
        s_stats[1] = *frameStats;

        Timeline::DataPoint dataPoint = {};
        dataPoint.m_timeInMs = 1000.0f * (float)((s_stats[1].display.FlipTime) / g_perfFreq);
        dataPoint.m_value = 1000.0f * (float)((s_stats[1].display.FlipTime - s_stats[0].display.FlipTime) / g_perfFreq);

        s_stats[0] = *frameStats;
        return dataPoint;
    };
    m_timeline[TIMELINE_VSYNC_MARGIN].m_maxY = 16.67f;
    m_timeline[TIMELINE_VSYNC_MARGIN].m_minY = -16.67f;
    m_timeline[TIMELINE_VSYNC_MARGIN].m_tickInterval = 16.67f;
    m_timeline[TIMELINE_VSYNC_MARGIN].NewDataPoint = [](const FrameStatistics* frameStats)->Timeline::DataPoint
    {
        static FrameStatistics s_stats[2] = {};
        s_stats[1] = *frameStats;

        Timeline::DataPoint dataPoint = {};
        dataPoint.m_timeInMs = 1000.0f * (float)((s_stats[1].display.FlipTime) / g_perfFreq);
        uint64_t timeFrameComplete = s_stats[1].present.GPUProcessTime;
        uint64_t timePreviousFrameVSync = s_stats[0].display.VSyncTime;
        uint64_t scheduledTimeVSync = timePreviousFrameVSync + (uint64_t)((1 / c_vsyncsPerSecond) * g_perfFreq);
        dataPoint.m_value = 1000.0f * (float)((scheduledTimeVSync - timeFrameComplete) / g_perfFreq);

        s_stats[0] = *frameStats;
        return dataPoint;
    };
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    // Because the window size doesn't change on Xbox, everything here is actually based on fixed values.

    auto device = m_deviceResources->GetD3DDevice();

    // The variable sizes for dynamic resolution
    m_frameWidth = c_frameWidthMaximum;
    m_frameHeight = c_frameHeightMaximum;
    m_frameViewportDynamic.TopLeftX = 0.0f;
    m_frameViewportDynamic.TopLeftY = 0.0f;
    m_frameViewportDynamic.Width = static_cast<float>(m_frameWidth);
    m_frameViewportDynamic.Height = static_cast<float>(m_frameHeight);
    m_frameViewportDynamic.MinDepth = 0.0f;
    m_frameViewportDynamic.MaxDepth = 1.0f;

    // Fixed viewport for UI rendering
    m_frameViewportIdeal.TopLeftX = 0.0f;
    m_frameViewportIdeal.TopLeftY = 0.0f;
    m_frameViewportIdeal.Width = static_cast<float>(c_frameWidthMaximum);
    m_frameViewportIdeal.Height = static_cast<float>(c_frameHeightMaximum);
    m_frameViewportIdeal.MinDepth = 0.0f;
    m_frameViewportIdeal.MaxDepth = 1.0f;

    m_uiManager.SetWindowSize(static_cast<int>(c_frameWidthMaximum), static_cast<int>(c_frameHeightMaximum));

    // Set up dynamic render targets
    {
        uint32_t widthSteps = CalculateResolutionSteps(c_frameWidthMinimum, c_frameWidthMaximum, c_frameWidthIncrement);
        uint32_t heightSteps = CalculateResolutionSteps(c_frameHeightMinimum, c_frameHeightMaximum, c_frameHeightIncrement);

        m_currentRezData = m_resolutionSet.InitializeMaxResolution(
            device,
            c_frameWidthMaximum,
            c_frameHeightMaximum,
            std::max(widthSteps, heightSteps),  // Maximum possible resolutions we can adjust to
            m_deviceResources->GetBackBufferFormat(),
            m_deviceResources->GetDepthBufferFormat()
        );
    }

    // Set UI sprite viewport
    m_uiSpriteBatch->SetViewport(m_frameViewportIdeal);

    // Set camera parameters.
    m_proj = XMMatrixPerspectiveFovLH(XM_PIDIV4, c_frameWidthMaximum / static_cast<float>(c_frameHeightMaximum), 500.0f, 0.1f);

    // Begin uploading texture resources
    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        m_smallFont = std::make_unique<SpriteFont>(device, resourceUpload,
            L"Assets/Fonts/SegoeUI_18.spritefont",
            m_srvPile->GetCpuHandle(SRVDescriptorHeapIndex::SRV_Font),
            m_srvPile->GetGpuHandle(SRVDescriptorHeapIndex::SRV_Font));

        m_ctrlFont = std::make_unique<SpriteFont>(device, resourceUpload,
            L"Assets/Fonts/XboxOneControllerLegendSmall.spritefont",
            m_srvPile->GetCpuHandle(SRVDescriptorHeapIndex::SRV_CtrlFont),
            m_srvPile->GetGpuHandle(SRVDescriptorHeapIndex::SRV_CtrlFont));

        auto finished = resourceUpload.End(m_deviceResources->GetCommandQueue());
        finished.wait();
    }
}
#pragma endregion
