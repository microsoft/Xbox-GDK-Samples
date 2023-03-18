//--------------------------------------------------------------------------------------
// LightParticle.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LightParticle.h"

#define PI  3.14159265f

using namespace Microsoft::WRL;
using namespace DirectX::SimpleMath;
using namespace DirectX;

namespace
{
    inline float FloatRand(float min, float max)
    {
        return min + ((float)rand() * (max - min) / RAND_MAX);
    }
}

ParticleSystem::ParticleSystem()
{
}

ParticleSystem::~ParticleSystem()
{
}

uint32_t ParticleSystem::GetLightCount()
{
    return m_particleCount;
}

void ParticleSystem::InitializePerParticleData(std::unique_ptr<DX::DeviceResources>& m_deviceResources)
{
    static const float g_LightRange = 15.0f;

    float *pAngles = new float[m_particleCount];
    PerParticleData *pMotionData = new PerParticleData[m_particleCount];

    Vector3 DiasCenterLow(0.0f, -160.0f, -210.0f);
    Vector3 DiasCenterHigh(0.0f, 0.0f, -210.0f);
    Vector3 CorridorCenterLeft(-23.0f, -160.0f, 104.0f);
    Vector3 CorridorCenterRight(23.0f, -160.0f, 104.0f);
    float MaxDiasRadius = 107.0f;
    float MaxRadiusShift = 18.0f;
    float MaxCorridorLongRadius = 202.0f;
    float MaxCorridorShortRadius = 100.0f;
    float CorridorCenterLeftYSpread = 126.0f;

    for (uint32_t i = 0; i < m_particleCount; ++i)
    {
        uint32_t Group = i % 3;
        pAngles[i] = FloatRand(0.0f, PI * 2.0f);
        pMotionData[i].AngleSpeed = FloatRand(0.01f, 0.1f);
        if (Group == 0) // Orbiting the dias chamber.
        {
            pMotionData[i].Radii.x = pMotionData[i].Radii.y = FloatRand(MaxDiasRadius - MaxRadiusShift, MaxDiasRadius);
            pMotionData[i].OrbitCenter = DiasCenterLow;
            pMotionData[i].OrbitCenter.y = FloatRand(DiasCenterLow.y, DiasCenterHigh.y);
        }
        else if (Group == 1)
        {
            pMotionData[i].Radii.y = FloatRand(MaxCorridorLongRadius - MaxRadiusShift, MaxCorridorLongRadius);
            pMotionData[i].Radii.x = FloatRand(MaxCorridorShortRadius - MaxRadiusShift, MaxCorridorShortRadius);
            pMotionData[i].OrbitCenter = CorridorCenterLeft;
            pMotionData[i].OrbitCenter.y = FloatRand(CorridorCenterLeft.y, CorridorCenterLeft.y + CorridorCenterLeftYSpread);
        }
        else if (Group == 2)
        {
            pMotionData[i].Radii.y = FloatRand(MaxCorridorLongRadius - MaxRadiusShift, MaxCorridorLongRadius);
            pMotionData[i].Radii.x = FloatRand(MaxCorridorShortRadius - MaxRadiusShift, MaxCorridorShortRadius);
            pMotionData[i].OrbitCenter = CorridorCenterRight;
            pMotionData[i].OrbitCenter.y = FloatRand(CorridorCenterLeft.y, CorridorCenterLeft.y + CorridorCenterLeftYSpread);
        }

        pMotionData[i].Range = g_LightRange;
    }

    // Copy into the SRV with the per particle data
    memcpy(m_PerParticleDataMappedMem, pMotionData, m_particleCount * sizeof(PerParticleData));
    m_PerParticleDataUpload->Unmap(0, nullptr);

    // Copy into the UAV with the angle data for each particle
    memcpy(m_ParticleAnglesMappedMem, pAngles, m_particleCount * sizeof(float));
    m_ParticleAnglesUpload->Unmap(0, nullptr);

    // Get the command list to execute copies into the default heaps
    auto cmdList = m_deviceResources->GetCommandList();
    m_deviceResources->ResetCommandList();

    // Copy upload resource to destination resources
    cmdList->CopyResource(m_PerParticleDataSRV.Get(), m_PerParticleDataUpload.Get());
    cmdList->CopyResource(m_ParticleAnglesUAV.Get(), m_ParticleAnglesUpload.Get());

    // Resource barrier
    D3D12_RESOURCE_BARRIER barriers[2];
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_PerParticleDataSRV.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_ParticleAnglesUAV.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

    cmdList->Close();
    ID3D12CommandList* cmdListArr[] = { cmdList };
    m_deviceResources->GetCommandQueue()->ExecuteCommandLists(1, cmdListArr);

    // Cleaning up
    delete [] pAngles;
    delete [] pMotionData;
}

