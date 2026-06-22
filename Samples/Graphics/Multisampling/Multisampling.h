//--------------------------------------------------------------------------------------
// Multisampling.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Util.h"

using namespace DirectX;

__declspec(selectany) D3D12XBOX_GPU_HARDWARE_CONFIGURATION g_gpuHardwareConfiguration;

inline bool IsDurangoClass()
{
    return D3D12XBOX_HARDWARE_VERSION_XBOX_ONE == g_gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_XBOX_ONE_S == g_gpuHardwareConfiguration.HardwareVersion;
}

inline bool IsScorpioClass()
{
    return D3D12XBOX_HARDWARE_VERSION_SCORPIO == g_gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_SCORPIO_DEVKIT == g_gpuHardwareConfiguration.HardwareVersion;
}

inline bool IsScarlettClass()
{
#ifdef _GAMING_XBOX_SCARLETT
    return D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_LOCKHART == g_gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_ANACONDA == g_gpuHardwareConfiguration.HardwareVersion
        || D3D12XBOX_HARDWARE_VERSION_XBOX_SCARLETT_DEVKIT == g_gpuHardwareConfiguration.HardwareVersion;
#else
    return false;
#endif
}

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

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

    // Device resources.
    std::unique_ptr<DX::DeviceResources>							m_deviceResources;

    // Rendering loop timer.
    uint64_t														m_frame;
    DX::StepTimer													m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>								m_gamePad;
    DirectX::GamePad::ButtonStateTracker							m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>						m_graphicsMemory;

    // The discrete UI options
    uint32_t														m_aliasingScene;
    uint32_t														m_viewMode;
    uint32_t														m_selectedSample;
    uint32_t														m_cameraMode;
    uint32_t														m_selectedLight;

    // Utility classes from DirectXTK12
    std::unique_ptr<BasicEffect>									m_frameEffect;
    std::unique_ptr<PrimitiveBatch<VertexPositionColor>>			m_prim;
    std::unique_ptr<SpriteBatch>									m_draw;
    std::unique_ptr<SpriteFont>										m_font;
    std::unique_ptr<SpriteFont>										m_controllerFont;

    // Hand-built resources for manual rendering
    struct VertexTexcoord
    {
        XMVECTOR position;
        XMVECTOR texcoord;
    };
    typedef VertexTexcoord VertexDataFullscreen[6];
    uint32_t														m_countOfVertexDataFullscreen;
    Microsoft::WRL::ComPtr<ID3D12Resource>							m_bufferFullscreen;
    D3D12_VERTEX_BUFFER_VIEW										m_vertexBufferViewFullscreen;
    Microsoft::WRL::ComPtr<ID3D12Resource>							m_textureSampleDot;
    D3D12_CPU_DESCRIPTOR_HANDLE										m_descriptorCpuSampleDot;
    Microsoft::WRL::ComPtr<ID3D12Resource>							m_textureFragmentDot;
    D3D12_CPU_DESCRIPTOR_HANDLE										m_descriptorCpuFragmentDot;

    // Hand-build resources for the Wheel of Fortune scene
    struct VertexColor
    {
        XMVECTOR position;
        XMVECTOR color;
    };
    static constexpr uint32_t                                       c_numWedges = 100;
    typedef VertexColor VertexDataWheelOfFortune[3 * c_numWedges];
    uint32_t														m_countOfVertexDataWheelOfFortune;
    Microsoft::WRL::ComPtr<ID3D12Resource>							m_bufferWheelOfFortune;
    D3D12_VERTEX_BUFFER_VIEW										m_vertexBufferViewWheelOfFortune;

    // The zoom source and dest area on the screen
    struct Rect
    {
        float left;
        float top;
        float right;
        float bottom;
    };
    Rect															m_zoomRectDst;
    Rect															m_zoomRectSrc;

    Transforms														m_activeTransforms;
    XMMATRIX														m_zoom;
    XMVECTOR														m_ambientColor;

    std::unique_ptr<Model>											m_meshSphere;
    D3D12_GPU_DESCRIPTOR_HANDLE										m_descriptorGpuMeshSphere;
    std::unique_ptr<Model>											m_sceneMesh[ALIASING_SCENE_COUNT];
    D3D12_GPU_DESCRIPTOR_HANDLE										m_descriptorGpuMesh[ALIASING_SCENE_COUNT];
    D3D12_VIEWPORT													m_viewportIndicator[Scene::c_numLights];

    // Shortcuts for m_Scene[ m_iAliasingScene ] and its members
    Scene*															m_activeScene;
    D3D12_GPU_DESCRIPTOR_HANDLE										m_activeDescriptorGpuMesh;
    CameraSettings*													m_activeCameraSettings;
    ZoomSettings*													m_activeZoomSettings;
    LightSettings*													m_activeLightSettings;

    // Multisampled render targets
    uint32_t                                                        m_backBufferWidth;
    uint32_t                                                        m_backBufferHeight;

    uint32_t														m_numQualityLevels[c_maxLogFragments + 2];
    uint32_t														m_logFragments;
    uint32_t														m_logSamples;
    uint32_t														m_quality;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>				m_renderTargets[c_maxLogFragments + 1];
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>						m_descriptorRTVs[c_maxLogFragments + 1];
    Microsoft::WRL::ComPtr<ID3D12Resource>							m_textureResolved;
    D3D12_CPU_DESCRIPTOR_HANDLE										m_descriptorCpuTextureResolved;
    D3D12_GPU_DESCRIPTOR_HANDLE										m_descriptorGpuTextureResolved;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>						m_descriptorCpuMultisample[c_maxLogFragments + 1];
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>						m_descriptorGpuMultisample[c_maxLogFragments + 1];
#if ENABLE_EQAA
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>						m_descriptorCpuFMaskNative[c_maxLogFragments + 1];
    std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>						m_descriptorGpuFMaskNative[c_maxLogFragments + 1];
    D3D12_CPU_DESCRIPTOR_HANDLE										m_descriptorCpuUAVResolved;
    D3D12_GPU_DESCRIPTOR_HANDLE										m_descriptorGpuUAVResolved;
    D3D12_CPU_DESCRIPTOR_HANDLE										m_descriptorRTVResolved;
