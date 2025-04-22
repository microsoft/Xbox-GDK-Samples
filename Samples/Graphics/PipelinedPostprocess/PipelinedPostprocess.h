//--------------------------------------------------------------------------------------
// PipelinedPostprocess.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Utils.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

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
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void DrawHUD();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void CreatePipelineStateObjects();

    bool GetFrameStatistics(D3D12XBOX_FRAME_PIPELINE_TOKEN frameToken, FrameStatistics& stats);

#ifdef _GAMING_XBOX_SCARLETT
        void SimulateFakeGPUPass(ID3D12GraphicsCommandList7* cmdList, wchar_t const* name, float desiredGpuFrameTimeInUs);
        void GetGpuStartTime(ID3D12GraphicsCommandList7* cmdList);
        void PostProcessDispatch(ID3D12GraphicsCommandList7* cmdList, uint32_t backbufferUAVIndex);
#else
        void SimulateFakeGPUPass(ID3D12GraphicsCommandList* cmdList, wchar_t const* name, float desiredGpuFrameTimeInUs);
        void GetGpuStartTime(ID3D12GraphicsCommandList* cmdList);
        void PostProcessDispatch(ID3D12GraphicsCommandList* cmdList, uint32_t backbufferUAVIndex);
#endif

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    XSystemDeviceType                               m_deviceType;

    // Helper variables
    bool                                            m_isFirstPipelineCall;
    bool                                            m_usePipelinedPostProcess;
    bool                                            m_transition;
    bool                                            m_useFakeGPULoad;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;
    std::unique_ptr<DirectX::Model>                 m_model;
    std::unique_ptr<DirectX::Model>                 m_dragonModel;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_texFactory;
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_controllerFont;
    std::unique_ptr<DirectX::EffectFactory>         m_fxFactory;
    std::unique_ptr<DirectX::CommonStates>          m_states;
    std::unique_ptr<DX::CPUTimer>                   m_cpuTimer;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;
    std::unique_ptr<DX::GPUTimer>                   m_gpuComputeTimer;
    MovingAvg                                       m_frameDeltaTime;
    MovingAvg                                       m_GPUGeometryFrameTime;
    MovingAvg                                       m_GPUPostprocessFrameTime;

    // Effects
    DirectX::Model::EffectCollection                m_modelEffect;
    DirectX::Model::EffectCollection                m_dragonModelEffect;

    // Pipeline State Objects
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_depthPrePassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_FXAAPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_gpuEnforceTimePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_getStartTimePSO;

    // MSAA target
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_postProcessRT;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_postProcessDS;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_RTHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_DSHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_postprocessRTVCpuHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_postprocessDSVCpuHandle;

    // Root Signatures
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_defaultRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_computeRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_gpuEnforceTimeRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_getStartTimeRS;

    // ATGTK objects
    DX::FlyCamera                                   m_sceneCamera;
    uint32_t                                        m_displayWidth;
    uint32_t                                        m_displayHeight;
    DirectX::SimpleMath::Matrix                     m_modelMatrix;
    DirectX::SimpleMath::Matrix                     m_dragonModelMatrix;

    // Manual fences
    D3D12_GPU_VIRTUAL_ADDRESS                       m_fenceCmpToGfx;
    void*                                           m_fenceAddress;
    uint64_t                                        m_fenceCmpToGfxValue;

    //Constant buffers
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_prepassCBRes;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_prepassCBVA;
    SceneConstants*                                 m_prepassCBMappedMemory;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_fxaaCBRes;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_fxaaCBVA;
    FXAAConstantBuffer*                             m_fxaaCBMappedMemory;

    // Enforce GPU Time Resource
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_gpuTimeResource;
    float                                           m_fakeGPULoadGeometryUS;
    float                                           m_fakeGPULoadTimePostProcessUS;
    MovingAvg                                       m_originToPresentTimeMS;
};
