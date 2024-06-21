//--------------------------------------------------------------------------------------
// Particles.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Particles.h"

#pragma warning( disable : 4324 4365 )

using namespace ATG;
using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
    const wchar_t* s_meshShaderFilename = L"PointSpritesMS.cso";
    const wchar_t* s_pixelShaderFilename = L"BasicPS.cso";

    constexpr float c_fixedStep = 1.0f / 30.0f; // Simulate at 30Hz

    // Some physicsl constants
    constexpr float c_particleMass = 1.0f;
    constexpr float c_particleMassInv = 1.0f / c_particleMass;
    
    constexpr float c_mediumDensity = 1.0f;     // Density of mystery medium
    constexpr float c_crossSectionCoeff = 0.5f; // Spoofed cross-sectional coefficient
    constexpr float c_dragCoeff = 0.5f * c_crossSectionCoeff * c_mediumDensity; // Drag Coefficient = (1/2) * C * p

    constexpr size_t c_particleBufferInitSize = 10000; // Set a high min count to reduce allocation ramp-up on app start.

    template <typename T>
    constexpr T RoundUpDiv(T num, T denom)
    {
        return (num + denom - 1) / denom;
    }

    template <typename T>
    size_t GetAlignedSize(T size)
    {
        const size_t alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
        const size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
        return alignedSize;
    }
}

ParticleSystem::ParticleSystem(
    FXMVECTOR position,
    float spawnRate,
    float springCoeff,
    float dragFactor,
    float speed,
    ValueRange lifetime,
    ValueRange size)
    : m_position{}
    , m_spawnFrequency(1.0f / spawnRate)
    , m_springCoeff(springCoeff)
    , m_dragFactor(dragFactor)
    , m_simulationTime(0.0f)
    , m_speed(speed)
    , m_angle(XM_2PI)
    , m_lifetime(lifetime)
    , m_size(size)
    , m_particles{}
{
    assert(spawnRate > 0.0f);

    XMStoreFloat3(&m_position, position);
    m_particles.reserve(c_particleBufferInitSize); // Prime particle list with arbitrarily large allocation
}

void ParticleSystem::CreateResources(DX::DeviceResources& deviceResources)
{
    m_device = deviceResources.GetD3DDevice();

    const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    auto cbDesc = CD3DX12_RESOURCE_DESC::Buffer(GetAlignedSize(sizeof(Constants)));

    DX::ThrowIfFailed(
        m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &cbDesc,
#ifdef _GAMING_XBOX
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
#else
            // on PC buffers are created in the common state and can be promoted without a barrier to VERTEX_AND_CONSTANT_BUFFER on first access
            D3D12_RESOURCE_STATE_COMMON,
#endif
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_constantBuffer.ReleaseAndGetAddressOf())));

    ReloadPipelineState(deviceResources);
}

void ParticleSystem::ReleaseDevice()
{
    m_rootSignature.Reset();
    m_particlePSO.Reset();
    m_constantBuffer.Reset();
    m_particleBuffer.Reset();
    m_particleBufferTemp.Reset();
    m_device.Reset();
}


void ParticleSystem::Update(float dt, const Matrix& view, const Matrix& proj)
{
    m_simulationTime += dt;

    // Kill dead particles
    for (size_t i = 0; i < m_particles.size(); ++i)
    {
        auto& p = m_particles[i];

        if (m_simulationTime - p.InitTime > p.Lifetime)
        {
            if (i != m_particles.size() - 1)
            {
                // Just overwrite it with the last particle and continue.
                p = m_particles.back();
                --i;
            }

            m_particles.pop_back();
        }
    }

    // Spawn new particles
    float lastSpawnTime = floorf((m_simulationTime - dt) / m_spawnFrequency) * m_spawnFrequency;

    size_t newCount = static_cast<uint32_t>(floorf((m_simulationTime - lastSpawnTime) / m_spawnFrequency));
    size_t oldCount = m_particles.size();

    m_particles.resize(oldCount + newCount);

    // Initialize new particles
    for (size_t i = oldCount; i < m_particles.size(); ++i)
    {
        float angle = m_angle.Gen(); // Set off in random direction in xz plane
        Vector3 direction(cosf(angle), 0, sinf(angle));

        Vector3 velocity;
        velocity = direction * m_speed; // Random speed in xz
        velocity.y = m_speed;           // Initial kick in y to break equilibrium

        auto& p = m_particles[i];
        p.Position = m_position;
        p.Velocity = velocity;
        p.InitTime = m_simulationTime;
        p.Lifetime = m_lifetime.Gen();
        p.Size     = m_size.Gen();
    }

    // Perform integration at fixed time intervals for stability
    for (auto& p : m_particles)
    {
        // Simulate particles over dt using a fixed timestep
        for (float accum = dt; accum >= 0; accum -= c_fixedStep)
        {
            // Only compute drag for horizontal velocity
            Vector3 velXZ = p.Velocity;
            velXZ.y = 0;

            float length = velXZ.Length();
            Vector3 direction = velXZ / length;

            // Compute forces
            Vector3 force;
            force.y = -m_springCoeff * (p.Position.y - m_position.y);     // Spring force proportional to displacement in y from rest position
            force.x = -m_dragFactor * c_dragCoeff * length * direction.x; // Drag force proportional to velocity in xz
            force.z = -m_dragFactor * c_dragCoeff * length * direction.z;

            Vector3 acceleration = force * c_particleMassInv;

            // Perform integration
            float stepSize = std::min(accum, c_fixedStep);

            p.Velocity = p.Velocity + acceleration * stepSize;
            p.Position = p.Position + p.Velocity * stepSize;
        }
    }

    // Update constants
    m_constants.ViewProj = (view * proj).Transpose();
    m_constants.ViewPosition = Vector3::Transform(Vector3::Zero, view.Invert());
    m_constants.ParticleCount = static_cast<uint32_t>(m_particles.size());
    m_constants.CameraUp = Vector3::TransformNormal(Vector3::Up, view.Invert());
    m_constants.SimulationTime = m_simulationTime;
    XMStoreFloat3(&m_constants.Color, DirectX::Colors::Red);

    // Update GPU Buffers
    TryResizeBuffers();
}

