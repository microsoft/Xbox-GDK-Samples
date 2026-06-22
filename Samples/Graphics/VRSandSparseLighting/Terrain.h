//--------------------------------------------------------------------------------------
// Terrain.h
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "DirectXHelpers.h"
#include "DescriptorHeap.h"
#include "PerformanceTimersXbox.h"


__inline void QuickBarrier(D3D12_RESOURCE_BARRIER &barrier, ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    barrier.Type = (before == after && before == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) ? D3D12_RESOURCE_BARRIER_TYPE_UAV : D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = 0;
}


inline float lerp(float a, float b, float f)
{
    return (1.f - f) * a + f * b;
}


struct Frustum
{
    float planes[5][4];
};



struct Constants
{
    // terrain 
    float invSourceHeightMapSize;
    float invHeightMapSize;
    uint32_t terrainTileSizeLog2;
    uint32_t terrainTileCountPerAxis;

    uint32_t frameIndex;
    float worldScaleY;
    float worldScale;
    float xzTranslation;

    float halfTexelHeightMapSize;
    float sourceToHeightMapSizeMultiplier;
    uint32_t sourceToHeightMapSizeLog2;
    float worldToHeightMapCoordMul;

    // for ray marching
    float lowResMaxHeightMapSize;
    DirectX::XMFLOAT3 rayMarchLightDir;
    DirectX::XMFLOAT3 lightDir;
    float stepSize;

    // camera
    DirectX::XMMATRIX worldViewProjectionMatrix;
    DirectX::XMFLOAT3 cameraPos;
    float depthRange;
    DirectX::XMFLOAT3 frustumHDelta;
    float depthRangeNegNearZDivFarZ;
    DirectX::XMFLOAT3 frustumVDelta;
    uint32_t htileInfo;
    DirectX::XMFLOAT3 frustumOrigin;
    float farZ;
    DirectX::XMFLOAT3 cameraForwardVector;
    uint32_t pad0;

    uint32_t renderTargetDimX;
    uint32_t renderTargetDimY;
    uint32_t sparseLightingBufferWidth;
    uint32_t sparseLightingBufferHeight;

    Frustum viewFrustum;

    float invRenderTargetDimX;
    float invRenderTargetDimY;
    float renderTargetHalfPixelOffsetX;
    float renderTargetHalfPixelOffsetY;
    
    // shading rate generation & sparse lighting
    float shadingRateTolerance;
    
    uint32_t sparseLightingFrameIndex;
    uint32_t sparseLightingLCopyCodeTL;         // top left
    uint32_t sparseLightingLCopyCodeTR;         // top right
    uint32_t sparseLightingLCopyCodeBL;         // bottom left
    uint32_t sparseLightingLCopyCodeBR;         // bottom right

    // count of tiles in shading rate image, including safe area on left and top, but not including safe area on bottom and right
    uint32_t vrsRightTileSize;
    uint32_t vrsBottomTileSize;

    // debug visualizations
    float debugSurfaceSizeFX;
    float debugSurfaceSizeFY;
    uint32_t debugSurfaceSizeUX;
    uint32_t debugSurfaceSizeUY;
};

static_assert((sizeof(Constants) % 16) == 0, "CB size not padded correctly");



class Terrain
{
public:
    struct ZoomLevel
    {
        enum Enum
        {
            None,
            x2,
            x4,
            x8,
            x16,
            Count
        };
    };

    struct DeblockerOptions
    {
        enum Enum
        {
            DeblockerOff,
            DeblockAndToneMap,
            DeblockStandAlonePass,
            Count
        };
    };

    struct RotateLitPixelOptions
    {
        enum Enum
        {
            Off,
            On,
            OncePerSecond,
            Count
        };
    };

    struct SparseBufferGeneration
    {
        enum Enum
        {
            CombinedDepthDecompress,
            StandAlone,
            Count
        };
    };

    struct SparseLightingTileSize
    {
        enum Enum
        {
            TileSize2x2,
            TileSize8x8fromHTile,
            Count
        };
    };

    struct Visualise
    {
        enum Enum
        {
            Normal,
            VRSShadingRateOverlay,
            SparseLightingShadingRateOverlay,
            NoHoleFilling,
            LightingCountBuffer,
            Coverage,
            Count
        };
    };

