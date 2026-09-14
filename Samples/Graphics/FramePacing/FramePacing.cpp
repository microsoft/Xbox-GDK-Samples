//--------------------------------------------------------------------------------------
// FramePacing.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FramePacing.h"

#include "ATGColors.h"
#include "CommonStates.h"
#include "Menu.h"
#include "ReadData.h"

extern void ExitSample() noexcept;

using namespace DirectX;
using namespace ATG::UITK;
using namespace FramePacingUtils;

using Microsoft::WRL::ComPtr;

namespace
{
    // Will store frequency of QueryPerformanceCounter
    static uint64_t g_cpuPerfFreq = 0ULL;

    // Will store frequency of GetTimestampFrequency
    static uint64_t g_gpuPerfFreq = 0ULL;
    static uint64_t g_cpuAnchorTimestamp = 0ULL;
    static uint64_t g_gpuAnchorTimestamp = 0ULL;

    uint64_t ConvertGpuToCpuTimestamp(uint64_t gpuTimestamp)
    {
        return g_cpuAnchorTimestamp - ((g_gpuAnchorTimestamp - gpuTimestamp) * g_cpuPerfFreq / g_gpuPerfFreq);
    }

    // Fixed offsets into descriptor heaps
    enum SRVDescriptorHeapIndex
    {
        SRV_Font = 0,
        SRV_CtrlFont,
        SRV_Count
    };

    PackedVector::XMCOLOR c_intervalColorsPacked[_countof(c_intervalColors)];
};

Sample::Sample() noexcept(false)
    : m_fenceValue(0ULL)
    , m_rtvDescriptorSize(0)
    , m_screenViewport{}
    , m_scissorRect{}
    , m_backBufferFormat(DXGI_FORMAT_B8G8R8A8_UNORM)
    , m_window(nullptr)
    , m_outputSize{ 0, 0, 1920, 1080 }
    , m_frame(0)
    , m_nextFrameBufferIndex(0)
    , m_nextFrameIndex(0)
    , m_currentUpdateFrameBuffer{}
    , m_currentRenderFrameBuffer{}
    , m_eventUpdateDone(nullptr)
    , m_eventRenderReceived(nullptr)
    , m_renderThread(nullptr)
    , m_fenceGraphicsToComputeValue(0)
    , m_fenceComputeToGraphicsValue(0)
    , m_enforceCpuUpdateTimeThread(nullptr)
    , m_enforceCpuRenderTimeThread(nullptr)
    , m_isExiting(false)
    , m_startCpuUpdateSignal(true)
    , m_updateThreadTimeInMs(0.0f)
    , m_endCpuUpdateSignal(false)
    , m_startCpuRenderSignal(true)
    , m_renderThreadTimeInMs(0.0f)
    , m_endCpuRenderSignal(false)
    , m_desiredFrameLoad{}
    , m_autoFrameLoad(false)
    , m_currentFrameLoadSequence(0)
    , m_framePacing{}
    , m_autoPacing(false)
    , m_frameEventsDirty(false)
    , m_frameRateMeasured(60.0f)
    , m_latencyMeasuredInMs(30.0f)
    , m_marginMeasuredInMs(3.3f)
    , m_tearLocation(0.0f)
    , m_paused(false)
    , m_graphZoom(1.0f)
    , m_graphOffset(0.0f)
    , m_queryCurrentFrame(0U)
{
    m_framePacing.m_frameRateTarget = FrameRate::Fps60;
    m_framePacing.m_frameThreshold = 0;
    m_framePacing.m_framePeriod = 2;
    m_framePacing.m_frameBuffers = 3;
    m_framePacing.m_frameOffsetInMs = 0;

    m_deviceType = XSystemGetDeviceType();

    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq))
    {
        throw std::runtime_error("QueryPerformanceFrequency");
    }
    g_cpuPerfFreq = static_cast<uint64_t>(freq.QuadPart);

    for (auto i = 0U; i < _countof(c_intervalColors); ++i)
    {
        PackedVector::XMStoreColor(&c_intervalColorsPacked[i], c_intervalColors[i]);
    }
}

Sample::~Sample()
{
    m_isExiting = true;

    if (m_eventUpdateDone)
    {
        SetEvent(m_eventUpdateDone);
    }
    if (m_eventRenderReceived)
    {
        SetEvent(m_eventRenderReceived);
    }

    if (m_renderThread)
    {
        m_renderThread->join();
        delete m_renderThread;
        m_renderThread = nullptr;
    }

    if (m_enforceCpuUpdateTimeThread)
    {
        m_enforceCpuUpdateTimeThread->join();
        delete m_enforceCpuUpdateTimeThread;
        m_enforceCpuUpdateTimeThread = nullptr;
    }

    if (m_enforceCpuRenderTimeThread)
    {
        m_enforceCpuRenderTimeThread->join();
        delete m_enforceCpuRenderTimeThread;
        m_enforceCpuRenderTimeThread = nullptr;
    }

    WaitForGpu();

    // This sample can present from either queue, so clear both queues before
    // member destruction releases the presentation resources.
    if (m_commandQueueGraphics)
    {
        std::ignore = m_commandQueueGraphics->PresentX(0, nullptr, nullptr);
    }
    if (m_commandQueueCompute)
    {
        std::ignore = m_commandQueueCompute->PresentX(0, nullptr, nullptr);
    }

    if (m_eventUpdateDone)
    {
        CloseHandle(m_eventUpdateDone);
    }
    if (m_eventRenderReceived)
    {
        CloseHandle(m_eventRenderReceived);
    }

}

// Initialize the Direct3D resources required to run.
void Sample::Initialize(HWND window)
{
    m_gamePad = std::make_unique<GamePad>();

    m_window = window;

    CreateDeviceDependentResources();
    RegisterFrameEvents();

    CreateWindowSizeDependentResources();

    InitializeUI();

    CreateThreads();

    CreateWorkers();

    CreateGraphs();

    CreateFrameLoadSequences();
}

void Sample::InitializeUI()
{
    auto layout = m_uiManager.LoadLayoutFromFile("Assets/UILayout.json");
    m_uiManager.AttachTo(layout, m_uiManager.GetRootElement());

    // Cache these for faster access
    m_fpsText = m_uiManager.FindTypedById<UIStaticText>(ID("fps_num"));
    m_latencyText = m_uiManager.FindTypedById<UIStaticText>(ID("latency_num"));
    m_marginText = m_uiManager.FindTypedById<UIStaticText>(ID("margin_num"));
    m_menuPanel = m_uiManager.FindTypedById<UIPanel>(ID("menu_panel"));

    m_menu.SetItemStatus(UI_PARAM_FRAME_LOAD_SEQUENCE, UIParam::STATUS_AUTO);
}

void Sample::CreateThreads()
{
    m_eventUpdateDone = CreateEvent(nullptr, FALSE, FALSE, L"Update Done");
    m_eventRenderReceived = CreateEvent(nullptr, FALSE, TRUE, L"Render Received");
    if (!m_eventUpdateDone || !m_eventRenderReceived)
    {
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEvent");
    }

    // Rename the main thread as Update Thread
    auto updateThreadName = L"Update Thread";
    auto updateThreadHandle = GetCurrentThread();
    auto updateCore = 0ULL;
    (void)SetThreadDescription(updateThreadHandle, updateThreadName);
    (void)SetThreadAffinityMask(updateThreadHandle, 1ULL << updateCore);   // Run on its own core
    (void)SetThreadPriority(updateThreadHandle, THREAD_PRIORITY_NORMAL);

    // Create Render Thread which loops over the rendering code
    auto renderThreadName = L"Render Thread";
    m_renderThread = new std::thread(&Sample::Render, this);
    auto renderThreadHandle = m_renderThread->native_handle();
    auto renderCore = 1ULL;
    (void)SetThreadDescription(renderThreadHandle, renderThreadName);
    (void)SetThreadAffinityMask(renderThreadHandle, 1ULL << renderCore);   // Run on its own core
    (void)SetThreadPriority(renderThreadHandle, THREAD_PRIORITY_NORMAL);
}

void Sample::CreateWorkers()
{
    auto device = m_device;

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
                IID_GRAPHICS_PPV_ARGS(m_gpuTimeResourceGraphics.ReleaseAndGetAddressOf())));
        m_gpuTimeResourceGraphics->SetName(L"Enforce GPU Time Resource Graphics");
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufDesc,
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_gpuTimeResourceCompute.ReleaseAndGetAddressOf())));
        m_gpuTimeResourceCompute->SetName(L"Enforce GPU Time Resource Compute");

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

    // Create worker threads to enforce CPU frame time
    {
        m_startCpuUpdateSignal = false; 
        m_endCpuUpdateSignal = false;

        auto cpuEnforceUpdateTimeThread = L"CPU Enforce Update Time Thread";
        m_enforceCpuUpdateTimeThread = new std::thread(&Sample::EnforceCpuUpdateTime, this);
        auto updateThreadHandle = m_enforceCpuUpdateTimeThread->native_handle();
        auto updateCore = 2ULL;
        (void)SetThreadDescription(updateThreadHandle, cpuEnforceUpdateTimeThread);
        (void)SetThreadAffinityMask(updateThreadHandle, 1ULL << updateCore);   // Run on its own core
        (void)SetThreadPriority(updateThreadHandle, THREAD_PRIORITY_NORMAL);   // Can bump priority if necessary to avoid preemption

        m_startCpuRenderSignal = false;
        m_endCpuRenderSignal = false;

        auto cpuEnforceRenderTimeThread = L"CPU Enforce Render Time Thread";
        m_enforceCpuRenderTimeThread = new std::thread(&Sample::EnforceCpuRenderTime, this);
        auto renderThreadHandle = m_enforceCpuRenderTimeThread->native_handle();
        auto renderCore = 3ULL;
        (void)SetThreadDescription(renderThreadHandle, cpuEnforceRenderTimeThread);
        (void)SetThreadAffinityMask(renderThreadHandle, 1ULL << renderCore);   // Run on its own core
        (void)SetThreadPriority(renderThreadHandle, THREAD_PRIORITY_NORMAL);   // Can bump priority if necessary to avoid preemption
    }
}

// Forces the CPU to wait until a predetermined time has elapsed.
void Sample::EnforceCpuUpdateTime()
{
    while (!m_isExiting)
    {
        if (m_startCpuUpdateSignal)
        {
            PIXBeginEvent(0, L"CPU artificial Update workload");

            assert(!m_endCpuUpdateSignal);

            m_startCpuUpdateSignal = false;
            LARGE_INTEGER startTime;
            QueryPerformanceCounter(&startTime);

            LARGE_INTEGER endTime;
            endTime.QuadPart = startTime.QuadPart + static_cast<int64_t>(m_updateThreadTimeInMs / 1000.0 * g_cpuPerfFreq);
            LARGE_INTEGER currentTime;
            do
            {
                QueryPerformanceCounter(&currentTime);
            } while (currentTime.QuadPart < endTime.QuadPart && !m_isExiting);

            m_endCpuUpdateSignal = true;

            PIXEndEvent();
        }
    }

    m_endCpuUpdateSignal = true;
}

