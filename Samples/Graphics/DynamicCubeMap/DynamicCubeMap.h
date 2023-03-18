//--------------------------------------------------------------------------------------
// DynamicCubeMap.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

// mesh shaders and geometry shaders have known driver bugs on Scarlett before the March 2023 GDK
#ifdef _GAMING_XBOX_SCARLETT
#define MS_AND_GS_ALLOWED (_GXDK_VER >= 0x585D0BCF)
#else
#define MS_AND_GS_ALLOWED 1
#endif

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
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

    enum class RenderMode
    {
        Loop,
#if MS_AND_GS_ALLOWED
        GeometryShader,
#endif
        Instancing,
#ifdef _GAMING_XBOX_SCARLETT
#if MS_AND_GS_ALLOWED
        MeshShader,
#endif
#endif
        Count
    };

    // Represents an instance of a scene object.
    struct ModelInfo
    {
        std::unique_ptr<DirectX::Model>    m_model;
        std::vector<ATG::MeshletSet>       m_meshlet;
        std::vector<std::vector<uint32_t>> m_texCoordOffsets;
        uint32_t                           m_descriptorOffset;
    };

private:

    struct ObjectInstance
    {
        DirectX::SimpleMath::Matrix m_world;
        ModelInfo* m_modelInfo;
        float m_scale;
    };

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderScene(
        ID3D12GraphicsCommandList* commandList,
        DirectX::XMMATRIX view,
        DirectX::XMMATRIX proj,
        UINT textureRootIndex,
        UINT vertexBufferRootIndex,
        UINT rootConstantsIndex,
        UINT numInstances
#ifdef _GAMING_XBOX_SCARLETT
        , bool useMeshShader
#endif
    );
    void RenderSceneIntoCubeMap();
    void RenderHUD(ID3D12GraphicsCommandList* commandList);
    void GenerateMips();

    void Clear();
    void SetRenderTargetsAndViewports();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void UpdateConstants(ID3D12GraphicsCommandList* commandList, DirectX::XMMATRIX view, DirectX::XMMATRIX proj, DirectX::SimpleMath::Matrix world, float scale);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // Direct3D 12 objects
    std::unique_ptr<DirectX::DescriptorHeap>        m_srvHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_dsvHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_rtvHeap;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignatureScene;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateScene;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignatureSampleCubeMap;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateSampleCubeMap;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignatureGenerateCubeMap;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGenerateCubeMap;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGenerateCubeMapInstanced;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGenerateCubeMapSingleView;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGenerateCubeMapMeshShader;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignatureGenerateMips;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateGenerateMips;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_cubeMapDepth;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_cubeMap;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_whiteTexture;

    DX::FlyCamera                                   m_camera;

    // Assets & Scene
    std::unique_ptr<DirectX::EffectTextureFactory>  m_textureFactory;
    std::vector<ModelInfo>                          m_modelInfo;
    std::vector<ObjectInstance>                     m_scene;
    std::unique_ptr<DirectX::GeometricPrimitive>    m_reflectiveSphere;
    DirectX::SimpleMath::Matrix                     m_sphereMatrix;
    DirectX::SimpleMath::Matrix                     m_amCubeMapViewAdjust[6];
    DirectX::SimpleMath::Matrix                     m_projCBM;

    // Options
    bool                                            m_useAsyncComputeForMipGeneration;
    RenderMode                                      m_renderMode;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>           m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_font;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
};
