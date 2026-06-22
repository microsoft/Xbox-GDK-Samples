//--------------------------------------------------------------------------------------
// CMaskDecode.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "FullScreenQuad\FullScreenQuad.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
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
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void RenderScene(ID3D12GraphicsCommandList* commandList);
    void RenderUI(ID3D12GraphicsCommandList* commandList);

    void InitializeColorResources();

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
    std::unique_ptr<DirectX::DescriptorHeap>        m_rtvDescriptorHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_dsvDescriptorHeap;
    std::unique_ptr<DirectX::CommonStates>          m_states;

    enum Descriptors
    {
        SceneTex,
        TextFont,
        ControllerFont,
        CmaskEncoded,
        CmaskDecoded,
        CmaskDecodedUAV,
        Count,
    };

    // Controls
    float                                           m_pitch;
    float                                           m_yaw;
    D3D12XBOX_HARDWARE_VERSION                      m_hwVersion;
    bool                                            m_msaa;         // With MSAA, Cmask devotes 2 bits to fast clear, otherwise 4 bits
    bool                                            m_subTile;      // Decode down to the subtile level (4x4 or 8x4 rather than 8x8)
    bool                                            m_reset;

    DirectX::SimpleMath::Matrix                     m_proj;
    DirectX::SimpleMath::Matrix                     m_view;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_colorTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_depthTexture;
    D3D12_RESOURCE_STATES                           m_colorTextureState;

    // Cmask
    uint32_t                                        m_widthCmask;
    uint32_t                                        m_heightCmask;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_cmaskDecodedTexture;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_decodePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_decodeRootSig;

    enum RootParameters : uint32_t
    {
        e_rootParameterCB = 0,
        e_rootParameterSRV,
        e_rootParameterUAV,
        e_numRootParameters
    };

    DirectX::GraphicsResource                       m_decodeCB;

    // Scene rendering
    std::unique_ptr<DirectX::EffectFactory>         m_fxFactory;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_modelResources;
    std::unique_ptr<DirectX::Model>                 m_model;
    DirectX::Model::EffectCollection                m_modelNormal;
    DirectX::Model::EffectCollection                m_modelMSAA;

    // Overlay rendering
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_overlayNoSubTilePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_overlaySubTilePSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_overlayReplacePSO;

    DirectX::GraphicsResource                       m_overlayCB;

    // UI rendering
    std::unique_ptr<DirectX::SpriteBatch>           m_batch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
};
