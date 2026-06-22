//--------------------------------------------------------------------------------------
// BuildYourOwnBundles.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "ModelEffect.h"
#include "ModelDrawHelper.h"
#include "PerformanceTimersXbox.h"

#pragma warning(disable : 4324)
// warning C4324: structure was padded due to alignment specifier

// On Xbox One, constant buffers should be aligned to 64 bytes
#define CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT 64
__declspec(align(CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)) struct ConstantBufferBase {};

namespace
{
    struct Frustum
    {
        DirectX::XMVECTOR   m_plane[6];
    };

    struct Instance
    {
        // .xyz is the translation and .w is the scaling factor
        DirectX::XMVECTOR   m_translateAndScale;

        DirectX::XMVECTOR   m_rotAxis;
        DirectX::XMMATRIX   m_modelToWorld;

        // .xyz is the center of the sphere and .w is the radius
        DirectX::XMVECTOR   m_boundingSphere;
        DirectX::XMVECTOR   m_boundingSphereCenter;
        float               m_boundingSphereRadius;

        float               m_rotSpeed;
        uint32_t            m_modelID;
        uint32_t            m_psoID;
    };

    struct InstanceOut /*: ConstantBufferBase*/
    {
        // .xyz is the center of the sphere and .w is the radius
        DirectX::XMVECTOR   m_boundingSphereCenterAndRadius;
        uint32_t            m_modelID;
        uint32_t            m_psoID;
    };
}

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

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

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_meshPSO;


    // Camera variables
    DirectX::XMVECTOR           m_camPos;
    DirectX::XMVECTOR           m_camLookAt;
    float                       m_camLookAtDist;
    DirectX::XMMATRIX           m_camView;
    DirectX::XMMATRIX           m_camProj;
    float                       m_Near;
    float                       m_Far;
    DirectX::XMMATRIX           m_worldViewProj;
    Frustum                     m_camFrustum;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_camFrustumCB;
    Frustum*                    m_camFrustumMappedCB;

    // Initialize
    void InitializeRootSignature(ID3D12Device* device);
    void InitializeCamera();

    struct ConstantBufferTransform /*: ConstantBufferBase*/
    {
        DirectX::XMMATRIX worldViewProj;
    };

    void LoadResources(ID3D12Device* device);

    // SpriteFont
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_fontResource;
    std::unique_ptr<DirectX::SpriteBatch>        m_fontBatch;
    std::unique_ptr<DirectX::SpriteFont>         m_fontText;
    std::unique_ptr<DirectX::SpriteFont>         m_fontController;
    std::unique_ptr<DirectX::EffectTextureFactory> m_textures;
    D3D12_GPU_DESCRIPTOR_HANDLE                  m_gpuDescHandleBYOBCompute;
    void InitializeSpriteFonts(ID3D12Device* device, const DirectX::RenderTargetState& rtState, DirectX::ResourceUploadBatch& resourceUpload);
    void RenderUI(ID3D12GraphicsCommandList* graphicsCmdList);

    // Mesh
    ModelLoader::ModelEffect*                        m_modelEffect[NUM_MODELS];
    Microsoft::WRL::ComPtr<ID3D12Resource>           m_meshDataCB;
    ModelLoader::ModelEffectConstants*               m_meshDataMappedCB;
    std::unique_ptr<DirectX::Model>                  m_model[NUM_MODELS];
    // In this sample we are assuming that the model has only 1 mesh and 1 part
    DirectX::ModelMeshPart*                          m_modelMeshPart[NUM_MODELS];
    Instance                                         m_meshInstances[NUM_INSTANCES];
    void LoadMeshes(ID3D12Device* device, const DirectX::RenderTargetState& rtState, DirectX::ResourceUploadBatch& resourceUpload);
    void CreateMeshInstances();

    // Timing Data
    std::unique_ptr<DX::GPUTimer>                    m_gpuTimer;
    std::unique_ptr<DX::CPUTimer>                    m_cpuTimer;

    // Culling
    uint32_t                            m_numMeshesDrawn;
    bool IsSphereOutsideFrustum(Frustum& frustum, DirectX::XMVECTOR& sphere); // sphere.xyz -> centre, sphere.w -> radius

    enum CULL_METHOD
    {
        CULL_METHOD_NO_CULL,
        CULL_METHOD_CULL,
        CULL_METHOD_COUNT
    };
    uint32_t m_cullMethod;

    enum DRAW_METHOD
    {
        DRAW_METHOD_BYOB_GPU,
        DRAW_METHOD_BYOB_UPDATE_EXECUTE_INSTANCES,
        DRAW_METHOD_BYOB_UPDATE_INSTANCES_EXECUTE_SINGLE,
        DRAW_METHOD_BYOB_RUNTIME_CREATE,
        DRAW_METHOD_BYOB_CLOSE_BUNDLE_X,
        DRAW_METHOD_DRAW_BUNDLES,
        DRAW_METHOD_DIRECT,
        DRAW_METHOD_COUNT
    };
    uint32_t m_drawMethod;


    // Bundles - Direct
    Microsoft::WRL::ComPtr<ID3D12RootSignature>       m_meshRootSignature;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_bundleDirectCmdAllocator[NUM_MODELS];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_bundleDirectCmdList[NUM_MODELS];
    void CreateDirectBundle(ID3D12Device* device);
    void SetStatesBeforeDraw(ID3D12GraphicsCommandList* graphicsCmdList, bool patchPSO, uint32_t instance, uint64_t cbOffset);

    // Bundles - BYOB
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_bundleBYOBCmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_bundleBYOBCmdList;
    struct BYOBBufferData
    {
        Microsoft::WRL::ComPtr<ID3D12Resource>        m_byobExecutableBuffer;
        UINT8*                                        m_byobBufferStart;
        UINT8*                                        m_byobBufferCurr;
        UINT8*                                        m_byobBufferEnd;
    };
    BYOBBufferData                                    m_byobBufferData[NUM_MODELS];
    BYOBBufferData                                    m_byobExecBuffer;
    BYOBBufferData                                    m_closeBundleXBuffer;
    void CreateBYOB(ID3D12Device* device);
    void SetStatesBYOB(uint32_t** writeAddress, bool patchPSO, bool setTexture, uint32_t* rootPacketHeader, uint32_t instance, uint64_t cbOffset);

    // Create bundles from the GPU
    void CreateBuffersForGPUBundles(ID3D12Device* device);
    BYOBBufferData                                    m_gpuBYOBAppendBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>            m_gpuBYOBCountBuffer;
    uint32_t*                                         m_gpuBYOBCountBufferMapped;

    // CloseBundleX
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>    m_bundleCloseBundleXCmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_bundleCloseBundleXCmdList;
    void CreateCloseBundleXCommandListAndBuffers(ID3D12Device* device);

    struct PSOData
    {
        D3D12XBOX_DESCRIPTOR_PIPELINE_STATE psoData;
#ifdef _GAMING_XBOX_SCARLETT
        uint32_t padding[1];
#else
        uint32_t padding[2];
#endif
    };
    struct VertexBufferData
    {
        uint32_t   m_descriptorLo;
        uint32_t   m_descriptorHi;
        uint32_t   m_strideInBytes;
        uint32_t   m_sizeInBytes;
    };
    struct IndexBufferData
    {
        uint32_t   m_packetHeader;
        uint32_t   m_bufferLocationLo;
        uint32_t   m_bufferLocationHi;
        uint32_t   m_padding;
    };
    struct DescriptorTableData
    {
        uint32_t   m_rootParameterIndex;
        uint32_t   m_gpuDescriptorHandle;
        uint32_t   m_padding0;
        uint32_t   m_padding1;
    };
    struct RootConstantBufferData
    {
        uint32_t   m_rootParameterIndex;
        uint32_t   m_bufferLocationLo;
        uint32_t   m_bufferLocationHi;
        uint32_t   m_padding;
    };
    struct DrawIndexedArgs
    {
        uint32_t packet;
        uint32_t InstanceCount;
        uint32_t StartIndexLocation;
        uint32_t IndexCountPerInstance;
        uint32_t BaseVertexLocation;
        uint32_t StartInstanceLocation;
    };

    struct ModelData : ConstantBufferBase
    {
        PSOData                 m_pso[NUM_PSO];
        VertexBufferData        m_vertexBufferData;
        IndexBufferData         m_indexBufferData;
        DescriptorTableData     m_descriptorTableData;
        RootConstantBufferData  m_rootConstantBufferData;
        DrawIndexedArgs         m_drawIndexedArgs;
    };

    struct ModelDataUpdatePerFrame : ConstantBufferBase
    {
        uint32_t   m_constantBufferLocationLo;
        uint32_t   m_constantBufferLocationHi;
        uint32_t   m_sizeofModelEffectConstants;
    };

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_computeDescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_gpuBYOBModelDataCB;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_gpuBYOBModelPerFrameDataCB;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_gpuBYOBInstanceDataCB;
    InstanceOut*                                        m_gpuBYOBInstanceDataMappedCB;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_gpuBYOBRSHeaderData;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_computeRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_gpuBYOBComputePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>         m_gpuBYOBComputeCullPSO;
    D3D12_CPU_DESCRIPTOR_HANDLE                         m_appendBufferDescriptorCPU;
    D3D12_CPU_DESCRIPTOR_HANDLE                         m_countBufferDescriptorCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                         m_appendBufferDescriptorGPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                         m_countBufferDescriptorGPU;
    bool m_gpuExecuteFlag;
    bool m_showInfo;

    void CreateComputePSO(ID3D12Device* device);

    void DrawBundles(ID3D12GraphicsCommandList* graphicsCmdList, size_t startRange);
    void DrawBYOBUpdateExecuteInstances(ID3D12GraphicsCommandList* graphicsCmdList, size_t startRange);
    void DrawBYOBUpdateInstancesSingleExecute(ID3D12GraphicsCommandList* graphicsCmdList, uint32_t currFrameIndex, uint32_t *graphicsRootPacketHeaders, size_t startRange);
    void DrawBYOBRuntimeCreate(ID3D12GraphicsCommandList* graphicsCmdList, uint32_t currFrameIndex, uint32_t *graphicsRootPacketHeaders, size_t startRange);
    void DrawBYOBUsingGPU(ID3D12GraphicsCommandList* graphicsCmdList, uint32_t currFrameIndex, uint32_t *graphicsRootPacketHeaders);
    void DrawMeshesUsingCloseBundleX(ID3D12GraphicsCommandList* graphicsCmdList, uint32_t currFrameIndex, size_t startRange);

    // Max size for each BYOB entry in the bundle
    static constexpr uint32_t MAX_PER_DRAW_SIZE =
        12 // SetGraphicsRootConstantBufferView
        + 12 // IASetIndexBuffer
        + 12 // IASetVertexBuffer - Single Vertex Buffer
        + 8  // SetGraphicsRootDescriptorTable (texture)
        + 24 // SetPipelineState
        + 24 // DrawIndexedInstanced
        ;

    // Max size for each entry in the bundle
    static constexpr uint32_t MAX_PER_DRAW_SIZE_CLOSE_BUNDLE_X =
        60 // SetGraphicsRootConstantBufferView
        + 40 // IASetIndexBuffer
        + 48 // IASetVertexBuffer - Single Vertex Buffer
        + 32 // SetGraphicsRootDescriptorTable (texture)
        + 48 // SetPipelineState
        + 60 // DrawIndexedInstanced
        + 24 // Primitive Topology
        ;
};