void ParticleSystem::UpdateSceneViewFrustrum(Matrix const& P)
{
    m_sceneFrustrum[0].x = P._14 + P._11;
    m_sceneFrustrum[0].y = P._24 + P._21;
    m_sceneFrustrum[0].z = P._34 + P._31;
    m_sceneFrustrum[0].w = P._44 + P._41;

    m_sceneFrustrum[1].x = P._14 - P._11;
    m_sceneFrustrum[1].y = P._24 - P._21;
    m_sceneFrustrum[1].z = P._34 - P._31;
    m_sceneFrustrum[1].w = P._44 - P._41;

    m_sceneFrustrum[2].x = P._14 + P._12;
    m_sceneFrustrum[2].y = P._24 + P._22;
    m_sceneFrustrum[2].z = P._34 + P._32;
    m_sceneFrustrum[2].w = P._44 + P._42;
    
    m_sceneFrustrum[3].x = P._14 - P._12;
    m_sceneFrustrum[3].y = P._24 - P._22;
    m_sceneFrustrum[3].z = P._34 - P._32;
    m_sceneFrustrum[3].w = P._44 - P._42;
    
    m_sceneFrustrum[4].x = P._13;
    m_sceneFrustrum[4].y = P._23;
    m_sceneFrustrum[4].z = P._33;
    m_sceneFrustrum[4].w = P._43;
    
    m_sceneFrustrum[5].x = P._14 - P._13;
    m_sceneFrustrum[5].y = P._24 - P._23;
    m_sceneFrustrum[5].z = P._34 - P._33;
    m_sceneFrustrum[5].w = P._44 - P._43;

    for (uint32_t i = 0; i < 6; ++i)
    {
        m_sceneFrustrum[i].Normalize();
    }
}

void ParticleSystem::Update(ID3D12GraphicsCommandList* cmdList, Matrix const& view,
    Matrix const& proj, float deltaTimeMS, uint32_t currentFrame, float lightRadius)
{
    PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, L"Particle-Pos-Update");
    std::ignore = deltaTimeMS;

    ID3D12DescriptorHeap* heaps[] = { m_shaderVisibleHeap->Heap() };
    cmdList->SetDescriptorHeaps(static_cast<uint32_t>(std::size(heaps)), heaps);

    // Clear the counter value on every update
    UINT clearValues[4] = {};
    cmdList->ClearUnorderedAccessViewUint(
        m_shaderVisibleHeap->GetGpuHandle(UAV_PARTICLE_POSITIONS_COUNTER),
        m_nonShaderVisibleHeap->GetFirstCpuHandle(),
        m_ParticlePosCounterRes.Get(),
        clearValues,
        0U,
        nullptr);

    cmdList->SetComputeRootSignature(m_particleComputeRS.Get());

    uint32_t startingDescriptor = 1;
    cmdList->SetComputeRootDescriptorTable(0, m_shaderVisibleHeap->GetGpuHandle(startingDescriptor));

    // Update the scene frustrum
    UpdateSceneViewFrustrum(proj);

    // Bind the CBV that has update constant data
    UpdateConstantsCB constants = {};
    constants.g_ParticleData.x = 0.035f; // x = movement step
    constants.g_ParticleData.y = lightRadius; // Light particle radius
    constants.g_ParticleData.z = static_cast<float>(m_particleCount); // Light particle radius
    constants.g_View = view.Transpose();
    constants.g_ViewFrustum[0] = m_sceneFrustrum[0];
    constants.g_ViewFrustum[1] = m_sceneFrustrum[1];
    constants.g_ViewFrustum[2] = m_sceneFrustrum[2];
    constants.g_ViewFrustum[3] = m_sceneFrustrum[3];
    constants.g_ViewFrustum[4] = m_sceneFrustrum[4];
    constants.g_ViewFrustum[5] = m_sceneFrustrum[5];
    memcpy(&m_updateConstantsMappedMem[currentFrame].data, &constants, sizeof(UpdateConstantsCB));
    D3D12_GPU_VIRTUAL_ADDRESS gpuAddress = m_updateConstantsAddress + currentFrame * sizeof(UpdateConstantsCBPadded);
    cmdList->SetComputeRootConstantBufferView(1, gpuAddress);

    // SetPipelineState
    cmdList->SetPipelineState(m_particleComputePSO.Get());

    // Dispatch
    uint32_t threadgroupSize = 128; // This must match what's in ParticleCompute.hlsl
    uint32_t dispatchCount = (m_particleCount + threadgroupSize - 1) / threadgroupSize;
    cmdList->Dispatch(dispatchCount, 1, 1);

    // Copy counter from resource to the indirect buffer
    {
        D3D12_RESOURCE_BARRIER barriers[2];
        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_indArgsRes.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_ParticlePosCounterRes.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
        cmdList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

        // Copy into the indirect buffer resource
        if (m_firstFrame)
        {
            cmdList->CopyResource( m_indArgsRes.Get(), m_indArgsUploadRes.Get());
            m_firstFrame = false;
        }

        // sizeof(uint64_t) is the gpuVAddress, sizeof(uint32_t) is the first arg of the draw call
        uint64_t addressInBytes = (currentFrame * sizeof(IndirectArguments)) + sizeof(uint64_t) + sizeof(uint32_t);

        // Only update for current frame -> (currentFrame * sizeof(IndirectArguments))
        cmdList->CopyBufferRegion(
            m_indArgsRes.Get(),
            addressInBytes,
            m_ParticlePosCounterRes.Get(),
            0,
            sizeof(uint32_t));

        barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_ParticlePosCounterRes.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(m_indArgsRes.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
        cmdList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);
    }

    PIXEndEvent(cmdList);
}

