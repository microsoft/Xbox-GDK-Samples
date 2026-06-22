//--------------------------------------------------------------------------------------
// DXRProceduralGeo.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Utils.h"

struct Vertex;

constexpr uint32_t hitgroupRecordCount = RayType::Count + IntersectionShaderType::TotalPrimitiveCount * RayType::Count;

// Represents a shader record for the tables that will be used on this sample.
// All records will be size of the maximum record size, even if not all use the same bindings.
struct CustomShaderRecord : public ShaderRecord
{
    CustomShaderRecord() = default;

    CustomShaderRecord(ID3D12StateObjectProperties* props, LPCWSTR exportName)
    {
        Initialize(props, exportName);
    }

    void InitializeNull()
    {
        uint8_t nullShaderId[32];
        memset(nullShaderId, 0, 32);
        memcpy_s(shaderIdentifier, sizeof(shaderIdentifier), nullShaderId, sizeof(shaderIdentifier));
    }

    // Extra bindings needed for this sample's shader tables.
    // Raygen and Miss shader table do not use these bindings.
    // The triangle geo entries of the table only use the materialCB bindings.
    // The procedural geo entries use all bindings.
    PrimitiveConstantBuffer materialCb;
    PrimitiveInstanceConstantBuffer aabbCB={};
};

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

    void Clear();
    void DrawHUD();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void SerializeAndCreateDXRRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>& rootSig);

    void CreateDXRRootSignatures();
    void CreateDXRPipelineStateObject();
   
    void BuildAccelerationStructures();

    void BuildBottomLevelASInstanceDescs(D3D12_GPU_VIRTUAL_ADDRESS* pBottomLevelASaddresses,
        Microsoft::WRL::ComPtr<ID3D12Resource>& outInstanceDescsResource);

    AccelerationStructureBuffers BuildBottomLevelAS(const std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>& geometryDescs,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);

    AccelerationStructureBuffers BuildTopLevelAS(AccelerationStructureBuffers* pBottomLevelAS,
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE);

    void CreateConstantBuffers();
    void CreateAABBPrimitiveAttributesBuffers();
    void BuildShaderTables();
    void CreateRaytracingOutputResource();

    void InitializeConstantBuffers();
    void UpdateAABBPrimitiveAttributes(float animationTime);

    // Geometry building methods
    void BuildProceduralGeometryAABBs();
    void BuildPlaneGeometry(DirectX::ResourceUploadBatch& upload);

    // Inline raytracing methods
    void CreateInlineDXRPipelineElements();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                m_deviceResources;
    uint32_t                                            m_width;
    uint32_t                                            m_height;

    // Application state
    uint64_t                                            m_frame;
    DX::StepTimer                                       m_timer;
    std::unique_ptr<DX::GPUTimer>                       m_gpuTimer;
    float                                               m_gpuTimerMeasuresMS[GPU_TIMER::TIMER_COUNT];
    float                                               m_animateGeometryTime;
    bool                                                m_animateGeometry;
    bool                                                m_animateLight;
    bool                                                m_useInlineRaytracing;

    // DirectX Raytracing (DXR) attributes
    Microsoft::WRL::ComPtr<ID3D12StateObject>           m_dxrStateObject;

    // DXR global Root Signature
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_DXRGlobalRootSignature;

    // DXR local Root Signature. One for triangle geo and one for procedural geometry
    Microsoft::WRL::ComPtr<ID3D12RootSignature>         m_DXRLocalRootSignature[LocalRootSignature::Type::Count];

    // constant buffers with data to be uploaded to GPU
    SceneConstantBuffer                                 m_sceneCB;
    PrimitiveConstantBuffer                             m_planeMaterialCB;
    PrimitiveConstantBuffer                             m_aabbMaterialCB[IntersectionShaderType::TotalPrimitiveCount];

    // Raytracing main scene CB resource
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_sceneCBResource;
    SceneConstantBuffer*                                m_pSceneCBMappedData;

    // Shader resource buffer with transform data per primitive
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_aabbPrimitiveAttributeBufferResource;
    PrimitiveInstancePerFrameBuffer*                    m_pAabbPrimitiveAttributeMappedData;

    // DXR output resource
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_DXROutputResource;
    D3D12_GPU_DESCRIPTOR_HANDLE                         m_DXROutputResourceUAVGpuDescriptorHandle;

    // Descriptor Heap
    std::unique_ptr<DirectX::DescriptorHeap>            m_descriptorHeap;

    // Inline raytracing d3d12 objects
    Microsoft::WRL::ComPtr<ID3D12RootSignature>          m_InlineRaytracingRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>          m_InlineRaytracingPSO;

    // List of AABBs that will be used to build BLAS
    std::vector<D3D12_RAYTRACING_AABB>                  m_AABBList;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_AABBResource;

    // Acceleration structure
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_bottomLevelAS[BottomLevelASType::Count];
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_topLevelAS;

    // Shader table resources
    ShaderBindingTable<CustomShaderRecord, 1, RayType::Count, hitgroupRecordCount>
                                                        m_shaderBindingTable;

    // Index buffer for triangle geo 
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_indexBufferResource;
    D3D12_GPU_DESCRIPTOR_HANDLE                         m_indexBufferGPUHandle;

    // Vertex buffer for triangle geo 
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_vertexBufferResource;
    D3D12_GPU_DESCRIPTOR_HANDLE                         m_vertexBufferGPUHandle;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>                   m_gamePad;
    std::unique_ptr<DirectX::Keyboard>                  m_keyboard;
    std::unique_ptr<DirectX::Mouse>                     m_mouse;

    // Scene camera
    DX::FlyCamera                                       m_camera;

    DirectX::GamePad::ButtonStateTracker                m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker             m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>            m_graphicsMemory;

    // UI
    std::unique_ptr<DirectX::DescriptorHeap>            m_HUDDescriptorHeap;
    std::unique_ptr<DirectX::SpriteBatch>               m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>                m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>                m_ctrlFont;
};
