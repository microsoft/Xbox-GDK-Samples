//--------------------------------------------------------------------------------------
// AutoRGB.h
//
// This sample shows how to extract a single ambient color from a given scene, and then
// use this to set the lamp colors from a LampArray API compatible device. 
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include <LampArray.h>
#include "utils.h"

class HeuristicsBase;

#define BACKBUFFER_COUNT    2U

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
{
public:

    Sample() noexcept(false);
    ~Sample() noexcept(false);

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
    void RenderDeferredScene(ID3D12GraphicsCommandList* commandList);
    void DrawHUD();
    void RenderScene();
    void DrawHistograms();
    void ComputeHistograms();
    void GetDataFromReadback();
    void RenderBGImage(ID3D12GraphicsCommandList* commandList);
    void UpdateSceneConstants(DirectX::SimpleMath::Matrix const& view, DirectX::SimpleMath::Matrix const& proj);
    void CreateGeometryBufferResourcesAndViews(uint32_t newWidth, uint32_t newHeight);
    void CreateIntermediateRTAndViews(uint32_t newWidth, uint32_t newHeight);
    void CreatePipelineStates();
    void InitializePerParticleData();
    DirectX::SimpleMath::Vector3 GatherSceneColorInfoFromHistogram(uint32_t * pointerToMainColor);

    // LampArray
    static void LampArrayCallback(void* context, bool isAttached, ILampArray* pLampArray);
    void LampArraySetUniqueColor(uint8_t red, uint8_t green, uint8_t blue);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Scene configuration variables
    uint32_t                                        m_lightDiameter;
    bool                                            m_lightsOn;

    // Set of heuristics
    HeuristicsBase*                                 m_heuristicsSet[COLOR_SELECTION_METHODS_COUNT];

    // BG textures
    uint32_t                                        m_currentSDRTexture;
    std::unique_ptr<DX::Texture>                    m_sdrTexture[c_NumImages];
    std::atomic_bool                                m_sdrTextureFinishedLoading[c_NumImages];

    std::unique_ptr<MovingAvg>                      m_movingAvg;
    uint32_t                                        m_displayWidth;
    uint32_t                                        m_displayHeight;
    SingleColor                                     m_sceneColorFromHistogram;
    uint32_t*                                       m_pReadbackBufferData;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::Model>                 m_model;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_texFactory;
    std::unique_ptr<DirectX::SpriteBatch>           m_spriteBatch;
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>            m_controllerFont;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_lightMesh;

    // Rendering loop timer.
    std::unique_ptr<DX::CPUTimer>                   m_cpuTimer;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;
    double                                          m_totalGPUTime;
    double                                          m_computeHistogramsTime;
    double                                          m_frameDeltaTime;
    double                                          m_readbackTime;

    // Intermediate RTV
    std::unique_ptr<DirectX::DescriptorHeap>        m_intermediateHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_intermediateRenderTgt[BACKBUFFER_COUNT];
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_intermediateRTV[BACKBUFFER_COUNT];

    // G-Buffer Resources
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_GBufferResourceAlbedo;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_GBufferResourceNormals;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_GBufferResourcePosition;
    D3D12_CLEAR_VALUE                               m_GBuferClrVal;

    // G-Buffer Heaps
    std::unique_ptr<DirectX::DescriptorHeap>        m_GBufferRTHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_GBufferSRHeap;
    std::unique_ptr<DirectX::DescriptorPile>        m_shaderVisibleHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_GPassAlbedoRTV;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_GPassNormalsRTV;
    D3D12_CPU_DESCRIPTOR_HANDLE                     m_GPassPositionRTV;

    // Handle for the depth as SRV
    D3D12_GPU_DESCRIPTOR_HANDLE                     m_depthSRVGPUHandle;

    // Resource for Tiled pass output
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_tiledOutputResource;

    // Full Screen Quad
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;

    // Root Signatures
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_gPassRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_ambientPassRS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_histogramCS_RS;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_lightTiledPassRS;

    // Pipeline State Objects
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_gPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_ambientPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_lightTiledPassPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_histogramCS_PSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_visualizeHistogram_PSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_singleColorBlockPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_BGImagesPSO;

    // Camera
    DX::FlyCamera                                   m_camera;
  
    // Light position's constant buffer
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_lightPosCBResource;
    lightPositionsCB*                               m_pLightPosCBMapped;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_lightPosCBVAddress;

    // scene constants
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_sceneConstantsResource;
    SceneConstantsStructPadded*                     m_pSceneConstantsMappedMem;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_sceneConstantsVirtualAddress;

    // Histogram
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_histogramFrame[BACKBUFFER_COUNT];
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_histogramFrameRB[BACKBUFFER_COUNT];
    std::unique_ptr<DirectX::DescriptorHeap>        m_nonShaderVisibleHeap;

    // LampArray
    LampArrayCallbackToken                          m_callbackToken{};
    std::vector<ILampArray*>                        m_lampArrays;
};