// Forces the CPU to wait until a predetermined time has elapsed.
void Sample::EnforceCpuRenderTime()
{
    while (!m_isExiting)
    {
        if (m_startCpuRenderSignal)
        {
            PIXBeginEvent(0, L"CPU artificial Render workload");

            assert(!m_endCpuRenderSignal);

            m_startCpuRenderSignal = false;
            LARGE_INTEGER startTime;
            QueryPerformanceCounter(&startTime);

            LARGE_INTEGER endTime;
            endTime.QuadPart = startTime.QuadPart + static_cast<int64_t>(m_renderThreadTimeInMs / 1000.0 * g_cpuPerfFreq);
            LARGE_INTEGER currentTime;
            do
            {
                QueryPerformanceCounter(&currentTime);
            } while (currentTime.QuadPart < endTime.QuadPart && !m_isExiting);

            m_endCpuRenderSignal = true;

            PIXEndEvent();
        }
    }

    m_endCpuRenderSignal = true;
}

void Sample::CreateGraphs()
{
    for (uint32_t lineGraphIndex = 0; lineGraphIndex < LINE_GRAPH_COUNT; ++lineGraphIndex)
    {
        m_lineGraph[lineGraphIndex].m_name = c_lineGraphNames[lineGraphIndex];
        m_lineGraphPanel[lineGraphIndex] = m_uiManager.FindTypedById<UIPanel>(ID(c_lineGraphLayoutNames[lineGraphIndex]));
    }
    m_lineGraph[LINE_GRAPH_FRAME_TIME_FLIP].m_maxX = 1.0f;
    m_lineGraph[LINE_GRAPH_FRAME_TIME_FLIP].m_minX = 0.0f;
    m_lineGraph[LINE_GRAPH_FRAME_TIME_FLIP].m_maxY = 50.0f;
    m_lineGraph[LINE_GRAPH_FRAME_TIME_FLIP].m_minY = 0.0f;
    m_lineGraph[LINE_GRAPH_FRAME_TIME_FLIP].m_tickInterval = 16.66f;
    m_lineGraph[LINE_GRAPH_FRAME_TIME_FLIP].m_createData = [](const FrameStatistics* frameStats)->DataPoint
    {
        static FrameStatistics s_stats[2] = {};
        s_stats[1] = *frameStats;

        DataPoint dataPoint = {};
        dataPoint.m_flipTimeInMs = 1000.0 * (frameStats->display.FlipTime / static_cast<double>(g_cpuPerfFreq));
        if (0ULL != s_stats[0].display.FlipTime)    // Can't get a frame duration until the 2nd flip
        {
            dataPoint.m_value = 1000.0f * static_cast<float>((s_stats[1].display.FlipTime - s_stats[0].display.FlipTime) / static_cast<double>(g_cpuPerfFreq));
        }

        s_stats[0] = *frameStats;
        return dataPoint;
    };
    m_lineGraph[LINE_GRAPH_FRAME_LATENCY].m_maxX = 1.0f;
    m_lineGraph[LINE_GRAPH_FRAME_LATENCY].m_minX = 0.0f;
    m_lineGraph[LINE_GRAPH_FRAME_LATENCY].m_maxY = 50.0f;
    m_lineGraph[LINE_GRAPH_FRAME_LATENCY].m_minY = 0.0f;
    m_lineGraph[LINE_GRAPH_FRAME_LATENCY].m_tickInterval = 16.66f;
    m_lineGraph[LINE_GRAPH_FRAME_LATENCY].m_createData = [](const FrameStatistics* frameStats)->DataPoint
    {
        DataPoint dataPoint = {};
        dataPoint.m_flipTimeInMs = 1000.0 * (frameStats->display.FlipTime / static_cast<double>(g_cpuPerfFreq));
        dataPoint.m_value = 1000.0f * static_cast<float>((frameStats->present.GPUProcessTime - frameStats->origin.SignalTime) / static_cast<double>(g_cpuPerfFreq));

        return dataPoint;
    };
    m_lineGraph[LINE_GRAPH_LATENCY_MARGIN].m_maxX = 1.0f;
    m_lineGraph[LINE_GRAPH_LATENCY_MARGIN].m_minX = 0.0f;
    m_lineGraph[LINE_GRAPH_LATENCY_MARGIN].m_maxY = 16.67f;
    m_lineGraph[LINE_GRAPH_LATENCY_MARGIN].m_minY = -16.67f;
    m_lineGraph[LINE_GRAPH_LATENCY_MARGIN].m_tickInterval = 16.67f;
    m_lineGraph[LINE_GRAPH_LATENCY_MARGIN].m_createData = [](const FrameStatistics* frameStats)->DataPoint
    {
        DataPoint dataPoint = {};
        dataPoint.m_flipTimeInMs = 1000.0 * (frameStats->display.FlipTime / static_cast<double>(g_cpuPerfFreq));
        auto frameOriginInMicroseconds = 1000.0 * 1000.0 * (frameStats->origin.SignalTime / static_cast<double>(g_cpuPerfFreq));
        auto frameBudgetInMicroseconds = frameStats->interval.LengthInMicroseconds * frameStats->interval.PeriodInIntervals
            - frameStats->origin.IntervalOffsetInMicroseconds;
        auto frameDeadlineInMicroseconds = frameOriginInMicroseconds + frameBudgetInMicroseconds;
        auto frameCompletionInMicroseconds = 1000.0 * 1000.0 * (frameStats->present.GPUProcessTime / static_cast<double>(g_cpuPerfFreq));
            
        dataPoint.m_value = static_cast<float>((frameDeadlineInMicroseconds - frameCompletionInMicroseconds) / 1000.0);

       return dataPoint;
    };

    for (uint32_t intervalGraphIndex = 0; intervalGraphIndex < INTERVAL_GRAPH_COUNT; ++intervalGraphIndex)
    {
        m_intervalGraph[intervalGraphIndex].m_name = c_intervalGraphNames[intervalGraphIndex];
        m_intervalGraphPanel[intervalGraphIndex] = m_uiManager.FindTypedById<UIPanel>(ID(c_intervalGraphLayoutNames[intervalGraphIndex]));
    }
    m_intervalGraph[INTERVAL_GRAPH_CPU_UPDATE].m_maxX = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_UPDATE].m_minX = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_UPDATE].m_maxY = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_UPDATE].m_minY = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_UPDATE].m_tickInterval = 1000.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_UPDATE].m_createData = [](const FrameTimestamps* frameTimestamps)->DataInterval
    {
        DataInterval dataInterval = {};
        dataInterval.m_frameBuffer = frameTimestamps->m_frameBuffer;
        dataInterval.m_flipTimeInMs = 1000.0 * frameTimestamps->m_flipTime / static_cast<double>(g_cpuPerfFreq);
        dataInterval.m_startInMs = 1000.0 * frameTimestamps->m_startCpuUpdate.QuadPart / static_cast<double>(g_cpuPerfFreq);
        dataInterval.m_stopInMs = 1000.0 * frameTimestamps->m_stopCpuUpdate.QuadPart / static_cast<double>(g_cpuPerfFreq);
        return dataInterval;
    };
    m_intervalGraph[INTERVAL_GRAPH_CPU_RENDER].m_maxX = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_RENDER].m_minX = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_RENDER].m_maxY = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_RENDER].m_minY = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_RENDER].m_tickInterval = 1000.0f;
    m_intervalGraph[INTERVAL_GRAPH_CPU_RENDER].m_createData = [](const FrameTimestamps* frameTimestamps)->DataInterval
    {
        DataInterval dataInterval = {};
        dataInterval.m_frameBuffer = frameTimestamps->m_frameBuffer;
        dataInterval.m_flipTimeInMs = 1000.0 * frameTimestamps->m_flipTime / static_cast<double>(g_cpuPerfFreq);
        dataInterval.m_startInMs = 1000.0 * frameTimestamps->m_startCpuRender.QuadPart / static_cast<double>(g_cpuPerfFreq);
        dataInterval.m_stopInMs = 1000.0 * frameTimestamps->m_stopCpuRender.QuadPart / static_cast<double>(g_cpuPerfFreq);
        return dataInterval;
    };
    m_intervalGraph[INTERVAL_GRAPH_GPU_GRAPHICS].m_maxX = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_GRAPHICS].m_minX = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_GRAPHICS].m_maxY = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_GRAPHICS].m_minY = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_GRAPHICS].m_tickInterval = 1000.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_GRAPHICS].m_createData = [](const FrameTimestamps* frameTimestamps)->DataInterval
    {
        DataInterval dataInterval = {};
        dataInterval.m_frameBuffer = frameTimestamps->m_frameBuffer;
        dataInterval.m_flipTimeInMs = 1000.0 * frameTimestamps->m_flipTime / static_cast<double>(g_cpuPerfFreq);
        dataInterval.m_startInMs = 1000.0 * ConvertGpuToCpuTimestamp(frameTimestamps->m_startGpuGraphics) / static_cast<double>(g_cpuPerfFreq);
        dataInterval.m_stopInMs = 1000.0 * ConvertGpuToCpuTimestamp(frameTimestamps->m_stopGpuGraphics) / static_cast<double>(g_cpuPerfFreq);
        return dataInterval;
    };
    m_intervalGraph[INTERVAL_GRAPH_GPU_COMPUTE].m_maxX = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_COMPUTE].m_minX = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_COMPUTE].m_maxY = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_COMPUTE].m_minY = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_COMPUTE].m_tickInterval = 1000.0f;
    m_intervalGraph[INTERVAL_GRAPH_GPU_COMPUTE].m_createData = [](const FrameTimestamps* frameTimestamps)->DataInterval
    {
        DataInterval dataInterval = {};
        dataInterval.m_frameBuffer = frameTimestamps->m_frameBuffer;
        dataInterval.m_flipTimeInMs = 1000.0 * frameTimestamps->m_flipTime / static_cast<double>(g_cpuPerfFreq);
        dataInterval.m_startInMs = 1000.0 * ConvertGpuToCpuTimestamp(frameTimestamps->m_startGpuCompute) / static_cast<double>(g_cpuPerfFreq);
        dataInterval.m_stopInMs = 1000.0 * ConvertGpuToCpuTimestamp(frameTimestamps->m_stopGpuCompute) / static_cast<double>(g_cpuPerfFreq);
        return dataInterval;
    };
    m_intervalGraph[INTERVAL_GRAPH_FLIP].m_maxX = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_FLIP].m_minX = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_FLIP].m_maxY = 1.0f;
    m_intervalGraph[INTERVAL_GRAPH_FLIP].m_minY = 0.0f;
    m_intervalGraph[INTERVAL_GRAPH_FLIP].m_tickInterval = 1000.0f;
    m_intervalGraph[INTERVAL_GRAPH_FLIP].m_createData = [](const FrameTimestamps* frameTimestamps)->DataInterval
    {
        static FrameTimestamps s_timestamps[2] = {};
        s_timestamps[1] = *frameTimestamps;

        DataInterval dataInterval = {};
        dataInterval.m_flipTimeInMs = 1000.0 * frameTimestamps->m_flipTime / static_cast<double>(g_cpuPerfFreq);
        if (0ULL != s_timestamps[0].m_flipTime)    // Can't get a frame duration until the 2nd flip
        {
            dataInterval.m_frameBuffer = s_timestamps[0].m_frameBuffer;
            dataInterval.m_startInMs = 1000.0 * s_timestamps[0].m_flipTime / static_cast<double>(g_cpuPerfFreq);
            dataInterval.m_stopInMs = 1000.0 * s_timestamps[1].m_flipTime / static_cast<double>(g_cpuPerfFreq);
        }

        s_timestamps[0] = *frameTimestamps;
        return dataInterval;
    };
}

