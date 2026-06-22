//--------------------------------------------------------------------------------------
// OfflineRT.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <OfflineRT/ShaderShared.h>

#include "DeviceResources.h"
#include "StepTimer.h"

class XGBVHComputer;

enum BVHCompressionMode
{
    NONE,
    CPU,        // March 2022 GDK
    GPU,        // March 2023 GDK
    DEHYDRATED  // March 2024 GDK
};

// A basic game implementation that creates a D3D12 device and
// provides a render loop.
class OfflineRT
{
public:

    OfflineRT() noexcept(false);
    ~OfflineRT() = default;

    OfflineRT(OfflineRT&&) = default;
    OfflineRT& operator= (OfflineRT&&) = default;

    OfflineRT(OfflineRT const&) = delete;
    OfflineRT& operator= (OfflineRT const&) = delete;

    // Initialization and management
    void Initialize(HWND window);
    void Shutdown();

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

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_descriptorHeap;
    std::unique_ptr<DirectX::SpriteBatch>       m_hudBatch;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_sceneConstants;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_rtOutput;

    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    XGBVHComputer*                              m_bvhComputer = nullptr;

    // Models (w. bottom-level acceleration structure):
    struct ModelInfo
    {
    
        ModelInfo(const wchar_t* name) :
            Name(name),
            OfflineModel(false),
            CompressionMode{},
            BLASMemory(0),
            ScratchRequiredToDecompress(0),
            ModelSRVs{},
            BLASSize(0),
            BLASCreationTime(0),
            BLASCreationScratchBytes(0),
            AlbedoColor{},
            SizeOfIndices(0)
        { }
        virtual ~ModelInfo() = default;

        std::wstring                            Name;

        bool                                    OfflineModel;           // True is this is an offline model, otherwise false

        BVHCompressionMode                      CompressionMode;        // BVH Compression mode used to create this model

#if (_GXDK_VER >= 0x55F00C6D) /* GDK Edition 220301 */
        // BVH Compression available in March 2022 Update 1 GDK onwards
        D3D12_GPU_VIRTUAL_ADDRESS               BLASMemory;
        UINT64                                  ScratchRequiredToDecompress;
#endif
        D3D12_GPU_DESCRIPTOR_HANDLE             ModelSRVs;              // Must be index buffer SRV followed by normal buffer SRV
        Microsoft::WRL::ComPtr<ID3D12Resource>  BLAS;
        UINT64                                  BLASSize;

        // Time (ms) it took to create the model BVH (for runtime models this is
        // GPU time and for offline models it is CPU time)
        float                                   BLASCreationTime;

        UINT64                                  BLASCreationScratchBytes;   // Amount of temporary memory required to build the BVH

        float4                                  AlbedoColor;
        uint32_t                                SizeOfIndices;
    };

    // Add model to list
    struct MDatModelInfo : public ModelInfo
    {
        MDatModelInfo(const wchar_t* name) : ModelInfo(name) { }

        Microsoft::WRL::ComPtr<ID3D12Resource> IndexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> VertexBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> CompressedBVHBuffer;
    };

    int                                         m_newModelIndex;
    int                                         m_currentModelIndex;
    std::vector<std::unique_ptr<ModelInfo>>     m_modelInfos;

    void AddModelFromSDKMesh(const wchar_t* filename, float4 albedoColor);
    void AddModelFromMDat(const wchar_t* filename, float4 albedoColor);

#if (_GXDK_VER >= 0x55F00C3C) /* GDK Edition 220301 */
    // BVH Compression available in March 2022 Update 1 GDK onwards
    template<class index_t>
    void AddModelFromCompressedMDat(const wchar_t* filename, float4 albedoColor, BVHCompressionMode bvhMode);

    void GPUDecompressAndRehydrateBVHs(ID3D12GraphicsCommandList* commandList);

    Microsoft::WRL::ComPtr<ID3D12Resource> m_bvhDecompressionScratchBuffer;
    bool                                   m_needToGPUDecompressOrRehydrateBVHs = false;
#endif

    /// Top-level acceleration structure (TLAS):
    static constexpr UINT kTLASInstanceCount = 1;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC  m_TLASBuildDesc;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TLASBuildScratch;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_TLAS;

    void AllocateTLAS();
    void BuildTLAS(ID3D12GraphicsCommandList6* commandList, size_t instanceModelIndex);

    // Global root signature:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_globalRootSignature;

    void CreateGlobalRootSignature();

    // Raytracing pipelines:
    enum class PipelineType
    {
        Runtime,                // Pipeline is fully runtime created
        OfflineCollections,     // Pipeline is runtime created from offline collections
        Offline                 // Pipeline is fully offline created
    };

    struct PipelineInfo
    {
        PipelineInfo(const wchar_t* name, PipelineType type)
            : Name(name)
            , Type(type)
            , BuildTime{}
            , CreateTime{}
        {
        }
        virtual ~PipelineInfo() = default;

        std::wstring                                Name;

        PipelineType                                Type;

        float                                       BuildTime;          // Time (ms) it took to build the pipeline offline
        float                                       CreateTime;         // Time (ms) it took to create the pipeline at runtime

        Microsoft::WRL::ComPtr<ID3D12StateObject>   StateObject;
        Microsoft::WRL::ComPtr<ID3D12Resource>      ShaderBindingTable;

        // Record and tables all point into ShaderBindingTable resource
        D3D12_GPU_VIRTUAL_ADDRESS_RANGE             RayGenRecord    = {};
        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE  HitGroupTable   = {};
        D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE  MissTable       = {};
    };

    int                                         m_currentPipelineIndex;
    std::vector<std::unique_ptr<PipelineInfo>>  m_pipelineInfos;

    void AddPipelineFromEmbeddedDXILLib();
    void AddPipelineFromSerializedCollections();
    void AddPipelineFromSerializedRTPSO();

    void AddShaderBindingTable(PipelineInfo* pipelineInfo, const wchar_t* rayGenShaderName, const wchar_t* hitGroupName, const wchar_t* missShaderName);

    // Camera controller:
    std::unique_ptr<DX::OrbitCamera>            m_camera;

    // Game pad input:
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // Rendering loop timer:
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;
    DX::GPUTimer                                m_gpuTimer;
};
