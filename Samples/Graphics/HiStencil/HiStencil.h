//--------------------------------------------------------------------------------------
// HiStencil.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// ATGTK headers
#include "FlyCamera.h"
#include "ControllerHelp.h"
#include "PerformanceTimersXbox.h"

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SampleEffect.h"

namespace
{
    enum class NUM_EFFECTS
    {
        Disc,
        TotalEffects
    };

    enum class NUM_MESHES
    {
        ExteriorMesh,
        TotalExteriorMeshes,
        InteriorMesh_Furnishing = TotalExteriorMeshes,
        InteriorMesh_Gears,
        InteriorMesh_Robot,
        TotalInteriorMeshes = InteriorMesh_Robot - ExteriorMesh,
        TotalIntExtMeshes = TotalInteriorMeshes + TotalExteriorMeshes,
        Disc1,
        Disc2,
        Disc3,
        Disc4,
        TotalMeshes
    };

};

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

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
    std::unique_ptr<DX::DeviceResources>           m_deviceResources;

    // Rendering loop timer.
    uint64_t                                       m_frame;
    DX::StepTimer                                  m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>              m_gamePad;
    DirectX::GamePad::ButtonStateTracker           m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>       m_graphicsMemory;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>    m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_depthStencilBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_depthStencilBufferEsram;
    D3D12_RESOURCE_STATES                          m_dsvState;
    D3D12_RESOURCE_STATES                          m_dsvStateEsram;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_hTileBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_hTileBufferEsram;
    D3D12_RESOURCE_STATES                          m_hTileState;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_rtBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_rtBufferEsram;
    D3D12_RESOURCE_STATES                          m_rtBufferState;
    D3D12_RESOURCE_STATES                          m_rtBufferStateEsram;
    void CreateResources(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload);

    // Rect Resources
    D3D12_VERTEX_BUFFER_VIEW                       m_rectVBView;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_rectVertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_rectPso;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_rectWithStencilPso;
    void CreateRectResources(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload);
    void RenderRect(ID3D12GraphicsCommandList* graphicsCmdList, bool withStencil);

    // Font
    std::unique_ptr<DirectX::SpriteBatch>          m_fontBatch;
    std::unique_ptr<DirectX::SpriteFont>           m_fontText;
    std::unique_ptr<DirectX::SpriteFont>           m_fontController;
    void InitializeSpriteFonts(ID3D12Device* device, DirectX::ResourceUploadBatch& resourceUpload, const DirectX::RenderTargetState& rtState);
    void RenderUI(ID3D12GraphicsCommandList * graphicsCmdList);

    // Camera
    std::unique_ptr<DX::FlyCamera>                 m_camera;

    // Effect
    std::unique_ptr<SampleEffect>                  m_effect[static_cast<uint32_t>(NUM_EFFECTS::TotalEffects)];
    std::unique_ptr<DirectX::PBREffect>            m_effectPbr[static_cast<uint32_t>(NUM_MESHES::TotalIntExtMeshes)];
    std::unique_ptr<DirectX::Model>                m_model[static_cast<uint32_t>(NUM_MESHES::TotalMeshes)];
    std::unique_ptr<DirectX::EffectTextureFactory> m_textures;
    std::unique_ptr<DirectX::CommonStates>         m_commonStates;

    void LoadMeshes(ID3D12Device * device, DirectX::ResourceUploadBatch& resourceUpload);
    void RenderExteriorMesh(ID3D12GraphicsCommandList * graphicsCmdList);
    void RenderInteriorMesh(ID3D12GraphicsCommandList * graphicsCmdList);

    // Depth buffer planes
    XG_RESOURCE_LAYOUT                             m_xgResLayout;
    XG_PLANE_LAYOUT*                               m_depthPlane;
    XG_PLANE_LAYOUT*                               m_stencilPlane;
    XG_PLANE_LAYOUT*                               m_hTilePlane;

    D3D12XBOX_GPU_HARDWARE_CONFIGURATION           m_hwConfig;

    D3D12_CPU_DESCRIPTOR_HANDLE                    m_mayPassAppendBufferDescriptorCPU;
    D3D12_CPU_DESCRIPTOR_HANDLE                    m_mayPassCountBufferDescriptorCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                    m_mayPassAppendBufferDescriptorGPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                    m_mayPassCountBufferDescriptorGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_mayPassAppendBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_mayPassCountBuffer;
    uint32_t*                                      m_mayPassCountBufferMapped;

    D3D12_CPU_DESCRIPTOR_HANDLE                    m_mayFailAppendBufferDescriptorCPU;
    D3D12_CPU_DESCRIPTOR_HANDLE                    m_mayFailCountBufferDescriptorCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                    m_mayFailAppendBufferDescriptorGPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                    m_mayFailCountBufferDescriptorGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_mayFailAppendBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_mayFailCountBuffer;
    uint32_t*                                      m_mayFailCountBufferMapped;

    D3D12_CPU_DESCRIPTOR_HANDLE                    m_smemSingleValueAppendBufferDescriptorCPU;
    D3D12_CPU_DESCRIPTOR_HANDLE                    m_smemSingleValueCountBufferDescriptorCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                    m_smemSingleValueAppendBufferDescriptorGPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                    m_smemSingleValueCountBufferDescriptorGPU;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_smemSingleValueAppendBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_smemSingleValueCountBuffer;
    uint32_t*                                      m_smemSingleValueCountBufferMapped;

    D3D12_GPU_DESCRIPTOR_HANDLE                    m_renderTargetUAVGPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                    m_renderTargetEsramUAVGPU;

    // HTile related							   
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_htileDescCB;
    Microsoft::WRL::ComPtr<ID3D12Resource>         m_stencilDataCB;
    uint32_t*                                      m_stencilCBDataCPU;

    // Compute Shader
    void CreateComputeShaderResources(ID3D12Device* device);
    Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_readHTileSr0ComputePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_readHTileSr0Sr1ComputePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_readHTileSr0WithCountersComputePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_readHTileSr0Sr1WithCountersComputePSO;
    void DispatchToCountHiSValues(ID3D12GraphicsCommandList* graphicsCmdList);
    void SingleTransition(ID3D12GraphicsCommandList* graphicsCmdList, ID3D12Resource* resource, D3D12_RESOURCE_STATES& beforeState, D3D12_RESOURCE_STATES afterState);
    void RenderStencilPixelsComputeShader(ID3D12GraphicsCommandList* graphicsCmdList);
    void RenderStencilPixelsPixelShader(ID3D12GraphicsCommandList* graphicsCmdList);

    // ExecuteIndirect							   
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_commandSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_writeStencilPixelsPSO;

    // Timers									   
    DX::GPUTimer                                   m_gpuTimer;
    DX::CPUTimer                                   m_cpuTimer;

    // Controls
    // Decide whether to render using PS or CS
    bool m_renderUsingCs;

    // Toggle Hi-Stencil
    bool m_hiSOn;

    // Toggle Hi-Stencil Control API
    bool m_hiStencilControlEnable;

    // Decide whether to resummarize HTile
    bool m_resummarize;

    // Show extra count info when running compute shader
    bool m_showCount;

    // Check console to determine if ESRAM is available
    bool m_isEsramAvailable;

    // Decide whether to put depth and color target in ESRAM
    bool m_inEsram;

    // Flag to control when to count the number of HTile tiles
    bool m_dispatchBeforeOccluder;

    // Flag to set depth stencil buffer to the pipeline to override HiS set by SetHiStencilControlX
    bool m_setDSVToOverrideHiSControlX;

    // Heap
    std::unique_ptr<DirectX::DescriptorHeap>       m_rtvHeap;
    std::unique_ptr<DirectX::DescriptorHeap>       m_dsvHeap;
    std::unique_ptr<DirectX::DescriptorHeap>       m_uavHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE                    m_uavCpuHeapStart;
    D3D12_GPU_DESCRIPTOR_HANDLE                    m_uavGpuHeapStart;

    // Help
    std::unique_ptr<ATG::Help>                     m_help;
    bool                                           m_showHelp;
};