    struct GPUPasses
    {
        enum Enum
        {
            ClearDepth,
            VRSSetShadingRate,
            GBuffer,
            DepthDecompressAndSparseBufferGeneration,
            DeferredLighting,
            Sky,
            Deblocker,
            ToneMap,
            CalcShadingRate,
            Count
        };
    };

    struct LODs
    {
        enum Enum
        {
            VeryLow,
            Low,
            Medium,
            High,
            Count
        };
    };

    struct RenderTargets
    {
        enum Enum
        {
            GBufferAlbedo,
            GBufferRoughnessNormal,
            GBufferDebugAndShadingRate,
            LitOutput,
            PostProcessOutput,
            Count
        };
    };
private:
    static constexpr uint32_t m_heightMapSizeLog2               = 11;                       // max 14 in theory
    static constexpr uint32_t m_heightMapSize                   = 1 << m_heightMapSizeLog2;
    static constexpr uint32_t m_sourceToHeightMapSizeLog2       = 14 - m_heightMapSizeLog2;
    static constexpr uint32_t m_sourceHeightMapSize             = m_heightMapSize << m_sourceToHeightMapSizeLog2;       // = 16384
    static constexpr uint32_t m_tileSizeLog2                    = 5;                        // seems to be a sweet point
    static constexpr uint32_t m_tileSize                        = 1 << m_tileSizeLog2;		// assumed to be >= 8 by shaders
    static constexpr uint32_t m_tileCountPerAxis                = m_heightMapSize / m_tileSize;
    static constexpr uint32_t m_tileCount                       = m_tileCountPerAxis * m_tileCountPerAxis;

    static constexpr uint32_t m_numVerts                        = m_tileSize * m_tileSize;
    static constexpr uint32_t m_heightMapToLowResMaxSizeLog2    = 3;
    static constexpr uint32_t m_lowResMaxHeightSize             = m_heightMapSize >> m_heightMapToLowResMaxSizeLog2;
    
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorsCBV_SRV_UAV;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorRTV;

    uint32_t                                        m_sparseLightingCountMapWidth;
    uint32_t                                        m_sparseLightingCountMapHeight;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_sparseLightingCountMap;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_sparseLightingCoordBuffer;

    uint32_t                                        m_shadingRateImageVRSSizeBytes;
    uint32_t                                        m_shadingRateImageVRSWidth;
    uint32_t                                        m_shadingRateImageVRSHeight;

    uint32_t                                        m_shadingRateImageSparseLightingSizeBytes;
    uint32_t                                        m_shadingRateImageSparseLightingWidth;
    uint32_t                                        m_shadingRateImageSparseLightingHeight;

    uint32_t                                        m_HTileInfo;

    bool                                            m_firstFrame;
    uint32_t                                        m_frameCount;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_HTileBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_LinearDepth;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_shadingRateImageVRS;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_shadingRateImageSparseLighting;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_litOutput;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_postProcessOutput;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_GbufferAlbedo;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_GbufferNormalRoughness;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_GbufferDebugAndShadingRate;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_sourceHeightMap[2];
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_displayHeightMap;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_normalMap;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_AABBTileMinMaxBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_lowResMaxHeight;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_instanceBuffer[LODs::Count];

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_indexBuffer[LODs::Count];
    uint32_t                                        m_numIndices[LODs::Count];

    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_globalRS;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_decompressDepthPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_decompressDepthAndBuildSparseBuffersPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_buildSparseBuffersPSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_terrainRenderPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_skyRenderPSO;    

    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_terrainGenerateHeightsPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_terrainComputeAABBandNormalsPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_terrainComputeLowResMaxHeight;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_terrainInitAABBsPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_terrainSmoothPSO;

    Microsoft::WRL::ComPtr<ID3D12CommandSignature>	m_commandSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>			m_drawIndexedInstancedArgsBuffer;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_terrainVisibilityPSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_deferredLightingPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_sparseLightingPSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_shadingRatePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_shadingRateDepthDiscontinuityCheckPSO;
    
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_sparseLightingShowPixelCopiesPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_visualizeShadingRatePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_visualizeCoveragePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_visualizeCountMapPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_zoomCopy0PSO;     // unfortunately we need two of these, one for each UAV. The validation layer cannot detect that we are not accessing the UAV for the other back buffer thats in present state
    Microsoft::WRL::ComPtr<ID3D12PipelineState>		m_zoomCopy1PSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_toneMapPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_deblockAndToneMapPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_deblockPSO;

