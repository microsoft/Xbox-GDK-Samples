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

#include <FidelityFX/host/ffx_fsr3.h>

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
    void OnConstrained() {}
    void OnUnConstrained();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    // thread-safe constant buffer allocator to pass to FidelityFX backend contexts for allocations
    static FfxConstantAllocation ffxAllocateConstantBuffer(void* data, FfxUInt64 dataSize);
    
    // resource allocator/deallocator
#ifdef _GAMING_XBOX
    static FfxErrorCode ffxAllocateResource(FfxEffect ffxEffect, D3D12_RESOURCE_STATES initialState, const D3D12_HEAP_PROPERTIES* pHeapProps,
        const D3D12_RESOURCE_DESC* pD3DDesc, const FfxResourceDescription* pFfxDesc,
        const D3D12_CLEAR_VALUE* pOptimizedClear, ID3D12Resource** ppD3DResource);
    static FfxErrorCode ffxReleaseResource(FfxEffect ffxEffect, ID3D12Resource* pResource);
#endif

private:

#ifdef _GAMING_XBOX
	static void FfxMsgCallback(FfxMsgType type, const wchar_t* message);
#else // _GAMING_DESKTOP
    static void FfxMsgCallback(uint32_t type, const wchar_t* message);
#endif // _GAMING_XBOX

    void Update(DX::StepTimer const& timer);
    void Render();
    void UpdateGLTFState();
    void RenderShadowMaps(ID3D12GraphicsCommandList* commandList);
    void RenderMotionVectors(ID3D12GraphicsCommandList* commandList);
    void RenderGTLFScene(ID3D12GraphicsCommandList* commandList);
    void RenderTonemapping(ID3D12GraphicsCommandList* commandList);
    void RenderMagnifier(ID3D12GraphicsCommandList* commandList);
    void RenderColorConversion(ID3D12GraphicsCommandList* commandList);
    void RenderParticlesIntoScene(ID3D12GraphicsCommandList* commandList);
	void RenderFSR3(ID3D12GraphicsCommandList* commandList);
    void Clear();

    void UpdateFrameInterpolationOnPotentialPrimaryOutputChange();

    void RenderUI(ID3D12GraphicsCommandList* commandList);
    void RenderUI_NoCallback(ID3D12GraphicsCommandList* commandList);

#ifdef _GAMING_XBOX
    void RenderUI_WithCallback(ID3D12GraphicsCommandList* commandList, const FfxPresentCallbackDescription* params);
#else //_GAMING_DESKTOP
    void RenderUI_WithCallback(ID3D12GraphicsCommandList* commandList, const ffxCallbackDescFrameGenerationPresent* params);
#endif // _GAMING_XBOX

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void SetProjectionJitter(uint32_t width, uint32_t height);
    void UpdateMagnifier(DirectX::GamePad::State& pad);

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

    #ifdef _GAMING_XBOX
    // On Xbox we use separate allocator for transient resources
    std::unique_ptr<struct FrameTransientResourceMemoryStackAllocator> m_transientAlloc2MbFsr3;
    std::unique_ptr<struct FrameTransientResourceMemoryStackAllocator> m_transientAlloc2MbFI;
    std::unique_ptr<struct FrameTransientResourceMemoryStackAllocator> m_transientAlloc64KFsr3;
    std::unique_ptr<struct FrameTransientResourceMemoryStackAllocator> m_transientAlloc64KFI;

    std::vector<ULONG_PTR>                                             m_transientPhysicalPages2Mb;
    std::vector<ULONG_PTR>                                             m_transientPhysicalPages64K;
    std::vector<ULONG_PTR>                                             m_transientPhysicalPages64K_2Mb;
    #endif //  _GAMING_XBOX

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
        FSR3ReactiveSRV,
        FSR3ReactiveUAV,
        FSR3TransparencySRV,
        FSR3TransparencyUAV,
        UIOutput_SRV0,
        UIOutput_SRV1,
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
        UIOutput_RTV0,
        UIOutput_RTV1,
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
    std::unique_ptr<DirectX::SpriteBatch>           m_batchForBackBufferRender;
    std::unique_ptr<DirectX::SpriteBatch>           m_batchForUIBufferRender;
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

    struct FSRConstants
    {
        DirectX::XMUINT4 Const0;
        DirectX::XMUINT4 Const1;
        DirectX::XMUINT4 Const2;
        DirectX::XMUINT4 Const3;
    };

    FSRConstants                                    m_fsrConsts;
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
    
    // Global ui options
    bool                                            m_frameInterpolationControls = true;
    bool                                            m_hideMagnifier = true;
    bool                                            m_showHighRefreshRateWarning = false;

#ifdef _GAMING_XBOX
    // FidelityFX Super Resolution 3 information
    typedef enum Fsr3BackendTypes : uint32_t
    {
        FSR3_BACKEND_SHARED_RESOURCES,
        FSR3_BACKEND_UPSCALING,
        FSR3_BACKEND_FRAME_INTERPOLATION,
        FSR3_BACKEND_COUNT
    } Fsr3BackendTypes;
    bool                                            m_ffxBackendInitialized = false;
    FfxInterface                                    m_ffxFsr3Backends[FSR3_BACKEND_COUNT] = {};
    FfxFsr3ContextDescription                       m_fsr3ContextDescription = {};
    FfxFsr3Context                                  m_fsr3Context;

    // FidelityFX Frame Interpolation information
    FfxFrameGenerationConfig                        m_frameGenerationConfig = {};
#else // _GAMING_DESKTOP
    ffx::Context                                    m_upscalingContext;
    ffx::Context                                    m_frameGenContext;
    ffx::ConfigureDescFrameGeneration               m_frameGenerationConfig{};
#endif // _GAMING_XBOX

    bool                                            m_doublebufferInSwapchain = false;
    bool                                            m_frameInterpolation = true;
    bool                                            m_allowAsyncCompute = true;
    bool                                            m_drawDebugTearLines = false;
    bool                                            m_drawDebugView = false;
    bool                                            m_presentInterpolatedOnly = false;
    bool                                            m_frameGenerationCallback = true;
    bool                                            m_asyncComputePresent = false;
    enum FICompositionModes
    {
        COMP_MODE_NONE = 0,                         // No handling (not recommended)
        COMP_MODE_TEXTURE,                          // Ui in a separate texture
        COMP_MODE_CALLBACK,                         // Ui composition callback (recommended)
        COMP_MODE_HUDLESS_TEXTURE,                  // Scene textures with/without ui elements
        COMP_MODE_COUNT
    };
    FICompositionModes                              m_compositionMode = COMP_MODE_CALLBACK;

#ifdef _GAMING_XBOX
    static FfxErrorCode RenderUI_FrameInterpolationCallback(const FfxPresentCallbackDescription* params, void*);
#else // _GAMING_DESKTOP
    static ffxReturnCode_t RenderUI_FrameInterpolationCallback(ffxCallbackDescFrameGenerationPresent* params, void* pUserCtx);
#endif // _GAMING_XBOX

    // Supplemental UI targets for frame interpolation ui handling
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_uiRenderTargets[2];
    D3D12_RESOURCE_STATES                           m_uiRenderTargetStates[2];
    unsigned int                                    m_currentUITarget = 1;	


    bool                                            m_useFSRReactiveMask;
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

    bool                                            m_resetRequested;
    bool                                            m_renderScaleChanged;
    bool                                            m_renderScaleWantsChange;
    unsigned int                                    m_pendingRenderScale;
    unsigned int                                    m_renderScale; // ==0 implies native
    unsigned int                                    m_numRenderScales = 5;

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
