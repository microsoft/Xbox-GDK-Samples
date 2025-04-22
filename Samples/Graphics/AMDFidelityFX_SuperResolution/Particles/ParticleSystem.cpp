//--------------------------------------------------------------------------------------
// ParticleSystem.cpp
//
// Modifications Copyright (C) 2022. Advanced Micro Devices, Inc. All Rights Reserved.
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
	constexpr float g_maxOrbitRadius = 2.0f;
	constexpr float g_maxEmitterHeight = 3.0f;
	constexpr float g_minEmitterHeight = 1.0f;
	constexpr int   g_maxParticles = 512;
	constexpr int   g_minParticles = 512;
	constexpr float g_particleLifeMin = 0.4f;
	constexpr float g_particleLifeMax = 1.9f;
	constexpr float g_particleSpeedMax = 1.65f;
	constexpr float g_particleSpeedMin = 0.55f;
	constexpr float g_particleDirectionHorizontalStrength = 1.0f;
	constexpr float g_particleMaxMass = 2.15f;
	constexpr float g_particleMinMass = 1.15f;
	constexpr float g_defaultParticleUpdateSpeed = 0.4f;
	constexpr float g_minParticleUpdateSpeed = 0.1f;
	constexpr float g_maxParticleUpdateSpeed = 1.2f;

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
    const D3D12_DEPTH_STENCIL_DESC g_depthLessNoZWriteDescReverse =
    {
        TRUE,                                           // DepthEnable
		D3D12_DEPTH_WRITE_MASK_ZERO,                    // DepthWriteMask
        D3D12_COMPARISON_FUNC_GREATER_EQUAL,            // DepthFunc
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
		, m_emitterRadius(2.0f)
		, m_emitterOrbitAngle(XM_PI)
		, m_emitterHeight(2.0f)
		, m_particleBounciness(2.35f)
		, m_particleUpdateSpeed(g_defaultParticleUpdateSpeed)
		, m_particleCount(512)
		, m_planeOrigin()
		, m_dataInitialized(false)
		, m_renderParticles(true)
		, m_updateParticles(true)
		, m_cbVertex{}
		, m_cbCompute{}
	{ }

	XMFLOAT3 ParticleSystem::GetEmitterPosition() const {
         return Vector3(0.75f, 2.7f, -4.9f );
    } 

	void ParticleSystem::Initialize(DX::DeviceResources* deviceResources, ResourceUploadBatch& resourceUpload, bool reverseDepth)
	{
        m_deviceResources = deviceResources;
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
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_COPY_DEST,
#else
                // on PC buffers are created in the common state and can be promoted without a barrier to COPY_DEST on first access
                D3D12_RESOURCE_STATE_COMMON,
#endif
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
			auto descMotionBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ParticleMotionData) * g_maxParticles, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
			DX::ThrowIfFailed(device->CreateCommittedResource(
				&defaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				&descMotionBuffer,
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_COPY_DEST,
#else
                // on PC buffers are created in the common state and can be promoted without a barrier to COPY_DEST on first access
                D3D12_RESOURCE_STATE_COMMON,
#endif
				nullptr,
				IID_GRAPHICS_PPV_ARGS(m_particleMotionData.ReleaseAndGetAddressOf())));
			DX::ThrowIfFailed(m_particleMotionData->SetName(L"Particle Motion Data"));

			D3D12_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
			descUAV.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			descUAV.Buffer.NumElements = g_maxParticles;
			descUAV.Buffer.StructureByteStride = sizeof(ParticleMotionData);
			device->CreateUnorderedAccessView(m_particleMotionData.Get(), nullptr, &descUAV, m_srvPile->GetCpuHandle(UAV_ParticleMotionData));
		}

        //-------------------------------------------------------
        // Create the particle instance buffer and views
        {
            auto descBuffer = CD3DX12_RESOURCE_DESC::Buffer(sizeof(XMFLOAT4) * g_maxParticles, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
            DX::ThrowIfFailed(device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descBuffer,
#ifdef _GAMING_XBOX
                D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
#else
                // on PC buffers are created in the common state and can be promoted without a barrier to UNORDERED_ACCESS on first access
                D3D12_RESOURCE_STATE_COMMON,
#endif
                nullptr,
                IID_GRAPHICS_PPV_ARGS(m_particleInstance.ReleaseAndGetAddressOf())));

            DX::ThrowIfFailed(m_particleInstance->SetName(L"Particle Instance"));

            D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
            descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            descSRV.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
            descSRV.Buffer.NumElements = g_maxParticles;
            descSRV.Buffer.StructureByteStride = sizeof(XMFLOAT4);

            device->CreateShaderResourceView(m_particleInstance.Get(), &descSRV, m_srvPile->GetCpuHandle(SRV_ParticleInstance));


            D3D12_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
            descUAV.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
            descUAV.Buffer.NumElements = g_maxParticles;
            descUAV.Buffer.StructureByteStride = sizeof(XMFLOAT4);

            device->CreateUnorderedAccessView(m_particleInstance.Get(), nullptr, &descUAV, m_srvPile->GetCpuHandle(UAV_ParticleInstance));

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
			descPSO.DepthStencilState = (reverseDepth) ? g_depthLessNoZWriteDescReverse : g_depthLessNoZWriteDesc;
			descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			descPSO.NumRenderTargets = 1;
            descPSO.RTVFormats[0] = DXGI_FORMAT_R16G16B16A16_FLOAT;
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

	void ParticleSystem::SetFloor(DirectX::XMFLOAT4 floor)
	{
        // we just have a single plane to bounce off the floor
        m_cbCompute.Plane = floor;
	}

	void ParticleSystem::Update(float elapsedTime, FXMMATRIX view, CXMMATRIX proj)
	{
		m_resetStart = 0;
		m_resetCount = 0;
    
		// Store per-frame compute shader constants.
		m_cbCompute.ParticleData.x = elapsedTime * m_particleUpdateSpeed;
		m_cbCompute.ParticleData.y = m_particleBounciness;
		m_cbCompute.ActiveCount = uint32_t(m_resetStart);
		m_cbCompute.EmitterPosition = GetEmitterPosition();
        m_cbCompute.EmitterRotation = Quaternion::CreateFromAxisAngle(Vector3::UnitY, XM_PI);

		// Set constant data for vertex data (specifically, clip space scaling values used in particle rendering).
		m_cbVertex.ClipSpaceScale.x = XMVectorGetX(proj.r[0]);
		m_cbVertex.ClipSpaceScale.y = XMVectorGetY(proj.r[1]);
		m_cbVertex.ClipSpaceScale.z = m_cbVertex.ClipSpaceScale.w = 0;

		Matrix viewProj = view * proj;
		XMStoreFloat4x4(&m_cbVertex.ViewProj, XMMatrixTranspose(viewProj));
	}

	void ParticleSystem::Render(ID3D12GraphicsCommandList* commandList, float renderscale )
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

        }

		if (m_renderParticles)
		{
			ScopedPixEvent draw(commandList, PIX_COLOR_DEFAULT, L"Draw");

			auto cbVertexMem = GraphicsMemory::Get().AllocateConstant(m_cbVertex);

			commandList->SetGraphicsRootSignature(m_renderRS.Get());
			commandList->SetPipelineState(m_renderPSO.Get());
			commandList->SetGraphicsRootConstantBufferView(RootParamCB_VS, cbVertexMem.GpuAddress());
			commandList->SetGraphicsRootDescriptorTable(RootParamUAV_VS, m_srvPile->GetGpuHandle(UAV_ParticleInstance));
			commandList->SetGraphicsRootDescriptorTable(RootParamrSRV_PS, m_srvPile->GetGpuHandle(SRV_Particle));
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

            // Set the viewport and scissor rect.
            D3D12_VIEWPORT viewport = m_deviceResources->GetScreenViewport();
            D3D12_RECT scissorRect = m_deviceResources->GetScissorRect();

            scissorRect.right = LONG((scissorRect.right) / renderscale);
            scissorRect.bottom = LONG((scissorRect.bottom) / renderscale);

            viewport.Width = static_cast<float>(scissorRect.right);
            viewport.Height = static_cast<float>(scissorRect.bottom);
             
            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);

            commandList->DrawInstanced(4u, m_particleCount, 0u, 0u);
		}
	}

	void ParticleSystem::InitializeResources(ID3D12GraphicsCommandList* commandList)
	{
		ScopedPixEvent initData(commandList, PIX_COLOR_DEFAULT, L"Initialize Data");

		// Populate the particle 'reset' and 'motion' buffers with some useful data.
		auto resetData = std::unique_ptr<ParticleResetData[]>(new ParticleResetData[g_maxParticles]);
		auto motionData = std::unique_ptr<ParticleMotionData[]>(new ParticleMotionData[g_maxParticles]);

		for (UINT i = 0; i < g_maxParticles; ++i)
		{
			XMVECTOR Direction = XMVectorSet(FloatRand(-0.3f, 0.3f), FloatRand(-1.0f, 1.0f), FloatRand(-0.3f, 0.3f), 0);

			resetData[i].AllottedLife = FloatRand(g_particleLifeMin, g_particleLifeMax);
			resetData[i].Speed = FloatRand(g_particleSpeedMin, g_particleSpeedMax);
			XMStoreFloat3(&resetData[i].Direction, XMVector3NormalizeEst(Direction));
            motionData[i].RemainingLife = resetData[i].InitLife = FloatRand(0, g_particleLifeMin);
            motionData[i].LastPosition = Vector3(0.75f, -5.7f, 15.9f); //Initial spawns outside view
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

		m_dataInitialized = true;
	}
}