// Reads text files which describe a repeating pattern of frame loads to simulate
void Sample::CreateFrameLoadSequences()
{
    m_frameLoadSequences.resize(_countof(c_frameLoadSequenceNames));
    for (auto sequence = 0U; sequence < _countof(c_frameLoadSequenceNames); ++sequence)
    {
        std::vector<FrameLoad>& frameLoadSequence = m_frameLoadSequences[sequence];

        wchar_t filePath[256];
        swprintf_s(filePath, L"FrameLoads\\%ls.txt", c_frameLoadSequenceFileNames[sequence]);
        auto frameLoadRawData = DX::ReadData(filePath);
        assert(frameLoadRawData.size() > 0ULL);   // Missing frame load sequence file

        std::istringstream rawData(std::string(
            reinterpret_cast<const char*>(frameLoadRawData.data()),
            frameLoadRawData.size()));

        char line[256] = "";
        uint32_t period = 0U;
        uint32_t linesFound = 0U;
        while (!rawData.eof())
        {
            rawData.getline(line, _countof(line));

            std::istringstream rawLine(line);

            char lineHeader[256] = "";
            rawLine >> lineHeader >> std::ws;

            auto parseValues = [&](float FrameLoad::* value)
            {
                auto next = 0U;
                uint32_t runLength = 0;
                float runValue = 0.0f;
                while (rawLine >> runLength >> runValue)
                {
                    if (runValue < 0.0f)
                    {
                        throw std::runtime_error("Frame load values must be non-negative");
                    }

                    for (auto i = 0U; i < runLength; ++i)
                    {
                        frameLoadSequence[next++].*value = runValue;
                    }
                }
                assert(period == next); // Invalid frame load file
            };

            if (0 == strcmp(lineHeader, "//"))
            {
                // comment
                continue;
            }
            else if (0 == strcmp(lineHeader, "Period:"))
            {
                ++linesFound;

                rawLine >> period;

                frameLoadSequence.resize(period);
            }
            else if (0 == strcmp(lineHeader, "CpuUpdate:"))
            {
                ++linesFound;
                parseValues(&FrameLoad::m_cpuUpdateTimeInMs);
            }
            else if (0 == strcmp(lineHeader, "CpuRender:"))
            {
                ++linesFound;
                parseValues(&FrameLoad::m_cpuRenderTimeInMs);
            }
            else if (0 == strcmp(lineHeader, "CpuGpuLag:"))
            {
                ++linesFound;
                parseValues(&FrameLoad::m_cpuToGpuTimeInMs);
            }
            else if (0 == strcmp(lineHeader, "GpuGraphics:"))
            {
                ++linesFound;
                parseValues(&FrameLoad::m_gpuGraphicsTimeInMs);
            }
            else if (0 == strcmp(lineHeader, "GpuCompute:"))
            {
                ++linesFound;
                parseValues(&FrameLoad::m_gpuComputeTimeInMs);
            }
        }

        assert(6U == linesFound); // Invalid frame load file
    }
}

Sample::FrameLoad Sample::GetFrameLoad(uint32_t frameIndex) const
{
    std::scoped_lock lock(m_settingsMutex);

    if (m_autoFrameLoad)
    {
        const auto& frameLoadSequence = m_frameLoadSequences[m_currentFrameLoadSequence];
        return frameLoadSequence[frameIndex % frameLoadSequence.size()];
    }

    return m_desiredFrameLoad;
}

#pragma region Frame Update
// Executes basic render loop.
void Sample::Tick()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Frame %llu", m_frame);

    PIXBeginEvent(PIX_COLOR_DEFAULT, L"WaitFrameEventX");

    // Prepare the command list to render a new frame.
    BeginFrame();

    PIXEndEvent();

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    // Game state is usually double-buffered, regardless of swap chain size
    // so can't let Update signal Render until Render acknowledges previous signal
    {
        PIXScopedEvent(PIX_COLOR_DEFAULT, L"Throttle on Render");
        auto result = WaitForSingleObject(m_eventRenderReceived, INFINITE);
        if (WAIT_OBJECT_0 != result)
        {
            throw std::runtime_error("Expected thread to receive signal");
        }
    }

    // Signal Render thread that a game state is available
    {
        auto result = SetEvent(m_eventUpdateDone);
        if (!result)
        {
            throw std::runtime_error("Expected event to get signaled");
        }
    }

    PIXEndEvent();
    m_frame++;
}

// Updates the world.
void Sample::Update(DX::StepTimer const& /*timer*/)
{
    static_assert(_countof(c_intervalColors) == c_maxBackBufferCount);
    PIXBeginEvent(c_intervalColorsPacked[m_currentUpdateFrameBuffer.m_frameBuffer], L"Update %d", m_currentUpdateFrameBuffer.m_frame);

    UpdateUI();

    m_updateThreadTimeInMs = m_currentUpdateFrameBuffer.m_frameLoad.m_cpuUpdateTimeInMs;

    // Start timer for artificial CPU load
    m_startCpuUpdateSignal = true;

    // Record timestamp with frame token
    TaggedCpuTimestamp startCpuUpdate;
    startCpuUpdate.m_token = m_currentUpdateFrameBuffer.m_token;
    startCpuUpdate.m_frameBuffer = m_currentUpdateFrameBuffer.m_frameBuffer;
    if (!QueryPerformanceCounter(&startCpuUpdate.m_time))
    {
        throw std::runtime_error("QueryPerformanceCounter");
    }
    m_startCpuUpdate.push(startCpuUpdate);

    // Wait for artificial CPU load
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Artificial CPU load");
    while (!m_endCpuUpdateSignal)
    {
        _mm_pause();
    }
    m_endCpuUpdateSignal = false;

    // Record timestamp with frame token
    TaggedCpuTimestamp stopCpuUpdate;
    stopCpuUpdate.m_token = m_currentUpdateFrameBuffer.m_token;
    stopCpuUpdate.m_frameBuffer = m_currentUpdateFrameBuffer.m_frameBuffer;
    if (!QueryPerformanceCounter(&stopCpuUpdate.m_time))
    {
        throw std::runtime_error("QueryPerformanceCounter");
    }
    m_stopCpuUpdate.push(stopCpuUpdate);

    PIXEndEvent();

    PIXEndEvent();
}

