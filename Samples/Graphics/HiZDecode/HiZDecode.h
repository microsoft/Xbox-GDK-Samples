//--------------------------------------------------------------------------------------
// HiZDecode.h
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
    void DrawHUD(ID3D12GraphicsCommandList* commandList);
    void InitializeDepthResources();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Represents an instance of a scene object.
    struct ObjectInstance
    {
        using EffectList = DirectX::Model::EffectCollection;

        DirectX::SimpleMath::Matrix world;
        DirectX::Model*             model       = nullptr;
        EffectList                  effects;
        uint32_t                    stencilRef  = 0;
        size_t                      txtOffset   = 0;
    };

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;

    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::CommonStates>          m_commonStates;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;
    std::unique_ptr<DirectX::DescriptorHeap>        m_dsvDescriptorHeap;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;

    enum Descriptors
    {
        TextFont,
        ControllerFont,
        HTileSRV,
        ZCompressedSRV,
        SCompressedSRV,
        HiZTextureSRV,
        HiSTextureSRV,
        ZDecompressedSRV,
        SDecompressedSRV,
        HiZTextureUAV,
        HiSTextureUAV,
        ZDecompressedUAV,
        SDecompressedUAV,
        Count,
    };

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;

    // Camera
    float                                           m_theta;
    float                                           m_phi;
    float                                           m_radius;
    DirectX::SimpleMath::Matrix                     m_proj;
    DirectX::SimpleMath::Matrix                     m_view;

    // Assets & Scene
    std::vector<std::unique_ptr<DirectX::Model>>    m_models;
    std::vector<ObjectInstance>                     m_scene;

    // HTile
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_depthTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_hTileBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_ZCompressedTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_SCompressedTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_HiZTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_HiSTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_ZDecompressedTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_SDecompressedTexture;

    void*                                           m_depthTextureAddress;
    D3D12XBOX_COMPONENT_PLACED_ADDRESSES            m_depthTextureAddresses;

    uint32_t                                        m_widthHtile;
    uint32_t                                        m_heightHtile;

    // Shaders and Root Signature for HTile decoding
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_decodePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_decodePSOStencil;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_decodeRootSig;

    // Shaders and Root Signature for Depth/Stencil decompression
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_decompressDepthPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_decompressDepthStencilPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_decompressRootSig;

    enum RootParameters : uint32_t
    {
        e_rootParameterCB = 0,
        e_rootParameterHTile,
        e_rootParameterHiZ_HiS,
        e_numRootParameters
    };

    DirectX::GraphicsResource                       m_decodeCB;

    // Overlay rendering
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_overlayPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_hizPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_hisPSO;

    DirectX::GraphicsResource                       m_overlayCB;

    D3D12_VIEWPORT                                  m_vpThumbnailHiZ;
    D3D12_VIEWPORT                                  m_vpThumbnailHiS;
    D3D12_VIEWPORT                                  m_vpThumbnailExpandedZ;
    D3D12_VIEWPORT                                  m_vpThumbnailExpandedS;

    // Controls
    D3D12XBOX_HARDWARE_VERSION                      m_hwVersion;

#ifdef _GAMING_XBOX_XBOXONE
    bool                                            m_esram;
#endif

    bool                                            m_stencil;
    bool                                            m_resummarize;
    bool                                            m_depthCompression;
    bool                                            m_decompress;
    bool                                            m_reset;

    // Performance
    DX::GPUTimer                                    m_gpuTimer;
};
