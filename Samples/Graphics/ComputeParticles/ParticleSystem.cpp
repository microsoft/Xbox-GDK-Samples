//--------------------------------------------------------------------------------------
// ParticleSystem.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "ParticleSystem.h"

// C4238: nonstandard extension used: class rvalue used as lvalue
#pragma warning(disable : 4238)

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
    // Various particle constants and default values.
    constexpr float g_maxOrbitRadius = 60.0f;
    constexpr float g_maxEmitterHeight = 25.0f;
    constexpr float g_minEmitterHeight = 2.0f;
    constexpr int   g_maxParticles = 2097152;
    constexpr int   g_minParticles = 128;
    constexpr float g_particleLifeMin = 24.0f;
    constexpr float g_particleLifeMax = 28.0f;
    constexpr float g_particleSpeedMax = 1.75f;
    constexpr float g_particleSpeedMin = 1.25f;
    constexpr float g_particleDirectionHorizontalStrength = 7.0f;
    constexpr float g_particleMaxMass = 1.75f;
    constexpr float g_particleMinMass = 0.65f;
    constexpr float g_defaultParticleUpdateSpeed = 4.0f;
    constexpr float g_minParticleUpdateSpeed = 0.1f;
    constexpr float g_maxParticleUpdateSpeed = 6.0f;

    // 1D-lookup table for the blackbody color spectrum - copied to an ID3D12Resource for shader reference.
    static const DWORD g_blackbodySpectrum[64] =
    {
        0xff380003,    0xff380011,    0xff38001f,    0xff38002d,    0xff38003b,    0xff380049,    0xff380057,    0xff380065,
        0xff380073,    0xff380081,    0xff38008f,    0xff38009d,    0xff3800ab,    0xff3800b9,    0xff3800c7,    0xff3800d5,
        0xff3800e3,    0xff3800f1,    0xff3800ff,    0xff5300ff,    0xff6500ff,    0xff7300ff,    0xff7e00ff,    0xff8912ff,
        0xff932cff,    0xff9d3fff,    0xffa54fff,    0xffad5eff,    0xffb46bff,    0xffbb78ff,    0xffc184ff,    0xffc78fff,
        0xffcc99ff,    0xffd1a3ff,    0xffd5adff,    0xffd9b6ff,    0xffddbeff,    0xffe1c6ff,    0xffe4ceff,    0xffe8d5ff,
        0xffebdcff,    0xffeee3ff,    0xfff0e9ff,    0xfff3efff,    0xfff5f5ff,    0xfff8fbff,    0xfef9ffff,    0xf9f6ffff,
        0xf5f3ffff,    0xf0f1ffff,    0xedefffff,    0xe9edffff,    0xe6ebffff,    0xe3e9ffff,    0xe0e7ffff,    0xdde6ffff,
        0xdae4ffff,    0xd8e3ffff,    0xd6e1ffff,    0xd3e0ffff,    0xd1dfffff,    0xcfddffff,    0xcedcffff,    0xccdbffff,
    };

    // Helper function to generate random value between min and max.
    inline float FloatRand(float min, float max)
    {
        return min + (((float)rand() / RAND_MAX) * (max - min));
    }

    // D3D12 blend state that performs additive color blending.
    const D3D12_BLEND_DESC g_additiveBlendDesc =
    {
        FALSE, // AlphaToCoverageEnable
        FALSE, // IndependentBlendEnable
        { {
                TRUE, // BlendEnable
                FALSE, // LogicOpEnable
                D3D12_BLEND_ONE, // SrcBlend
                D3D12_BLEND_ONE, // DestBlend
                D3D12_BLEND_OP_ADD, // BlendOp
                D3D12_BLEND_ZERO, // SrcBlendAlpha
                D3D12_BLEND_ONE, // DestBlendAlpha
                D3D12_BLEND_OP_ADD, // BlendOpAlpha
                D3D12_LOGIC_OP_NOOP,
                D3D12_COLOR_WRITE_ENABLE_ALL
            } }
    };

    // D3D12 depth stencil state that tests depth with a less than check, but disables depth writes and stencil.
    const D3D12_DEPTH_STENCIL_DESC g_depthLessNoZWriteDesc =
    {
        TRUE,                                           // DepthEnable
        D3D12_DEPTH_WRITE_MASK_ZERO,                    // DepthWriteMask
        D3D12_COMPARISON_FUNC_LESS,                     // DepthFunc
        FALSE,                                          // StencilEnable
        D3D12_DEFAULT_STENCIL_READ_MASK,                // StencilReadMask
        D3D12_DEFAULT_STENCIL_WRITE_MASK,               // StencilWriteMask
        {                                               // FrontFace - D3D11_DEPTH_STENCILOP_DESC
            D3D12_STENCIL_OP_KEEP,                      // StencilFailOp
            D3D12_STENCIL_OP_KEEP,                      // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,                      // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS                // StencilFunc
        },
        {                                               // BackFace - D3D11_DEPTH_STENCILOP_DESC
            D3D12_STENCIL_OP_KEEP,                      // StencilFailOp
            D3D12_STENCIL_OP_KEEP,                      // StencilDepthFailOp
            D3D12_STENCIL_OP_KEEP,                      // StencilPassOp
            D3D12_COMPARISON_FUNC_ALWAYS                // StencilFunc
        }
    };
}