void ParticleSystem::Render(ID3D12GraphicsCommandList* cmdList, uint32_t currentFrame,
    Matrix const& view, Matrix const& proj)
{
    PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, L"Particle-Render");

    D3D12_RESOURCE_BARRIER barriers[1];
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_ParticlePosRes.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

    cmdList->SetGraphicsRootSignature(m_particleDrawRS.Get());

    LightParticlesSceneConsts perObjectData = {};
    perObjectData.g_ViewProj = (view * proj).Transpose();
    memcpy(&m_PerObjectCBMappedMem[currentFrame].data, &perObjectData, sizeof(LightParticlesSceneConsts));
    uint64_t address = m_PerObjectCBAddress + currentFrame * sizeof(LightParticlesSceneConstsPadded);
    cmdList->SetGraphicsRootConstantBufferView(0, address);

    cmdList->SetPipelineState(m_particleDrawPSO.Get());

    cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    cmdList->IASetIndexBuffer(&m_particleIBV);
    cmdList->IASetVertexBuffers(0, 1, &m_particleVBV);

    // The argbuffer we use depends on which frame we are in
    uint64_t argBufferOffset = currentFrame * sizeof(IndirectArguments);

    // Execute indirect
    cmdList->ExecuteIndirect(m_cmdSignature.Get(), 1/*max cmd count*/, m_indArgsRes.Get(), argBufferOffset, nullptr, 0);

    // The resource that contains the particle's positions need ts to be transitioned back to UAV for the compute pass
    barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(m_ParticlePosRes.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    cmdList->ResourceBarrier(static_cast<uint32_t>(std::size(barriers)), barriers);

    PIXEndEvent(cmdList);
}