    D3D12_INDEX_BUFFER_VIEW							m_indexBufferView[LODs::Count];

    struct TerrainRootSignature
    {
        enum Enum
        {
            CB0,
            SRVs,
            UAVs,
            CB1,
            Count
        };
    };

    struct Descriptors
    {
        enum Enum
        {
            SourceHeightMapSRV0,
            SourceHeightMapSRV1,
            HeightMapSRV,
            NormalMapSRV,
            AABBTileMinMaxSRV,
            LowResMaxHeightSRV,
            InstanceBufferVeryLowResSRV,
            InstanceBufferLowResSRV,
            InstanceBufferMediumResSRV,
            InstanceBufferHighResSRV,
            LitOutputSRV,
            PostProcessSRV,
            GbufferAlbedoSRV,
            GBufferRoughnessNormalSRV,
            GBufferDebugAndShadingRateSRV,
            LinearDepthSRV,
            LinearDepthHalfResSRV,
            SparseLightingCountSRV,
            SparseLightingCoordsSRV,
            DepthSRV,
            HTileSRV,
            ShadingRateImage8x8_SRV,
            ShadingRateImage2x2_SRV,

            SourceHeightMapUAV0,
            SourceHeightMapUAV1,
            HeightMapUAV,
            NormalMapUAV_Mip0,
            NormalMapUAV_Mip1,
            NormalMapUAV_Mip2,
            NormalMapUAV_Mip3,
            AABBTileMinMaxUAV,
            InstanceBufferVeryLowResUAV,
            InstanceBufferLowResUAV,
            InstanceBufferMediumResUAV,
            InstanceBufferHighResUAV,
            DrawArgsUAV,
            LitOutputUAV,
            PostProcessUAV,
            GbufferAlbedoUAV,
            GBufferRoughnessNormalUAV,
            GBufferDebugAndShadingRateUAV,
            ShadingRateImage8x8_UAV,
            ShadingRateImage2x2_UAV,
            LowResMaxHeightUAV,
            LinearDepthUAV,
            LinearDepthHalfResUAV,
            SparseLightingCountUAV,
            SparseLightingCoordsUAV,
            BackBufferUAV0,
            BackBufferUAV1,

            Count
        };
    };

    static uint16_t GetIndex(uint16_t x, uint16_t y);
    static uint16_t *WriteTriangle(uint16_t *indices, uint16_t i0, uint16_t i1, uint16_t i2);
    static uint16_t *WriteTriangleFan2(uint16_t *indices, uint16_t sharedIndex, uint16_t i0, uint16_t i1, uint16_t i2);
    static uint16_t *WriteTriangleFan3(uint16_t *indices, uint16_t sharedIndex, uint16_t i0, uint16_t i1, uint16_t i2, uint16_t i3);
    static uint16_t *WriteTriangleFan4(uint16_t *indices, uint16_t sharedIndex, uint16_t i0, uint16_t i1, uint16_t i2, uint16_t i3, uint16_t i4);
    static uint16_t *WriteQuad(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);

