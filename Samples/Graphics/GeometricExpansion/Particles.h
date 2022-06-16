//--------------------------------------------------------------------------------------
// Particles.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Shared.h"
#include "DeviceResources.h"

namespace ATG
{
    struct ValueRange
    {
        float Min;
        float Max;

        float Span() const noexcept { return Max - Min; }
        float Gen() const noexcept { return ((float)rand() / RAND_MAX) * Span() + Min; }
        float Interp(float t) const noexcept { return t * Span() + Min; }

        ValueRange() : Min(0.0f), Max(1.0f) { }
        ValueRange(float min, float max) : Min(min), Max(max) { }
        ValueRange(float max) : Min(0.0f), Max(max) { }

        ValueRange operator*(float val) { return ValueRange{ Min * val, Max * val }; }
        void operator*=(float val) { Min *= val, Max *= val; }
    };

    class ParticleSystem
    {
    public:
        ParticleSystem(
            DirectX::FXMVECTOR position,
            float spawnRate,
            float springCoeff,
            float dragFactor,
            float speed,
            ValueRange lifetime,
            ValueRange size
        );

        void SetSpawnRate(float spawnRate) { m_spawnFrequency = 1.0f / std::max(1e-4f, spawnRate); }
        void SetSpringCoefficent(float coeff) { m_springCoeff = std::max(1e-4f, coeff); }
        void SetDragFactor(float factor) { m_dragFactor = std::max(0.0f, factor); }
        void SetInitialSpeed(float speed) { m_speed = std::max(1e-4f, speed); }

        void CreateResources(DX::DeviceResources& deviceResources);
        void ReloadPipelineState(DX::DeviceResources& deviceResources);

        void Update(float deltaTime, const DirectX::SimpleMath::Matrix& view, const DirectX::SimpleMath::Matrix& proj);
        void Draw(ID3D12GraphicsCommandList6* commandList, DirectX::GraphicsMemory* graphicsMemory);

    private:
        void TryResizeBuffers();

    private:
        // Spawning properties
        DirectX::XMFLOAT3       m_position;
        float                   m_spawnFrequency;
        float                   m_springCoeff;
        float                   m_dragFactor;
        float                   m_simulationTime;
        float                   m_speed;
        ValueRange              m_angle;
        ValueRange              m_lifetime;
        ValueRange              m_size;

        // CPU Simulation State
        Constants               m_constants;
        std::vector<Particle>   m_particles;

        // D3D12 API Objects
        ID3D12Device*           m_device;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_particlePSO;

        Microsoft::WRL::ComPtr<ID3D12Resource>      m_constantBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_particleBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_particleBufferTemp;
    };
}
