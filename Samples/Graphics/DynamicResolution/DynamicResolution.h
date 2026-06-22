//--------------------------------------------------------------------------------------
// DynamicResolution.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "Menu.h"
#include "PerformanceTimersXbox.h"
#include "ResolutionSet.h"
#include "StepTimer.h"
#include "Utility.h"
#include "Timeline.h"

// UI toolkit
#include "UIManager.h"
#include "UIInputState.h"
#include "UIWidgets.h"
#include "UIStyleRendererD3D.h"

using namespace ATG::UITK;

namespace
{
    constexpr float c_vsyncsPerSecond = 59.94f; // target 60fps
}

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample : public D3DResourcesProvider
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
    void InitializeUI();
    bool CheckDeviceType();

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

    // UIStyleManager::D3DResourcesProvider interface methods
    virtual ID3D12Device* GetD3DDevice() override { return m_deviceResources->GetD3DDevice(); }
    virtual ID3D12CommandQueue* GetCommandQueue() const override { return m_deviceResources->GetCommandQueue(); }
    virtual ID3D12GraphicsCommandList* GetCommandList() const override
    {
        return m_deviceResources->GetCommandList();
    }

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderUI();

    void EnforceCpuTime();

    bool GetFrameStatistics(D3D12XBOX_FRAME_PIPELINE_TOKEN frameToken, FrameStatistics& stats);
    void RecordFrameStatistics();
    void DetermineResolution();

    void SetAndClearTargets(
        const D3D12_VIEWPORT* viewport,
        const D3D12_CPU_DESCRIPTOR_HANDLE* rtvDescriptor,
        const D3D12_CPU_DESCRIPTOR_HANDLE* dsvDescriptor,
        const float* colorRGBA);

    void CreateDeviceDependentResources();
    void CreateWorkers();
    void CreateTimelines();
    void CreateWindowSizeDependentResources();

    // Represents an instance of a scene object.
    struct ObjectInstance
    {
        using EffectList = DirectX::Model::EffectCollection;

        DirectX::SimpleMath::Matrix world;
        DirectX::Model*             model=nullptr;
        EffectList                  effects;
    };

    static constexpr uint32_t                       c_numBackBuffers = 3;

    XSystemDeviceType                               m_deviceType;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;
    std::unique_ptr<DX::GPUTimer>                   m_profiler;

    // Input device.
    ATG::UITK::UIInputState                         m_uiInputState;
    std::unique_ptr<DirectX::GamePad>               m_gamePad;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::CommonStates>          m_commonStates;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;
    
    // HUD
    ATG::UITK::UIManager                            m_uiManager;
    std::shared_ptr<ATG::UITK::UIPanel>             m_sidePanel;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_dynamicRezText;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_widthText;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_heightText;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_presetText;
    std::shared_ptr<ATG::UITK::UIStaticText>        m_fpsText;

    std::unique_ptr<DirectX::SpriteBatch>           m_uiSpriteBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
    std::unique_ptr<DirectX::BasicEffect>           m_triEffect;
    std::unique_ptr<DirectX::BasicEffect>           m_lineEffect;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_primBatch;
    Menu                                            m_menu;
    Timeline                                        m_timeline[TIMELINE_COUNT];

    // Camera
    float                                           m_cameraAngle;
    float                                           m_cameraElevation;
    float                                           m_cameraDistance;
    DirectX::SimpleMath::Matrix                     m_proj;
    DirectX::SimpleMath::Matrix                     m_view;

    // Assets & Scene                                    
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;
    std::vector<std::unique_ptr<DirectX::Model>>    m_models;
    std::vector<ObjectInstance>                     m_scene;

    // Sample settings
    bool                                            m_useDynamicResolution;
    bool                                            m_moveCamera;
    bool                                            m_showPanel;

    // The adjustable dimensions
    uint32_t                                        m_frameWidth;
    uint32_t                                        m_frameHeight;
    D3D12_VIEWPORT                                  m_frameViewportDynamic;
    D3D12_VIEWPORT                                  m_frameViewportIdeal;
    static constexpr uint32_t                       c_frameWidthIncrement = 1920 / 10;
    static constexpr uint32_t                       c_frameHeightIncrement = 0;
    static constexpr uint32_t                       c_frameWidthMinimum = 1280;
    static constexpr uint32_t                       c_frameHeightMinimum = 1080;
    static constexpr uint32_t                       c_frameWidthMaximum = 1920;
    static constexpr uint32_t                       c_frameHeightMaximum = 1080;

    // Resources for the possible resolutions
    ResolutionSet                                   m_resolutionSet;
    ResolutionData                                  m_currentRezData;

    // The adjustable processing loads
    float                                           m_desiredGpuFrameTimeInMs;
    float                                           m_desiredCpuFrameTimeInMs;

    // The functions which tell us whether to reduce/increase resolution and by how much
    float EstimateGpuPercentTimeRelativeToIdeal(uint32_t width, uint32_t height)
    {
        // Estimate performance of each resolution step relative to the default resolution.
        // This function can be heuristic, or based on measurements.
        // It should lean to the conservative side (higher than actual percent).
        float pctIdealPixelCount = (width * height) / (float)(c_frameWidthMaximum * c_frameHeightMaximum);

        // Estimate half fixed cost, half per-pixel cost
        return 0.5f + 0.5f * pctIdealPixelCount;
    }
    bool ReduceGpuToPercentTime(float percentTime, uint32_t& width, uint32_t& height)
    {
        float gpuPercentTimeRelativeToIdeal = EstimateGpuPercentTimeRelativeToIdeal(width, height);

        // Find the largest resolution which is predicted to reduce current GPU time to percentTime
        for (;
            width >= c_frameWidthMinimum && height >= c_frameHeightMinimum;
            width -= c_frameWidthIncrement, height -= c_frameHeightIncrement)
        {
            if (EstimateGpuPercentTimeRelativeToIdeal(width, height) / gpuPercentTimeRelativeToIdeal < percentTime)
            {
                return true;
            }
        }

        width = c_frameWidthMinimum;
        height = c_frameHeightMinimum;
        return false;
    }

    // Shaders to enforce GPU frame time
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_getStartTimePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_getStartTimeRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_gpuEnforceTimePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_gpuEnforceTimeRS;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_gpuTimeResource;

    // Thread to enforce CPU frame time
    std::thread*                                    m_cpuEnforceTimeThread;

    // Flag to ensure thread finishes cleanly at sample exit
    std::atomic<bool>                               m_isExiting;

    // CPU frame time signals
    std::atomic<bool>                               m_startCpuFrameSignal;
    std::atomic<bool>                               m_endCpuFrameSignal;

    // Frame statistics
    static constexpr uint32_t                       c_maxFramesCpuToFlipLatency = c_numBackBuffers;
    float                                           m_frameRate;
    float                                           m_tearLocation;

    Ring<D3D12XBOX_FRAME_PIPELINE_TOKEN, 8>         m_frameTokens;
    Ring<FrameStatistics, 256>                      m_frameStatistics;
    std::unordered_map<D3D12XBOX_FRAME_PIPELINE_TOKEN, FrameData> m_frameDataMap;
};