void Sample::UpdateUI()
{
    std::scoped_lock stateLock(m_settingsMutex, m_uiMutex);

    float elapsedTime = float(m_timer.GetElapsedSeconds());

    auto pad = m_gamePad->GetState(0);
    if (pad.IsConnected())
    {
        m_gamePadButtons.Update(pad);

        if (pad.IsViewPressed())
        {
            ExitSample();
        }

        if (m_gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_autoPacing = !m_autoPacing;

            // Store off manual settings for later restore
            static FramePacing manualFramePacing = {};
            if (m_autoPacing)
            {
                manualFramePacing = m_framePacing;
            }
            else
            {
                m_framePacing = manualFramePacing;

                m_menu.SetItemValue< uint32_t >(UI_PARAM_FRAME_RATE, static_cast<uint32_t>(m_framePacing.m_frameRateTarget));
                m_menu.SetItemValue<int>(UI_PARAM_FRAME_THRESHOLD, static_cast<int>(m_framePacing.m_frameThreshold));
                m_menu.SetItemValue<int>(UI_PARAM_FRAME_PERIOD, static_cast<int>(m_framePacing.m_framePeriod));
                m_menu.SetItemValue<int>(UI_PARAM_FRAME_BUFFERS, static_cast<int>(m_framePacing.m_frameBuffers));
                m_menu.SetItemValue< float >(UI_PARAM_FRAME_OFFSET, m_framePacing.m_frameOffsetInMs);

                m_frameEventsDirty = true;
            }

            m_menu.SetItemStatus(UI_PARAM_FRAME_RATE, UIParam::STATUS_DEFAULT);
            m_menu.SetItemStatus(UI_PARAM_FRAME_THRESHOLD, UIParam::STATUS_DEFAULT);
            m_menu.SetItemStatus(UI_PARAM_FRAME_PERIOD, m_autoPacing ? UIParam::STATUS_AUTO : UIParam::STATUS_DEFAULT);
            m_menu.SetItemStatus(UI_PARAM_FRAME_BUFFERS, m_autoPacing ? UIParam::STATUS_AUTO : UIParam::STATUS_DEFAULT);
            m_menu.SetItemStatus(UI_PARAM_FRAME_OFFSET, m_autoPacing ? UIParam::STATUS_AUTO : UIParam::STATUS_DEFAULT);
        }

        if (m_gamePadButtons.x == GamePad::ButtonStateTracker::PRESSED)
        {
            m_autoFrameLoad = !m_autoFrameLoad;

            // Store off manual settings for later restore
            static FrameLoad manualFrameLoad = {};
            if (m_autoFrameLoad)
            {
                manualFrameLoad = m_desiredFrameLoad;
            }
            else
            {
                m_desiredFrameLoad = manualFrameLoad;

                m_menu.SetItemValue< float >(UI_PARAM_CPU_UPDATE_LOAD, static_cast<float>(m_desiredFrameLoad.m_cpuUpdateTimeInMs));
                m_menu.SetItemValue< float >(UI_PARAM_CPU_RENDER_LOAD, static_cast<float>(m_desiredFrameLoad.m_cpuRenderTimeInMs));
                m_menu.SetItemValue< float >(UI_PARAM_CPU_TO_GPU_LAG, static_cast<float>(m_desiredFrameLoad.m_cpuToGpuTimeInMs));
                m_menu.SetItemValue< float >(UI_PARAM_GPU_GRAPHICS_LOAD, static_cast<float>(m_desiredFrameLoad.m_gpuGraphicsTimeInMs));
                m_menu.SetItemValue< float >(UI_PARAM_GPU_COMPUTE_LOAD, static_cast<float>(m_desiredFrameLoad.m_gpuComputeTimeInMs));
            }

            m_menu.SetItemStatus(UI_PARAM_FRAME_LOAD_SEQUENCE, m_autoFrameLoad ? UIParam::STATUS_DEFAULT : UIParam::STATUS_AUTO);

            m_menu.SetItemStatus(UI_PARAM_CPU_UPDATE_LOAD, m_autoFrameLoad ? UIParam::STATUS_AUTO : UIParam::STATUS_DEFAULT);
            m_menu.SetItemStatus(UI_PARAM_CPU_RENDER_LOAD, m_autoFrameLoad ? UIParam::STATUS_AUTO : UIParam::STATUS_DEFAULT);
            m_menu.SetItemStatus(UI_PARAM_CPU_TO_GPU_LAG, m_autoFrameLoad ? UIParam::STATUS_AUTO : UIParam::STATUS_DEFAULT);
            m_menu.SetItemStatus(UI_PARAM_GPU_GRAPHICS_LOAD, m_autoFrameLoad ? UIParam::STATUS_AUTO : UIParam::STATUS_DEFAULT);
            m_menu.SetItemStatus(UI_PARAM_GPU_COMPUTE_LOAD, m_autoFrameLoad ? UIParam::STATUS_AUTO : UIParam::STATUS_DEFAULT);
        }

        if (m_gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_paused = !m_paused;
        }

        // Change zoom based on triggers
        float zoom = m_graphZoom;
        float zoomSpeed = 1.0f;
        float zoomMax = 40.0f;
        zoom *= (1.0f + 0.01f * zoomSpeed * pad.triggers.left);
        zoom *= (1.0f - 0.01f * zoomSpeed * pad.triggers.right);
        zoom = std::min(zoom, 1.0f);
        zoom = std::max(zoom, 1.0f / zoomMax);
        m_graphZoom = zoom;

        // Change offset based on right thumbstick
        float offset = m_graphOffset;
        offset += 0.1f * zoomSpeed * m_graphZoom * pad.thumbSticks.rightX;
        offset = std::min(offset, 0.0f);
        offset = std::max(offset, -1.0f + m_graphZoom);
        m_graphOffset = offset;

        m_menu.Update(m_gamePadButtons);
    }
    else
    {
        m_gamePadButtons.Reset();
    }

    m_inputState.Update(elapsedTime, *m_gamePad);
    m_uiManager.Update(elapsedTime, m_inputState);

    // Set artificial processor loads
    if (m_autoFrameLoad)
    {
        m_currentFrameLoadSequence = m_menu.GetItemValue<uint32_t>(UI_PARAM_FRAME_LOAD_SEQUENCE);

        const auto& frameLoad = m_currentUpdateFrameBuffer.m_frameLoad;

        m_menu.SetItemValue< float >(UI_PARAM_CPU_UPDATE_LOAD, static_cast<float>(frameLoad.m_cpuUpdateTimeInMs));
        m_menu.SetItemValue< float >(UI_PARAM_CPU_RENDER_LOAD, static_cast<float>(frameLoad.m_cpuRenderTimeInMs));
        m_menu.SetItemValue< float >(UI_PARAM_CPU_TO_GPU_LAG, static_cast<float>(frameLoad.m_cpuToGpuTimeInMs));
        m_menu.SetItemValue< float >(UI_PARAM_GPU_GRAPHICS_LOAD, static_cast<float>(frameLoad.m_gpuGraphicsTimeInMs));
        m_menu.SetItemValue< float >(UI_PARAM_GPU_COMPUTE_LOAD, static_cast<float>(frameLoad.m_gpuComputeTimeInMs));
    }
    else
    {
        auto getNonnegativeTimeInMs = [&](uint32_t item)
        {
            const auto valueInMs = std::max(0.0f, m_menu.GetItemValue<float>(item));
            m_menu.SetItemValue<float>(item, valueInMs);
            return valueInMs;
        };

        m_desiredFrameLoad.m_cpuUpdateTimeInMs = getNonnegativeTimeInMs(UI_PARAM_CPU_UPDATE_LOAD);
        m_desiredFrameLoad.m_cpuRenderTimeInMs = getNonnegativeTimeInMs(UI_PARAM_CPU_RENDER_LOAD);
        m_desiredFrameLoad.m_cpuToGpuTimeInMs = getNonnegativeTimeInMs(UI_PARAM_CPU_TO_GPU_LAG);
        m_desiredFrameLoad.m_gpuGraphicsTimeInMs = getNonnegativeTimeInMs(UI_PARAM_GPU_GRAPHICS_LOAD);
        m_desiredFrameLoad.m_gpuComputeTimeInMs = getNonnegativeTimeInMs(UI_PARAM_GPU_COMPUTE_LOAD);
    }

    if (m_autoPacing)
    {
        // Automatic pacing derives the period, buffering, and origin offset.
        // The target rate and immediate threshold remain title choices.
        m_framePacing.m_frameRateTarget = static_cast<FrameRate>(m_menu.GetItemValue<uint32_t>(UI_PARAM_FRAME_RATE));
        m_framePacing.m_frameThreshold = static_cast<uint32_t>(m_menu.GetItemValue<int>(UI_PARAM_FRAME_THRESHOLD));
        CalculateAutoPacing();
        m_menu.SetItemValue<int>(UI_PARAM_FRAME_PERIOD, static_cast<int>(m_framePacing.m_framePeriod));
        m_menu.SetItemValue<int>(UI_PARAM_FRAME_BUFFERS, static_cast<int>(m_framePacing.m_frameBuffers));
        m_menu.SetItemValue< float >(UI_PARAM_FRAME_OFFSET, m_framePacing.m_frameOffsetInMs);

        m_frameEventsDirty = true;
    }
    else
    {
        // Change frame pacing via the menu
        auto frameRate = static_cast<FrameRate>(m_menu.GetItemValue< uint32_t >(UI_PARAM_FRAME_RATE));
        auto frameRateChanged = (m_framePacing.m_frameRateTarget != frameRate);
        m_framePacing.m_frameRateTarget = frameRate;

        auto frameThreshold = static_cast<uint32_t>(m_menu.GetItemValue<int>(UI_PARAM_FRAME_THRESHOLD));
        auto frameThresholdChanged = (m_framePacing.m_frameThreshold != frameThreshold);
        m_framePacing.m_frameThreshold = frameThreshold;

        auto framePeriod = static_cast<uint32_t>(m_menu.GetItemValue<int>(UI_PARAM_FRAME_PERIOD));
        auto framePeriodChanged = (m_framePacing.m_framePeriod != framePeriod);
        m_framePacing.m_framePeriod = framePeriod;

        auto frameBuffers = static_cast<uint32_t>(m_menu.GetItemValue<int>(UI_PARAM_FRAME_BUFFERS));
        auto frameBuffersChanged = (m_framePacing.m_frameBuffers != frameBuffers);
        m_framePacing.m_frameBuffers = frameBuffers;

        auto frameOffsetInMs = std::max(0.0f, m_menu.GetItemValue<float>(UI_PARAM_FRAME_OFFSET));
        m_menu.SetItemValue<float>(UI_PARAM_FRAME_OFFSET, frameOffsetInMs);
        auto frameOffsetInMsChanged = (m_framePacing.m_frameOffsetInMs != frameOffsetInMs);
        m_framePacing.m_frameOffsetInMs = frameOffsetInMs;

        if (frameRateChanged || framePeriodChanged || frameOffsetInMsChanged)
        {
            m_frameEventsDirty = true;
        }

        if (frameThresholdChanged)
        {
            // No action necessary here
        }

        if (frameBuffersChanged)
        {
            // No action necessary here
        }
    }
}
#pragma endregion

#pragma region Frame Render
// Draws the scene.
void Sample::Render()
{
    while (!m_isExiting)
    {
        // Wait for Update thread to signal that a game state is available
        {
            PIXScopedEvent(PIX_COLOR_DEFAULT, L"Wait on Update");
            auto result = WaitForSingleObject(m_eventUpdateDone, INFINITE);
            if (WAIT_OBJECT_0 != result)
            {
                throw std::runtime_error("Expected thread to receive signal");
            }
        }

        if (m_isExiting)
        {
            SetEvent(m_eventRenderReceived);
            break;
        }

        RecordFrameStatistics();

        // Statistics consume queues populated by the update thread. Release the
        // producer only after the render thread has finished reading them.
        if (!SetEvent(m_eventRenderReceived))
        {
            throw std::runtime_error("Expected event to get signaled");
        }

        AdvanceSwapChain();

        static_assert(_countof(c_intervalColors) == c_maxBackBufferCount);
        PIXBeginEvent(c_intervalColorsPacked[m_currentRenderFrameBuffer.m_frameBuffer], L"Render %d", m_currentRenderFrameBuffer.m_frame);

        m_frameTokens.push(m_currentRenderFrameBuffer.m_token);
        const auto& frameLoad = m_currentRenderFrameBuffer.m_frameLoad;

        // Start timer for artificial CPU load
        m_renderThreadTimeInMs = frameLoad.m_cpuToGpuTimeInMs;
        m_startCpuRenderSignal = true;

        // Record timestamp with frame token
        TaggedCpuTimestamp startCpuRender;
        startCpuRender.m_token = m_currentRenderFrameBuffer.m_token;
        startCpuRender.m_frameBuffer = m_currentRenderFrameBuffer.m_frameBuffer;
        if (!QueryPerformanceCounter(&startCpuRender.m_time))
        {
            throw std::runtime_error("QueryPerformanceCounter");
        }
        m_startCpuRender.push(startCpuRender);

        // Wait for artificial CPU load
        PIXBeginEvent(PIX_COLOR_DEFAULT, L"Artificial CPU to GPU lag");
        while (!m_endCpuRenderSignal && !m_isExiting)
        {
            _mm_pause();
        }
        if (m_isExiting)
        {
            PIXEndEvent();
            PIXEndEvent();
            break;
        }
        m_endCpuRenderSignal = false;
        PIXEndEvent();

        // Start timer for artificial CPU load
        m_renderThreadTimeInMs = std::max(0.0f, frameLoad.m_cpuRenderTimeInMs - frameLoad.m_cpuToGpuTimeInMs);
        m_startCpuRenderSignal = true;

        // Reset command list and allocator.
        auto commandAllocatorGraphicsStartTimer = m_commandAllocatorGraphicsStartTimer[m_currentRenderFrameBuffer.m_frameBuffer].Get();
        auto commandListGraphicsStartTimer = m_commandListGraphicsStartTimer[m_currentRenderFrameBuffer.m_frameBuffer].Get();
        DX::ThrowIfFailed(commandAllocatorGraphicsStartTimer->Reset());
        DX::ThrowIfFailed(commandListGraphicsStartTimer->Reset(commandAllocatorGraphicsStartTimer, nullptr));

        auto commandAllocatorGraphics = m_commandAllocatorGraphics[m_currentRenderFrameBuffer.m_frameBuffer].Get();
        auto commandListGraphics = m_commandListGraphics[m_currentRenderFrameBuffer.m_frameBuffer].Get();
        DX::ThrowIfFailed(commandAllocatorGraphics->Reset());
        DX::ThrowIfFailed(commandListGraphics->Reset(commandAllocatorGraphics, nullptr));

        auto commandAllocatorCompute = m_commandAllocatorCompute[m_currentRenderFrameBuffer.m_frameBuffer].Get();
        auto commandListCompute = m_commandListCompute[m_currentRenderFrameBuffer.m_frameBuffer].Get();
        DX::ThrowIfFailed(commandAllocatorCompute->Reset());
        DX::ThrowIfFailed(commandListCompute->Reset(commandAllocatorCompute, nullptr));

        auto commandQueueGraphics = m_commandQueueGraphics.Get();

        static_assert(_countof(c_intervalColors) == c_maxBackBufferCount);
        PIXBeginEvent(commandListGraphicsStartTimer, c_intervalColorsPacked[m_currentRenderFrameBuffer.m_frameBuffer], L"Graphics work %d", m_currentRenderFrameBuffer.m_frame);

        // Timestamp for GPU work
        m_queryCurrentFrame = (m_queryCurrentFrame + 1U) % c_queryFrames;
        auto queryIndex = m_queryCurrentFrame * c_queryCountPerFrame;
        commandListGraphicsStartTimer->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
        TaggedGpuTimestamp startGpuGraphics;
        startGpuGraphics.m_token = m_currentRenderFrameBuffer.m_token;
        startGpuGraphics.m_frameBuffer = m_currentRenderFrameBuffer.m_frameBuffer;
        startGpuGraphics.m_index = queryIndex++;
        m_startGpuGraphics.push(startGpuGraphics);

        // Start timer for artificial GPU graphics load
        BeginEnforceGpuTime(commandListGraphicsStartTimer, m_gpuTimeResourceGraphics.Get());

        // Kick off the timer work immediately
        DX::ThrowIfFailed(commandListGraphicsStartTimer->Close());
        commandQueueGraphics->ExecuteCommandLists(1, CommandListCast(&commandListGraphicsStartTimer));

        // Transition the render target into the correct state to allow for drawing into it.
        auto backBuffer = m_renderTargets[m_currentRenderFrameBuffer.m_frameBuffer].Get();
        D3D12_RESOURCE_BARRIER barrierPresentToRenderTarget = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandListGraphics->ResourceBarrier(1, &barrierPresentToRenderTarget);

        Clear();

        RenderUI();

        // Transition to UAV for write by async compute
        D3D12_RESOURCE_BARRIER barrierRenderTargetToUnorderedAccess = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        commandListGraphics->ResourceBarrier(1, &barrierRenderTargetToUnorderedAccess);

        // Wait for artificial GPU load
        EndEnforceGpuTime(commandListGraphics, m_gpuTimeResourceGraphics.Get(), frameLoad.m_gpuGraphicsTimeInMs);

        // Timestamp for GPU work
        commandListGraphics->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
        TaggedGpuTimestamp stopGpuGraphics;
        stopGpuGraphics.m_token = m_currentRenderFrameBuffer.m_token;
        stopGpuGraphics.m_frameBuffer = m_currentRenderFrameBuffer.m_frameBuffer;
        stopGpuGraphics.m_index = queryIndex++;
        m_stopGpuGraphics.push(stopGpuGraphics);

        // Graphics work for the frame ends here

        PIXEndEvent(commandListGraphics);

        DX::ThrowIfFailed(commandListGraphics->Close());

        // The timer can overlap the preceding frame's compute work. Wait before
        // the main graphics list starts using this frame's recycled back buffer.
        commandQueueGraphics->Wait(m_fenceComputeToGraphics.Get(), m_fenceComputeToGraphicsValue);
        commandQueueGraphics->ExecuteCommandLists(1, CommandListCast(&commandListGraphics));

        // Signal that graphics is done, compute can start
        commandQueueGraphics->Signal(m_fenceGraphicsToCompute.Get(), ++m_fenceGraphicsToComputeValue);

        auto commandQueueCompute = m_commandQueueCompute.Get();

        static_assert(_countof(c_intervalColors) == c_maxBackBufferCount);
        PIXBeginEvent(commandListCompute, c_intervalColorsPacked[m_currentRenderFrameBuffer.m_frameBuffer], L"Compute work %d", m_currentRenderFrameBuffer.m_frame);

        // Wait for graphics to be done before compute can start
        commandQueueCompute->Wait(m_fenceGraphicsToCompute.Get(), m_fenceGraphicsToComputeValue);

        // Timestamp for GPU work
        commandListCompute->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
        TaggedGpuTimestamp startGpuCompute;
        startGpuCompute.m_token = m_currentRenderFrameBuffer.m_token;
        startGpuCompute.m_frameBuffer = m_currentRenderFrameBuffer.m_frameBuffer;
        startGpuCompute.m_index = queryIndex++;
        m_startGpuCompute.push(startGpuCompute);

        // Start timer for artificial GPU graphics load 
        BeginEnforceGpuTime(commandListCompute, m_gpuTimeResourceCompute.Get());

        // Transition to Present state for the PresentX call
        D3D12_RESOURCE_BARRIER barrierUnorderedAccessToPresent = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PRESENT);
        commandListCompute->ResourceBarrier(1, &barrierUnorderedAccessToPresent);

        // Wait for artificial GPU load
        EndEnforceGpuTime(commandListCompute, m_gpuTimeResourceCompute.Get(), frameLoad.m_gpuComputeTimeInMs);

        // Timestamp for GPU work
        commandListCompute->EndQuery(m_queryHeap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
        TaggedGpuTimestamp stopGpuCompute;
        stopGpuCompute.m_token = m_currentRenderFrameBuffer.m_token;
        stopGpuCompute.m_frameBuffer = m_currentRenderFrameBuffer.m_frameBuffer;
        stopGpuCompute.m_index = queryIndex++;
        m_stopGpuCompute.push(stopGpuCompute);

        assert(queryIndex == (m_queryCurrentFrame + 1U) * c_queryCountPerFrame);
        commandListCompute->ResolveQueryData(m_queryHeap.Get(),
            D3D12_QUERY_TYPE_TIMESTAMP,
            m_queryCurrentFrame * c_queryCountPerFrame,
            c_queryCountPerFrame,
            m_queryBuffer.Get(),
            m_queryCurrentFrame * c_queryCountPerFrame * sizeof(uint64_t));

        PIXEndEvent(commandListCompute);

        // Compute work for the frame ends here
        DX::ThrowIfFailed(commandListCompute->Close());
        commandQueueCompute->ExecuteCommandLists(1, CommandListCast(&commandListCompute));

        // Signal that compute is done, graphics can start (2 frames from now)
        commandQueueCompute->Signal(m_fenceComputeToGraphics.Get(), ++m_fenceComputeToGraphicsValue);

        // Show the new frame.
        EndFrame();
        m_graphicsMemory->Commit(commandQueueGraphics);

        // Wait for any remaining artificial CPU load after the real render,
        // submission, presentation, and transient-memory work is complete.
        PIXBeginEvent(PIX_COLOR_DEFAULT, L"Artificial CPU load");
        while (!m_endCpuRenderSignal && !m_isExiting)
        {
            _mm_pause();
        }
        if (m_isExiting)
        {
            PIXEndEvent();
            PIXEndEvent();
            break;
        }
        m_endCpuRenderSignal = false;

        TaggedCpuTimestamp stopCpuRender;
        stopCpuRender.m_token = m_currentRenderFrameBuffer.m_token;
        stopCpuRender.m_frameBuffer = m_currentRenderFrameBuffer.m_frameBuffer;
        if (!QueryPerformanceCounter(&stopCpuRender.m_time))
        {
            throw std::runtime_error("QueryPerformanceCounter");
        }
        m_stopCpuRender.push(stopCpuRender);

        PIXEndEvent();
        PIXEndEvent();
    }
}

