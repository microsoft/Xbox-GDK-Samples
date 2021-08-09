//--------------------------------------------------------------------------------------
// File: ParticleSystem.h
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#define SHARED_CPP
#include "Shared.h"

namespace ATG
{
    using FXMVECTOR = DirectX::FXMVECTOR;
    using GXMVECTOR = DirectX::GXMVECTOR;
    using CXMVECTOR = DirectX::CXMVECTOR;
    using CXMMATRIX = DirectX::CXMMATRIX;
    using FXMMATRIX = DirectX::FXMMATRIX;

    //--------------------------------------------------------------------------------------
    // Constants

    static const int g_MaxGroundBursts = 20;

    //--------------------------------------------------------------------------------------
    // Vertex definition for particle vertices

    struct ParticleVertex
    {
        DirectX::XMFLOAT3   Pos;
        DirectX::XMFLOAT2   UV;
        float               Life;
        float               Rot;
        DWORD               Color;
    };

    //--------------------------------------------------------------------------------------
    // Per-particle simulation information.

    struct Particle
    {
        DirectX::XMFLOAT3   Pos;
        DirectX::XMFLOAT3   Dir;
        DWORD               Color;
        float               Radius;
        float               Life;
        float               Fade;
        float               Rot;
        float               RotRate;
        bool                IsVisible;
    };

    // Forward declaration
    class ParticleWorld;

    //--------------------------------------------------------------------------------------
    // ParticleSystem
    // Base class for a particle system.

#pragma warning(push)
#pragma warning(disable:4324)   // Supress the "structure has been padded..." warning.


    class ParticleSystem
    {
    protected:

        DirectX::XMFLOAT3 m_direction;
        DirectX::XMFLOAT3 m_dirVariance;
        DirectX::XMFLOAT4 m_color0;
        DirectX::XMFLOAT4 m_color1;
        DirectX::XMFLOAT3 m_posMul;
        DirectX::XMFLOAT3 m_dirMul;
        DirectX::XMFLOAT3 m_center;
        DirectX::XMFLOAT4 m_flashColor;

        Particle* m_particles;

        int m_numParticles;

        float m_spread;
        float m_lifeSpan;
        float m_startSize;
        float m_endSize;
        float m_sizeExponent;
        float m_startSpeed;
        float m_endSpeed;
        float m_speedExponent;
        float m_fadeExponent;
        float m_rollAmount;
        float m_windFalloff;

        int m_numStreamers;
        float m_speedVariance;

        float m_currentTime;
        float m_startTime;


        bool m_hasStarted;

    public:
        ParticleSystem();
        virtual ~ParticleSystem() = default;

        HRESULT             CreateParticleSystem(ParticleWorld* pWorld, int NumParticles);
        void                SetSystemAttributes(FXMVECTOR vCenter,
            float Spread, float LifeSpan, float FadeExponent,
            float StartSize, float EndSize, float SizeExponent,
            float StartSpeed, float EndSpeed, float SpeedExponent,
            float RollAmount, float WindFalloff,
            int NumStreamers, float SpeedVariance,
            FXMVECTOR Direction, FXMVECTOR DirVariance,
            GXMVECTOR Color0, CXMVECTOR Color1,
            CXMVECTOR PosMul, CXMVECTOR DirMul);

        void                SetCenter(FXMVECTOR Center) { XMStoreFloat3(&m_center, Center); }
        void                SetStartTime(float StartTime) { m_startTime = StartTime; }
        void                SetStartSpeed(float StartSpeed) { m_startSpeed = StartSpeed; }
        void                SetFlashColor(FXMVECTOR FlashColor) { XMStoreFloat4(&m_flashColor, FlashColor); }
        DirectX::XMVECTOR   GetFlashColor() const { return XMLoadFloat4(&m_flashColor); }
        float               GetCurrentTime() const { return m_currentTime; }
        float               GetLifeSpan() const { return m_lifeSpan; }
        int                 GetNumParticles() const { return m_numParticles; }
        DirectX::XMVECTOR   GetCenter() const { return XMLoadFloat3(&m_center); }

        virtual void        Init();
        virtual void        AdvanceSystem(float ElapsedTime, FXMVECTOR Right, FXMVECTOR Up, FXMVECTOR WindVel, GXMVECTOR Gravity);
    };

#pragma warning(pop)

    //--------------------------------------------------------------------------------------
    // MushroomParticleSystem
    // Implementation class for the mushroom part of the mushroom cloud particle system.
    //--------------------------------------------------------------------------------------

    class MushroomParticleSystem : public ParticleSystem
    {
    public:
        virtual void Init();
        virtual void AdvanceSystem(float ElapsedTime, FXMVECTOR Right, FXMVECTOR Up, FXMVECTOR WindVel, GXMVECTOR Gravity);
    };