namespace ATG
{
    ParticleSystem::ParticleSystem()
        : m_resetStart(0)
        , m_resetCount(0)
        , m_emitterRadius(4.0f)
        , m_emitterOrbitAngle(XM_PI)
        , m_emitterHeight(12.0f)
        , m_particleBounciness(0.35f)
        , m_particleUpdateSpeed(g_defaultParticleUpdateSpeed)
        , m_particleCount(4096)
        , m_planeOrigin()
        , m_spheres{}
        , m_geomCount(0)
        , m_dataInitialized(false)
        , m_renderParticles(true)
        , m_updateParticles(true)
        , m_cbVertex{}
        , m_cbCompute{}
    { }

    XMFLOAT3 ParticleSystem::GetEmitterPosition() const { return Vector3(m_emitterRadius * cosf(m_emitterOrbitAngle), m_emitterHeight, m_emitterRadius * sinf(m_emitterOrbitAngle)); }
    XMFLOAT4 ParticleSystem::GetEmitterRotation() const { return Quaternion::CreateFromAxisAngle(Vector3::UnitY, XM_PI - m_emitterOrbitAngle); }

    void ParticleSystem::Initialize(DX::DeviceResources* deviceResources, ResourceUploadBatch& resourceUpload)
    {
        auto device = deviceResources->GetD3DDevice();

        // Create descriptor heaps for our various resource descriptors.
        m_srvPile = std::make_unique<DescriptorPile>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
            128,
            HeapCount);
        m_srvPileCpu = std::make_unique<DescriptorPile>(device,
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            128,
            CPU_HeapCount);

        auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