void Sample::RenderUI()
{
    std::scoped_lock uiLock(m_uiMutex);

    auto commandListGraphics = m_commandListGraphics[m_currentRenderFrameBuffer.m_frameBuffer].Get();

    ScopedPixEvent RenderUI(commandListGraphics, PIX_COLOR_DEFAULT, L"Render UI");

    m_uiSpriteBatch->Begin(commandListGraphics);    // This remains active for the rest of the text UI

    // Render predefined UI (background panels, etc.)
    {
        // Average the last few frames to see changes more easily
        constexpr uint32_t numGhostFrames = 5;

        auto updateMetric = [](Ring<float, numGhostFrames>& history, float value, const std::shared_ptr<UIStaticText>& text)
        {
            history.push_back(value);

            float average = 0.0f;
            for (auto sample : history)
            {
                average += sample;
            }
            average /= history.size();

            std::ostringstream displayText;
            displayText.precision(1);
            displayText << std::fixed << average;
            text->SetDisplayText(displayText.str());
        };

        static Ring<float, numGhostFrames> s_prevFrameRate;
        static Ring<float, numGhostFrames> s_prevLatency;
        static Ring<float, numGhostFrames> s_prevMargin;
        updateMetric(s_prevFrameRate, m_frameRateMeasured, m_fpsText);
        updateMetric(s_prevLatency, m_latencyMeasuredInMs, m_latencyText);
        updateMetric(s_prevMargin, m_marginMeasuredInMs, m_marginText);

        m_uiManager.Render();
    }   

    // Render tear indicator (lags by a few frames)
    {
        auto frameWidth = (m_outputSize.right - m_outputSize.left);
        auto frameHeight = (m_outputSize.bottom - m_outputSize.top);

        m_triEffect->Apply(commandListGraphics);
        m_primBatch->Begin(commandListGraphics);

        constexpr float indicatorWidthInPixels = 190.0f;
        constexpr float indicatorHeightInPixels = 2.0f;
        const float labelLineSpacing = m_smallFont->GetLineSpacing();
        const float labelHeightInPixels = 2.0f * labelLineSpacing;

        float indicatorWidth = 2.0f * indicatorWidthInPixels / frameWidth;
        float indicatorHeight = 2.0f * indicatorHeightInPixels / frameHeight;
        float tearLocation = std::clamp(m_tearLocation, 0.0f, 1.0f);
        float tearIndicatorY = 2.0f * (1.0f - tearLocation) - 1.0f;
        tearIndicatorY = std::clamp(tearIndicatorY, -1.0f + indicatorHeight / 2.0f, 1.0f - indicatorHeight / 2.0f);
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
        float backgroundHeight = 2.0f * labelHeightInPixels / frameHeight;
        float labelTopInPixels = std::clamp(tearLocation * frameHeight, 0.0f, frameHeight - labelHeightInPixels);
        float labelTop = 1.0f - 2.0f * labelTopInPixels / frameHeight;
        float labelBottom = labelTop - backgroundHeight;
        Vector4 backgroundColor(0.0980392156f, 0.0980392156f, 0.0980392156f, 0.95f);

        qvul.position.x = (qvll.position.x += 2.0f / frameWidth);
        qvul.position.y = qvur.position.y = labelTop;
        qvll.position.y = qvlr.position.y = labelBottom;
        qvul.color = qvur.color = qvll.color = qvlr.color = backgroundColor;

        m_primBatch->DrawQuad(qvul, qvll, qvlr, qvur);

        m_primBatch->End();

        int indicatorWidthPixels = static_cast<int>(m_screenViewport.Width * indicatorWidth / 2.0f);
        XMFLOAT2 textPos = XMFLOAT2(m_screenViewport.Width - indicatorWidthPixels + 10.0f, labelTopInPixels);

        m_smallFont->DrawString(m_uiSpriteBatch.get(), L"Tear Indicator", textPos, tearIndicatorColor, 0.0f, { 0.0f, 0.0f }, 1.0f);

        wchar_t tearPercentage[16] = {};
        swprintf_s(tearPercentage, L"%.0f%%", tearLocation * 100.0f);
        textPos.y += labelLineSpacing;
        m_smallFont->DrawString(m_uiSpriteBatch.get(), tearPercentage, textPos, tearIndicatorColor, 0.0f, { 0.0f, 0.0f }, 1.0f);
    }

    // Render menu
    {
        // Set descriptor heaps
        ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap(), m_commonStates->Heap() };
        commandListGraphics->SetDescriptorHeaps(uint32_t(_countof(heaps)), heaps);

        auto panelRect = m_menuPanel->GetMarginedRectInPixels();

        m_menu.Render(m_uiSpriteBatch.get(), m_smallFont.get(), m_ctrlFont.get(), { (float)panelRect.x, (float)panelRect.y, });
    }

    m_uiSpriteBatch->End();

    // Draw line graphs
    {
        for (uint32_t i = 0; i < _countof(m_lineGraph); ++i)
        {
            auto panelRect = m_lineGraphPanel[i]->GetMarginedRectInPixels();
            D3D12_VIEWPORT panelViewport =
            {
                (float)panelRect.x,             // float TopLeftX;
                (float)panelRect.y,             // float TopLeftY;
                (float)panelRect.width,         // float Width;
                (float)panelRect.height,        // float Height;
                0.0f,                           // float MinDepth;
                1.0f,                           // float MaxDepth;
            };
            m_lineGraph[i].Render(commandListGraphics, m_primBatch.get(), m_lineEffect.get(), m_uiSpriteBatch.get(), m_smallFont.get(), &m_screenViewport, &panelViewport);
        }
    }

    // Draw interval graphs
    {
        for (uint32_t i = 0; i < _countof(m_intervalGraph); ++i)
        {
            auto panelRect = m_intervalGraphPanel[i]->GetMarginedRectInPixels();
            D3D12_VIEWPORT panelViewport =
            {
                (float)panelRect.x,             // float TopLeftX;
                (float)panelRect.y,             // float TopLeftY;
                (float)panelRect.width,         // float Width;
                (float)panelRect.height,        // float Height;
                0.0f,                           // float MinDepth;
                1.0f,                           // float MaxDepth;
            };
            m_intervalGraph[i].Render(commandListGraphics, m_primBatch.get(), m_triEffect.get(), m_uiSpriteBatch.get(), m_smallFont.get(), &m_screenViewport, &panelViewport);
        }
    }
}

