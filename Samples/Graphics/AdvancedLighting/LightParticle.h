//--------------------------------------------------------------------------------------
// LightParticle.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#define MAX_LIGHTS_DEFAULT  1024

#include "DeviceResources.h"
#include "utils.h"

enum DHEAP_DESCRIPTOR_INDICES
{
    UAV_PARTICLE_POSITIONS_COUNTER,
    UAV_PARTICLE_ANGLES,
    UAV_PARTICLE_POSITIONS,
    SRV_PER_PARTICLE_DATA,
    HEAP_DESCRIPTOR_COUNT
};

struct PerParticleData
{
    DirectX::SimpleMath::Vector3    OrbitCenter;
    DirectX::SimpleMath::Vector2    Radii;
    float                           AngleSpeed;
    float                           Range;
};

struct LightParticlePosData
{
    DirectX::SimpleMath::Vector4 position;
};

// Indirect arg structs
#pragma pack(push) // Need to match HLSL packing
#pragma pack(4)
struct IndirectArguments
{
    D3D12_GPU_VIRTUAL_ADDRESS       SRV1;
    D3D12_DRAW_INDEXED_ARGUMENTS    Draw;
};
#pragma pack(pop)

class ParticleSystem
{
public:

    ParticleSystem();
    virtual ~ParticleSystem();

    uint32_t GetLightCount();

    void initialize(std::unique_ptr<DX::DeviceResources>& m_deviceResources, uint32_t maxNumberOfLights = 1024);

    void Update(ID3D12GraphicsCommandList* cmdList, DirectX::SimpleMath::Matrix const& view,
        DirectX::SimpleMath::Matrix const& proj, float deltaTimeMS, uint32_t currentFrame, float lightRadius);

    void Render(ID3D12GraphicsCommandList*cmdList, uint32_t currentFrame, DirectX::SimpleMath::Matrix const& view,
        DirectX::SimpleMath::Matrix const& proj);

    void Reset();

    // This has info about each light position (in view) and light index in w (lights means the particles)
    ID3D12Resource* GetParticlePositionResource()
    {
        return m_ParticlePosRes.Get();
    }

private:
    void InitializePerParticleData(std::unique_ptr<DX::DeviceResources>& m_deviceResources);

    void UpdateSceneViewFrustrum(DirectX::SimpleMath::Matrix const& proj);

    uint32_t                                        m_particleCount;
    DirectX::SimpleMath::Vector4                    m_sceneFrustrum[6];
    bool                                            m_firstFrame = true;
    Icosahedron                                     m_particleMeshData;

    std::unique_ptr<DirectX::DescriptorHeap>        m_shaderVisibleHeap;
    std::unique_ptr<DirectX::DescriptorHeap>        m_nonShaderVisibleHeap;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_updateConstantsRes;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_updateConstantsAddress;
    UpdateConstantsCBPadded*                        m_updateConstantsMappedMem = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_PerObjectCBRes;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_PerObjectCBAddress;
    LightParticlesSceneConstsPadded*                m_PerObjectCBMappedMem = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_PerParticleDataUpload;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_PerParticleDataSRV;
    PerParticleData*                                m_PerParticleDataMappedMem = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_ParticleAnglesUpload;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_ParticleAnglesUAV;
    float*                                          m_ParticleAnglesMappedMem = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_ParticlePosRes;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_ParticlePosCounterRes;

    // PSO and RS for particles compute update pass
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_particleComputeRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_particleComputePSO;

    // PSO and RS for particles draw pass (with Execute Indirect)
    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_particleDrawRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_particleDrawPSO;

    // Execute Indirect API objects
    Microsoft::WRL::ComPtr<ID3D12CommandSignature>  m_cmdSignature;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_indArgsRes;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_indArgsUploadRes;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_ICountResource;

    // Particle vertex and index buffer
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_particleVertexBufferRes;
    Microsoft::WRL::ComPtr<ID3D12Resource>          m_particleIndexBufferRes;
    D3D12_VERTEX_BUFFER_VIEW                        m_particleVBV;
    D3D12_INDEX_BUFFER_VIEW                         m_particleIBV;
};
