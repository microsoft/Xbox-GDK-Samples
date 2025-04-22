//--------------------------------------------------------------------------------------
// ParticleSystem.h
//
// Helper class to manage the particle simulation and rendering logic.
//
// Modifications Copyright (C) 2022. Advanced Micro Devices, Inc. All Rights Reserved.
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"

#define CPP_SHARED
#include "Shared.h"

namespace ATG
{
    // Manages a GPU particle system.
    class ParticleSystem
    {
    public:
        ParticleSystem();

        uint32_t GetParticleCount() const { return uint32_t(m_particleCount); }
        float GetParticleBounciness() const { return m_particleBounciness; }
        float GetParticleUpdateSpeed() const { return m_particleUpdateSpeed; }

        DirectX::XMFLOAT3 GetEmitterPosition() const;

        // Initializes the resources.
        void Initialize(DX::DeviceResources* device, DirectX::ResourceUploadBatch& resourceUpload, bool reverseDepth = false);
         
        void SetFloor(DirectX::XMFLOAT4 floor);

        // Updates control variables of the simulaton.
        void Update(float elapsedTime, DirectX::FXMMATRIX view, DirectX::CXMMATRIX proj);

        // Steps the simulation and renders the current state.
        void Render(ID3D12GraphicsCommandList* commandList, float renderscale);

    private:
        void InitializeResources(ID3D12GraphicsCommandList* commandList);

    private:
        // indexes of root parameters
        enum RootParameters
        {
            RootParamCB_VS = 0,
            RootParamUAV_VS,
            RootParamrSRV_PS,
            RootParamCount
        };

        enum ComputeRootParameters
        {
            ComputeRootParamCB = 0,
            ComputeRootParamUAV,
            ComputeRootParamSRV,
            ComputeRootParamCount
        };

        // indexes into the descriptor heap
        enum DescriptorHeapIndex
        {
            UAV_ParticleMotionData = 0,
            UAV_ParticleInstance,
            SRV_ParticleInstance,
            SRV_ParticleResetData,
            SRV_ParticleColors,
            SRV_Particle,
            HeapCount
        };

        enum CPUDescriptorHeapIndex
        {
            CPU_ParticleCounter = 0,
            CPU_HeapCount = CPU_ParticleCounter + 1
        };

    private:
        DX::DeviceResources* m_deviceResources;
        // pipeline objects
        Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_renderRS;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_renderPSO;
        Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_computeRS;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_advancePSO;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_resetPSO;

        // Particle animation and rendering buffers
        Microsoft::WRL::ComPtr<ID3D12Resource>          m_particleInstance;
        Microsoft::WRL::ComPtr<ID3D12Resource>          m_particleMotionData;
        Microsoft::WRL::ComPtr<ID3D12Resource>          m_particleResetData;

        // Texture resources
        Microsoft::WRL::ComPtr<ID3D12Resource>          m_particleTex;

        // heaps
        std::unique_ptr<DirectX::DescriptorPile>        m_srvPile;
        std::unique_ptr<DirectX::DescriptorPile>        m_srvPileCpu;

        int                                             m_resetStart;
        int                                             m_resetCount;

        float                                           m_emitterRadius;
        float                                           m_emitterOrbitAngle;
        float                                           m_emitterHeight;

        float                                           m_particleBounciness;
        float                                           m_particleUpdateSpeed;
        unsigned int                                    m_particleCount;

        DirectX::XMFLOAT4                               m_planeOrigin; 

        // Particle rendering/update flags
        bool                                            m_dataInitialized;
        bool                                            m_renderParticles;
        bool                                            m_updateParticles;

        // Constant buffer data
        PerComponent                                    m_cbVertex;
        ParticleUpdateConstants                         m_cbCompute;
    };
}