    //--------------------------------------------------------------------------------------
    // StalkParticleSystem
    // Implementation class for the stalk part of the mushroom cloud particle system.
    //--------------------------------------------------------------------------------------

    class StalkParticleSystem : public ParticleSystem
    {
    public:
        virtual void Init();
        virtual void AdvanceSystem(float ElapsedTime, FXMVECTOR Right, FXMVECTOR Up, FXMVECTOR WindVel, GXMVECTOR Gravity);
    };

    //--------------------------------------------------------------------------------------
    // GroundBurstParticleSystem
    // Implementation class for the ground-burst particle system.
    //--------------------------------------------------------------------------------------

    class GroundBurstParticleSystem : public ParticleSystem
    {
    public:
        virtual void Init();
        virtual void AdvanceSystem(float ElapsedTime, FXMVECTOR Right, FXMVECTOR Up, FXMVECTOR WindVel, GXMVECTOR Gravity);
    };

    //--------------------------------------------------------------------------------------
    // ParticleWorld
    // Class that encapsulates and manages all the scene's particle systems.
    //--------------------------------------------------------------------------------------

    class ParticleWorld
    {
    public:
        ParticleWorld();

        bool IsDeferred() const { return m_isRenderingDeferred; }
        void ToggleDeferred() { m_isRenderingDeferred = !m_isRenderingDeferred; }
        D3D12_GPU_VIRTUAL_ADDRESS GetGlowLightConstant() const { return m_glowLightsCBAddr; }

        Particle* GetNextParticleBlock(UINT ParticleCount)
        {
            auto Index = size_t(m_numUsedParticles);
            m_numUsedParticles += int(ParticleCount);
            return &m_particleArray[Index];
        }

        void CreateDeviceDependentResources(DX::DeviceResources* deviceResources, DirectX::ResourceUploadBatch& resourceUpload, const DirectX::Model& terrainModel);
        void CreateWindowSizeDependendentResources(DX::DeviceResources* deviceResources);

        void Update(bool IsSuspended, FXMMATRIX view, CXMMATRIX proj, CXMVECTOR camPos);
        void Render(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE hRT, D3D12_CPU_DESCRIPTOR_HANDLE hDS);

    private:
        void RenderForward(ID3D12GraphicsCommandList* commandList);
        void RenderDeferred(ID3D12GraphicsCommandList* commandList, D3D12_CPU_DESCRIPTOR_HANDLE hRT, D3D12_CPU_DESCRIPTOR_HANDLE hDS);

        void CreateParticleArray(int MaxParticles);
        void DestroyParticleArray();
        void SortParticles(FXMVECTOR EyePosition);
        void CopyParticlesToVertexBuffer(ParticleVertex* pVB, FXMVECTOR EyePosition, FXMVECTOR Right, FXMVECTOR Up);

        void BuildTerrainPositionArray(const DirectX::Model& model);

    private:
        // indexes of resources into the resource heap
        enum DescriptorHeapIndex
        {
            SRV_ParticleTex = 0,
            SRV_DiffuseTex,
            SRV_NormalTex,
            HeapCount
        };

        enum RTVDescriptorHeapIndex
        {
            RTV_NormalTex = 0,
            RTV_DiffuseTex,
            RTV_HeapCount
        };

        // indexes of root parameters
        enum RootParameters
        {
            RootParamCBPerFrame = 0,
            RootParamCBGlow,
            RootParamSRV,
            RootParamCount
        };

    private:
        std::unique_ptr<DirectX::DescriptorPile>     m_srvPile;
        std::unique_ptr<DirectX::DescriptorPile>     m_rtvPile;

        Microsoft::WRL::ComPtr<ID3D12Resource>      m_particleTex;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_diffuseTex;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_normalTex;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_forwardPSO;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_deferredPSO;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_quadPSO;

        std::vector<DirectX::XMFLOAT3>              m_terrainVertexPositions;
        std::unique_ptr<ParticleSystem>             m_particleSystems[g_MaxGroundBursts];

        bool                                        m_isRenderingDeferred;

        std::unique_ptr<Particle[]>                 m_particleArray;
        std::unique_ptr<int[]>                      m_particleIndices;
        std::unique_ptr<float[]>                    m_particleDepths;
        int                                         m_numParticlesToDraw;
        int                                         m_numUsedParticles;
        int                                         m_numActiveParticles;

        cbParticle                                  m_particleConstants;
        cbGlowLights                                m_glowConstants;

        D3D12_VERTEX_BUFFER_VIEW                    m_vbView;
        D3D12_GPU_VIRTUAL_ADDRESS                   m_glowLightsCBAddr;
    };
}