// Queries the frame statistics used to visualize and automatically tune frame pacing.
bool Sample::GetFrameStatistics(D3D12XBOX_FRAME_PIPELINE_TOKEN frameToken, FrameStatistics& stats)
{
    constexpr uint32_t expectedStatCount = 5;
    D3D12XBOX_FRAME_STATISTICS rawStats[expectedStatCount];
    D3D12XBOX_FRAME_STATISTICS_TYPE typeSet = D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_INTERVAL |
        D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_EVENT |
        D3D12XBOX_FRAME_STATISTICS_TYPE_RENDER |
        D3D12XBOX_FRAME_STATISTICS_TYPE_PRESENT |
        D3D12XBOX_FRAME_STATISTICS_TYPE_DISPLAY;
    assert(__popcnt(typeSet) == expectedStatCount);

    uint32_t statCount = expectedStatCount;
    HRESULT hr = m_device->GetFrameStatisticsX(
        frameToken,
        typeSet,
        &statCount,
        rawStats);

    if (hr == S_FALSE || statCount < expectedStatCount)
    {
        // Results not available yet
        return false;
    }

    DX::ThrowIfFailed(hr);

    assert(statCount == expectedStatCount);

    // PresentX submits one composited display plane, so only plane 0 belongs to this sample.
    for (auto& rawStat : rawStats)
    {
        switch (rawStat.Type)
        {
        case D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_INTERVAL:
            stats.interval = rawStat.Interval;
            break;
        case D3D12XBOX_FRAME_STATISTICS_TYPE_FRAME_EVENT:
            stats.origin = rawStat.Event;
            break;
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
        case D3D12XBOX_FRAME_STATISTICS_TYPE_INPUT:
#ifdef _GAMING_XBOX_SCARLETT
        case D3D12XBOX_FRAME_STATISTICS_TYPE_POWERSCALING:
#endif
        case D3D12XBOX_FRAME_STATISTICS_TYPE_NONE:
        default:
            assert(false);
        }
    }

    return true;
}

// Keeps a record of the frame statistics for graphing purposes.
// The automatic pacing policy consumes this history, while the graphs visualize the same data.
void Sample::RecordFrameStatistics()
{
    std::scoped_lock statisticsLock(m_frameStatisticsMutex);

    uint64_t totalFrameTime = 0;
    uint32_t completedFrameIntervals = 0;
    bool paused = m_paused;

    // The render/update event handshake prevents the update thread from
    // appending timestamps while this routine drains the shared queues.
    // Check all unprocessed frames, stop if we find an unfinished one
    while (m_frameTokens.size() > 0)
    {
        auto frameToken = m_frameTokens.front();

        FrameStatistics stats{};

        static auto firstToken = true;  // Driver doesn't provide statistics for the first presented frame
        if (!firstToken && !GetFrameStatistics(frameToken, stats))
        {
            // Results not available yet
            break;
        }
        m_frameTokens.pop();

        // Since frame statistics were ready, timestamps must also be ready
        auto startCpuUpdate = std::move(m_startCpuUpdate.front());
        m_startCpuUpdate.pop();
        assert(frameToken == startCpuUpdate.m_token);

        auto stopCpuUpdate = std::move(m_stopCpuUpdate.front());
        m_stopCpuUpdate.pop();
        assert(frameToken == stopCpuUpdate.m_token);

        auto startCpuRender = std::move(m_startCpuRender.front());
        m_startCpuRender.pop();
        assert(frameToken == startCpuRender.m_token);

        auto stopCpuRender = std::move(m_stopCpuRender.front());
        m_stopCpuRender.pop();
        assert(frameToken == stopCpuRender.m_token);

        auto startGpuGraphics = m_startGpuGraphics.front();
        m_startGpuGraphics.pop();
        assert(frameToken == startGpuGraphics.m_token);

        auto stopGpuGraphics = m_stopGpuGraphics.front();
        m_stopGpuGraphics.pop();
        assert(frameToken == stopGpuGraphics.m_token);

        auto startGpuCompute = m_startGpuCompute.front();
        m_startGpuCompute.pop();
        assert(frameToken == startGpuCompute.m_token);

        auto stopGpuCompute = m_stopGpuCompute.front();
        m_stopGpuCompute.pop();
        assert(frameToken == stopGpuCompute.m_token);

        if (firstToken)
        {
            firstToken = false;
            break;
        }

        if (!m_frameStatistics.empty())
        {
            totalFrameTime += stats.display.FlipTime - m_frameStatistics.back().display.FlipTime;
            ++completedFrameIntervals;
        }

        m_frameStatistics.push_back(stats);

        for (uint32_t lineGraphIndex = 0; lineGraphIndex < _countof(m_lineGraph); ++lineGraphIndex)
        {
            m_lineGraph[lineGraphIndex].ZoomX(m_graphZoom, m_graphOffset);
            if (!paused)
            {
                m_lineGraph[lineGraphIndex].Update(&m_frameStatistics.back());
            }
        }

        FrameTimestamps frameTimestamps;
        frameTimestamps.m_frameBuffer = startCpuUpdate.m_frameBuffer;
        assert(frameTimestamps.m_frameBuffer == stopCpuUpdate.m_frameBuffer);
        assert(frameTimestamps.m_frameBuffer == startCpuRender.m_frameBuffer);
        assert(frameTimestamps.m_frameBuffer == stopCpuRender.m_frameBuffer);
        assert(frameTimestamps.m_frameBuffer == startGpuGraphics.m_frameBuffer);
        assert(frameTimestamps.m_frameBuffer == stopGpuGraphics.m_frameBuffer);
        assert(frameTimestamps.m_frameBuffer == startGpuCompute.m_frameBuffer);
        assert(frameTimestamps.m_frameBuffer == stopGpuCompute.m_frameBuffer);

        frameTimestamps.m_flipTime = stats.display.FlipTime;

        frameTimestamps.m_startCpuUpdate = startCpuUpdate.m_time;
        frameTimestamps.m_stopCpuUpdate = stopCpuUpdate.m_time;
        frameTimestamps.m_startCpuRender = startCpuRender.m_time;
        frameTimestamps.m_stopCpuRender = stopCpuRender.m_time;

        // Update calibration between CPU and GPU timestamps
        DX::ThrowIfFailed(m_commandQueueGraphics->GetClockCalibration(&g_gpuAnchorTimestamp, &g_cpuAnchorTimestamp));

        D3D12_RANGE readRange =
        {
            startGpuGraphics.m_index * sizeof(uint64_t),
            (startGpuGraphics.m_index + c_queryCountPerFrame) * sizeof(uint64_t),
        };
        void* timestampData = nullptr;
        m_queryBuffer->Map(0U, &readRange, &timestampData);

        auto timestamps = reinterpret_cast<const uint64_t*>(timestampData) + startGpuGraphics.m_index;
        frameTimestamps.m_startGpuGraphics = *timestamps++;
        frameTimestamps.m_stopGpuGraphics = *timestamps++;
        frameTimestamps.m_startGpuCompute = *timestamps++;
        frameTimestamps.m_stopGpuCompute = *timestamps++;

        m_queryBuffer->Unmap(0U, nullptr);

        m_frameTimestamps.push_back(frameTimestamps);

        for (uint32_t intervalGraphIndex = 0; intervalGraphIndex < _countof(m_intervalGraph); ++intervalGraphIndex)
        {
            m_intervalGraph[intervalGraphIndex].ZoomX(m_graphZoom, m_graphOffset);
            if (!paused)
            {
                m_intervalGraph[intervalGraphIndex].Update(&m_frameTimestamps.back());
            }
        }
    }

    if (!paused && completedFrameIntervals > 0)
    {
        const auto& latestFrameStats = m_frameStatistics.back();
        m_tearLocation = std::clamp(latestFrameStats.display.PercentScanned / 100.0f, 0.0f, 1.0f);
        m_frameRateMeasured = static_cast<float>(
            static_cast<double>(completedFrameIntervals) * g_cpuPerfFreq / totalFrameTime);

        m_latencyMeasuredInMs = 1000.0f * static_cast<float>(
            (latestFrameStats.present.GPUProcessTime - latestFrameStats.origin.SignalTime) / static_cast<double>(g_cpuPerfFreq));

        auto frameOriginInMicroseconds = 1000.0 * 1000.0 * (latestFrameStats.origin.SignalTime / static_cast<double>(g_cpuPerfFreq));
        auto frameBudgetInMicroseconds = latestFrameStats.interval.LengthInMicroseconds * latestFrameStats.interval.PeriodInIntervals
            - latestFrameStats.origin.IntervalOffsetInMicroseconds;
        auto frameDeadlineInMicroseconds = frameOriginInMicroseconds + frameBudgetInMicroseconds;
        auto frameCompletionInMicroseconds = 1000.0 * 1000.0 * (latestFrameStats.present.GPUProcessTime / static_cast<double>(g_cpuPerfFreq));
        m_marginMeasuredInMs = static_cast<float>((frameDeadlineInMicroseconds - frameCompletionInMicroseconds) / 1000.0);
    }
}

// The algorithm to decide on frame pacing parameters automatically
void Sample::CalculateAutoPacing()
{
    std::scoped_lock statisticsLock(m_frameStatisticsMutex);

    // average the last N frame latencies
    const auto desiredHistoryCount = 8U;
    const auto minHistoryCount = 2U;
    const auto maxFrameLatencyInMicroseconds = 100.0f * 1000.0f;
    auto frameCount = 0U;
    auto averageFrameLatencyInMicroseconds = 0.0f;
    for (auto it = m_frameStatistics.crbegin(); it != m_frameStatistics.crend(); ++it)
    {
        if (frameCount == desiredHistoryCount)
        {
            break;
        }
        auto frameLatencyInMicroseconds = 1000.0f * 1000.0f * static_cast<float>(
            (it->present.GPUProcessTime - it->origin.SignalTime) / static_cast<double>(g_cpuPerfFreq));
        if (frameLatencyInMicroseconds > maxFrameLatencyInMicroseconds)
        {
            // We might have paused in the debugger or taken a PIX capture
            continue;
        }
        averageFrameLatencyInMicroseconds += frameLatencyInMicroseconds;
        ++frameCount;
    }
    if (frameCount < minHistoryCount)
    {
        // Not enough information
        return;
    }

    averageFrameLatencyInMicroseconds /= frameCount;

    // Use the historical average to calculate when the schedule the ORIGIN event.
    // For safety, let's try to finish 1/2 frame interval early.
    // Example:
    //     - We are running at 60 fps, and average frame latency is 35.0 ms.
    //     - We would like to finish 8.3 ms before the vblank.
    //     - So we need to start 35.0 + 8.3 ms before a vblank, or 43.3 ms.
    //     - That makes a period in intervals of 43.3 ms / 16.7 ms, rounded up to 3.
    //     - And an offset of 3 * 16.7 ms - 43.3 ms = 6.8 ms.
    uint32_t frameIntervalsInMicroseconds[] =
    {
        D3D12XBOX_FRAME_INTERVAL_120_HZ,
        D3D12XBOX_FRAME_INTERVAL_60_HZ,
        D3D12XBOX_FRAME_INTERVAL_40_HZ,
        D3D12XBOX_FRAME_INTERVAL_30_HZ,
    };
    static_assert(_countof(c_frameRateNames) == _countof(frameIntervalsInMicroseconds), "Mismatch between enums");
    auto frameIntervalInMicroseconds = frameIntervalsInMicroseconds[static_cast<uint32_t>(m_framePacing.m_frameRateTarget)];
    constexpr uint32_t maxFramePeriod = c_maxBackBufferCount - 1U;
    auto maxAllowedLatencyInMicroseconds = float(maxFramePeriod * frameIntervalInMicroseconds - 100U);
    auto latencyBufferInMicroseconds = frameIntervalInMicroseconds / 2UL;   // leave half a frame of latency margin
    auto paddedFrameLatencyInMicroseconds = averageFrameLatencyInMicroseconds + latencyBufferInMicroseconds;
    paddedFrameLatencyInMicroseconds = std::min(paddedFrameLatencyInMicroseconds, maxAllowedLatencyInMicroseconds);
    m_framePacing.m_framePeriod = static_cast<uint32_t>(paddedFrameLatencyInMicroseconds) / frameIntervalInMicroseconds + 1U;
    m_framePacing.m_frameOffsetInMs = (m_framePacing.m_framePeriod * frameIntervalInMicroseconds - paddedFrameLatencyInMicroseconds) / 1000.0f;
    m_framePacing.m_frameBuffers = m_framePacing.m_framePeriod + 1U;

}

