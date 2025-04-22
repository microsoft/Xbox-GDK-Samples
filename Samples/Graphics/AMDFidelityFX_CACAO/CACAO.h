//--------------------------------------------------------------------------------------
// CACAO.h
//
// Combined Adaptive Compute Ambient Occlusion (CACAO)
//
// Modifications Copyright (C) 2021. Advanced Micro Devices, Inc. All Rights Reserved.
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
#include "GLTF/GltfDepthPass.h"
#include "GLTF/GltfMotionVectorsPass.h"
#include "GLTF/GltfFile.h"

#include <FidelityFX/host/ffx_cacao.h>

typedef struct CacaoPreset {
    bool useDownsampledSsao;
    FfxCacaoSettings settings;
} CacaoPreset;


static std::vector<const wchar_t*> s_FfxCacaoPresetNames = {
    L"Native - Adaptive Quality",
    L"Native - High Quality",
    L"Native - Medium Quality",
    L"Native - Low Quality",
    L"Native - Lowest Quality",
    L"Downsampled - Adaptive Quality",
    L"Downsampled - High Quality",
    L"Downsampled - Medium Quality",
    L"Downsampled - Low Quality",
    L"Downsampled - Lowest Quality"
};

static const CacaoPreset s_FfxCacaoPresets[] = {
    // Native - Adaptive Quality
    {
        /* useDownsampledSsao */ false,
        {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_HIGHEST,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 2,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 5.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.1f,
    }
},
// Native - High Quality
{
    /* useDownsampledSsao */ false,
    {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_HIGH,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 2,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 5.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.1f,
    }
},
// Native - Medium Quality
{
    /* useDownsampledSsao */ false,
    {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_MEDIUM,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 2,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 5.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.1f,
    }
},
// Native - Low Quality
{
    /* useDownsampledSsao */ false,
    {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_LOW,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 6,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 5.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.1f,
    }
},
// Native - Lowest Quality
{
    /* useDownsampledSsao */ false,
    {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_LOWEST,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 6,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 5.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.1f,
    }
},
// Downsampled - Highest Quality
{
    /* useDownsampledSsao */ true,
    {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_HIGHEST,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 2,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 5.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.1f,
    }
},
// Downsampled - High Quality
{
    /* useDownsampledSsao */ true,
    {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_HIGH,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 2,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 5.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.1f,
    }
},
// Downsampled - Medium Quality
{
    /* useDownsampledSsao */ true,
    {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_MEDIUM,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 3,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 5.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.2f,
    }
},
// Downsampled - Low Quality
{
    /* useDownsampledSsao */ true,
    {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_LOW,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 6,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 8.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.8f,
    }
},
// Downsampled - Lowest Quality
{
    /* useDownsampledSsao */ true,
    {
        /* radius                            */ 1.2f,
        /* shadowMultiplier                  */ 1.0f,
        /* shadowPower                       */ 1.50f,
        /* shadowClamp                       */ 0.98f,
        /* horizonAngleThreshold             */ 0.06f,
        /* fadeOutFrom                       */ 20.0f,
        /* fadeOutTo                         */ 40.0f,
        /* qualityLevel                      */ FFX_CACAO_QUALITY_LOWEST,
        /* adaptiveQualityLimit              */ 0.75f,
        /* blurPassCount                     */ 6,
        /* sharpness                         */ 0.98f,
        /* temporalSupersamplingAngleOffset  */ 0.0f,
        /* temporalSupersamplingRadiusOffset */ 0.0f,
        /* detailShadowStrength              */ 0.5f,
        /* generateNormals                   */ false,
        /* bilateralSigmaSquared             */ 8.0f,
        /* bilateralSimilarityDistanceSigma  */ 0.8f,
    }
}
};

