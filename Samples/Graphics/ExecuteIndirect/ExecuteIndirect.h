//--------------------------------------------------------------------------------------
// ExecuteIndirect.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

using namespace DirectX;

#include "DeviceResources.h"
#include "StepTimer.h"

#ifdef _GAMING_XBOX
#define CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT 64
#else
#define CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
#endif

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    // Properties
    bool RequestHDRMode() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

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

    // Options
    uint32_t                                    m_drawMethod;
    bool                                        m_cull;

    // Meshes
    enum MESH_SHAPE
    {
        MESH_SHAPE_TETRAHEDRON,
        MESH_SHAPE_CUBE,
        MESH_SHAPE_OCTAHEDRON,
        MESH_SHAPE_DODECAHEDRON,
        MESH_SHAPE_ICOSAHEDRON,

        MESH_SHAPE_COUNT
    };

    typedef uint16_t Index;
    struct Vertex
    {
        XMVECTOR                                m_position;
    };
    ID3D12Resource*                             m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW                     m_indexBufferView;
    ID3D12Resource*                             m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                    m_vertexBufferView;
    const static uint32_t                       m_indexCountInElements[MESH_SHAPE_COUNT];
    const static uint32_t                       m_vertexCountInElements[MESH_SHAPE_COUNT];
    static uint32_t                             m_indexBufferStart[MESH_SHAPE_COUNT];
    static int32_t                              m_vertexBufferStart[MESH_SHAPE_COUNT];
    float                                       m_meshRadius;

    void InitializeMeshes();

    // Instance paramters
    static const uint32_t                       m_maxInstances = 2048U;
    static const uint32_t                       m_cullThreadgroupSize = 64U;
    float                                       m_timeTotal;
    struct s_BoundingSphere
    {
        XMVECTOR                                m_centerAndRadius;
    };
    struct s_Instance
    {
        s_BoundingSphere                        m_boundingSphere;       // Use the vacant .w component for radius
        XMVECTOR                                m_rotAxis;
        float                                   m_rotSpeed;
        uint32_t                                m_mesh;
    };
    s_Instance                                  m_instances[m_maxInstances];
    ID3D12Resource*                             m_instanceBuffer;

    void InitializeInstances();

    // Camera parameters
    XMMATRIX                                    m_viewProj;
    XMVECTOR                                    m_cameraPosition;
    XMVECTOR                                    m_cameraDirection;
    XMVECTOR                                    m_cameraRight;
    XMVECTOR                                    m_cameraUp;
    float                                       m_translationSpeed;
    float                                       m_rotationSpeed;
    float                                       m_rotationScale;
    float                                       m_nearPlane;
    float                                       m_farPlane;
    int32_t                                     m_viewportWidth;
    int32_t                                     m_viewportHeight;
    struct Frustum
    {
        XMVECTOR                                m_plane[6];
    };
    Frustum                                     m_cameraFrustum;

    // Constant buffers
#pragma warning(suppress: 4324)     // structure was padded due to __declspec(align())
    __declspec(align(CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)) struct ConstantBufferBase {};

    struct ConstantBufferTransform : ConstantBufferBase
    {
        XMMATRIX                                m_worldViewProj;
        XMMATRIX                                m_world;
    };
    ID3D12Resource*                             m_constantBufferTransform;

    struct ConstantBufferTint : ConstantBufferBase
    {
        XMVECTOR                                m_color;
        XMVECTOR                                m_lightDir;
    };
    ID3D12Resource*                             m_constantBufferTint;

    struct ConstantBufferFrustum : ConstantBufferBase
    {
        Frustum                                 m_frustum;
    };
    ID3D12Resource*                             m_constantBufferFrustum;

    void InitializeConstantBuffers();

    // Root signatures
    ID3D12RootSignature*                        m_rootSignatureMesh;
    ID3D12RootSignature*                        m_rootSignatureCull;

    void InitializeRootSignatures();

    // Pipeline states
    ID3D12PipelineState*                        m_pipelineStateMesh;
    ID3D12PipelineState*                        m_pipelineStateCull;

    void InitializePipelineStates();

    // Indirect parameters
    enum INDIRECT_BUFFER
    {
        INDIRECT_BUFFER_UPLOAD,
        INDIRECT_BUFFER_PRE_CULL,
        INDIRECT_BUFFER_POST_CULL,

        INDIRECT_BUFFER_COUNT
    };
#pragma pack(push)                  // Need to match HLSL packing
#pragma pack(4)
    struct s_IndirectArgs
    {
        D3D12_GPU_VIRTUAL_ADDRESS               m_constantBuffer0;
        D3D12_GPU_VIRTUAL_ADDRESS               m_constantBuffer1;
        D3D12_DRAW_INDEXED_ARGUMENTS            m_drawIndexedArgs;
    };
#pragma pack(pop)
    typedef UINT64 s_IndirectCount;
    ID3D12CommandSignature*                     m_commandSignature;
    ID3D12Resource*                             m_argumentBuffer[INDIRECT_BUFFER_COUNT];
    ID3D12Resource*                             m_countBuffer[INDIRECT_BUFFER_COUNT];
    ID3D12Resource*                             m_countReadbackBuffer;
    ID3D12DescriptorHeap*                       m_indirectArgsCPUDescriptorHeap;
    ID3D12DescriptorHeap*                       m_indirectArgsGPUDescriptorHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE                 m_argumentDescriptorCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                 m_argumentDescriptorGPU;
    D3D12_CPU_DESCRIPTOR_HANDLE                 m_countDescriptorCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE                 m_countDescriptorGPU;
    UINT64                                      m_executeCount;

    void InitializeIndirectParameters();

    // Rendering
    void RenderScene();

    BOOL CullSphereFrustum(const s_BoundingSphere& sphere, const Frustum& frustum);
    void UploadIndirectArguments();
    void ComputeCulling();

    // UI
    struct ResourceDescriptors
    {
        enum : uint32_t
        {
            FontUI,
            FontController, 

            Count
        };
    };

    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptorHeap;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_fontUI;
    std::unique_ptr<DirectX::SpriteFont>        m_fontController;

    void RenderUI();

    // Timer
    std::unique_ptr<DX::CPUTimer>               m_cpuTimerFrame;
    std::unique_ptr<DX::GPUTimer>               m_gpuTimerFrame;
};