// Helper method to clear the back buffers.
void Sample::Clear()
{
    auto commandListGraphics = m_commandListGraphics[m_currentRenderFrameBuffer.m_frameBuffer].Get();
    PIXBeginEvent(commandListGraphics, PIX_COLOR_DEFAULT, L"Clear");

    // Clear the views.
    auto rtvDescriptor = GetRenderTargetView(m_currentRenderFrameBuffer.m_frameBuffer);

    commandListGraphics->OMSetRenderTargets(1, &rtvDescriptor, FALSE, nullptr);
    commandListGraphics->ClearRenderTargetView(rtvDescriptor, ATG::Colors::Background, 0, nullptr);

    // Set the viewport and scissor rect.
    commandListGraphics->RSSetViewports(1, &m_screenViewport);
    commandListGraphics->RSSetScissorRects(1, &m_scissorRect);

    PIXEndEvent(commandListGraphics);
}

// Start a timer to enforce a minimum of GPU busy time
void Sample::BeginEnforceGpuTime(GraphicsCommandList* commandList, ID3D12Resource* bufTime)
{
    // Be sure to kick this off immediately to start the countdown
    ScopedPixEvent Workload(commandList, 0, L"Get GPU start time");

    commandList->SetComputeRootSignature(m_getStartTimeRS.Get());
    commandList->SetComputeRootUnorderedAccessView(0, bufTime->GetGPUVirtualAddress());
    commandList->SetPipelineState(m_getStartTimePSO.Get());

    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(bufTime);
    commandList->ResourceBarrier(1, &barrier);
    commandList->Dispatch(1, 1, 1);
    commandList->ResourceBarrier(1, &barrier);
}

// Wait on a timer to enforce a minimum of GPU busy time
void Sample::EndEnforceGpuTime(GraphicsCommandList* commandList, ID3D12Resource* bufTime, float durationInMs)
{
    ScopedPixEvent Workload(commandList, 0, L"Artificial GPU load");

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

    auto desiredFrameTimeInTicks = static_cast<uint64_t>(durationInMs * 1000.0f) * clockRate;

    commandList->SetComputeRootSignature(m_gpuEnforceTimeRS.Get());
    commandList->SetComputeRoot32BitConstants(0, 2, &desiredFrameTimeInTicks, 0);
    commandList->SetComputeRootUnorderedAccessView(1, bufTime->GetGPUVirtualAddress());
    commandList->SetPipelineState(m_gpuEnforceTimePSO.Get());

    auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(bufTime);
    commandList->ResourceBarrier(1, &barrier);
    commandList->Dispatch(1, 1, 1);
    commandList->ResourceBarrier(1, &barrier);
}

#pragma endregion

#pragma region Message Handlers
// Message handlers
void Sample::OnSuspending()
{
    m_commandQueueGraphics->SuspendX(0);
    m_commandQueueCompute->SuspendX(0);
}

void Sample::OnResuming()
{
    m_commandQueueGraphics->ResumeX();
    m_commandQueueCompute->ResumeX();

    {
        std::scoped_lock settingsLock(m_settingsMutex);
        RegisterFrameEvents();
        m_frameEventsDirty = false;
    }
    m_timer.ResetElapsedTime();
    m_gamePadButtons.Reset();
    m_inputState.Reset();
}
#pragma endregion

#pragma region Direct3D Resources
// These are the resources that depend on the device.
void Sample::CreateDeviceDependentResources()
{
    // Create the DX12 API device object.
    D3D12XBOX_CREATE_DEVICE_PARAMETERS params = {};
    params.Version = D3D12_SDK_VERSION;

#if defined(_DEBUG)
    // Enable the debug layer.
    params.ProcessDebugFlags = D3D12_PROCESS_DEBUG_FLAG_DEBUG_LAYER_ENABLED;
#elif defined(PROFILE)
    // Enable the instrumented driver.
    params.ProcessDebugFlags = D3D12XBOX_PROCESS_DEBUG_FLAG_INSTRUMENTED;
#endif

    params.GraphicsCommandQueueRingSizeBytes = static_cast<uint32_t>(D3D12XBOX_DEFAULT_SIZE_BYTES);
    params.GraphicsScratchMemorySizeBytes = static_cast<uint32_t>(D3D12XBOX_DEFAULT_SIZE_BYTES);
    params.ComputeScratchMemorySizeBytes = static_cast<uint32_t>(D3D12XBOX_DEFAULT_SIZE_BYTES);
#ifdef _GAMING_XBOX_SCARLETT
    params.CreateDeviceFlags = D3D12XBOX_CREATE_DEVICE_FLAG_NONE;
#endif

    HRESULT hr = D3D12XboxCreateDevice(
        nullptr,
        &params,
        IID_GRAPHICS_PPV_ARGS(m_device.ReleaseAndGetAddressOf()));
#ifdef _DEBUG
    if (hr == D3D12_ERROR_DRIVER_VERSION_MISMATCH)
    {
#ifdef _GAMING_XBOX_SCARLETT
        OutputDebugStringA("ERROR: Running a d3d12_xs.lib (Xbox Series X|S) linked binary on an Xbox One is not supported\n");
#else
        OutputDebugStringA("ERROR: Running a d3d12_x.lib (Xbox One) linked binary on a Xbox Series X|S in 'Scarlett' mode is not supported\n");
#endif
    }
#endif
    DX::ThrowIfFailed(hr);
    m_device->SetName(L"FramePacing device");

    auto device = m_device.Get();

    // Create the command queue.
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        DX::ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_GRAPHICS_PPV_ARGS(m_commandQueueGraphics.ReleaseAndGetAddressOf())));

        m_commandQueueGraphics->SetName(L"Command queue graphics");
    }
    {
        D3D12XBOX_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
        queueDesc.EngineOrPipeIndex = 0U;
        queueDesc.QueueIndex = 0U;

        DX::ThrowIfFailed(device->CreateCommandQueueX(&queueDesc, IID_GRAPHICS_PPV_ARGS(m_commandQueueCompute.ReleaseAndGetAddressOf())));

        m_commandQueueCompute->SetName(L"Command queue compute");
    }

    auto commandQueueGraphics = m_commandQueueGraphics.Get();

    // Create descriptor heaps for render target views and depth stencil views.
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.NumDescriptors = c_maxBackBufferCount;
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

    DX::ThrowIfFailed(device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_GRAPHICS_PPV_ARGS(m_rtvDescriptorHeap.ReleaseAndGetAddressOf())));

    m_rtvDescriptorHeap->SetName(L"Render target descriptor heap");

    m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    // Create a command allocator for each back buffer that will be rendered to.
    for (uint32_t n = 0; n < c_maxBackBufferCount; n++)
    {
        wchar_t name[256] = {};

        // Create a command list for recording graphics commands.
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocatorGraphics[n].ReleaseAndGetAddressOf())));

        swprintf_s(name, L"Command allocator graphics %u", n);
        m_commandAllocatorGraphics[n]->SetName(name);

        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorGraphics[n].Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandListGraphics[n].ReleaseAndGetAddressOf())));
        DX::ThrowIfFailed(m_commandListGraphics[n]->Close());

        swprintf_s(name, L"Command list graphics %u", n);
        m_commandListGraphics[n]->SetName(name);

        // Create a separate command list for immediate kickoff of timers.
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_GRAPHICS_PPV_ARGS(m_commandAllocatorGraphicsStartTimer[n].ReleaseAndGetAddressOf())));

        swprintf_s(name, L"Command allocator graphics start timer %u", n);
        m_commandAllocatorGraphicsStartTimer[n]->SetName(name);


        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocatorGraphicsStartTimer[n].Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandListGraphicsStartTimer[n].ReleaseAndGetAddressOf())));
        DX::ThrowIfFailed(m_commandListGraphicsStartTimer[n]->Close());

        swprintf_s(name, L"Command list graphics start timer %u", n);
        m_commandListGraphicsStartTimer[n]->SetName(name);
    }
    for (uint32_t n = 0; n < c_maxBackBufferCount; n++)
    {
        DX::ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_GRAPHICS_PPV_ARGS(m_commandAllocatorCompute[n].ReleaseAndGetAddressOf())));

        wchar_t name[256] = {};
        swprintf_s(name, L"Command allocator compute %u", n);
        m_commandAllocatorCompute[n]->SetName(name);

        // Create a command list for recording compute commands.
        DX::ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, m_commandAllocatorCompute[n].Get(), nullptr, IID_GRAPHICS_PPV_ARGS(m_commandListCompute[n].ReleaseAndGetAddressOf())));
        DX::ThrowIfFailed(m_commandListCompute[n]->Close());

        swprintf_s(name, L"Command compute list %u", n);
        m_commandListCompute[n]->SetName(name);
    }

    // Create a fence for tracking GPU execution progress.
    DX::ThrowIfFailed(device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fence.ReleaseAndGetAddressOf())));
    m_fenceValue++;

    m_fence->SetName(L"Queue drain fence");

    m_fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
    if (!m_fenceEvent.IsValid())
    {
        throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "CreateEventEx");
    }

    // By default, validation may wait on preceding GPU work at ExecuteCommandLists.
    // Those waits distort frame timings, which defeats some of the features of the sample.
    // But it's worth leaving this flag enabled in typical validated builds of your title.
    auto flags = device->GetDebugFlagsX();
    flags |= D3D12XBOX_DEBUG_FLAG_ENABLE_OUT_OF_ORDER_CMD_LIST_VALIDATION;
    device->SetDebugFlagsX(flags);

    m_graphicsMemory = std::make_unique<GraphicsMemory>(device);

    // State objects
    m_commonStates = std::make_unique<DirectX::CommonStates>(device);

    // Create heap
    m_srvPile = std::make_unique<DescriptorPile>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        128,
        SRVDescriptorHeapIndex::SRV_Count);

    // Set up fences to synchronize between graphics and compute work
    m_fenceGraphicsToComputeValue = 0ULL;
    DX::ThrowIfFailed(device->CreateFence(m_fenceGraphicsToComputeValue, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fenceGraphicsToCompute.ReleaseAndGetAddressOf())));
    m_fenceGraphicsToCompute->SetName(L"Fence graphics to compute");

    m_fenceComputeToGraphicsValue = 0ULL;
    DX::ThrowIfFailed(device->CreateFence(m_fenceComputeToGraphicsValue, D3D12_FENCE_FLAG_NONE, IID_GRAPHICS_PPV_ARGS(m_fenceComputeToGraphics.ReleaseAndGetAddressOf())));
    m_fenceComputeToGraphics->SetName(L"Fence compute to graphics");

    auto styleRenderer = std::make_unique<UIStyleRendererD3D>(*this, m_outputSize.right, m_outputSize.bottom);
    m_uiManager.GetStyleManager().InitializeStyleRenderer(std::move(styleRenderer));

    {
        ResourceUploadBatch resourceUpload(device);
        resourceUpload.Begin();

        auto backBufferRts = RenderTargetState(m_backBufferFormat, DXGI_FORMAT_UNKNOWN);
        auto spritePSD = SpriteBatchPipelineStateDescription(backBufferRts, &CommonStates::AlphaBlend);
        m_uiSpriteBatch = std::make_unique<SpriteBatch>(device, resourceUpload, spritePSD);

        auto finished = resourceUpload.End(commandQueueGraphics);
        finished.wait();

        m_primBatch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(device);

        EffectPipelineStateDescription effectPSD(
            &VertexPositionColor::InputLayout,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            backBufferRts);
        m_triEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, effectPSD);

        effectPSD.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        m_lineEffect = std::make_unique<BasicEffect>(device, EffectFlags::VertexColor, effectPSD);
    }

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

        auto finished = resourceUpload.End(commandQueueGraphics);
        finished.wait();
    }

    // Timestamp queries
    {
        DX::ThrowIfFailed(m_commandQueueGraphics->GetTimestampFrequency(&g_gpuPerfFreq));

        // Need enough for 4 timestamps per frame and as many frames as could be in flight
        D3D12_QUERY_HEAP_DESC queryHeapDesc =
        {
            D3D12_QUERY_HEAP_TYPE_TIMESTAMP,        // D3D12_QUERY_HEAP_TYPE Type;
            c_queryCountPerFrame * c_queryFrames,           // UINT Count;
                                                    // UINT NodeMask;
        };
        DX::ThrowIfFailed(m_device->CreateQueryHeap(&queryHeapDesc,
            IID_GRAPHICS_PPV_ARGS(m_queryHeap.GetAddressOf())));

        D3D12_RESOURCE_DESC queryBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(queryHeapDesc.Count * sizeof(uint64_t));
        const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        DX::ThrowIfFailed(
            device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &queryBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_queryBuffer.ReleaseAndGetAddressOf())));
        m_queryBuffer->SetName(L"Query Buffer");
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Sample::CreateWindowSizeDependentResources()
{
    if (!m_window)
    {
        throw std::logic_error("Call SetWindow with a valid Win32 window handle");
    }

    // Wait until all previous GPU work is complete.
    WaitForGpu();

    // Ensure we present a blank screen before cleaning up resources.
    DX::ThrowIfFailed(m_commandQueueGraphics->PresentX(0, nullptr, nullptr));
    DX::ThrowIfFailed(m_commandQueueCompute->PresentX(0, nullptr, nullptr));

    // Release resources that are tied to the swap chain and update fence values.
    for (uint32_t n = 0; n < c_maxBackBufferCount; n++)
    {
        m_renderTargets[n].Reset();
    }

    // Determine the render target size in pixels.
    const uint32_t backBufferWidth = std::max<uint32_t>(static_cast<uint32_t>(m_outputSize.right - m_outputSize.left), 1u);
    const uint32_t backBufferHeight = std::max<uint32_t>(static_cast<uint32_t>(m_outputSize.bottom - m_outputSize.top), 1u);

    // Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
    CD3DX12_HEAP_PROPERTIES swapChainHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    D3D12_RESOURCE_DESC swapChainBufferDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        m_backBufferFormat,
        backBufferWidth,
        backBufferHeight,
        1, // This resource has only one texture.
        1  // Use a single mipmap level.
    );
    swapChainBufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_CLEAR_VALUE swapChainOptimizedClearValue = {};
    swapChainOptimizedClearValue.Format = m_backBufferFormat;

    for (uint32_t n = 0; n < c_maxBackBufferCount; n++)
    {
        DX::ThrowIfFailed(m_device->CreateCommittedResource(
            &swapChainHeapProperties,
            D3D12_HEAP_FLAG_ALLOW_DISPLAY,
            &swapChainBufferDesc,
            D3D12_RESOURCE_STATE_PRESENT,
            &swapChainOptimizedClearValue,
            IID_GRAPHICS_PPV_ARGS(m_renderTargets[n].GetAddressOf())));

        wchar_t name[25] = {};
        swprintf_s(name, L"Render target %u", n);
        m_renderTargets[n]->SetName(name);

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = m_backBufferFormat;
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDescriptor(
            m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            static_cast<INT>(n), m_rtvDescriptorSize);
        m_device->CreateRenderTargetView(m_renderTargets[n].Get(), &rtvDesc, rtvDescriptor);
    }

    // Set the 3D rendering viewport and scissor rectangle to target the entire window.
    m_screenViewport.TopLeftX = m_screenViewport.TopLeftY = 0.f;
    m_screenViewport.Width = static_cast<float>(backBufferWidth);
    m_screenViewport.Height = static_cast<float>(backBufferHeight);
    m_screenViewport.MinDepth = D3D12_MIN_DEPTH;
    m_screenViewport.MaxDepth = D3D12_MAX_DEPTH;

    m_scissorRect.left = m_scissorRect.top = 0;
    m_scissorRect.right = static_cast<LONG>(backBufferWidth);
    m_scissorRect.bottom = static_cast<LONG>(backBufferHeight);

    m_uiManager.SetWindowSize(m_outputSize.right, m_outputSize.bottom);

    // Fixed viewport for UI rendering
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(m_outputSize.right);
    viewport.Height = static_cast<float>(m_outputSize.bottom);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    // Set UI sprite viewport
    m_uiSpriteBatch->SetViewport(viewport);
}
#pragma endregion