constexpr int FFX_CACAO_PRESET_COUNT = _countof(s_FfxCacaoPresets);

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
    void OnConstrained() {}
    void OnUnConstrained();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    // thread-safe constant buffer allocator to pass to FidelityFX backend contexts for allocations
    static FfxConstantAllocation ffxAllocateConstantBuffer(void* data, FfxUInt64 dataSize);

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void RenderUI(ID3D12GraphicsCommandList* commandList);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void SetTAAProjectionJitter(uint32_t width, uint32_t height, uint32_t& sampleIndex);

    void CreateCacaoContexts();
    void DestroyCacaoContexts();

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
        MotionVectorsSRV,
        MotionVectorDepthSRV,
        TaaIntOutputSRV,
        TaaIntOutputUAV,
        TaaOutputSRV,
        TaaOutputUAV,
        TaaHistorySRV,
        TaaHistoryUAV, 
        Tonemap_OutputSRV,
        Tonemap_OutputUAV,
        CacaoOutputSRV,
        CacaoOutputUAV,
        GltfSceneSRV,
        NormalsSRV,
        Count
    };

    enum RTDescriptors
    {
        SceneRTV,
        GltfSceneRTV,
        MotionVectorsRTV,
        Tonemap_OutputRTV,
        TaaOutputRTV,
        NormalsRTV,
        RTCount
    };

    enum DSDescriptors
    {
        ShadowAtlasDV,
        MotionVectorDepthDSV,
        DSCount
    };

    // Controls
    float                                           m_pitch;
    float                                           m_yaw;

    DirectX::SimpleMath::Matrix                     m_proj;
    DirectX::SimpleMath::Matrix                     m_view;

    DirectX::SimpleMath::Vector4                    m_lookAt;
    DirectX::SimpleMath::Vector4                    m_eye;

    bool                                            m_bIsDisplayInHDRMode;

    // Scene rendering
    std::unique_ptr<DirectX::EffectFactory>         m_fxFactory;
    std::vector<std::shared_ptr<DirectX::IEffect>>  m_modelEffect;
    
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
    AMDTK::GltfDepthPass*                           m_gltfDepth;
    AMDTK::GltfMotionVectorsPass*                   m_gltfMotionVectors;
    AMDTK::GLTFFile*                                m_gltfModel;

    std::unique_ptr<DirectX::EffectFactory>         m_gltfFxFactory;

    size_t m_currentCamera;
    std::vector<std::shared_ptr<DirectX::IEffect>>  m_gltfEffect;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_shadowAtlas;
    size_t                                          m_shadowAtlasIdx;

    DirectX::SimpleMath::Matrix                     m_prevViewProj;
    DirectX::SimpleMath::Matrix                     m_prevView;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_gltfScene;
    D3D12_RESOURCE_STATES                           m_gltfSceneState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_normals;
    D3D12_RESOURCE_STATES                           m_normalsState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_finalScene;
    D3D12_RESOURCE_STATES                           m_finalSceneState;
     
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_cacaoOutput;
    D3D12_RESOURCE_STATES                           m_cacaoOutputState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_motionVectors;
    D3D12_RESOURCE_STATES                           m_motionVectorsState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_motionVectorDepth;
    D3D12_RESOURCE_STATES                           m_motionVectorDepthState;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_taaPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_taaRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_taaPostPassPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_taaPostPassRS;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_taaIntermediateOutput;
    D3D12_RESOURCE_STATES                           m_taaIntermediateOutputState;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_taaOutput;
    D3D12_RESOURCE_STATES                           m_taaOutputState;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_taaHistory;
    D3D12_RESOURCE_STATES                           m_taaHistoryState;

    struct TaaOptionsCb {
        int reset;
    };

    TaaOptionsCb m_taaOptions;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_tonemapperOutput;
    D3D12_RESOURCE_STATES                           m_tonemapperOutputState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_tonemapperCSPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_tonemapperCSRS;

#ifdef _GAMING_XBOX
    XDisplayHdrModeInfo                             m_displayModeHdrInfo;
#endif

    struct ColorConversionConstants
    {
        int u_displayMode;
    };

    ColorConversionConstants                        m_colorConversionConstants;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_colorConversionPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_colorConversionRS;

    struct ToneMapperConstants
    {
        float exposure;
        int mode;
        float scale;
    };

    ToneMapperConstants             m_tonemapperConstants;

    int32_t                         m_cacaoPreset = _countof(s_FfxCacaoPresets);
    FfxCacaoSettings                m_cacaoSettings;
    FfxInterface                    m_ffxInterface;
    FfxCacaoContext                 m_cacaoContext;
    FfxCacaoContext                 m_cacaoDownsampledContext;
    bool                            m_useDownsampledSSAO = false;
    bool                            m_cacaoEnabled;
    bool                            m_cacaoAOOnly;    

    struct CompositeConstants
    {
        float cacaoAOFactor;
        int cacaoBufferView;
    };
    CompositeConstants              m_compositeConstants;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_compositeAOPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_compositeAORS;

#ifdef _GAMING_XBOX
    void SetDisplayMode() noexcept;
#endif

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
