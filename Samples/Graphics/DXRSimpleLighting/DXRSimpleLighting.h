//--------------------------------------------------------------------------------------
// DXRSimpleLighting.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "HlslInterface.h"

// Ideally, shader records would have a variable (and potentially empty) number of local root arguments.
// All shader records are kept to the same size for simplicity and readability.

struct SimpleLightingRecord : public ShaderRecord
{
public:
    SimpleLightingRecord() = default;

    SimpleLightingRecord(ID3D12StateObjectProperties* props, const wchar_t* exportName, CubeConstants& localRootArguments) : localRootArguments(localRootArguments)
    {
        Initialize(props, exportName);
    }

    SimpleLightingRecord(ID3D12StateObjectProperties* props, const wchar_t* exportName)
    {
        Initialize(props, exportName);
    }

    CubeConstants localRootArguments;
};

namespace GlobalRootSignatureParams {
    enum Value {
        OutputViewSlot = 0,
        SceneConstantSlot,
        AccelerationStructureSlot,
        VertexBuffersSlot,
        Count
    };
}

namespace LocalRootSignatureParams {
    enum Value {
        CubeConstantSlot = 0,
        Count
    };
}

namespace InlineRootSignatureParams {
    enum Value {
        OutputViewSlot = 0,
        SceneConstantSlot,
        CubeConstantSlot,
        ScreenConstantSlot,
        AccelerationStructureSlot,
        VertexBuffersSlot,
        Count
    };
}

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

private:
    static const uint32_t                                       c_frameCount = 2;

    // Structure that wraps a D3D12 resource with a descriptor handle for its corresponding SRV.
    struct D3DBuffer
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = {};
        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle = {};
    };

    enum class StaticDescriptors
    {
        Font = 0,
        Control = 1,
        Reserve = 2
    };

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Init Scene functions
    void InitializeScene();

    // Render Raytracing functions
    void BuildTLAS();
    void BuildGeometry(ResourceUploadBatch& upload);
    void BuildShaderTables();
    void CreateAccelerationStructureResources(ResourceUploadBatch& upload);
    void CreateDXRDeviceResources(ResourceUploadBatch& upload);
    void CreateInlineRaytracingResources();
    void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* pRaytracingPipeline);
    void CreateRootSignatures();
    void CreateRaytracingOutputResource();
    void CreateRaytracingPipelineStateObject();
    void CreateUIResources(ResourceUploadBatch& upload);
    void CopyRaytracingOutputToBackbuffer();
    void DoRaytracing();
    void DoInlineRaytracing();
    void UpdateScene();
    void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, Microsoft::WRL::ComPtr<ID3D12RootSignature>* rootSig);

    // Other Direct3D Functions
    void AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, uint32_t descriptorIndex);
    void CreateBufferSRV(D3DBuffer* pBuffer, uint32_t numElements, uint32_t elementSize, uint32_t descriptorIndex);
    void DrawHUD(ID3D12GraphicsCommandList* commandList);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>                    m_deviceResources;
    RECT                                                    m_outputSize;

    // Rendering loop timer.
    uint64_t                                                m_frame;
    DX::StepTimer                                           m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>                       m_gamePad;
    DirectX::GamePad::ButtonStateTracker                    m_gamePadButtons;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>                   m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>                    m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>                    m_ctrlFont;

    // DirectX Raytracing (DXR) attributes
    Microsoft::WRL::ComPtr<ID3D12StateObject>               m_dxrStateObject;
    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties>     m_dxrStateObjectProperties;

    // Root signatures
    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_raytracingGlobalRootSignature;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_raytracingLocalRootSignature;

    // Descriptors
    enum Descriptors
    {
        IndexBuffer,
        VertexBuffer,
        RaytracingOutputUAV,
        Count
    };

    std::unique_ptr<DirectX::DescriptorHeap>                m_descriptorHeap;

    // Buffer Resources
    SceneConstants                                          m_sceneConstantData;
    CubeConstants                                           m_cubeConstantData;
    ScreenConstants                                         m_screenConstantData;
    D3DBuffer                                               m_indexBuffer;
    D3DBuffer                                               m_vertexBuffer;
    Matrix                                                  m_transforms[3];

    // Acceleration structure
    static const unsigned int                               c_numTLASInstances = 3; // These include the scene's center cube, static directional light, and rotating point light
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_bottomLevelAccelerationStructure;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_topLevelAccelerationStructure;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_BLASScratch;
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_TLASScratch;

    // Raytracing output
    Microsoft::WRL::ComPtr<ID3D12Resource>                  m_raytracingOutput;
    D3D12_GPU_DESCRIPTOR_HANDLE                             m_raytracingOutputResourceUAVGpuDescriptor;

    // Shader Table
    ShaderBindingTable<SimpleLightingRecord, 1 ,1 ,1 >      m_shaderBindingTable;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>                m_graphicsMemory;
    DirectX::GraphicsResource                               m_sceneCB;
    DirectX::GraphicsResource                               m_cubeCB;
    DirectX::GraphicsResource                               m_screenCB;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_rootSignature;
    std::unique_ptr<DirectX::DescriptorPile>                m_fontSrvPile;
    DX::FlyCamera                                           m_camera;
    std::unique_ptr<DX::GPUTimer>                           m_gpuTimer;

    enum class GPUTimerID
    {
        BuildTLAS,
        Raytracing,
        CopyToBackBuffer,
        Total
    };

    // Scene constants, updated per-frame.
    float                                                   m_curRotationAngleRad;

    // Inline Raytracing
    bool                                                    m_inlineRaytracingEnabled;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>             m_inlineRaytracingRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>             m_inlineRaytracingPSO;
};