void ParticleSystem::initialize(std::unique_ptr<DX::DeviceResources>& m_deviceResources, uint32_t maxNumberOfLights)
{
    // Create all the d3d12 heaps, resources and views that this class will use.
    m_particleCount = maxNumberOfLights;
    assert(m_particleCount > 0);

    uint64_t framesInflight = m_deviceResources->GetBackBufferCount();
    ID3D12Device* device = m_deviceResources->GetD3DDevice();

    // Create descriptor heap - This will be used for the particle's compute shader.
    m_shaderVisibleHeap = std::make_unique<DescriptorHeap>(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        HEAP_DESCRIPTOR_COUNT);

    m_nonShaderVisibleHeap = std::make_unique<DescriptorHeap>(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        1);

    // Create the resources (for SRV and UAV).
    {
        D3D12_HEAP_PROPERTIES heapPropsDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        D3D12_HEAP_PROPERTIES heapPropsUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        /// SRV buffer that holds all the per particle data (motion data)
        size_t bufferSize = m_particleCount * sizeof(PerParticleData);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

        // Upload (intermediate) resource 
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapPropsUpload,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_PerParticleDataUpload.ReleaseAndGetAddressOf())));
        m_PerParticleDataUpload->SetName(L"PerParticleDataResUpload");
        DX::ThrowIfFailed(
            m_PerParticleDataUpload->Map(0, nullptr, reinterpret_cast<void**>(&m_PerParticleDataMappedMem)));

        // Destination resource
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapPropsDefault,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_PerParticleDataSRV.ReleaseAndGetAddressOf())));
        m_PerParticleDataSRV->SetName(L"PerParticleDataResource");

        /// Resource that holds the angle data per particle.
        bufferSize = m_particleCount * sizeof(float);
        resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

        // Upload (intermediate) resource 
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapPropsUpload,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_ParticleAnglesUpload.ReleaseAndGetAddressOf())));
        m_ParticleAnglesUpload->SetName(L"PerParticleAngleResUpload");

        DX::ThrowIfFailed(
            m_ParticleAnglesUpload->Map(0, nullptr, reinterpret_cast<void**>(&m_ParticleAnglesMappedMem)));

        // Destination resource
        resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapPropsDefault,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_ParticleAnglesUAV.ReleaseAndGetAddressOf())));
        m_ParticleAnglesUAV->SetName(L"ParticleAnglesResource");

        /// Resource for the particle's position. Later passed to ExecuteIndirect.
        bufferSize = m_particleCount * sizeof(LightParticlePosData);
        resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapPropsDefault,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_ParticlePosRes.ReleaseAndGetAddressOf())));
        m_ParticlePosRes->SetName(L"ParticlePositionsRes");

        // UAV Counter for ParticlePositionsRes
        bufferSize = sizeof(uint32_t);
        resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        resDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapPropsDefault,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_ParticlePosCounterRes.ReleaseAndGetAddressOf())));
        m_ParticlePosCounterRes->SetName(L"ParticlePositionsCounterRes");
    }

    // Create the views (UAV and Indirect stuff).
    {
        // UAV which holds the angle data per particle.
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
            uavDesc.Buffer.NumElements = m_particleCount;
            uavDesc.Buffer.StructureByteStride = 0;
            device->CreateUnorderedAccessView(m_ParticleAnglesUAV.Get(), nullptr, &uavDesc, m_shaderVisibleHeap->GetCpuHandle(UAV_PARTICLE_ANGLES));
        }

        // UAV that holds the position for each particle (plus its UAV counter).
        {
            D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Format = DXGI_FORMAT_UNKNOWN;
            uavDesc.Buffer.NumElements = m_particleCount;
            uavDesc.Buffer.StructureByteStride = sizeof(LightParticlePosData);
            device->CreateUnorderedAccessView(m_ParticlePosRes.Get(), m_ParticlePosCounterRes.Get(), &uavDesc, m_shaderVisibleHeap->GetCpuHandle(UAV_PARTICLE_POSITIONS));

            uavDesc = {};
            uavDesc.Format = DXGI_FORMAT_R32_UINT;
            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            uavDesc.Buffer.NumElements = 1;
            uavDesc.Buffer.StructureByteStride = 0;
            device->CreateUnorderedAccessView(m_ParticlePosCounterRes.Get(), nullptr, &uavDesc, m_nonShaderVisibleHeap->GetFirstCpuHandle());

            device->CopyDescriptorsSimple(
                1,                                                                  // Just the counter resource desc
                m_shaderVisibleHeap->GetCpuHandle(UAV_PARTICLE_POSITIONS_COUNTER),  // dst (shader-visible heap)
                m_nonShaderVisibleHeap->GetFirstCpuHandle(),                        // src (non-shader-visible heap)
                D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        // SRV buffer that holds all the per particle data (motion data)
        {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;
            srvDesc.Buffer.NumElements = m_particleCount;
            srvDesc.Buffer.StructureByteStride = sizeof(PerParticleData);
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            device->CreateShaderResourceView(m_PerParticleDataSRV.Get(), &srvDesc, m_shaderVisibleHeap->GetCpuHandle(SRV_PER_PARTICLE_DATA));
        }
    }

    // Initialize data, this cannot happen before creating the resources.
    InitializePerParticleData(m_deviceResources);

    // Create the resource for the CBVs (one for compute pass, other for indirect draw).
    {
        // For Compute Pass (culling and particle positions).
        D3D12_HEAP_PROPERTIES heapPropsUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        size_t dispatchCallsPerFrame = 1;
        size_t bufferSize = framesInflight * dispatchCallsPerFrame * sizeof(UpdateConstantsCBPadded);
        D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapPropsUpload,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_updateConstantsRes.ReleaseAndGetAddressOf())));
        m_updateConstantsRes->SetName(L"UpdateConstantsCBResource");

        m_updateConstantsAddress = m_updateConstantsRes->GetGPUVirtualAddress();
        DX::ThrowIfFailed(
            m_updateConstantsRes->Map(0, nullptr, reinterpret_cast<void**>(&m_updateConstantsMappedMem)));

        // For Indirect Draw Call.
        size_t indirectCallsPerFrame = 1;
        bufferSize = framesInflight * indirectCallsPerFrame * sizeof(LightParticlesSceneConstsPadded);
        resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(device->CreateCommittedResource(
            &heapPropsUpload,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_GRAPHICS_PPV_ARGS(m_PerObjectCBRes.ReleaseAndGetAddressOf())));
        m_PerObjectCBRes->SetName(L"PerObjectCBResource");

        m_PerObjectCBAddress = m_PerObjectCBRes->GetGPUVirtualAddress();
        DX::ThrowIfFailed(
            m_PerObjectCBRes->Map(0, nullptr, reinterpret_cast<void**>(&m_PerObjectCBMappedMem)));
    }

    // Load Shader, create RS and PSO's.
    // This is for the compute shader that handles the particle movement.
    {
        auto csBlob = DX::ReadData(L"ParticlesCompute.cso");
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, csBlob.data(), csBlob.size(), IID_GRAPHICS_PPV_ARGS(m_particleComputeRS.ReleaseAndGetAddressOf())));

        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = m_particleComputeRS.Get();
        psoDesc.CS = { csBlob.data(), csBlob.size() };
        DX::ThrowIfFailed(
            device->CreateComputePipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_particleComputePSO.ReleaseAndGetAddressOf())));
        m_particleComputePSO->SetName(L"ParticleComputePSO");
    }

    // For the particles drawing, setup mesh, PSO, input layout and Root Signature
    {
        // Root signature
        auto vsBlob = DX::ReadData(L"ParticleVS.cso");
        auto psBlob = DX::ReadData(L"ParticlePS.cso");
        DX::ThrowIfFailed(
            device->CreateRootSignature(0, vsBlob.data(), vsBlob.size(), IID_GRAPHICS_PPV_ARGS(m_particleDrawRS.ReleaseAndGetAddressOf())));

        // Input layout
        D3D12_INPUT_ELEMENT_DESC gElemDescPosition = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        D3D12_INPUT_ELEMENT_DESC gElemDescColor = { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
        D3D12_INPUT_ELEMENT_DESC gInputElemArr[] = { gElemDescPosition, gElemDescColor };

        // Pipeline state object
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { gInputElemArr, static_cast<uint32_t>(std::size(gInputElemArr)) };
        psoDesc.pRootSignature = m_particleDrawRS.Get();
        psoDesc.VS = { vsBlob.data(), vsBlob.size() };
        psoDesc.PS = { psBlob.data(), psBlob.size() };
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleMask = UINT32_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = m_deviceResources->GetBackBufferFormat();
        psoDesc.SampleDesc.Count = 1;
        DX::ThrowIfFailed(
            device->CreateGraphicsPipelineState(&psoDesc, IID_GRAPHICS_PPV_ARGS(m_particleDrawPSO.ReleaseAndGetAddressOf())));
        m_particleDrawPSO->SetName(L"ParticleDrawPSO");

        m_particleMeshData.initialize();

        // Vertex buffer for particle rendering
        {
            D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            size_t bufferSize = m_particleMeshData.GetVertexSizeBytes();
            D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_particleVertexBufferRes.ReleaseAndGetAddressOf())));
            m_particleVertexBufferRes->SetName(L"ParticleVertexBufferRes");
        
            m_particleVBV.BufferLocation = m_particleVertexBufferRes->GetGPUVirtualAddress();
            m_particleVBV.SizeInBytes = static_cast<uint32_t>(bufferSize);
            m_particleVBV.StrideInBytes = m_particleMeshData.GetVertexStride();
        
            void* mappedMem = nullptr;
            DX::ThrowIfFailed(m_particleVertexBufferRes->Map(0, nullptr, &mappedMem));
            memcpy(reinterpret_cast<Vector3*>(mappedMem), m_particleMeshData.GetVertices(), bufferSize);
            m_particleVertexBufferRes->Unmap(0, nullptr);
        }

        // Index Buffer for particle rendering
        {
            D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            size_t bufferSize = m_particleMeshData.GetIndicesSizeBytes();
            D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_particleIndexBufferRes.ReleaseAndGetAddressOf())));
            m_particleIndexBufferRes->SetName(L"ParticleIndexBufferRes");

            m_particleIBV.BufferLocation = m_particleIndexBufferRes->GetGPUVirtualAddress();
            m_particleIBV.SizeInBytes = static_cast<uint32_t>(bufferSize);
            m_particleIBV.Format = m_particleMeshData.GetIndexFormat();

            void* mappedMem = nullptr;
            DX::ThrowIfFailed(m_particleIndexBufferRes->Map(0, nullptr, &mappedMem));
            memcpy(reinterpret_cast<Vector3*>(mappedMem), m_particleMeshData.GetIndices(), bufferSize);
            m_particleIndexBufferRes->Unmap(0, nullptr);
        }
    }

    //Execute Indirect related resources
    {
        // Command Signature
        D3D12_INDIRECT_ARGUMENT_DESC indArgsDesc[2] = {};
        indArgsDesc[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
        indArgsDesc[0].ShaderResourceView.RootParameterIndex = 1;
        indArgsDesc[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

        D3D12_COMMAND_SIGNATURE_DESC cmdSigDesc = {};
        cmdSigDesc.ByteStride = sizeof(IndirectArguments);
        cmdSigDesc.NumArgumentDescs = static_cast<uint32_t>(std::size(indArgsDesc));
        cmdSigDesc.pArgumentDescs = indArgsDesc;
        DX::ThrowIfFailed(
            device->CreateCommandSignature(&cmdSigDesc, m_particleDrawRS.Get(), IID_GRAPHICS_PPV_ARGS(m_cmdSignature.ReleaseAndGetAddressOf())));

        // One resource will be for indirect command buffer
        {
            D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            size_t bufferSize = framesInflight * sizeof(IndirectArguments);
            auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
            resDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
#ifdef _GAMING_XBOX
            resDesc.Flags |= D3D12XBOX_RESOURCE_FLAG_ALLOW_INDIRECT_BUFFER;
#endif

            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_indArgsRes.ReleaseAndGetAddressOf())));
            m_indArgsRes->SetName(L"EIArgumentsResource");
        }

        // This resource will be used to upload the initial indirect args
        {
            D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            size_t bufferSize = framesInflight * sizeof(IndirectArguments);
            auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_indArgsUploadRes.ReleaseAndGetAddressOf())));
            m_indArgsUploadRes->SetName(L"EIArgumentsResourceUpload");

            IndirectArguments* indArgsMappedMem = nullptr;
            DX::ThrowIfFailed(m_indArgsUploadRes->Map(0, nullptr, reinterpret_cast<void**>(&indArgsMappedMem)));

            IndirectArguments initialState[2] = {};
            for (size_t i = 0; i < framesInflight; ++i)
            {
                initialState[i].SRV1 = m_ParticlePosRes->GetGPUVirtualAddress();
                initialState[i].Draw.IndexCountPerInstance = m_particleMeshData.GetIndexCount();
                initialState[i].Draw.InstanceCount = 1;
            }
            memcpy(indArgsMappedMem, initialState, sizeof(initialState));
            m_indArgsUploadRes->Unmap(0, nullptr);
        }
    }
}

void ParticleSystem::Reset()
{
    m_shaderVisibleHeap.reset();
    m_nonShaderVisibleHeap.reset();

    m_updateConstantsRes.Reset();
    m_PerObjectCBRes.Reset();
    m_PerParticleDataUpload.Reset();
    m_PerParticleDataSRV.Reset();
    m_ParticleAnglesUpload.Reset();
    m_ParticleAnglesUAV.Reset();
    m_ParticlePosRes.Reset();
    m_ParticlePosCounterRes.Reset();
    m_particleComputeRS.Reset();
    m_particleComputePSO.Reset();
    m_particleDrawRS.Reset();
    m_particleDrawPSO.Reset();
    m_cmdSignature.Reset();
    m_indArgsRes.Reset();
    m_indArgsUploadRes.Reset();
    m_ICountResource.Reset();
    m_particleVertexBufferRes.Reset();
    m_particleIndexBufferRes.Reset();
}