#pragma region Frame Scheduling
// Set frame interval and register for frame events
void Sample::RegisterFrameEvents()
{
    auto device = m_device.Get();

    uint32_t frameIntervalsInMicroseconds[] =
    {
        D3D12XBOX_FRAME_INTERVAL_120_HZ,
        D3D12XBOX_FRAME_INTERVAL_60_HZ,
        D3D12XBOX_FRAME_INTERVAL_40_HZ,
        D3D12XBOX_FRAME_INTERVAL_30_HZ,
    };
    static_assert(_countof(c_frameRateNames) == _countof(frameIntervalsInMicroseconds), "Mismatch between enums");
    auto frameIntervalInMicroseconds = frameIntervalsInMicroseconds[static_cast<uint32_t>(m_framePacing.m_frameRateTarget)];
    auto periodInIntervals = m_framePacing.m_framePeriod;
    auto intervalOffsetInMicroseconds = static_cast<uint32_t>(1000.0f * m_framePacing.m_frameOffsetInMs);

    // Set frame interval and register for frame events
    DX::ThrowIfFailed(device->SetFrameIntervalX(
        nullptr,
        frameIntervalInMicroseconds,
        periodInIntervals,
        D3D12XBOX_FRAME_INTERVAL_FLAG_NONE));

    DX::ThrowIfFailed(device->ScheduleFrameEventX(
        D3D12XBOX_FRAME_EVENT_ORIGIN,
        intervalOffsetInMicroseconds,
        nullptr,
        D3D12XBOX_SCHEDULE_FRAME_EVENT_FLAG_NONE));
}

// Prepare to render the next frame.
void Sample::AdvanceSwapChain()
{
    // Update the back buffer index.
    std::scoped_lock swapChainLock(m_swapChainMutex);
    m_currentRenderFrameBuffer = m_swapChain.front();
    m_swapChain.pop();
}

// Prepare the command list and render target for rendering.
void Sample::BeginFrame()
{
    auto device = m_device.Get();
    FramePacing framePacing{};
    {
        std::scoped_lock settingsLock(m_settingsMutex);
        if (m_frameEventsDirty)
        {
            RegisterFrameEvents();
            m_frameEventsDirty = false;
        }
        framePacing = m_framePacing;
    }
    const auto frameLoad = GetFrameLoad(m_nextFrameIndex);

    // Wait until frame start is signaled
    D3D12XBOX_FRAME_PIPELINE_TOKEN framePipelineToken = D3D12XBOX_FRAME_PIPELINE_TOKEN_NULL;
    DX::ThrowIfFailed(device->WaitFrameEventX(D3D12XBOX_FRAME_EVENT_ORIGIN, INFINITE, nullptr, D3D12XBOX_WAIT_FRAME_EVENT_FLAG_NONE, &framePipelineToken));

    {
        std::scoped_lock swapChainLock(m_swapChainMutex);
        m_currentUpdateFrameBuffer.m_frameBuffer = m_nextFrameBufferIndex;
        m_currentUpdateFrameBuffer.m_token = framePipelineToken;
        m_currentUpdateFrameBuffer.m_frame = m_nextFrameIndex;
        m_currentUpdateFrameBuffer.m_frameLoad = frameLoad;
        m_currentUpdateFrameBuffer.m_framePacing = framePacing;
        m_swapChain.push(m_currentUpdateFrameBuffer);
    }

    m_nextFrameBufferIndex = (m_nextFrameBufferIndex + 1) % framePacing.m_frameBuffers;
    ++m_nextFrameIndex;
}

// Present the contents of the swap chain to the screen.
void Sample::EndFrame()
{
    PIXBeginEvent(PIX_COLOR_DEFAULT, L"Present");
    D3D12XBOX_PRESENT_PARAMETERS params = {};

    // A late frame is displayed immediately only while scanout is within this
    // percentage of the screen. Zero disables limited tearing; 100 permits a
    // late flip anywhere in the scan.
    params.ImmediateThresholdPercent = static_cast<float>(m_currentRenderFrameBuffer.m_framePacing.m_frameThreshold);

    // Present the backbuffer using the PresentX API.
    D3D12XBOX_PRESENT_PLANE_PARAMETERS planeParameters = {};
    planeParameters.Token = m_currentRenderFrameBuffer.m_token;
    planeParameters.ResourceCount = 1;
    ID3D12Resource* backBuffer = m_renderTargets[m_currentRenderFrameBuffer.m_frameBuffer].Get();
    planeParameters.ppResources = &backBuffer;

    // Present from the Compute queue!
    auto commandQueueCompute = m_commandQueueCompute.Get();
    DX::ThrowIfFailed(commandQueueCompute->PresentX(1, &planeParameters, &params));

    PIXEndEvent();
}

// Wait for pending GPU work to complete.
void Sample::WaitForGpu() noexcept
{
    if (!m_fence || !m_fenceEvent.IsValid())
    {
        return;
    }

    // Drain the queues serially so shared fence values remain monotonic.
    ID3D12CommandQueue* queues[] = { m_commandQueueGraphics.Get(), m_commandQueueCompute.Get() };
    for (auto queue : queues)
    {
        if (queue && SUCCEEDED(queue->Signal(m_fence.Get(), m_fenceValue)))
        {
            if (SUCCEEDED(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent.Get())))
            {
                WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
                m_fenceValue++;
            }
        }
    }
}
#pragma endregion