#endif
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>				m_depthStencils[c_maxLogFragments + 1];
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>						m_descriptorDSVs[c_maxLogFragments + 1];

    D3D12_GPU_DESCRIPTOR_HANDLE										m_descriptorGpuSamplerPoint;
    D3D12_GPU_DESCRIPTOR_HANDLE										m_descriptorGpuSamplerLinear;

    D3D12_VIEWPORT													m_viewportZoom;

    // Root signatures
    enum DESCRIPTOR_TYPE
    {
        DESCRIPTOR_TYPE_CBV,
        DESCRIPTOR_TYPE_SAMPLER,
        DESCRIPTOR_TYPE_SRV,
        DESCRIPTOR_TYPE_UAV,

        DESCRIPTOR_TYPE_COUNT
    };
    enum SHADER_TYPE
    {
        SHADER_TYPE_VERTEX,
        SHADER_TYPE_GEOMETRY,
        SHADER_TYPE_PIXEL,

        GRAPHICS_SHADER_TYPE_COUNT,

        SHADER_TYPE_COMPUTE = 0,

        COMPUTE_SHADER_TYPE_COUNT,
    };
    uint32_t														m_graphicsRootElement[GRAPHICS_SHADER_TYPE_COUNT][DESCRIPTOR_TYPE_COUNT];
    Microsoft::WRL::ComPtr<ID3D12RootSignature>						m_rootSignatureGraphics;
    uint32_t														m_computeRootElement[COMPUTE_SHADER_TYPE_COUNT][DESCRIPTOR_TYPE_COUNT];
    Microsoft::WRL::ComPtr<ID3D12RootSignature>						m_rootSignatureCompute;

    // Descriptor heaps
    static constexpr uint32_t                                       m_maxPendingFrames = 3;
    DescriptorHeapWithCount< 256U >									m_descriptorHeapUpload;
    DescriptorHeapWithCount< 1024U >								m_descriptorHeapResource;
    DescriptorHeapWithCount< 32U >									m_descriptorHeapSampler;
    DescriptorHeapWithCount< 32U >									m_descriptorHeapRenderTarget;
    DescriptorHeapWithCount< 32U >									m_descriptorHeapDepthStencil;

    // Descriptor rings
    DescriptorRing<>												m_descriptorRing;

    // Pipeline states
    Microsoft::WRL::ComPtr<ID3D12PipelineState>*					m_pipelineStateWheelOfFortuneMSAA[c_maxLogFragments + 1];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStateMesh;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>*					m_pipelineStateMeshMSAA[c_maxLogFragments + 1];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStateWireframe;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStateGrid;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStatePointSprite;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStateTexture;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStateTextureSingleSample;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStateTextureAllSamples;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStateTextureNearestSample;