    static uint16_t *WriteSideB_21(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteSideR_21(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteSideL_21(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteSideT_21(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);

    static uint16_t *WriteSideT_42(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteSideB_42(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteSideL_42(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteSideR_42(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);

    static uint16_t *WriteCornerFan_TL(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteCornerFan_TR(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteCornerFan_BL(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteCornerFan_BR(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);

    static uint16_t *WriteCornerCutOut_TL(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteCornerCutOut_TR(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteCornerCutOut_BR(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);
    static uint16_t *WriteCornerCutOut_BL(uint16_t *indices, uint16_t x, uint16_t y, uint16_t scale);

    void CreateIndexBuffers(std::unique_ptr<DX::DeviceResources> &deviceResources, std::unique_ptr<DirectX::GraphicsMemory> &graphicsMemory, DirectX::ResourceUploadBatch &upload);

public:
    Terrain() :
        m_sparseLightingCountMapWidth(0),
        m_sparseLightingCountMapHeight(0),
        m_shadingRateImageVRSSizeBytes(0),
        m_shadingRateImageVRSWidth(0),
        m_shadingRateImageVRSHeight(0),
        m_shadingRateImageSparseLightingSizeBytes(0),
        m_shadingRateImageSparseLightingWidth(0),
        m_shadingRateImageSparseLightingHeight(0),
        m_HTileInfo(0),
        m_firstFrame(true),
        m_frameCount(0),
        m_numIndices{},
        m_indexBufferView{}
    {
    }

    void Initialize(
        std::unique_ptr<DX::DeviceResources> &deviceResource,
        std::unique_ptr<DirectX::GraphicsMemory> &graphicsMemory
    );

    void SwitchToCompute(std::unique_ptr<DX::DeviceResources> &deviceResources, DirectX::GraphicsResource &cb);
    void SwitchToGraphics(std::unique_ptr<DX::DeviceResources> &deviceResources, DirectX::GraphicsResource &cb);
    void CalculateShadingRate(std::unique_ptr<DX::DeviceResources> &deviceResources, DirectX::GraphicsResource &cb, DX::GPUCommandListTimer<ID3D12GraphicsCommandList> &gpuTimer, bool VRSenabled, bool generateSparseLighting2x2TileSize, bool depthDiscontinuityCheck);

    void Render(
        const DirectX::XMFLOAT4X4& proj,
        const DirectX::XMFLOAT4X4& view,
        const DirectX::XMFLOAT4X4& viewProj,
        float farZ,
        float depthRange,
        float depthRangeNegNearZ,
        const DirectX::XMFLOAT3& position,
        std::unique_ptr<DX::DeviceResources> &deviceResource,
        std::unique_ptr<DirectX::GraphicsMemory> &graphicsMemory,
        const float elapsedTimeSeconds,
        DX::GPUCommandListTimer<ID3D12GraphicsCommandList> &gpuTimer,
        float shadingRateTolerance,
        float sunHeight,
        bool VRSenabled,
        bool sparseLightingEnabled,
        Terrain::SparseBufferGeneration::Enum sparseBufferGeneration,
        Terrain::SparseLightingTileSize::Enum sparseLightingTileSize,
        bool depthDiscontinuityCheck,
        Terrain::DeblockerOptions::Enum deblockerOptions,
        Terrain::RotateLitPixelOptions::Enum rotateLitPixelOptions,
        Terrain::Visualise::Enum debugVisualisation,
        Terrain::ZoomLevel::Enum zoomLevel
        );

    void RenderScene(
        std::unique_ptr<DX::DeviceResources>& deviceResources,
        DirectX::GraphicsResource& cb,
        uint32_t frameIndex,
        DX::GPUCommandListTimer<ID3D12GraphicsCommandList>& gpuTimer,
        bool VRSenabled,
        bool sparseLightingEnabled,
        Terrain::SparseBufferGeneration::Enum sparseBufferGeneration,
        Terrain::SparseLightingTileSize::Enum sparseLightingTileSize,
        bool depthDiscontinuityCheck,
        Terrain::DeblockerOptions::Enum deblockerOptions
        );

    void RenderTerrain(
        ID3D12GraphicsCommandList *commandList,
        uint32_t frameIndex,
        DX::GPUCommandListTimer<ID3D12GraphicsCommandList> &gpuTimer
        );

    void DecompressDepthAndBuildSparseBuffers(
        std::unique_ptr<DX::DeviceResources>& deviceResources,
        DirectX::GraphicsResource& cb,
        DX::GPUCommandListTimer<ID3D12GraphicsCommandList>& gpuTimer,
        bool sparseLightingEnabled,
        Terrain::SparseBufferGeneration::Enum sparseBufferGeneration,
        Terrain::SparseLightingTileSize::Enum sparseLightingTileSize
        );

    void DeferredLight(
        std::unique_ptr<DX::DeviceResources> &deviceResources,
        DirectX::GraphicsResource& cb,
        DX::GPUCommandListTimer<ID3D12GraphicsCommandList>& gpuTimer,
        bool sparseLightingEnabled
        );

    void RenderSky(
        ID3D12GraphicsCommandList* commandList,
        DX::GPUCommandListTimer<ID3D12GraphicsCommandList> &gpuTimer
        );

    void ToneMap(
        std::unique_ptr<DX::DeviceResources>& deviceResources,
        DirectX::GraphicsResource& cb,
        Terrain::DeblockerOptions::Enum deblockerOptions,
        DX::GPUCommandListTimer<ID3D12GraphicsCommandList>& gpuTimer
        );

    void GeneateTerrain(
        std::unique_ptr<DX::DeviceResources> &deviceResources
        );
};
