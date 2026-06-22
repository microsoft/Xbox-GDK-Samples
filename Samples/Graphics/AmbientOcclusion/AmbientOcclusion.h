//--------------------------------------------------------------------------------------
// AmbientOcclusion.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
{
public:
    Sample() noexcept(false);
    ~Sample() = default;

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

    struct PipelineStates
    {
        enum
        {
            Fp32,
            Fp16,
            Count
        };
    };

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void BlurAndUpsample(uint32_t highResWidth, uint32_t highResHeight, uint32_t lowResWidth, uint32_t lowResHeight);
    void ComputeAO(const float TanHalfFovH, const uint32_t bufferWidth, const uint32_t bufferHeight, const uint32_t arrayCount);
    void RenderSSAO(uint32_t frameCount);
    void RenderGTAO();

    void RenderHUD();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // D3D12 Objects
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignatureZpp;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateZpp;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignatureLighting;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateLighting;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignatureSSAO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStatePrepareDepthBuffers1;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStatePrepareDepthBuffers2;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateBlurAndUpsample;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateBlurAndUpsampleHigh;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateBlurAndUpsampleFinal[PipelineStates::Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateBlurAndUpsampleFinalHigh[PipelineStates::Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateAoRender1[PipelineStates::Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateAoRender2[PipelineStates::Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateAoRender1SampleExhaustively;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateAoRender2SampleExhaustively;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignatureGTAO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStatePrefilterDepths;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGTAOLow[PipelineStates::Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGTAOMedium[PipelineStates::Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGTAOHigh[PipelineStates::Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGTAOUltra[PipelineStates::Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateDenoisePass;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateDenoiseLastPass;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGenerateNormals;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignatureAoTex;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateAoTex;
    std::unique_ptr<DirectX::DescriptorHeap>        m_srvHeap;
    std::unique_ptr<DX::Texture>                    m_depthDownsize1;
    std::unique_ptr<DX::Texture>                    m_depthDownsize2;
    std::unique_ptr<DX::Texture>                    m_depthDownsize3;
    std::unique_ptr<DX::Texture>                    m_depthDownsize4;
    std::unique_ptr<DX::Texture>                    m_depthTiled1;
    std::unique_ptr<DX::Texture>                    m_depthTiled2;
    std::unique_ptr<DX::Texture>                    m_depthTiled3;
    std::unique_ptr<DX::Texture>                    m_depthTiled4;
    std::unique_ptr<DX::Texture>                    m_aoMerged1;
    std::unique_ptr<DX::Texture>                    m_aoMerged2;
    std::unique_ptr<DX::Texture>                    m_aoMerged3;
    std::unique_ptr<DX::Texture>                    m_aoMerged4;
    std::unique_ptr<DX::Texture>                    m_aoSmooth1;
    std::unique_ptr<DX::Texture>                    m_aoSmooth2;
    std::unique_ptr<DX::Texture>                    m_aoSmooth3;
    std::unique_ptr<DX::Texture>                    m_aoHigh1;
    std::unique_ptr<DX::Texture>                    m_aoHigh2;
    std::unique_ptr<DX::Texture>                    m_aoHigh3;
    std::unique_ptr<DX::Texture>                    m_aoHigh4;
    std::unique_ptr<DX::Texture>                    m_aoFullRes;
    std::unique_ptr<DX::Texture>                    m_linearDepth;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_grayTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_hTileBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_depthPlane;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_depthOutput;
    std::unique_ptr<DX::Texture>                    m_normals;
    std::unique_ptr<DX::Texture>                    m_workingAO;
    std::unique_ptr<DX::Texture>                    m_workingAOTemp;
    std::unique_ptr<DX::Texture>                    m_workingEdges;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_hilbertLUT;
    uint32_t                                        m_hTileInfo;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;

    // Assets & Scene
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;
    std::unique_ptr<DirectX::Model>                 m_model;

    // Camera
    DX::FlyCamera                                   m_camera;

    // Options
    uint32_t                                        m_selectedOption;
    uint32_t                                        m_hierarchyDepth;
    uint32_t                                        m_ssaoQuality;
    float                                           m_noiseFilterTolerance;
    float                                           m_blurTolerance;
    float                                           m_upsampleTolerance;
    float                                           m_rejectionFalloff;
    float                                           m_accentuation;
    bool                                            m_showAOTexture;
    bool                                            m_applyAO;
    bool                                            m_useFp16;
    bool                                            m_sampleExhaustively;
    uint32_t                                        m_aoImplementation;
    XeGTAO::GTAOSettings                            m_settings;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_font;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
};
