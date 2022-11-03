//--------------------------------------------------------------------------------------
// Antialiasing.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "MSAAHelper.h"
#include "RenderTexture.h"
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

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void RenderScene(ID3D12GraphicsCommandList* commandList);
    void RenderZoom(ID3D12GraphicsCommandList* commandList);
    void RenderUI(ID3D12GraphicsCommandList* commandList);
    void RenderSMAA(ID3D12GraphicsCommandList* commandList, size_t renderTargetSRV, size_t depthSRV, float blendFactor, D3D12_GPU_VIRTUAL_ADDRESS cb);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptors;
    std::unique_ptr<DirectX::DescriptorHeap>        m_renderDescriptors;
    std::unique_ptr<DirectX::CommonStates>          m_states;

    enum Descriptors
    {
        SceneTex,
        SceneTex2,
        DepthTex,
        DepthTex2,
        MSAAColorSRV,
        MSAADepthSRV,
        DepthStencilSRV,
        FinalTex,
        Pass1,
        Pass2,
        AreaTex,
        SearchTex,
        Font,
        CtrlFont,
        ColorCtrlFont,
        Count
    };

    enum RTDescriptors
    {
        SceneRTV,
        SceneRTV2,
        DepthRTV,
        DepthRTV2,
        FinalRTV,
        PassRTV1,
        PassRTV2,
        RTCount
    };

    // Controls
    float                                           m_pitch;
    float                                           m_yaw;
    bool                                            m_hardwareAA;
    int                                             m_msaaCount;

    enum class AntialiasMethods
    {
        FXAA,
        SMAA,
        SMAA2X,
        NONE,
        Count
    };

    AntialiasMethods                                m_antialias;

    enum class SMAAEdgeTechnique
    {
        DEPTH,
        COLOR,
        LUMA
    };

    SMAAEdgeTechnique                               m_smaaEdge;

    DirectX::SimpleMath::Matrix                     m_proj;
    DirectX::SimpleMath::Matrix                     m_view;

    // Scene rendering
    std::unique_ptr<DirectX::EffectFactory>         m_fxFactory;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_modelResources;
    std::unique_ptr<DirectX::Model>                 m_model;
    DirectX::Model::EffectCollection                m_modelEffect[4]; // No MSAA, 2x MSAA, 4x MSAA, 8x MSAA

    // Render Target resources
    std::unique_ptr<DX::RenderTexture>              m_scene;
    std::unique_ptr<DX::MSAAHelper>                 m_msaaHelper[3]; // 2x MSAA, 4x MSAA, 8x MSAA
    std::unique_ptr<DX::RenderTexture>              m_final;

    // Additional resources for SMAA2x
    std::unique_ptr<DX::RenderTexture>              m_scene2;
    std::unique_ptr<DX::RenderTexture>              m_depth;
    std::unique_ptr<DX::RenderTexture>              m_depth2;
    std::unique_ptr<DX::RenderTexture>              m_pass1;
    std::unique_ptr<DX::RenderTexture>              m_pass2;

    // Zoom view
    D3D12_VIEWPORT                                  m_zoomViewport;
    D3D12_RECT                                      m_zoomRect;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_zoomPSO;
    std::unique_ptr<DirectX::BasicEffect>           m_lineEFfect;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_lines;

    // UI rendering
    std::unique_ptr<DirectX::SpriteBatch>           m_batch;
    std::unique_ptr<DirectX::SpriteFont>            m_font;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
    std::unique_ptr<DirectX::SpriteFont>            m_colorCtrlFont;

    // Post-processing
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_passthroughPSO;

    // FXAA
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_fxaaPSO;

    // SMAA
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_areaTex;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_searchTex;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_seperatePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_seperateDepthPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_edgeDetectPSO[3];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_blendWeightsPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_neighborBlendPSO;
};