#if ENABLE_EQAA
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStateResolveGraphicsUbershader;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>						m_pipelineStateResolveComputeUbershader;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>*					m_pipelineStateResolveGraphicsNative[c_maxLogFragments + 1];
    Microsoft::WRL::ComPtr<ID3D12PipelineState>*					m_pipelineStateResolveComputeNative[c_maxLogFragments + 1];

    // No UI implemented for these, just change them in code
    bool															m_resolveUsingPixelShader;	// TODO: Test these
    bool															m_resolveUsingUbershader;
#endif

    // Constant buffers --- each one should match the associated declaration in the .hlsl files
    ConstantBufferIndirect< ConstantBufferTransform >				m_constantBufferTransform;
    ConstantBufferIndirect< ConstantBufferLight >					m_constantBufferLight;
    ConstantBufferIndirect< ConstantBufferGrid >					m_constantBufferGrid;
    ConstantBufferIndirect< ConstantBufferPointSprite >				m_constantBufferPointSprite;
    ConstantBufferIndirect< ConstantBufferPointSpriteTransform >	m_constantBufferPointSpriteTransform;
    ConstantBufferIndirect< ConstantBufferViewMode >				m_constantBufferViewMode;

#if ENABLE_EQAA
    ConstantBufferIndirect< ConstantBufferFMask >					m_constantBufferFMask;
    ConstantBufferIndirect< ConstantBufferEQAA >					m_constantBufferEQAA;
#endif

    // Subroutines for Initialize
    HRESULT InitializeHeaps(_In_ ID3D12Device* const device);
    HRESULT InitializeSceneMeshes(_In_ ID3D12Device* const device, _In_ ResourceUploadBatch& resourceUpload, _In_ EffectTextureFactory& effectTextureFactory);
    HRESULT InitializeLightIndicator(_In_ ID3D12Device* const device);
    HRESULT InitializeRootSignatures(_In_ ID3D12Device* const device);
    HRESULT InitializePipelineStates(_In_ ID3D12Device* const device);
    HRESULT InitializeConstantBuffers(_In_ ID3D12Device* const device);
    HRESULT InitializeFullScreen(_In_ ID3D12Device* const device);
    HRESULT InitializeWheelOfFortune(_In_ ID3D12Device* const device);
    HRESULT InitializeSamplers(_In_ ID3D12Device* const device);
    HRESULT InitializeMultisampling(_In_ ID3D12Device* const device);

    // Subroutines for Update
    void UpdateZoom();
    void UpdateTransform();
    void UpdateLight();
    void UpdateMSAA(uint32_t logFragments, uint32_t quality);

    // Subroutines for Render
    void RenderScene(ID3D12GraphicsCommandList* const commandList,
        const XMMATRIX& worldViewProj,
        const XMMATRIX& world);
    void RenderLights(ID3D12GraphicsCommandList* const commandList);
    void RenderZoom(ID3D12GraphicsCommandList* commandList, const XMMATRIX& worldViewProj);
    void RenderFrame(ID3D12GraphicsCommandList* commandList, const D3D12_RECT& rct,
        const XMVECTOR& color0 = Colors::Red,
        const XMVECTOR& color1 = Colors::Black);
    void RenderVisualization(ID3D12GraphicsCommandList* commandList);
    void RenderWireframe(ID3D12GraphicsCommandList* commandList, const XMMATRIX& worldViewProj);
    void RenderPixelGrid(ID3D12GraphicsCommandList* commandList, const XMMATRIX& transform);
    void RenderSampleDots(ID3D12GraphicsCommandList* commandList, const XMMATRIX& transform);
    void RenderResolvedSurface(ID3D12GraphicsCommandList* commandList);
    void RenderUI(ID3D12GraphicsCommandList* commandList);
#if ENABLE_EQAA
    void RenderManualResolve(ID3D12GraphicsCommandList* commandList);
#endif
};
