//--------------------------------------------------------------------------------------
// FramePacing.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "CommonStates.h"
#include "Menu.h"
#include "StepTimer.h"
#include "Graph.h"
#include "UITK.h"
#include "Utility.h"


#ifdef _GAMING_XBOX_SCARLETT
using Device = ID3D12Device8;
using GraphicsCommandList = ID3D12GraphicsCommandList5;
#else
using Device = ID3D12Device2;
using GraphicsCommandList = ID3D12GraphicsCommandList;
#endif

class Sample final : public ATG::UITK::D3DResourcesProvider
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return false; }

    // ATG::UITK::D3DResourcesProvider
    ID3D12Device* GetD3DDevice() override { return m_device.Get(); }
    ID3D12CommandQueue* GetCommandQueue() const override { return m_commandQueueGraphics.Get(); }
    ID3D12GraphicsCommandList* GetCommandList() const override { return m_commandListGraphics[m_currentRenderFrameBuffer.m_frameBuffer].Get(); }

private:
    void CreateThreads();
    void CreateWorkers();
    void CreateGraphs();
    void CreateFrameLoadSequences();

    void InitializeUI();

    void Update(DX::StepTimer const& timer);
    void UpdateUI();
    void Render();
    void RenderUI();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void EnforceCpuUpdateTime();
    void EnforceCpuRenderTime();

    void BeginEnforceGpuTime(GraphicsCommandList* commandList, ID3D12Resource* bufTime);
    void EndEnforceGpuTime(GraphicsCommandList* commandList, ID3D12Resource* bufTime, float durationInMs);

    void RegisterFrameEvents();
    void AdvanceSwapChain();
    void BeginFrame();
    void EndFrame();
    void WaitForGpu() noexcept;

    bool GetFrameStatistics(D3D12XBOX_FRAME_PIPELINE_TOKEN frameToken, FramePacingUtils::FrameStatistics& stats);
    void RecordFrameStatistics();

    void CalculateAutoPacing();

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(uint32_t index) const noexcept
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(index), m_rtvDescriptorSize);
    }

    static constexpr uint32_t c_maxBackBufferCount = 4;

    // Direct3D objects.
    Microsoft::WRL::ComPtr<Device>                  m_device;
    Microsoft::WRL::ComPtr<GraphicsCommandList>     m_commandListGraphics[c_maxBackBufferCount];
    Microsoft::WRL::ComPtr<GraphicsCommandList>     m_commandListGraphicsStartTimer[c_maxBackBufferCount];
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>      m_commandQueueGraphics;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  m_commandAllocatorGraphics[c_maxBackBufferCount];
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  m_commandAllocatorGraphicsStartTimer[c_maxBackBufferCount];

    Microsoft::WRL::ComPtr<GraphicsCommandList>     m_commandListCompute[c_maxBackBufferCount];
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>      m_commandQueueCompute;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>  m_commandAllocatorCompute[c_maxBackBufferCount];

    // Swap chain objects.
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_renderTargets[c_maxBackBufferCount];

    // Presentation fence objects.
    Microsoft::WRL::ComPtr<ID3D12Fence>             m_fence;
    uint64_t                                        m_fenceValue;
    Microsoft::WRL::Wrappers::Event                 m_fenceEvent;

    // Direct3D rendering objects.
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_rtvDescriptorHeap;
    uint32_t                                        m_rtvDescriptorSize;
    D3D12_VIEWPORT                                  m_screenViewport;
    D3D12_RECT                                      m_scissorRect;

    // Direct3D properties.
    DXGI_FORMAT                                     m_backBufferFormat;

    // Cached device properties.
    HWND                                            m_window;
    RECT                                            m_outputSize;

    // Console mode
    XSystemDeviceType                               m_deviceType;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    struct FrameLoad
    {
        float m_cpuUpdateTimeInMs;
        float m_cpuRenderTimeInMs;
        float m_cpuToGpuTimeInMs;                   // Lag in kicking off work
        float m_gpuGraphicsTimeInMs;
        float m_gpuComputeTimeInMs;
    };

    struct FramePacing
    {
        FramePacingUtils::FrameRate                 m_frameRateTarget;
        uint32_t                                    m_frameThreshold;
        uint32_t                                    m_framePeriod;
        uint32_t                                    m_frameBuffers;
        float                                       m_frameOffsetInMs;
    };

    // Swap chain
    struct FrameBuffer
    {
        uint32_t                                    m_frame;                    // frame count
        D3D12XBOX_FRAME_PIPELINE_TOKEN              m_token;                    // frame token
        uint32_t                                    m_frameBuffer;              // backbuffer index
        FrameLoad                                   m_frameLoad;                // immutable workload for this frame
        FramePacing                                 m_framePacing;              // schedule used to acquire this frame
    };
    std::queue<FrameBuffer>                         m_swapChain;                // communication between main thread and render thread
    std::mutex                                      m_swapChainMutex;
    uint32_t                                        m_nextFrameBufferIndex;     // touched only by main thread
    uint32_t                                        m_nextFrameIndex;           // touched only by main thread
    FrameBuffer                                     m_currentUpdateFrameBuffer; // touched only by main thread
    FrameBuffer                                     m_currentRenderFrameBuffer; // touched only by render thread

    // UITK
    ATG::UITK::UIManager                            m_uiManager;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_fpsText;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_latencyText;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_marginText;
    ATG::UITK::UIInputState                         m_inputState;
    std::mutex                                      m_uiMutex;                  // protects UI state shared by update and render
    FramePacingUtils::Menu                          m_menu;
    std::shared_ptr<ATG::UITK::UIPanel>             m_menuPanel;
    FramePacingUtils::LineGraph                     m_lineGraph[FramePacingUtils::LINE_GRAPH_COUNT];
    std::shared_ptr<ATG::UITK::UIPanel>             m_lineGraphPanel[FramePacingUtils::LINE_GRAPH_COUNT];
    FramePacingUtils::IntervalGraph                 m_intervalGraph[FramePacingUtils::INTERVAL_GRAPH_COUNT];
    std::shared_ptr<ATG::UITK::UIPanel>             m_intervalGraphPanel[FramePacingUtils::INTERVAL_GRAPH_COUNT];
    std::unique_ptr<DirectX::SpriteBatch>           m_uiSpriteBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
    std::unique_ptr<DirectX::BasicEffect>           m_triEffect;
    std::unique_ptr<DirectX::BasicEffect>           m_lineEffect;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_primBatch;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::CommonStates>          m_commonStates;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;

    // CPU synchronization
    HANDLE                                          m_eventUpdateDone;
    HANDLE                                          m_eventRenderReceived;
    std::thread*                                    m_renderThread;

    // GPU synchronization
    Microsoft::WRL::ComPtr<ID3D12Fence>             m_fenceGraphicsToCompute;
    uint64_t                                        m_fenceGraphicsToComputeValue;
    Microsoft::WRL::ComPtr<ID3D12Fence>             m_fenceComputeToGraphics;
    uint64_t                                        m_fenceComputeToGraphicsValue;

    // Shaders to enforce GPU frame time
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_getStartTimePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_getStartTimeRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_gpuEnforceTimePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_gpuEnforceTimeRS;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_gpuTimeResourceGraphics;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_gpuTimeResourceCompute;

    // Threads to enforce CPU frame time
    std::thread*                                    m_enforceCpuUpdateTimeThread;
    std::thread*                                    m_enforceCpuRenderTimeThread;

    // Flag to ensure thread finishes cleanly at sample exit
    std::atomic<bool>                               m_isExiting;

    // CPU frame time signals
    std::atomic<bool>                               m_startCpuUpdateSignal;
    float                                           m_updateThreadTimeInMs;
    std::atomic<bool>                               m_endCpuUpdateSignal;
    std::atomic<bool>                               m_startCpuRenderSignal;
    float                                           m_renderThreadTimeInMs;
    std::atomic<bool>                               m_endCpuRenderSignal;

    // The adjustable processing loads
    FrameLoad                                       m_desiredFrameLoad;
    bool                                            m_autoFrameLoad;
    uint32_t                                        m_currentFrameLoadSequence;
    using FrameLoadSequence = std::vector<FrameLoad>;
    std::vector<FrameLoadSequence>                  m_frameLoadSequences;
    FrameLoad GetFrameLoad(uint32_t frameIndex) const;

    // The adjustable frame pacing options
    FramePacing                                     m_framePacing;
    bool                                            m_autoPacing;
    bool                                            m_frameEventsDirty;
    mutable std::mutex                              m_settingsMutex;

    // Frame statistics
    float                                           m_frameRateMeasured;
    float                                           m_latencyMeasuredInMs;
    float                                           m_marginMeasuredInMs;
    float                                           m_tearLocation;

    // Pause
    std::atomic<bool>                               m_paused;
    std::atomic<float>                              m_graphZoom;
    std::atomic<float>                              m_graphOffset;

    // Timestamps
    struct TaggedCpuTimestamp
    {
        D3D12XBOX_FRAME_PIPELINE_TOKEN              m_token;
        LARGE_INTEGER                               m_time;
        uint32_t                                    m_frameBuffer;
    };
    std::queue<TaggedCpuTimestamp>                  m_startCpuUpdate;
    std::queue<TaggedCpuTimestamp>                  m_stopCpuUpdate;
    std::queue<TaggedCpuTimestamp>                  m_startCpuRender;
    std::queue<TaggedCpuTimestamp>                  m_stopCpuRender;

    Microsoft::WRL::ComPtr<ID3D12QueryHeap>         m_queryHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_queryBuffer;
    static constexpr uint32_t                       c_queryCountPerFrame = 4U;
    static constexpr uint32_t                       c_queryFrames = 8U;
    uint32_t                                        m_queryCurrentFrame;

    struct TaggedGpuTimestamp
    {
        D3D12XBOX_FRAME_PIPELINE_TOKEN              m_token;
        uint32_t                                    m_index;
        uint32_t                                    m_frameBuffer;
    };
    std::queue<TaggedGpuTimestamp>                  m_startGpuGraphics;
    std::queue<TaggedGpuTimestamp>                  m_stopGpuGraphics;
    std::queue<TaggedGpuTimestamp>                  m_startGpuCompute;
    std::queue<TaggedGpuTimestamp>                  m_stopGpuCompute;

    std::queue<D3D12XBOX_FRAME_PIPELINE_TOKEN>      m_frameTokens;
    std::mutex                                      m_frameStatisticsMutex;     // protects history used by automatic pacing
    FramePacingUtils::Ring<FramePacingUtils::FrameStatistics, 256> m_frameStatistics;
    FramePacingUtils::Ring<FramePacingUtils::FrameTimestamps, 256> m_frameTimestamps;
};
