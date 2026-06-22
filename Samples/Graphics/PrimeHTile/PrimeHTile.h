//--------------------------------------------------------------------------------------
// PrimeHTile.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "ControllerHelp.h"
#include "PerformanceTimersXbox.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

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

    void RenderUI(ID3D12GraphicsCommandList*);

    void InitializeDepthResources();

    struct Instance
    {
        DirectX::PackedVector::XMHALF4 PositionAndScale;
    };

    void BuildInstanceTransforms(Instance * const instances, uint32_t recursionDepth, uint32_t layerOffset, const Instance &parent);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;
    std::unique_ptr<DirectX::DescriptorHeap>    m_dsvDescriptorHeap;

    enum Descriptors
    {
        TextFont,
        ControllerFont,
        NormalMap,
        HeightMap,
        Occlusion,
        HTileUAV,
        Count,
    };

    enum DSVDescriptors
    {
        DSVDepth,
        DSVOcclusion,
        DSVCount,
    };

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // Render
    enum class PerSampleBiasMode
    {
        None,
        Furthest3x3,
        Furthest3x3AndPhantoms,
        Count
    };

    static constexpr size_t                     c_maxRecursionDepth = 7;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_normalMap;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_heightMap;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_depthOnlyVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_instanceBuffer;

    uint32_t                                    m_vertexBufferSize;
    uint32_t                                    m_depthOnlyVertexBufferSize;
    uint32_t                                    m_indexBufferSize;
    uint32_t                                    m_instanceBufferSize;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_bumpPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_parallaxPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_depthOnlyPSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_primeHTilePSO[static_cast<int>(PerSampleBiasMode::Count)][2];

    enum RootParameterIndex
    {
        Texture1SRV,
        Texture2SRV,
        ConstantBuffer,
        RootParameterCount
    };

    enum RootParameterIndexCS
    {
        ConstantBufferCS,
        TextureCS_SRV,
        TextureCS_UAV,
        RootParameterCountCS
    };

    enum RootParameterIndexVD
    {
        TextureVD,
        ConstantBuffer0VD,
        ConstantBuffer1VD,
        RootParameterCountVD
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSigCS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSigVD;

    DirectX::SimpleMath::Matrix                 m_view;
    DirectX::SimpleMath::Matrix                 m_proj;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_depthTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_occlusionTexture;
    void*                                       m_depthTextureAddress;
    D3D12XBOX_COMPONENT_PLACED_ADDRESSES        m_depthTextureAddresses;

    // hTile
    uint32_t                                    m_widthHtile;
    uint32_t                                    m_heightHtile;
    uint32_t                                    m_htileInfo;
    XSystemDeviceType                           m_deviceType;

    // Depth Visualize
    DirectX::GraphicsResource                   m_cbDepthVisualize;

    // UI rendering
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteBatch>       m_batchThumbnail;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    std::unique_ptr<ATG::Help>                  m_help;

    // Controls
    bool                                        m_showHelp;
    bool                                        m_stencil;
    bool                                        m_enableHTilePrePopulation;
    bool                                        m_enableParallaxMapping;
    bool                                        m_enableRotation;
    bool                                        m_reset;
    bool                                        m_firstFrameSinceRTAllocation;

    PerSampleBiasMode                           m_perSampleBiasMode;

    int                                         m_primingIterationCount;
    int                                         m_requestedPrimingIterationCount;
    int                                         m_currentIterationCount;
    float                                       m_hTileEncodeFloatDepthBias;
    float                                       m_occlusionRenderScale;
    float                                       m_mainRenderScale;
    float                                       m_curRotationAngle;

    int                                         m_maxInstances;
    int                                         m_instancesCountAtLevel[c_maxRecursionDepth];
    int                                         m_maxInstancesAtLevel[c_maxRecursionDepth];

    // Performance
    enum PerfQuery
    {
        Setup,
        OcclusionRender,
        Priming,
        MainScene
    };

    DX::GPUTimer                                m_gpuTimer;
};
