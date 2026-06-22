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
#include "GpuProfiler.h"

__inline void QuickBarrier(D3D12_RESOURCE_BARRIER &barrier, ID3D12Resource *resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
    barrier.Type = (before == after && before == D3D12_RESOURCE_STATE_UNORDERED_ACCESS) ? D3D12_RESOURCE_BARRIER_TYPE_UAV : D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    barrier.Transition.Subresource = 0;
}


__inline float lerp(float a, float b, float f)
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
    float invSourceTextureSize;
    float invTextureSize;
    uint32_t terrainTileSizeLog2;
    uint32_t tileCountPerAxis;

    uint32_t frameIndex;
    float worldScaleY;
    float worldScale;
    float xzTranslation;

    float halfTexelOffsetSourceSize;
    float sourceToHeightMapSizeMultiplier;
    uint32_t sourceToHeightMapSizeLog2;
    float worldToTextureCoordMul;

    // for ray marching
    float lowResMaxHeightMapSize;
    DirectX::XMFLOAT3 rayMarchLightDir;
    DirectX::XMFLOAT3 lightDir;
    float stepSize;

    // camera
    DirectX::XMMATRIX worldViewProjectionMatrix;
    DirectX::XMFLOAT3 cameraPos;
    uint32_t pad2;
    DirectX::XMFLOAT3 frustumHDelta;
    uint32_t pad3;
    DirectX::XMFLOAT3 frustumVDelta;
    uint32_t pad4;
    DirectX::XMFLOAT3 frustumOrigin;
    uint32_t pad5;
    Frustum viewFrustum;
    
    // shading rate generation 
    float depthCompare;
    uint32_t discardSampleCountMaxRate;
    uint32_t discardSampleCountHalfRate;
    float depthTolerance;
    float sobelTolerance;
    float invInputDimX;
    float invInputDimY;
    uint32_t halfResInputMode;

    float shadingRateSurfaceSizeFX;
    float shadingRateSurfaceSizeFY;
    uint32_t shadingRateSurfaceSizeUX;
    uint32_t shadingRateSurfaceSizeUY;
};

static_assert((sizeof(Constants) % 16) == 0, "CB size not padded correctly");



class Terrain
{
public:
    struct ShadingRateCalculation
    {
        // & 1 for half res
        // & 2 for wave32
        enum Enum
        {
            Wave64_FullRes,
            Wave64_HalfRes,
            Wave32_FullRes,
            Wave32_HalfRes,
            Count
        };
    };

    struct Visualise
    {
        enum Enum
        {
            NoVRS,
            VRS,
            ShadingRateOverlay,
            ShadingRate,
            Diff,
            Count
        };
    };

    struct RenderTechnqiue
    {
        enum Enum
        {
            Normal,
            AOandShadows,
            Count
        };
    };

    struct GPUPasses
    {
        enum Enum
        {
            NoVRSClear,
            NoVRSTerrain,
            NoVRSSky,
            VRSClear,
            VRSTerrain,
            VRSSky,
            VRSCalcShadingRate,
            VRSSetShadingRate,
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
private:
    static constexpr uint32_t  m_heightMapSizeLog2              = 11;                       // max 14 in theory
    static constexpr uint32_t  m_heightMapSize                  = 1 << m_heightMapSizeLog2;
    static constexpr uint32_t  m_sourceToHeightMapSizeLog2      = 14 - m_heightMapSizeLog2;
    static constexpr uint32_t  m_sourceHeightMapSize            = m_heightMapSize << m_sourceToHeightMapSizeLog2;       // = 16384
    static constexpr uint32_t  m_tileSizeLog2                   = 5;                        // seems to be a sweet point
    static constexpr uint32_t  m_tileSize                       = 1 << m_tileSizeLog2;		// assumed to be >= 8 by shaders
    static constexpr uint32_t  m_tileCountPerAxis               = m_heightMapSize / m_tileSize;
    static constexpr uint32_t  m_tileCount                      = m_tileCountPerAxis * m_tileCountPerAxis;

    static constexpr uint32_t  m_numVerts                       = m_tileSize * m_tileSize;
    static constexpr uint32_t  m_heightMapToLowResMaxSizeLog2   = 3;
    static constexpr uint32_t  m_lowResMaxHeightSize            = m_heightMapSize >> m_heightMapToLowResMaxSizeLog2;
    
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorsCBV_SRV_UAV;
    std::unique_ptr<DirectX::DescriptorHeap>        m_resourceDescriptorRTV;

    uint32_t                                        m_shadingRateImageSizeBytes;
    uint32_t                                        m_shadingRateWidth;
    uint32_t                                        m_shadingRateHeight;

