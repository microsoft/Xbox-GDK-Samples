//--------------------------------------------------------------------------------------
// AdvancedLighting.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "LightParticle.h"
#include "utils.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

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
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void DrawHUD();
    void UpdateSceneConstants(DirectX::SimpleMath::Matrix const& view, DirectX::SimpleMath::Matrix const& proj, uint32_t frameIndex);

    void CreatePipelineStates();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();
    void CreateGeometryBufferResourcesAndViews(uint32_t newWidth, uint32_t newHeight);

    // Scene configuration variables
    LIGHT_TECHNIQUE                                 m_lightTechniqueMode;
    uint32_t                                        m_lightDiameter;
    bool                                            m_drawParticles;
    bool                                            m_lightsOn;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;
    uint32_t                                        m_displayWidth;
    uint32_t                                        m_displayHeight;

    // Particles
    ParticleSystem                                  m_particles;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;
    std::unique_ptr<DX::CPUTimer>                   m_cpuTimer;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;
    double                                          m_deltaTime[GPU_TIMER_NUM];
    double                                          m_frameDeltaTime;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    std::unique_ptr<DirectX::Keyboard>              m_keyboard;
    std::unique_ptr<DirectX::Mouse>                 m_mouse;

    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker         m_keyboardButtons;

    // G-Buffer Resources
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_GBufferResourceAlbedo;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_GBufferResourceNormals;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_GBufferResourcePosition;
    D3D12_CLEAR_VALUE                               m_GBuferClrVal;

    // G-Buffer Heaps
    std::unique_ptr<DirectX::DescriptorHeap>        m_GBufferRTHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_GBufferSRHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_GPassAlbedoRTV;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_GPassNormalsRTV;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_GPassPositionRTV;

    // Handle for the depth as SRV
    D3D12_GPU_DESCRIPTOR_HANDLE                     m_depthSRVGPUHandle;

    // Sprite batch
    std::unique_ptr<DirectX::SpriteBatch>           m_spriteBatch;

    // Resource for Tiled pass output
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_tiledOutputResource;
    D3D12_GPU_DESCRIPTOR_HANDLE                     m_TiledComputeGPUHandle;

    // For per object matrices (world)
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_perObjectCBResource;
    PerObjectStructPadded*                          m_perObjectCBMapped;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_perObjectCBVAddress;

    // For passing array of ZRanges to clustered
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_zClusterRangesCBResource;
    zClusterRangesCBPadded*                         m_zClusterRangesCBMapped;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_zClusterRangesCBVAddress;

    // Full Screen Quad
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;

    // Root Signatures
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_gPassRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_ambientPassRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_lightVolumePassRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_lightTiledPassRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_ClusteringPassRS;

    // Pipeline State Objects
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_gPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_ambientPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_lightVolumePassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_lightTiledPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_ClusteringPassPSO;

    // Sphere mesh for instancing
    std::unique_ptr<DirectX::GeometricPrimitive>    m_lightMesh;

    // Camera
    DX::FlyCamera                                   m_camera;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;
    std::unique_ptr<DirectX::Model>                 m_model;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_texFactory;

    // scene constants
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_sceneConstantsResource;
    SceneConstantsStructPadded*                     m_sceneConstantsMappedMem;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_sceneConstantsVirtualAddress;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
};
