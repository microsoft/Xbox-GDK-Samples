//--------------------------------------------------------------------------------------
// HistogramCS.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "PerformanceTimersXbox.h"
#include "FullScreenQuad\FullScreenQuad.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
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

    enum HistogramTechnique
    {
        HistUncoalescedBrute = 0,
        HistBrute,
        HistTGSMUncoalesced,
        HistTGSM,
        HistCount,
    };

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void RenderScene(ID3D12GraphicsCommandList* commandList);
    void RenderUI(ID3D12GraphicsCommandList* commandList);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;
    std::unique_ptr<DirectX::DescriptorHeap>    m_rtvDescriptorHeap;
    std::unique_ptr<DirectX::DescriptorHeap>    m_dsvDescriptorHeap;
    std::unique_ptr<DirectX::DescriptorHeap>    m_cpuDescriptors;
    std::unique_ptr<DirectX::CommonStates>      m_states;

    enum Descriptors : size_t
    {
        Backbuffer0,
        Backbuffer1,
        Backbuffer2,
        TextFont,
        ControllerFont,
        ColorCtrlFont,
        SRV_Histogram,
        UAV_Histogram,
        Count
    };

    // Controls
    float                                       m_pitch;
    float                                       m_yaw;

    DirectX::SimpleMath::Matrix                 m_proj;
    DirectX::SimpleMath::Matrix                 m_view;

    // Scene rendering
    std::unique_ptr<DirectX::EffectFactory>         m_fxFactory;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_modelResources;
    DirectX::Model::EffectCollection                m_modelEffects;
    std::unique_ptr<DirectX::Model>                 m_model;

    // UI rendering
    std::unique_ptr<DirectX::SpriteBatch>           m_batch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
    std::unique_ptr<DirectX::SpriteFont>            m_colorCtrlFont;

    // Histogram
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_csRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_histogramCS[HistCount];

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_histogram;

    HistogramTechnique                              m_technique;

    DX::GPUTimer                                    m_gpuTimer;

    // Histogram visualization
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_visualizeHistogram;
};
