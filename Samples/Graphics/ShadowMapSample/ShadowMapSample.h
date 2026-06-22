//--------------------------------------------------------------------------------------
// ShadowMapSample.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Scene.h"
#include "Utils.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();
    void UpdateConstantResources(ID3D12GraphicsCommandList* commandList);
    void PipelineStateSetup();
    void SetupShadowMapPSO();
    void SetRootSignature();

    void ChangeMsaaLevel(MSAA_LEVEL newLevel);
    BOOL IsFilterCorrect(uint32_t iRemapping);
    BOOL IsUsingPCF(PCF_MODE mode);
    BOOL IsMsaa(MSAA_LEVEL lvl, DEPTH_REMAPPING filter);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void InitializeShadowParameters(ID3D12Device *device, DXGI_FORMAT depthFormat);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;
    
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;
    DOUBLE                                          m_deltaTime[RENDER_PASSES_COUNT];

    // Root Signature(s) and all the pipeline state objects
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_shadowPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_resolvePassPSO[DEPTH_REMAPPING_COUNT-1];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_filterHrzPassPSO[MSAA_ON_OFF_COUNT][DEPTH_REMAPPING_COUNT-1][BLUR_KERNEL_COUNT][BLUR_TYPE_COUNT];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_filterVrtPassPSO[DEPTH_REMAPPING_COUNT-1][BLUR_KERNEL_COUNT][BLUR_TYPE_COUNT];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_shadingPassPCFSampleCmpStep1PSO[BLUR_KERNEL_COUNT-1][BLUR_TYPE_COUNT];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_shadingPassPCFSampleCmpStep2PSO[BLUR_KERNEL_COUNT-1][BLUR_TYPE_COUNT];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_shadingPassPCFGatherCmpPSO[BLUR_KERNEL_COUNT - 1][BLUR_TYPE_COUNT];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_shadingPassNoPCFPSO[DEPTH_REMAPPING_COUNT];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_litWithoutShadowsPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_lightMeshPSO;

    // Shadow pass resource and views
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_shadowDSHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_shadowTexResource;
    D3D12_VIEWPORT                                  m_shadowViewport;
    D3D12_RECT                                      m_shadowScissorRect;

    // Variance shadow resource and views
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_resourceVariance[2];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_RTVHeapVariance;

    // Exponential shadow resource and views
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_resourceExponential[2];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_RTVHeapExponential;

    // ExponentialVariance shadow resource and views
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_resourceExpVariance[2];
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_RTVHeapExpVariance;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>    m_SRVHeapExpVariance;

    // Pile for fonts and diffuse and normal textures
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;

    // DirectionalLightCB variables
    DirectionalLightInfo                            m_dirLight;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_dirLightCBResource;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_dirLightCBAddress;
    CBLightStructPadded*                            m_dirLightCBMappedMem;

    // Contains all the shader BLOBS
    ShaderInfo                                      m_BLOBS;

    // CB for linearizing depth
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_remapCBResource;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_remapCBAddress;
    CBRemapStructPadded*                            m_remapCBMappedMem;

    // For geometry pass
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_resolveFSQVertexBufferRes;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_resolveFSQIndexBufferRes;
    D3D12_VERTEX_BUFFER_VIEW                        m_resolveFSQVertexBuffer;
    D3D12_INDEX_BUFFER_VIEW                         m_resolveFSQIndexBuffer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // Light mesh, for having a visual aid
    std::unique_ptr<DirectX::GeometricPrimitive>    m_lightMesh;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_lightMeshCBResource;
    CBTransformStructPadded*                        m_lightMeshCBMappedMem;

    // Scene constants resource and related
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_sceneConstantsCBResource;
    CBSceneConstStructPadded*                       m_sceneConstantsCBMappedMem;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_sceneConstantsCBAddress;
    uint32_t                                        m_sceneConstantsNumBindings = 2;

    // Camera
    DX::FlyCamera                                   m_camera;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;

    // Scene settings
    Scene*                                          m_pActiveScene;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
};
