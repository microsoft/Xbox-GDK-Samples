//--------------------------------------------------------------------------------------
// SuperResolution.h
//
// Modifications Copyright (C) 2021, 2022. Advanced Micro Devices, Inc. All Rights Reserved.
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
#include "Particles/ParticleSystem.h"
#include "GLTF/GltfResources.h"
#include "GLTF/GltfPbrPass.h" 
#include "GLTF/GltfDepthPass.h"
#include "GLTF/GltfMotionVectorsPass.h"
#include "GLTF/GltfFile.h"

#include <FidelityFX/host/ffx_fsr1.h>
#include <FidelityFX/host/ffx_fsr2.h>

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

    static void FfxMsgCallback(FfxMsgType type, const wchar_t* message);

    void Update(DX::StepTimer const& timer);
    void Render();
    void UpdateGLTFState();
    void RenderShadowMaps(ID3D12GraphicsCommandList* commandList);
    void RenderMotionVectors(ID3D12GraphicsCommandList* commandList);
    void RenderGTLFScene(ID3D12GraphicsCommandList* commandList);
    void RenderTAA(ID3D12GraphicsCommandList* commandList);
    void RenderTonemapping(ID3D12GraphicsCommandList* commandList);
    void RenderMagnifier(ID3D12GraphicsCommandList* commandList);
    void RenderColorConversion(ID3D12GraphicsCommandList* commandList);
    void RenderParticlesIntoScene(ID3D12GraphicsCommandList* commandList);
    void RenderFSR1(ID3D12GraphicsCommandList* commandList);
    void RenderFSR2(ID3D12GraphicsCommandList* commandList);
    void Clear();

    void RenderUI(ID3D12GraphicsCommandList* commandList);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void SetProjectionJitter(uint32_t width, uint32_t height);
    void UpdateMagnifier(DirectX::GamePad::State& pad);

    void EnableFSR(bool enable);
    void UpdateFSRContext(bool enabled);

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
        RCAS_OutputUAV,
        RCAS_OutputSRV,
        Upsample_OutputUAV,
        Upsample_OutputSRV,
        MagnifierOutputSRV, 
        Tonemap_OutputSRV,
        Tonemap_OutputUAV,
        GltfSceneSRV,
        GltfSceneUAV,
        ParticlesSRV,
        ParticlesUAV,
        FSR2ReactiveSRV,
        FSR2ReactiveUAV,
        FSR2TransparencySRV,
        FSR2TransparencyUAV,
        Count
    };

    enum RTDescriptors
    {
        SceneRTV,
        GltfSceneRTV,
        MotionVectorsRTV,
        Tonemap_OutputRTV,
        MagnifierOutputRTV,
        TaaOutputRTV,
        TaaHistoryRTV,
        ParticlesRTV,
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
    DirectX::Model::EffectCollection                m_modelEffect;
    
    // UI rendering
    std::unique_ptr<DirectX::SpriteBatch>           m_batch;
    std::unique_ptr<DirectX::SpriteFont>            m_font;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
    std::unique_ptr<DirectX::SpriteFont>            m_colorCtrlFont;

    // Post-processing
    std::unique_ptr<DX::FullScreenQuad>             m_fullScreenQuad;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_passthroughPSO; 

    ATG::ParticleSystem                             m_particleSystem;
    std::unique_ptr<DX::GPUTimer>                   m_gpuTimer;
    std::unique_ptr<DX::CPUTimer>                   m_cpuTimer;

    AMDTK::GLTFResources                            m_gltfResources;
    AMDTK::GltfPbrPass*                             m_gltfPBR;
    AMDTK::GltfDepthPass*                           m_gltfDepth;
    AMDTK::GltfMotionVectorsPass*                   m_gltfMotionVectors;
    AMDTK::GLTFFile*                                m_gltfModel;
    std::unique_ptr<DirectX::EffectFactory>         m_gltfFxFactory;

    DirectX::Model::EffectCollection                m_gltfEffect;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_shadowAtlas;
    size_t                                          m_shadowAtlasIdx;

    DirectX::SimpleMath::Matrix                     m_prevViewProj;
    DirectX::SimpleMath::Matrix                     m_currentViewProj;
    DirectX::SimpleMath::Matrix                     m_prevView;
    DirectX::SimpleMath::Matrix                     m_prevProj;

    bool                                            m_initialResourceClearsRequired;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_gltfScene;
    D3D12_RESOURCE_STATES                           m_gltfSceneState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_normals;
    D3D12_RESOURCE_STATES                           m_normalsState;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_finalScene;
    D3D12_RESOURCE_STATES                           m_finalSceneOutputState;

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
        int scaleHistory;
        float prevScale;
        float currentScale;
        int reverseDepth;
    };

    TaaOptionsCb m_taaOptions;

    enum class Mode : unsigned int
    {
        FSR2 = 0,
        Native,
        Bilinear,
        FSR1,
        Count
    };

    struct BilinearConstants
    {
        DirectX::XMUINT4 Const0;
        DirectX::XMUINT4 Const1;
    };

    BilinearConstants                               m_bilinearConsts;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_easuPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_easuRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_rcasPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rcasRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_bilinearUpsamplePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_bilinearUpsampleRS;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_rcasOutput;
    D3D12_RESOURCE_STATES                           m_rcasOutputState;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_upsampleOutput;
    D3D12_RESOURCE_STATES                           m_upsampleOutputState;
    Mode                                            m_pendingMode;
    Mode                                            m_currentMode; 

    bool                                            m_rcasEnable;
    float                                           m_rcasSharpness;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_tonemapperOutput;
    D3D12_RESOURCE_STATES                           m_tonemapperOutputState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_tonemapperCSPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_tonemapperCSRS;

    float                                           m_jitterX;
    float                                           m_jitterY;
    float                                           m_jitterPrevX;
    float                                           m_jitterPrevY;
    double                                          m_deltaTime;
    // FidelityFX Super Resolution information
    FfxFsr1ContextDescription						m_fsr1ContextDescription = {};
    FfxFsr1Context									m_fsr1Context;

    // FidelityFX Super Resolution 2 information
    FfxFsr2ContextDescription						m_fsr2ContextDescription = {};
    FfxFsr2Context									m_fsr2Context;
    bool                                            m_useFSR2ReactiveMask;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_particles;
    D3D12_RESOURCE_STATES                           m_particlesState;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_reactive;
    D3D12_RESOURCE_STATES                           m_reactiveState;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_transparency;
    D3D12_RESOURCE_STATES                           m_transparencyState;

    struct ResolveParticleConstants
    {
        float factor; 
    };
    ResolveParticleConstants                        m_resolveParticleConstants;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_resolveParticlePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_resolveParticleRS;

    struct MagnifierConstants
    {
        uint32_t    uImageWidth;
        uint32_t    uImageHeight;
        int         iMousePos[2];             

        float       fBorderColorRGB[4];     // Linear RGBA

        float       fMagnificationAmount;   // [1-...]
        float       fMagnifierScreenRadius; // [0-1]
        mutable int iMagnifierOffset[2];    // in pixels
    };

    MagnifierConstants                              m_magnifierConstants;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_magnifierOutput;
    D3D12_RESOURCE_STATES                           m_magnifierOutputState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_magnifierPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_magnifierRS;