void ParticleSystem::Draw(ID3D12GraphicsCommandList6* commandList, GraphicsMemory* graphicsMemory)
{
    // Copy cpu memory to upload buffer
    auto upload = graphicsMemory->Allocate(GetAlignedSize(sizeof(Constants)) + m_particles.size() * sizeof(Particle));

    std::memcpy(static_cast<uint8_t*>(upload.Memory()), &m_constants, sizeof(Constants));
    std::memcpy(static_cast<uint8_t*>(upload.Memory()) + GetAlignedSize(sizeof(Constants)), m_particles.data(), m_particles.size() * sizeof(Particle));

    // Copy from upload buffer to resource
    DirectX::TransitionResource(commandList, m_particleBuffer.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
    DirectX::TransitionResource(commandList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

    commandList->CopyBufferRegion(m_constantBuffer.Get(), 0, upload.Resource(), 0, sizeof(Constants));
    commandList->CopyBufferRegion(m_particleBuffer.Get(), 0, upload.Resource(), GetAlignedSize(sizeof(Constants)), m_particles.size() * sizeof(Particle));

    DirectX::TransitionResource(commandList, m_particleBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    DirectX::TransitionResource(commandList, m_constantBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

    // Draw
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->SetPipelineState(m_particlePSO.Get());
        
    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffer->GetGPUVirtualAddress());
    commandList->SetGraphicsRootShaderResourceView(1, m_particleBuffer->GetGPUVirtualAddress());

    auto count = RoundUpDiv((uint32_t)m_particles.size(), GROUP_SIZE / 4u);
    commandList->DispatchMesh(count, 1, 1);
}

void ParticleSystem::ReloadPipelineState(DX::DeviceResources& deviceResources)
{
    auto device = deviceResources.GetD3DDevice();

    auto meshShader = DX::ReadData(s_meshShaderFilename);
    auto pixelShader = DX::ReadData(s_pixelShaderFilename);

    DX::ThrowIfFailed(device->CreateRootSignature(0, meshShader.data(), meshShader.size(), IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature        = m_rootSignature.Get();
    psoDesc.MS                    = { meshShader.data(), meshShader.size() };
    psoDesc.PS                    = { pixelShader.data(), pixelShader.size() };
    psoDesc.NumRenderTargets      = 1;
    psoDesc.RTVFormats[0]         = deviceResources.GetBackBufferFormat();
    psoDesc.DSVFormat             = deviceResources.GetDepthBufferFormat();
    psoDesc.RasterizerState       = CommonStates::CullNone;
    psoDesc.BlendState            = CommonStates::AlphaBlend;
    psoDesc.DepthStencilState     = CommonStates::DepthRead;
    psoDesc.SampleMask            = UINT_MAX;
    psoDesc.SampleDesc            = DefaultSampleDesc();
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        
    auto meshStreamDesc = CD3DX12_PIPELINE_MESH_STATE_STREAM(psoDesc);

    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
    streamDesc.SizeInBytes                   = sizeof(meshStreamDesc);
    streamDesc.pPipelineStateSubobjectStream = &meshStreamDesc;

    DX::ThrowIfFailed(device->CreatePipelineState(&streamDesc, IID_GRAPHICS_PPV_ARGS(m_particlePSO.ReleaseAndGetAddressOf())));
}

void ParticleSystem::TryResizeBuffers()
{
    assert(m_device != nullptr);

    constexpr size_t minSize = sizeof(Particle) * c_particleBufferInitSize; 

    size_t newSize = m_particles.size() * sizeof(Particle) * 2; // Over-allocate by two to reduce number of allocations
    newSize = std::max(newSize, minSize);

    m_particleBufferTemp = nullptr; // Release the old particle buffer, which we cached for a frame to avoid read after release

    // Don't resize if it's beneath our high watermark
    if (m_particleBuffer != nullptr)
    {
        if (newSize < m_particleBuffer->GetDesc().Width)
        {
            return;
        }
    }
                
    m_particleBufferTemp = m_particleBuffer; // Temporarily cache the old particle buffer to avoid releasing before its last usage is complete.

    // Reallocate particle buffer
    const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    auto particleDesc = CD3DX12_RESOURCE_DESC::Buffer(GetAlignedSize(newSize));

    DX::ThrowIfFailed(
        m_device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &particleDesc,
#ifdef _GAMING_XBOX
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
#else
            // on PC buffers are created in the common state and can be promoted without a barrier to NON_PIXEL_SHADER_RESOURCE on first access
            D3D12_RESOURCE_STATE_COMMON,
#endif
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_particleBuffer.ReleaseAndGetAddressOf())));
}
