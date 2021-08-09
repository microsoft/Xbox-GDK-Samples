//--------------------------------------------------------------------------------------
// Fluid.cpp
//
// Demonstrates basic Navier-Stokes flow simulation using Compute Shader 5 and 3D textures
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Fluid.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

namespace
{
	static const XMVECTOR g_bottom = XMVectorScale(XMVectorSet(VolumeSize, VolumeSize, VolumeSize, 0), 1.0f / 10);
	static const XMVECTOR g_top = XMVectorScale(XMVectorSet(9 * VolumeSize, 9 * VolumeSize, 9 * VolumeSize, 0), 1.0f / 10);

	// Helper for resource barrier.
	void TransitionResource(
		ID3D12GraphicsCommandList* commandList,
		ID3D12Resource* resource,
		D3D12_RESOURCE_STATES stateBefore,
		D3D12_RESOURCE_STATES stateAfter,
		D3D12_RESOURCE_BARRIER_FLAGS flag)
	{
		assert(commandList != nullptr);
		assert(resource != nullptr);

		if (stateBefore == stateAfter)
			return;

		D3D12_RESOURCE_BARRIER desc = {};
		desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		desc.Transition.pResource = resource;
		desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		desc.Transition.StateBefore = stateBefore;
		desc.Transition.StateAfter = stateAfter;
		desc.Flags = flag;

		commandList->ResourceBarrier(1, &desc);
	}
}

Fluid::Fluid()
	: m_emitterCenter(0.0f)
	, m_resetSim(true)
{ }

void Fluid::Initialize(ID3D12Device* device, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat)
{
	m_srvPile = std::make_unique<DescriptorPile>(
		device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		128,
		HeapCount);

	Create3DTextures(device);

	// Create the simulation PSOs
	{
		auto advectColorCS = DX::ReadData(L"AdvectColorCS.cso");
		auto advectVelocityCS = DX::ReadData(L"AdvectVelocityCS.cso");
		auto vorticityCS = DX::ReadData(L"VorticityCS.cso");
		auto confinementCS = DX::ReadData(L"ConfinementCS.cso");
		auto gaussian1CS = DX::ReadData(L"Gaussian1CS.cso");
		auto gaussian4CS = DX::ReadData(L"Gaussian4CS.cso");
		auto divergenceCS = DX::ReadData(L"DivergenceCS.cso");
		auto jacobiCS = DX::ReadData(L"JacobiCS.cso");
		auto projectCS = DX::ReadData(L"ProjectCS.cso");

		// Strip the root signature from one of the shaders (they all leverage the same root signature.)
		DX::ThrowIfFailed(device->CreateRootSignature(
			0,
			advectColorCS.data(),
			advectColorCS.size(),
			IID_GRAPHICS_PPV_ARGS(m_computeRS.ReleaseAndGetAddressOf())));

		D3D12_COMPUTE_PIPELINE_STATE_DESC descComputePSO = {};
		descComputePSO.pRootSignature = m_computeRS.Get();
		descComputePSO.CS.pShaderBytecode = advectColorCS.data();
		descComputePSO.CS.BytecodeLength = advectColorCS.size();
		DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_advectColorPSO.ReleaseAndGetAddressOf())));

		descComputePSO.CS.pShaderBytecode = advectVelocityCS.data();
		descComputePSO.CS.BytecodeLength = advectVelocityCS.size();
		DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_advectVelocityPSO.ReleaseAndGetAddressOf())));

		descComputePSO.CS.pShaderBytecode = vorticityCS.data();
		descComputePSO.CS.BytecodeLength = vorticityCS.size();
		DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_vorticityPSO.ReleaseAndGetAddressOf())));

		descComputePSO.CS.pShaderBytecode = confinementCS.data();
		descComputePSO.CS.BytecodeLength = confinementCS.size();
		DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_confinementPSO.ReleaseAndGetAddressOf())));

		descComputePSO.CS.pShaderBytecode = gaussian1CS.data();
		descComputePSO.CS.BytecodeLength = gaussian1CS.size();
		DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_gaussian1PSO.ReleaseAndGetAddressOf())));

		descComputePSO.CS.pShaderBytecode = gaussian4CS.data();
		descComputePSO.CS.BytecodeLength = gaussian4CS.size();
		DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_gaussian4PSO.ReleaseAndGetAddressOf())));

		descComputePSO.CS.pShaderBytecode = divergenceCS.data();
		descComputePSO.CS.BytecodeLength = divergenceCS.size();
		DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_divergencePSO.ReleaseAndGetAddressOf())));

		descComputePSO.CS.pShaderBytecode = jacobiCS.data();
		descComputePSO.CS.BytecodeLength = jacobiCS.size();
		DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_jacobiPSO.ReleaseAndGetAddressOf())));

		descComputePSO.CS.pShaderBytecode = projectCS.data();
		descComputePSO.CS.BytecodeLength = projectCS.size();
		DX::ThrowIfFailed(device->CreateComputePipelineState(&descComputePSO, IID_GRAPHICS_PPV_ARGS(m_projectPSO.ReleaseAndGetAddressOf())));
	}

	// Create the volume rendering PSO
	{
		// Load shaders
		auto renderVS = DX::ReadData(L"RenderVS.cso");
		auto renderPS = DX::ReadData(L"RenderPS.cso");

		// Strip the root signature from one of the shaders (they both leverage the same root signature.)
		DX::ThrowIfFailed(device->CreateRootSignature(
			0,
			renderVS.data(),
			renderVS.size(),
			IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));

		D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
		descPso.InputLayout.pInputElementDescs = nullptr;
		descPso.InputLayout.NumElements = 0;
		descPso.pRootSignature = m_rootSignature.Get();
		descPso.VS.pShaderBytecode = renderVS.data();
		descPso.VS.BytecodeLength = renderVS.size();
		descPso.PS.pShaderBytecode = renderPS.data();
		descPso.PS.BytecodeLength = renderPS.size();
		descPso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		descPso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		descPso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		descPso.SampleMask = UINT_MAX;
		descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		descPso.NumRenderTargets = 1;
		descPso.RTVFormats[0] = colorFormat;
		descPso.DSVFormat = depthFormat;
		descPso.SampleDesc.Count = 1;

		DX::ThrowIfFailed(device->CreateGraphicsPipelineState(&descPso, IID_GRAPHICS_PPV_ARGS(m_renderPSO.ReleaseAndGetAddressOf())));
	}
}