#ifdef _GAMING_XBOX
    XDisplayHdrModeInfo                             m_displayModeHdrInfo;
    void SetDisplayMode() noexcept;
#endif

    struct ColorConversionConstants
    {
        int u_displayMode;
        float u_displayMinLuminancePerNits; // display min luminanace in units of 80 nits
        float u_displayMaxLuminancePerNits; // display max luminanace in units of 80 nits
        int gamma2;
    };

    ColorConversionConstants                        m_colorConversionConstants;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_colorConversionPSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_colorConversionRS;

    struct ToneMapperConstants
    {
        float exposure;
        int mode;
        float scale;
        int gamma2;
    };

    ToneMapperConstants                             m_tonemapperConstants;

    bool                                            m_renderScaleChanged;
    bool                                            m_renderScaleWantsChange;
    unsigned int                                    m_pendingRenderScale;
    unsigned int                                    m_renderScale; // ==0 implies native
    const unsigned int                              m_numRenderScales = 5;

    const float                                     m_renderScaleRatio[5] = { 1.0f, 1.5f,  1.7f, 2.0f, 3.0f };
    const wchar_t*                                  m_fsrModes[5] = { L"Native", L"Quality", L"Balanced", L"Performance", L"Ultra Performance"};

    float                                           m_lodBias;
    unsigned int                                    m_prevScale;

    D3D12_RECT GetScaledRect(const D3D12_RECT& rect)
    {
        D3D12_RECT scaledRect = {};
        scaledRect.right = LONG((rect.right) / m_renderScaleRatio[(int)GetRenderScale()]);
        scaledRect.bottom = LONG((rect.bottom) / m_renderScaleRatio[(int)GetRenderScale()]);
        return scaledRect;
    }

    unsigned int GetRenderScale() const
    {
        if (m_currentMode == Mode::Native)
        {
            return 0; // 0 == native
        }
        return m_renderScale;
    }

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