        //-------------------------------------------------------
        // Create the buffer containing the initial particle data used when resetting particles.
        {
            auto descResetBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ParticleResetData) * g_maxParticles);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descResetBuffer,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_particleResetData.ReleaseAndGetAddressOf())));
            DX::ThrowIfFailed(m_particleResetData->SetName(L"Particle Reset Data"));

            D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
            descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            descSRV.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            descSRV.Buffer.NumElements = g_maxParticles;
            descSRV.Buffer.StructureByteStride = sizeof(ParticleResetData);
            device->CreateShaderResourceView(m_particleResetData.Get(), &descSRV, m_srvPile->GetCpuHandle(SRV_ParticleResetData));
        }

        //-------------------------------------------------------
        // Create the particle motion data buffer (containing current particle state) and views.
        {
            DX::ThrowIfFailed(
                CreateUAVBuffer(device, sizeof(ParticleMotionData) * g_maxParticles, m_particleMotionData.ReleaseAndGetAddressOf(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE));
            SetDebugObjectName(m_particleMotionData.Get(), L"Particle Motion Data");

            D3D12_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
            descUAV.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            descUAV.Buffer.NumElements = g_maxParticles;
            descUAV.Buffer.StructureByteStride = sizeof(ParticleMotionData);
            device->CreateUnorderedAccessView(m_particleMotionData.Get(), nullptr, &descUAV, m_srvPile->GetCpuHandle(UAV_ParticleMotionData));
        }

        //-------------------------------------------------------
        // Create the particle instance buffer and views
        {
            DX::ThrowIfFailed(
                CreateUAVBuffer(device, sizeof(XMFLOAT4) * g_maxParticles, m_particleInstance.ReleaseAndGetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
            SetDebugObjectName(m_particleInstance.Get(), L"Particle Instance");

            D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
            descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            descSRV.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            descSRV.Buffer.NumElements = g_maxParticles;
            descSRV.Buffer.StructureByteStride = sizeof(XMFLOAT4);

            device->CreateShaderResourceView(m_particleInstance.Get(), &descSRV, m_srvPile->GetCpuHandle(SRV_ParticleInstance));

            DX::ThrowIfFailed(
                CreateUAVBuffer(device, sizeof(UINT), m_particleCounter.ReleaseAndGetAddressOf(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
            SetDebugObjectName(m_particleCounter.Get(), L"Particle Counter");

            D3D12_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
            descUAV.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            descUAV.Buffer.NumElements = g_maxParticles;
            descUAV.Buffer.StructureByteStride = sizeof(XMFLOAT4);

            device->CreateUnorderedAccessView(m_particleInstance.Get(), m_particleCounter.Get(), &descUAV, m_srvPile->GetCpuHandle(UAV_ParticleInstance));

            // The second UAV is needed for the Clear operation to reset the counter
            D3D12_UNORDERED_ACCESS_VIEW_DESC descCounterUAV = {};
            descCounterUAV.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            descCounterUAV.Buffer.NumElements = 1;
            descCounterUAV.Buffer.StructureByteStride = sizeof(UINT);

            device->CreateUnorderedAccessView(m_particleCounter.Get(), nullptr, &descCounterUAV, m_srvPileCpu->GetCpuHandle(CPU_ParticleCounter));
            device->CreateUnorderedAccessView(m_particleCounter.Get(), nullptr, &descCounterUAV, m_srvPile->GetCpuHandle(UAV_ParticleCounter));
        }

        //-------------------------------------------------------
        // Create the indirect parameters buffer, used for ExecuteIndirect call.
        {
            auto descIndirectBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UINT) * 4, D3D12XBOX_RESOURCE_FLAG_PREFER_INDIRECT_BUFFER);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descIndirectBuffer,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_executeIndirectParams.ReleaseAndGetAddressOf())));

            DX::ThrowIfFailed(m_executeIndirectParams->SetName(L"Execute Indirect Params"));

            // Create a command signature for ExecuteIndirect
            D3D12_INDIRECT_ARGUMENT_DESC indirectParameter = {};
            indirectParameter.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

            UINT InstanceIndirectData[] = { 4, 0, 0, 0 };
            D3D12_COMMAND_SIGNATURE_DESC descCommandSignature = {};
            descCommandSignature.ByteStride = sizeof(InstanceIndirectData);
            descCommandSignature.NumArgumentDescs = 1;
            descCommandSignature.pArgumentDescs = &indirectParameter;

            DX::ThrowIfFailed(device->CreateCommandSignature(&descCommandSignature, nullptr, IID_GRAPHICS_PPV_ARGS(m_commandSignature.ReleaseAndGetAddressOf())));
        }

        //-------------------------------------------------------
        // Create the blackbody spectrum lookup texture resource.
        {
            const D3D12_RESOURCE_DESC descTex = CD3DX12_RESOURCE_DESC::Tex1D(DXGI_FORMAT_R8G8B8A8_UNORM, _countof(g_blackbodySpectrum), 1, 1);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descTex,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_blackbodyLut.ReleaseAndGetAddressOf())));

            D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
            descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            descSRV.Format = descTex.Format;
            descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
            descSRV.Texture1D.MipLevels = 1;
            device->CreateShaderResourceView(m_blackbodyLut.Get(), &descSRV, m_srvPile->GetCpuHandle(SRV_Blackbody));
        }

        //-------------------------------------------------------
        // Create the PSOs for particle computation.
        {
            auto advanceParticleCS = DX::ReadData(L"AdvanceParticlesCS.cso");
            auto resetIgnoredParticleCS = DX::ReadData(L"ResetIgnoredParticlesCS.cso");

            DX::ThrowIfFailed(device->CreateRootSignature(
                0,
                advanceParticleCS.data(),
                advanceParticleCS.size(),
                IID_GRAPHICS_PPV_ARGS(m_computeRS.ReleaseAndGetAddressOf())));

            D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
            descComputePSO.pRootSignature = m_computeRS.Get();
            descComputePSO.CS.pShaderBytecode = advanceParticleCS.data();
            descComputePSO.CS.BytecodeLength = advanceParticleCS.size();
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_advancePSO.ReleaseAndGetAddressOf())));

            descComputePSO.CS.pShaderBytecode = resetIgnoredParticleCS.data();
            descComputePSO.CS.BytecodeLength = resetIgnoredParticleCS.size();
            DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_resetPSO.ReleaseAndGetAddressOf())));
        }

        //-------------------------------------------------------
        // Create the PSO for particle rendering.

        {
            auto particleVS = DX::ReadData(L"ParticleVS.cso");
            auto particlePS = DX::ReadData(L"ParticlePS.cso");

            DX::ThrowIfFailed(device->CreateRootSignature(
                0,
                particleVS.data(),
                particleVS.size(),
                IID_GRAPHICS_PPV_ARGS(m_renderRS.ReleaseAndGetAddressOf())));

            // Set up appropriate states for particle rendering (depth to no-write, additive alpha blending)
            D3D12_GRAPHICS_PIPELINE_STATE_DESC descPSO = {};
            descPSO.pRootSignature = m_renderRS.Get();
            descPSO.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            descPSO.BlendState = g_additiveBlendDesc;
            descPSO.DepthStencilState = g_depthLessNoZWriteDesc;
            descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            descPSO.NumRenderTargets = 1;
            descPSO.RTVFormats[0] = deviceResources->GetBackBufferFormat();
            descPSO.DSVFormat = deviceResources->GetDepthBufferFormat();
            descPSO.SampleDesc.Count = 1;
            descPSO.SampleMask = UINT_MAX;

            descPSO.VS.pShaderBytecode = particleVS.data();
            descPSO.VS.BytecodeLength = particleVS.size();
            descPSO.PS.pShaderBytecode = particlePS.data();
            descPSO.PS.BytecodeLength = particlePS.size();

            DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPSO, IID_GRAPHICS_PPV_ARGS(m_renderPSO.ReleaseAndGetAddressOf())));
        }

        // Load the particle alpha-mask texture.
        DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, L"particle.dds", m_particleTex.ReleaseAndGetAddressOf()));
        device->CreateShaderResourceView(m_particleTex.Get(), nullptr, m_srvPile->GetCpuHandle(SRV_Particle));
    }

    void ParticleSystem::GenerateGeometry(const Model& model)
    {
        for (auto& mesh : model.meshes)
        {
            if (m_geomCount == 0)
            {
                auto v = XMLoadFloat3(&mesh->boundingBox.Center);
                XMStoreFloat4(&m_planeOrigin, v);
                m_planeOrigin.w = mesh->boundingBox.Extents.x;
            }
            else
            {
                auto v = XMLoadFloat3(&mesh->boundingSphere.Center);
                XMStoreFloat4(&m_spheres[m_geomCount - 1], v);
                m_spheres[m_geomCount - 1].w = mesh->boundingBox.Extents.x;
            }

            ++m_geomCount;
        }

        // Copy scene bounding geometry data to compute constant buffer
        m_cbCompute.Plane = m_planeOrigin;
        assert(m_geomCount == g_sphereCount + 1);
        for (UINT i = 0; i < g_sphereCount; ++i)
        {
            m_cbCompute.Spheres[i] = m_spheres[i];
        }
    }

    void ParticleSystem::Update(float elapsedTime, GamePad::State& pad, GamePad::ButtonStateTracker& gamePadButtons, FXMMATRIX view, CXMMATRIX proj)
    {
        m_resetStart = 0;
        m_resetCount = 0;

        if (gamePadButtons.a == GamePad::ButtonStateTracker::PRESSED)
        {
            m_renderParticles = !m_renderParticles;
        }

        if (gamePadButtons.b == GamePad::ButtonStateTracker::PRESSED)
        {
            m_updateParticles = !m_updateParticles;
        }

        if (pad.IsRightShoulderPressed())
        {
            m_emitterHeight += pad.thumbSticks.rightY * elapsedTime * 5.0f;
            m_emitterHeight = std::max(std::min(g_maxEmitterHeight, m_emitterHeight), g_minEmitterHeight);

            m_emitterOrbitAngle += pad.thumbSticks.leftX * elapsedTime;
            if (m_emitterOrbitAngle > XM_PI * 2)
            {
                m_emitterOrbitAngle -= XM_PI * 2;
            }
            else if (m_emitterOrbitAngle < 0)
            {
                m_emitterOrbitAngle += XM_PI * 2;
            }

            // Our emitter orbit radius is adjusted by left stick y-axis input, and is clamped to an allowed range
            m_emitterRadius -= pad.thumbSticks.leftY * elapsedTime * 12.0f;
            m_emitterRadius = std::max(std::min(g_maxOrbitRadius, m_emitterRadius), 0.0f);

            if (pad.triggers.right > 0.0f)
            {
                m_particleUpdateSpeed += pad.triggers.right * 255.0f * 0.01f * elapsedTime;
            }
            else if (pad.triggers.left > 0.0f)
            {
                m_particleUpdateSpeed -= pad.triggers.left * 255.0f * 0.01f * elapsedTime;
            }
            m_particleUpdateSpeed = std::min(std::max(g_minParticleUpdateSpeed, m_particleUpdateSpeed), g_maxParticleUpdateSpeed);
        }
        else
        {
            if (pad.triggers.right > 0.0f)
            {
                m_particleBounciness += pad.triggers.right * 255.0f * 0.001f * elapsedTime;
            }
            else if (pad.triggers.left > 0.0f)
            {
                m_particleBounciness -= pad.triggers.left * 255.0f * 0.001f * elapsedTime;
            }
            m_particleBounciness = std::min(std::max(0.0f, m_particleBounciness), 1.0f);
        }

        if (m_updateParticles)
        {
            // If we're currently updating particles, then allow user to change the active particle count.
            if (gamePadButtons.dpadDown == GamePad::ButtonStateTracker::PRESSED)
            {
                m_particleCount = std::max(g_minParticles, m_particleCount / 2);
            }
            else if (gamePadButtons.dpadUp == GamePad::ButtonStateTracker::PRESSED)
            {
                m_resetStart = m_particleCount;
                m_resetCount = g_maxParticles - m_resetStart;
                m_particleCount = std::min(g_maxParticles, m_particleCount * 2);
            }
        }

        // Store per-frame compute shader constants.
        m_cbCompute.ParticleData.x = elapsedTime * m_particleUpdateSpeed;
        m_cbCompute.ParticleData.y = m_particleBounciness;
        m_cbCompute.ActiveCount = uint32_t(m_resetStart);
        m_cbCompute.EmitterPosition = GetEmitterPosition();
        m_cbCompute.EmitterRotation = GetEmitterRotation();

        // Set constant data for vertex data (specifically, clip space scaling values used in particle rendering).
        m_cbVertex.ClipSpaceScale.x = XMVectorGetX(proj.r[0]);
        m_cbVertex.ClipSpaceScale.y = XMVectorGetY(proj.r[1]);
        m_cbVertex.ClipSpaceScale.z = m_cbVertex.ClipSpaceScale.w = 0;

        Matrix viewProj = view * proj;
        XMStoreFloat4x4(&m_cbVertex.ViewProj, XMMatrixTranspose(viewProj));

        // Grab the planes for the current view frustum, in world space, by using the combined camera/projection transform.
        XMFLOAT4X4 P;
        XMStoreFloat4x4(&P, viewProj);
        m_cbCompute.ViewFrustum[0].x = P._14 + P._11;
        m_cbCompute.ViewFrustum[0].y = P._24 + P._21;
        m_cbCompute.ViewFrustum[0].z = P._34 + P._31;
        m_cbCompute.ViewFrustum[0].w = P._44 + P._41;

        m_cbCompute.ViewFrustum[1].x = P._14 - P._11;
        m_cbCompute.ViewFrustum[1].y = P._24 - P._21;
        m_cbCompute.ViewFrustum[1].z = P._34 - P._31;
        m_cbCompute.ViewFrustum[1].w = P._44 - P._41;

        m_cbCompute.ViewFrustum[2].x = P._14 + P._12;
        m_cbCompute.ViewFrustum[2].y = P._24 + P._22;
        m_cbCompute.ViewFrustum[2].z = P._34 + P._32;
        m_cbCompute.ViewFrustum[2].w = P._44 + P._42;

        m_cbCompute.ViewFrustum[3].x = P._14 - P._12;
        m_cbCompute.ViewFrustum[3].y = P._24 - P._22;
        m_cbCompute.ViewFrustum[3].z = P._34 - P._32;
        m_cbCompute.ViewFrustum[3].w = P._44 - P._42;

        m_cbCompute.ViewFrustum[4].x = P._13;
        m_cbCompute.ViewFrustum[4].y = P._23;
        m_cbCompute.ViewFrustum[4].z = P._33;
        m_cbCompute.ViewFrustum[4].w = P._43;

        m_cbCompute.ViewFrustum[5].x = P._14 - P._13;
        m_cbCompute.ViewFrustum[5].y = P._24 - P._23;
        m_cbCompute.ViewFrustum[5].z = P._34 - P._33;
        m_cbCompute.ViewFrustum[5].w = P._44 - P._43;

        // Make sure all plane equations are normalized.
        for (UINT i = 0; i < 6; ++i)
        {
            XMVECTOR v = XMLoadFloat4(&m_cbCompute.ViewFrustum[i]);
            v = XMPlaneNormalize(v);
            XMStoreFloat4(&m_cbCompute.ViewFrustum[i], v);
        }
    }

    void ParticleSystem::Render(ID3D12GraphicsCommandList* commandList)
    {
        if (!m_dataInitialized)
        {
            InitializeResources(commandList);
        }

        ScopedPixEvent particles(commandList, PIX_COLOR_DEFAULT, L"Particles");

        ID3D12DescriptorHeap* pHeaps[] = { m_srvPile->Heap() };
        commandList->SetDescriptorHeaps(_countof(pHeaps), pHeaps);

        if (m_updateParticles)
        {
            ScopedPixEvent update(commandList, PIX_COLOR_DEFAULT, L"Update");

            // Clear the counter value on every update
            const UINT clearCounter[] = { 0, 0, 0, 0 };
            commandList->ClearUnorderedAccessViewUint(
                m_srvPile->GetGpuHandle(UAV_ParticleCounter),
                m_srvPileCpu->GetCpuHandle(CPU_ParticleCounter),
                m_particleCounter.Get(),
                clearCounter,
                0,
                nullptr);

            commandList->SetComputeRootSignature(m_computeRS.Get());

            // Update constant buffer contents.
            auto cbCompute = GraphicsMemory::Get().AllocateConstant(m_cbCompute);
            commandList->SetComputeRootConstantBufferView(ComputeRootParamCB, cbCompute.GpuAddress());

            // Set compute-shader resources and unordered access views.
            commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV, m_srvPile->GetGpuHandle(UAV_ParticleMotionData));
            commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV, m_srvPile->GetGpuHandle(SRV_ParticleResetData));

            // If we need to reset particles (previously inactive particles are being added this frame),
            // then run the reset compute shader.
            if (m_resetCount > 0)
            {
                commandList->SetPipelineState(m_resetPSO.Get());

                // The dispatch count in x is the number of particles to be reset, divided by our threadgroup size.
                commandList->Dispatch((m_resetCount + g_threadGroupSize - 1) / g_threadGroupSize, 1, 1);

                D3D12_RESOURCE_BARRIER descUAVBarrier = {};
                descUAVBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                descUAVBarrier.UAV.pResource = m_particleMotionData.Get();
                commandList->ResourceBarrier(1, &descUAVBarrier);
            }

            // Run the particle advance shader.
            commandList->SetPipelineState(m_advancePSO.Get());

            // Again, the dispatch count in x is the number of particles to be updated, divided by the threadgroup size.
            commandList->Dispatch((m_particleCount + g_threadGroupSize - 1) / g_threadGroupSize, 1, 1);

            // Grab the number of particles the compute shader actually added to the AppendStructuredBuffer. This step
            // is necessary so the DrawInstancedIndirect call later on can know the right number of instances to draw.
            TransitionResource(commandList, m_particleCounter.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
            TransitionResource(commandList, m_executeIndirectParams.Get(), D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT, D3D12_RESOURCE_STATE_COPY_DEST);
            commandList->CopyBufferRegion(
                m_executeIndirectParams.Get(),    // ID3D12Resource *pDstBuffer
                sizeof(UINT),                     // UINT64 DstOffset
                m_particleCounter.Get(),          // ID3D12Resource *pSrcBuffer
                0,                                // INT64 SrcOffset
                sizeof(UINT));                    // UINT64 NumBytes

            TransitionResource(commandList, m_executeIndirectParams.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            TransitionResource(commandList, m_particleCounter.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }

        if (m_renderParticles)
        {
            ScopedPixEvent draw(commandList, PIX_COLOR_DEFAULT, L"Draw");

            auto cbVertexMem = GraphicsMemory::Get().AllocateConstant(m_cbVertex);

            commandList->SetGraphicsRootSignature(m_renderRS.Get());
            commandList->SetPipelineState(m_renderPSO.Get());
            commandList->SetGraphicsRootConstantBufferView(RootParamCB_VS, cbVertexMem.GpuAddress());
            commandList->SetGraphicsRootDescriptorTable(RootParamSRV_VS, m_srvPile->GetGpuHandle(SRV_ParticleInstance));
            commandList->SetGraphicsRootDescriptorTable(RootParamrSRV_PS, m_srvPile->GetGpuHandle(SRV_Particle));
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

            TransitionResource(commandList, m_particleInstance.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            commandList->ExecuteIndirect(m_commandSignature.Get(), 1, m_executeIndirectParams.Get(), 0, nullptr, 0);
            TransitionResource(commandList, m_particleInstance.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }
    }

    void ParticleSystem::InitializeResources(ID3D12GraphicsCommandList* commandList)
    {
        ScopedPixEvent initData(commandList, PIX_COLOR_DEFAULT, L"Initialize Data");

        // Initialize the execute indirect parameter buffer contents.
        UINT InstanceIndirectData[] = { 4, 0, 0, 0 };
        auto indirectMem = GraphicsMemory::Get().AllocateConstant(InstanceIndirectData);

        commandList->CopyBufferRegion(m_executeIndirectParams.Get(), 0, indirectMem.Resource(), indirectMem.ResourceOffset(), sizeof(InstanceIndirectData));
        TransitionResource(commandList, m_executeIndirectParams.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);

        // Populate the particle 'reset' and 'motion' buffers with some useful data.
        auto resetData = std::unique_ptr<ParticleResetData[]>(new ParticleResetData[g_maxParticles]);
        auto motionData = std::unique_ptr<ParticleMotionData[]>(new ParticleMotionData[g_maxParticles]);

        for (UINT i = 0; i < g_maxParticles; ++i)
        {
            XMVECTOR Direction = XMVectorSet(g_particleDirectionHorizontalStrength, FloatRand(-1.0f, 1.0f), FloatRand(-1.0f, 1.0f), 0);

            resetData[i].AllottedLife = FloatRand(g_particleLifeMin, g_particleLifeMax);
            resetData[i].Speed = FloatRand(g_particleSpeedMin, g_particleSpeedMax);
            XMStoreFloat3(&resetData[i].Direction, XMVector3NormalizeEst(Direction));

            motionData[i].RemainingLife = resetData[i].InitLife = FloatRand(0, g_particleLifeMin);
            motionData[i].LastPosition.x = m_emitterRadius * cosf(m_emitterOrbitAngle);
            motionData[i].LastPosition.y = m_emitterHeight;
            motionData[i].LastPosition.z = m_emitterRadius * sinf(m_emitterOrbitAngle);
            motionData[i].Mass = FloatRand(g_particleMinMass, g_particleMaxMass);
            XMStoreFloat3(&motionData[i].Velocity, resetData[i].Direction * resetData[i].Speed);
        }

        // Initialize the particle reset data buffer contents.
        {
            size_t bufferSize = sizeof(ParticleResetData) * g_maxParticles;
            auto res = GraphicsMemory::Get().Allocate(bufferSize, 16, GraphicsMemory::TAG_COMPUTE);
            std::memcpy(res.Memory(), resetData.get(), bufferSize);

            commandList->CopyBufferRegion(m_particleResetData.Get(), 0, res.Resource(), res.ResourceOffset(), bufferSize);
            TransitionResource(commandList, m_particleResetData.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
        }

        // Initialize the particle motion data buffer contents.
        {
            size_t bufferSize = sizeof(ParticleMotionData) * g_maxParticles;
            auto res = GraphicsMemory::Get().Allocate(bufferSize, 16, GraphicsMemory::TAG_COMPUTE);
            std::memcpy(res.Memory(), motionData.get(), bufferSize);

            commandList->CopyBufferRegion(m_particleMotionData.Get(), 0, res.Resource(), res.ResourceOffset(), bufferSize);
            TransitionResource(commandList, m_particleMotionData.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        }

        // Initialize the blackbody spectrum lookup texture contents.
        {
            size_t bufferSize = sizeof(g_blackbodySpectrum);
            auto uploadMem = GraphicsMemory::Get().Allocate(bufferSize, D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT, GraphicsMemory::TAG_TEXTURE);
            std::memcpy(uploadMem.Memory(), g_blackbodySpectrum, bufferSize);

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
            footprint.Offset = uploadMem.ResourceOffset();
            footprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            footprint.Footprint.Width = _countof(g_blackbodySpectrum);
            footprint.Footprint.Height = 1;
            footprint.Footprint.Depth = 1;
            footprint.Footprint.RowPitch = D3D12XBOX_TEXTURE_DATA_PITCH_ALIGNMENT;

            auto lut = CD3DX12_TEXTURE_COPY_LOCATION(m_blackbodyLut.Get(), 0);
            auto upload = CD3DX12_TEXTURE_COPY_LOCATION	(uploadMem.Resource(), footprint);
            commandList->CopyTextureRegion(&lut, 0, 0, 0, &upload, nullptr);
            DirectX::TransitionResource(commandList, m_blackbodyLut.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        m_dataInitialized = true;
    }
}
