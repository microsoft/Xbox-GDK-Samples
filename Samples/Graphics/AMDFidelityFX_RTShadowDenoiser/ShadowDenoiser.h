//--------------------------------------------------------------------------------------
// ShadowDenoiser.h
//
// Modifications Copyright (C) 2020. Advanced Micro Devices, Inc. All Rights Reserved.
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
#include "GLTF/GltfResources.h"
#include "GLTF/GltfPbrPass.h" 
#include "GLTF/GltfMotionVectorsPass.h"
#include "GLTF/GltfRTShadowPass.h"
#include "GLTF/GltfFile.h"

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

    void Clear(bool includenormals);

    void RenderUI(ID3D12GraphicsCommandList* commandList);

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
    std::unique_ptr<DirectX::DescriptorHeap>        m_depthDescriptors;
    std::unique_ptr<DirectX::CommonStates>          m_states;

    enum Descriptors
    {
        SceneTex,
        Font,
        CtrlFont,
        ColorCtrlFont,
        FinalSceneSRV,
        FinalSceneUAV,
        NormalsSRV,
        MotionVectorsSRV,
        MotionVectorDepthSRV,
        PreviousDepthSRV,
        MomentsBuffer0SRV,
        MomentsBuffer1SRV,
        MomentsBuffer0UAV,
        MomentsBuffer1UAV,
        HistoryBuffer0SRV,
        HistoryBuffer1SRV,
        HistoryBuffer0UAV,
        HistoryBuffer1UAV,
        IntermediateReprojectionSRV,
        IntermediateReprojectionUAV,
        IntermediateFilterSRV,
        IntermediateFilterUAV,
        IntermediateFilter2SRV,
        IntermediateFilter2UAV,
        TileMetadataSRV,
        TileMetadataUAV,
        RaytraceResultSRV,
        RaytraceResultUAV,
        NoisyRTResultSRV,
        NoisyRTResultUAV,
        DenoisedOutputSRV,
        DenoisedOutputUAV,
        SobolBufferSRV,
        SobolBufferUAV,
        ScramblingTileBufferSRV,
        ScramblingTileBufferUAV,
        Count
    };

    enum RTDescriptors
    {
        SceneRTV,
        NormalsRTV,
        MotionVectorsRTV,
        RTCount
    };

    enum DSDescriptors
    {
        ShadowAtlasDV,
        MotionVectorDepthDSV,
        PreviousDepthDSV,
        DSCount
    };

    // Controls
    float                                           m_pitch;
    float                                           m_yaw;

    DirectX::SimpleMath::Matrix                     m_proj;
    DirectX::SimpleMath::Matrix                     m_view;

    DirectX::SimpleMath::Vector4                    m_lookAt;
    DirectX::SimpleMath::Vector4                    m_eye;

    // Scene rendering
    std::unique_ptr<DirectX::EffectFactory>         m_fxFactory;
    DirectX::Model::EffectCollection                m_modelEffect;

    // Render Target resources
    std::unique_ptr<DX::RenderTexture>              m_scene;
    
    // UI rendering
    std::unique_ptr<DirectX::SpriteBatch>           m_batch;
    std::unique_ptr<DirectX::SpriteFont>            m_font;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
    std::unique_ptr<DirectX::SpriteFont>            m_colorCtrlFont;

    // Post-processing
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_passthroughPSO;

    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;

    AMDTK::GLTFResources                            m_gltfResources;
    AMDTK::GltfPbrPass*                             m_gltfPBR;
    AMDTK::GltfMotionVectorsPass*                   m_gltfMotionVectors;
    AMDTK::GltfRTShadowPass*                        m_gltfRTShadows;
    AMDTK::GLTFFile*                                m_gltfModel;

    std::unique_ptr<DirectX::EffectFactory>         m_gltfFxFactory;

    size_t m_currentCamera;
    DirectX::Model::EffectCollection                m_gltfEffect;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_shadowAtlas;
    size_t                                          m_shadowAtlasIdx;

    DirectX::SimpleMath::Matrix                     m_prevViewProj;


    Microsoft::WRL::ComPtr<ID3D12Resource>          m_normals;
    D3D12_RESOURCE_STATES                           m_normalsState;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_compositeShadowsPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_compositeShadowsRS;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_finalScene;
    D3D12_RESOURCE_STATES                           m_finalSceneOutputState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_motionVectors;
    D3D12_RESOURCE_STATES                           m_motionVectorsState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_motionVectorDepth;
    D3D12_RESOURCE_STATES                           m_motionVectorDepthState;
     
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_previousDepth;
    D3D12_RESOURCE_STATES                           m_previousDepthState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_historyMomentsBuffer;
    D3D12_RESOURCE_STATES                           m_historyMomentsBufferState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_momentsBuffer;
    D3D12_RESOURCE_STATES                           m_momentsBufferState;
     
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_historyBuffer0;
    D3D12_RESOURCE_STATES                           m_historyBuffer0State;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_historyBuffer1;
    D3D12_RESOURCE_STATES                           m_historyBuffer1State;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_reprojectionResultIntermediateBuffer;
    D3D12_RESOURCE_STATES                           m_reprojectionResultIntermediateBufferState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_intermediateFilterResult;
    D3D12_RESOURCE_STATES                           m_intermediateFilterResultState;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_intermediateFilterResult2;
    D3D12_RESOURCE_STATES                           m_intermediateFilterResult2State;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_tileMetadata;
    D3D12_RESOURCE_STATES                           m_tileMetadataState; 

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_raytraceResult;
    D3D12_RESOURCE_STATES                           m_raytraceResultState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_noisyRTResult;
    D3D12_RESOURCE_STATES                           m_noisyRTResultState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_denoisedOutput;
    D3D12_RESOURCE_STATES                           m_denoisedOutputState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_sobolBuffer;
    D3D12_RESOURCE_STATES                           m_sobolBufferState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_scramblingTileBuffer;
    D3D12_RESOURCE_STATES                           m_scramblingTileBufferState;

    // Denoiser PSO's
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_denoiseTileClassificationPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_denoiseTileClassificationRS;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_denoiseFilterPass0PSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_denoiseFilterPass0RS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_denoiseFilterPass1PSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_denoiseFilterPass1RS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_denoiseFilterPass2PSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_denoiseFilterPass2RS;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_prepareShadowMapPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_prepareShadowMapRS;

    // Options
    bool                                            m_useInlineRT;

    void InitNoiseBuffers(ID3D12GraphicsCommandList* commandList);

    // Resource helpers
    void TransitionTo(ID3D12GraphicsCommandList* commandList, Microsoft::WRL::ComPtr<ID3D12Resource>& resource, D3D12_RESOURCE_STATES* stateTracker, D3D12_RESOURCE_STATES afterState);
    void CreateResource(
        Microsoft::WRL::ComPtr<ID3D12Resource>& resource,
        LPCWSTR name,
        UINT64 width, UINT height,
        DXGI_FORMAT format,
        D3D12_RESOURCE_FLAGS flags,
        D3D12_CLEAR_VALUE* clearValue,
        CD3DX12_HEAP_PROPERTIES heapProps,
        D3D12_RESOURCE_STATES initalState,
        D3D12_RESOURCE_STATES* stateTracker,
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE uavHandle,
        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle
    );
};