    Microsoft::WRL::ComPtr<ID3D12Resource>			m_shadingRateImage;
    Microsoft::WRL::ComPtr<ID3D12Resource>			m_renderedImage[2];
    Microsoft::WRL::ComPtr<ID3D12Resource>			m_sourceHeightMap[2];
    Microsoft::WRL::ComPtr<ID3D12Resource>			m_displayHeightMap;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_normalMap;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_AABBTileMinMaxBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>			m_lowResMaxHeight;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_instanceBuffer[LODs::Count];

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_indexBuffer[LODs::Count];
    uint32_t                                        m_numIndices[LODs::Count];

    Microsoft::WRL::ComPtr<ID3D12RootSignature>		m_globalRS;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_terrainRenderPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_terrainRenderAOOnlyPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_skyRenderPSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_terrainGenerateHeightsPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_terrainComputeAABBandNormalsPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_terrainComputeLowResMaxHeight;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_terrainInitAABBsPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_terrainSmoothPSO;

    Microsoft::WRL::ComPtr<ID3D12CommandSignature>  m_commandSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_drawIndexedInstancedArgsBuffer;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_terrainVisibilityPSO;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_shadingRatePSOs[ShadingRateCalculation::Count];
    
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_generateShadingRateHalfResWriteToHilePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_generateShadingRateFullResWriteToHilePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_debugVisualizeShadingRatePSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_debugVisualizeShadingRateOverlayPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_debugVisualizeAbsDiffPSO;

    D3D12_INDEX_BUFFER_VIEW                         m_indexBufferView[LODs::Count];

    struct ComputeTerrainRootConstants
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
            InstanceBufferVeryLowResSRV,
            InstanceBufferLowResSRV,
            InstanceBufferMediumResSRV,
            InstanceBufferHighResSRV,
            RenderedImage1x1SRV,
            RenderedImageVRSSRV,
            ShadingRateMapSRV,
            LowResMaxHeightSRV,

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
            RenderedImage1x1UAV,
            RenderedImageVRSUAV,
            ShadingRateMapUAV,
            LowResMaxHeightUAV,

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

    void CreateIndexBuffers(std::unique_ptr<DX::DeviceResources> &deviceResources,
        std::unique_ptr<DirectX::GraphicsMemory> &graphicsMemory, DirectX::ResourceUploadBatch &upload);

public:
    Terrain() :
        m_shadingRateImageSizeBytes(0),
        m_shadingRateWidth(0),
        m_shadingRateHeight(0),
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
    void CalculateShadingRate(std::unique_ptr<DX::DeviceResources> &deviceResources, DirectX::GraphicsResource &cb, ShadingRateCalculation::Enum shadingRateCalc, DirectX::DX12Timer &gpuTimer);
    void Render(std::unique_ptr<DX::DeviceResources> &deviceResources, uint32_t rtvIndex, uint32_t frameIndex, DirectX::DX12Timer &gpuTimer, GPUPasses::Enum terrainPoass, GPUPasses::Enum skyPass, RenderTechnqiue::Enum renderTechnique);
    void RenderNoVRS(std::unique_ptr<DX::DeviceResources> &deviceResources, uint32_t frameIndex, DirectX::DX12Timer &gpuTimer, RenderTechnqiue::Enum renderTechnique);
    void RenderVRS(std::unique_ptr<DX::DeviceResources> &deviceResources, uint32_t frameIndex, DirectX::DX12Timer &gpuTimer, RenderTechnqiue::Enum renderTechnique);

    void Render(
        const DirectX::XMFLOAT4X4 &viewProj,
        const DirectX::XMFLOAT3 &position,
        std::unique_ptr<DX::DeviceResources> &deviceResource,
        std::unique_ptr<DirectX::GraphicsMemory> &graphicsMemory,
        const float elapsedTimeSeconds,
        DirectX::DX12Timer &gpuTimer,
        Visualise::Enum visualise,
        RenderTechnqiue::Enum renderTechnique,
        float sobelTolerance,
        ShadingRateCalculation::Enum shadingRateCalc,
        float sunHeight
    );

    void RenderScene(
        ID3D12GraphicsCommandList *commandList,
        uint32_t frameIndex,
        DirectX::DX12Timer &gpuTimer,
        GPUPasses::Enum terrainTimer,
        GPUPasses::Enum skyTimer,
        RenderTechnqiue::Enum renderTechnique);

    void GeneateTerrain(
        std::unique_ptr<DX::DeviceResources> &deviceResource
    );
};