void Fluid::Update(float elapsedTime, const GamePad::State& pad)
{
	m_emitterCenter = std::min(1.0f, std::max(0.0f, m_emitterCenter + (pad.triggers.right - pad.triggers.left) * elapsedTime));
	m_emitterRot.Rotate(-pad.thumbSticks.leftX * elapsedTime, -pad.thumbSticks.leftY * elapsedTime);

	if (pad.IsXPressed())
	{
		m_emitterCenter = 0.0f;
		m_emitterRot.Reset();
	}
}

void Fluid::Step(ID3D12GraphicsCommandList* commandList, float elapsedTime)
{
	ScopedPixEvent update(commandList, PIX_COLOR_DEFAULT, L"Fluid Update");

	cbCS computeCB;
	computeCB.Epsilon = 0.12f;         // the larger this is, the more "curly" the smoke is
	computeCB.Forward = 1.0f;
	computeCB.Modulate = 1.0f;
	computeCB.DeltaTime = elapsedTime;
	XMStoreFloat3(&computeCB.EmitterCenter, XMVectorLerp(g_bottom, g_top, m_emitterCenter));
	XMStoreFloat3(&computeCB.EmitterDir, XMVector3Rotate(g_forwardVec, m_emitterRot.GetRotationQuaternion()));

	ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap() };
	commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	commandList->SetComputeRootSignature(m_computeRS.Get());

	if (m_resetSim)
	{
		ResetSim(commandList);
	}

	auto mem = GraphicsMemory::Get().AllocateConstant(computeCB);
	commandList->SetComputeRootConstantBufferView(ComputeRootParamCB0, mem.GpuAddress());

	TransitionResource(commandList, m_color[0].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	TransitionResource(commandList, m_colorOneEighth.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);

	// preload partial barriers
	TransitionResource(commandList, m_velocity[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
	TransitionResource(commandList, m_tempVector.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);

	// Advect color
	{
		TransitionResource(commandList, m_color[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		commandList->SetPipelineState(m_advectColorPSO.Get());
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV0, m_srvPile->GetGpuHandle(SRV_Velocity0));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV1, m_srvPile->GetGpuHandle(SRV_Color0));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_Color1));
		commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);

		TransitionResource(commandList, m_color[0].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
	}

	// Advect velocity
	{
		TransitionResource(commandList, m_velocity[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

		commandList->SetPipelineState(m_advectVelocityPSO.Get());
		commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_Velocity1));
		commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);
	}

	// Partial barrier on velocity 0
	TransitionResource(commandList, m_velocity[0].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);

	// Vorticity confinement
	{
		// Vorticity
		TransitionResource(commandList, m_velocity[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		TransitionResource(commandList, m_tempVector.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

		commandList->SetPipelineState(m_vorticityPSO.Get());
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV0, m_srvPile->GetGpuHandle(SRV_Velocity1));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_TempVector));
		commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);

		// Confinement
		TransitionResource(commandList, m_tempVector.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		TransitionResource(commandList, m_velocity[0].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

		commandList->SetPipelineState(m_confinementPSO.Get());
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV0, m_srvPile->GetGpuHandle(SRV_TempVector));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV1, m_srvPile->GetGpuHandle(SRV_Velocity1));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_Velocity0));
		commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);

		TransitionResource(commandList, m_velocity[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
		TransitionResource(commandList, m_velocity[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
		TransitionResource(commandList, m_tempVector.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
	}

	// Apply external forces
	{
		TransitionResource(commandList, m_color[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		TransitionResource(commandList, m_color[0].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
		TransitionResource(commandList, m_colorOneEighth.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

		commandList->SetPipelineState(m_gaussian1PSO.Get());
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV0, m_srvPile->GetGpuHandle(SRV_Color1));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_Color0));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV1, m_srvPile->GetGpuHandle(UAV_ColorOneEigth));
		commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);

		TransitionResource(commandList, m_velocity[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);
		TransitionResource(commandList, m_velocity[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

		commandList->SetPipelineState(m_gaussian4PSO.Get());
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV0, m_srvPile->GetGpuHandle(SRV_Velocity0));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_Velocity1));
		commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);

		TransitionResource(commandList, m_velocity[0].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY);
	}

	// Compute velocity divergence
	{
		TransitionResource(commandList, m_velocity[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		TransitionResource(commandList, m_tempVector.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

		commandList->SetPipelineState(m_divergencePSO.Get());
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV0, m_srvPile->GetGpuHandle(SRV_Velocity1));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_TempVector));
		commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);
	}

	// Compute pressure
	{
		commandList->SetPipelineState(m_jacobiPSO.Get());
		TransitionResource(commandList, m_tempVector.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// 6 is the number of Jacobi iterations
		// We divide by 2 is because we do 2 Jacobi iterations in the for body, and we want to ensure ping-pong-ing to the right result resource (pressure field)
		for (int i = 0; i < 6 / 2; ++i)
		{
			TransitionResource(commandList, m_tempScalar.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV0, m_srvPile->GetGpuHandle(SRV_Pressure));
			commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV1, m_srvPile->GetGpuHandle(SRV_TempVector));
			commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_TempScalar));
			commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);

			TransitionResource(commandList, m_pressure.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			TransitionResource(commandList, m_tempScalar.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

			commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV0, m_srvPile->GetGpuHandle(SRV_TempScalar));
			commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_Pressure));
			commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);

			TransitionResource(commandList, m_pressure.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}
	}

	// Project velocity
	{
		TransitionResource(commandList, m_velocity[0].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_BARRIER_FLAG_END_ONLY);

		commandList->SetPipelineState(m_projectPSO.Get());
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV0, m_srvPile->GetGpuHandle(SRV_Pressure));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamSRV1, m_srvPile->GetGpuHandle(SRV_Velocity1));
		commandList->SetComputeRootDescriptorTable(ComputeRootParamUAV0, m_srvPile->GetGpuHandle(UAV_Velocity0));
		commandList->Dispatch(VolumeSize / GroupSize, VolumeSize / GroupSize, VolumeSize / GroupSize);

		TransitionResource(commandList, m_velocity[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	// prepare the resources for rendering
	TransitionResource(commandList, m_color[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	TransitionResource(commandList, m_colorOneEighth.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Fluid::Render(ID3D12GraphicsCommandList* commandList, DirectX::FXMMATRIX viewProj, DirectX::HXMVECTOR eyePosition)
{
	cbRender renderCB;
	XMStoreFloat4x4(&renderCB.World, XMMatrixIdentity());
	XMStoreFloat4x4(&renderCB.WorldViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat3(&renderCB.EyePosition, eyePosition);
	auto cbAddr = GraphicsMemory::Get().AllocateConstant(renderCB).GpuAddress();

	// Set root signature and pipeline state
	ID3D12DescriptorHeap* heaps[] = { m_srvPile->Heap() };
	commandList->SetDescriptorHeaps(ARRAYSIZE(heaps), heaps);

	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->SetPipelineState(m_renderPSO.Get());

	commandList->SetGraphicsRootConstantBufferView(RenderRootParamCB0, cbAddr);
	commandList->SetGraphicsRootDescriptorTable(RenderRootParamSRV0, m_srvPile->GetGpuHandle(SRV_Color0));
	commandList->SetGraphicsRootDescriptorTable(RenderRootParamSRV1, m_srvPile->GetGpuHandle(SRV_ColorOneEigth));

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Draw the cube
	commandList->DrawInstanced(36, 1, 0, 0);
}

void Fluid::ResetSim(ID3D12GraphicsCommandList* commandList)
{
	ScopedPixEvent reset(commandList, PIX_COLOR_DEFAULT, L"Fluid Reset");

	D3D12_RESOURCE_BARRIER preBarriers[] =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(m_color[0].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_color[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_pressure.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_tempScalar.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_velocity[0].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_velocity[1].Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_tempVector.Get(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_colorOneEighth.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
	};
	commandList->ResourceBarrier(_countof(preBarriers), preBarriers);

	const FLOAT zeros[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	commandList->ClearUnorderedAccessViewFloat(m_srvPile->GetGpuHandle(UAV_Color0), m_srvPile->GetCpuHandle(UAV_Color0), m_color[0].Get(), zeros, 0, nullptr);
	commandList->ClearUnorderedAccessViewFloat(m_srvPile->GetGpuHandle(UAV_Color1), m_srvPile->GetCpuHandle(UAV_Color1), m_color[1].Get(), zeros, 0, nullptr);
	commandList->ClearUnorderedAccessViewFloat(m_srvPile->GetGpuHandle(UAV_Pressure), m_srvPile->GetCpuHandle(UAV_Pressure), m_pressure.Get(), zeros, 0, nullptr);
	commandList->ClearUnorderedAccessViewFloat(m_srvPile->GetGpuHandle(UAV_TempScalar), m_srvPile->GetCpuHandle(UAV_TempScalar), m_tempScalar.Get(), zeros, 0, nullptr);

	commandList->ClearUnorderedAccessViewFloat(m_srvPile->GetGpuHandle(UAV_Velocity0), m_srvPile->GetCpuHandle(UAV_Velocity0), m_velocity[0].Get(), zeros, 0, nullptr);
	commandList->ClearUnorderedAccessViewFloat(m_srvPile->GetGpuHandle(UAV_Velocity1), m_srvPile->GetCpuHandle(UAV_Velocity1), m_velocity[1].Get(), zeros, 0, nullptr);
	commandList->ClearUnorderedAccessViewFloat(m_srvPile->GetGpuHandle(UAV_TempVector), m_srvPile->GetCpuHandle(UAV_TempVector), m_tempVector.Get(), zeros, 0, nullptr);

	commandList->ClearUnorderedAccessViewFloat(m_srvPile->GetGpuHandle(UAV_ColorOneEigth), m_srvPile->GetCpuHandle(UAV_ColorOneEigth), m_colorOneEighth.Get(), zeros, 0, nullptr);

	D3D12_RESOURCE_BARRIER postBarriers[] =
	{
		CD3DX12_RESOURCE_BARRIER::Transition(m_color[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_color[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_pressure.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_tempScalar.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_velocity[0].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_velocity[1].Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_tempVector.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_colorOneEighth.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
	};
	commandList->ResourceBarrier(_countof(postBarriers), postBarriers);

	m_resetSim = false;
}

void Fluid::Create3DTextures(ID3D12Device* device)
{
	D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex3D(
		DXGI_FORMAT_R16_FLOAT,
		VolumeSize,
		VolumeSize,
		VolumeSize,
		1,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	CreateResourceAndViews(device, &desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, SRV_Color0, UAV_Color0, m_color[0].ReleaseAndGetAddressOf(), L"Color 0");
	CreateResourceAndViews(device, &desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, SRV_Color1, UAV_Color1, m_color[1].ReleaseAndGetAddressOf(), L"Color 1");
	CreateResourceAndViews(device, &desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, SRV_Pressure, UAV_Pressure, m_pressure.ReleaseAndGetAddressOf(), L"Pressure");
	CreateResourceAndViews(device, &desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, SRV_TempScalar, UAV_TempScalar, m_tempScalar.ReleaseAndGetAddressOf(), L"Temp Scalar");

	desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	CreateResourceAndViews(device, &desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, SRV_Velocity0, UAV_Velocity0, m_velocity[0].ReleaseAndGetAddressOf(), L"Velocity 0");
	CreateResourceAndViews(device, &desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, SRV_Velocity1, UAV_Velocity1, m_velocity[1].ReleaseAndGetAddressOf(), L"Velocity 1");
	CreateResourceAndViews(device, &desc, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, SRV_TempVector, UAV_TempVector, m_tempVector.ReleaseAndGetAddressOf(), L"Temp Vector");

	desc.Format = DXGI_FORMAT_R16_FLOAT;
	desc.Width = VolumeSize / 8;
	desc.Height = VolumeSize / 8;
	desc.DepthOrArraySize = VolumeSize / 8;
	CreateResourceAndViews(device, &desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, SRV_ColorOneEigth, UAV_ColorOneEigth, m_colorOneEighth.ReleaseAndGetAddressOf(), L"Color One Eighth");
}

void Fluid::CreateResourceAndViews(
	ID3D12Device* device,
	const D3D12_RESOURCE_DESC* pDesc,
	D3D12_RESOURCE_STATES initState,
	int srvHeapIndex,
	int uavHeapIndex,
	ID3D12Resource** resource,
	const wchar_t *name)
{
	// Create the resource with COPY_DEST state because we are about to zero fill it below:
	const D3D12_HEAP_PROPERTIES defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	DX::ThrowIfFailed(device->CreateCommittedResource(
		&defaultHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		pDesc,
		initState,
		nullptr,
		IID_GRAPHICS_PPV_ARGS(resource)));
	DX::ThrowIfFailed((*resource)->SetName(name));

	D3D12_SHADER_RESOURCE_VIEW_DESC descSRV = {};
	descSRV.Format = pDesc->Format;
	descSRV.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	descSRV.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	descSRV.Texture3D.MipLevels = pDesc->MipLevels;
	device->CreateShaderResourceView(*resource, &descSRV, m_srvPile->GetCpuHandle(size_t(srvHeapIndex)));

	D3D12_UNORDERED_ACCESS_VIEW_DESC descUAV = {};
	descUAV.Format = pDesc->Format;
	descUAV.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	descUAV.Texture3D.WSize = pDesc->DepthOrArraySize;
	device->CreateUnorderedAccessView(*resource, nullptr, &descUAV, m_srvPile->GetCpuHandle(size_t(uavHeapIndex)));
}