//--------------------------------------------------------------------------------------
// SampleEffect.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SampleEffect.h"
#include "DirectXHelpers.h"
#include "CommonHeader.h"

#include "AlignedNew.h"

using namespace DirectX;

#pragma warning(disable : 4324)
// On Xbox One, constant buffers should be aligned to 64 bytes
#define CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT 64
_declspec(align(CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)) struct SampleEffectConstants
{
	XMMATRIX worldViewProj;
};

static_assert((sizeof(SampleEffectConstants) % 16) == 0, "CB size not padded correctly");

_declspec(align(CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)) class SampleEffect::Impl
    : public AlignedNew<SampleEffect::Impl>
{
public:
	Impl(_In_ ID3D12Device* device,
		const _In_ EffectPipelineStateDescription* effectPsoDesc,
		const uint32_t backbufferCount,
		const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
		const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob,
		bool enableRootSignature,
		bool enableConstantBuffer);
	Impl() = delete;

	void Apply(_In_ ID3D12GraphicsCommandList* commandList);

	ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); };
	void UpdateConstants(FXMMATRIX& worldViewProj);
	void SetDescriptors(_In_ ID3D12GraphicsCommandList* commandList, uint32_t currFrameIndex, D3D12_GPU_DESCRIPTOR_HANDLE texture); 
	void SetAllDirtyFlags();

private:
	void CreateRootSignature(_In_ ID3D12Device * device, _In_ const D3D12_SHADER_BYTECODE* vertexShaderBlob);
	void CreatePSO(_In_ ID3D12Device * device,
		const _In_ EffectPipelineStateDescription* effectPsoDesc,
		const _In_ D3D12_SHADER_BYTECODE* m_vertexShaderBlob,
		const _In_ D3D12_SHADER_BYTECODE* m_pixelShaderBlob);

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
	SampleEffectConstants                       m_effectConstants;
	SampleEffectConstants*                      m_effectConstantsCB;
	Microsoft::WRL::ComPtr<ID3D12Resource>      m_effectDataCB;
	D3D12_GPU_DESCRIPTOR_HANDLE                 m_textureGPUHandle;

	// Dirty flags
	bool                                        m_dirtyCB;
	bool                                        m_dirtyTexture;
};

SampleEffect::Impl::Impl(_In_ ID3D12Device * device,
	const _In_ EffectPipelineStateDescription * effectPsoDesc,
	const uint32_t backbufferCount,
	const _In_ D3D12_SHADER_BYTECODE * vertexShaderBlob,
	const _In_ D3D12_SHADER_BYTECODE * pixelShaderBlob,
	bool enableRootSignature,
	bool enableConstantBuffer) :
	m_dirtyCB(true)
{
	if (enableRootSignature)
		CreateRootSignature(device, vertexShaderBlob);

	CreatePSO(device, effectPsoDesc, vertexShaderBlob, pixelShaderBlob);

	if (enableConstantBuffer)
	{
		memset(&m_effectConstants, 0, sizeof(SampleEffectConstants));

		// Create Constant Buffer
		D3D12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC meshDataBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
			sizeof(SampleEffectConstants) * backbufferCount                     // UINT64 width,
																				// D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
		);													                    // UINT64 alignment = 0 )

		DX::ThrowIfFailed(device->CreateCommittedResource(
			&uploadHeapProp, 											        // _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,					    // D3D12_HEAP_FLAGS HeapFlags,
			&meshDataBufferDesc,										        // _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,		                            // D3D12_RESOURCE_STATES InitialState,
			nullptr,											                // _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
			IID_GRAPHICS_PPV_ARGS(m_effectDataCB.ReleaseAndGetAddressOf())	    // REFIID riidResource,
																				// _Outptr_opt_ void** ppvResource)
		));
		m_effectDataCB->SetName(L"Mesh Data Constant Buffer");
		m_effectDataCB->Map(0, nullptr, reinterpret_cast<void**>(&m_effectConstantsCB));
	}
}

void SampleEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
	// Set the PSO
	commandList->SetPipelineState(m_pso.Get());
}

void SampleEffect::Impl::UpdateConstants(FXMMATRIX& worldViewProj)
{
	m_effectConstants.worldViewProj = XMMatrixTranspose(worldViewProj);
	m_dirtyCB = true;
}

void SampleEffect::Impl::CreateRootSignature(_In_ ID3D12Device* device, _In_ const D3D12_SHADER_BYTECODE* vertexShaderBlob)
{
	if (vertexShaderBlob->pShaderBytecode)
	{
		DX::ThrowIfFailed(
			device->CreateRootSignature(0, vertexShaderBlob->pShaderBytecode, vertexShaderBlob->BytecodeLength,
				IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
		m_rootSignature->SetName(L"Main Root Signature");
	}
	else
	{
		throw std::exception("Vertex Shader blob is invalid");
	}
}

void SampleEffect::Impl::CreatePSO(_In_ ID3D12Device* device,
	const _In_ EffectPipelineStateDescription* effectPsoDesc,
	const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
	const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob)
{
	assert(vertexShaderBlob);

	D3D12_SHADER_BYTECODE pixelShaderBlobForPSO;

	if (pixelShaderBlob)
	{
		pixelShaderBlobForPSO = *pixelShaderBlob;
	}
	else
	{
		pixelShaderBlobForPSO.BytecodeLength = 0;
		pixelShaderBlobForPSO.pShaderBytecode = nullptr;
	}
	effectPsoDesc->CreatePipelineState(device, m_rootSignature.Get(), *vertexShaderBlob, pixelShaderBlobForPSO, m_pso.ReleaseAndGetAddressOf());
}

void SampleEffect::Impl::SetDescriptors(_In_ ID3D12GraphicsCommandList* commandList, uint32_t currFrameIndex, D3D12_GPU_DESCRIPTOR_HANDLE texture)
{
	// Set the texture descriptors
	if (texture.ptr)
	{
		if (!m_dirtyTexture && m_textureGPUHandle.ptr != texture.ptr)
			m_dirtyTexture = true;
		m_textureGPUHandle = texture;

		if (m_dirtyTexture)
		{
			commandList->SetGraphicsRootDescriptorTable(static_cast<uint32_t>(CommonHeader::RootParameterIndex::DescriptorTableSRV), m_textureGPUHandle);
			m_dirtyTexture = false;
		}
	}

	// Set constants
	if (m_dirtyCB)
	{
		// Assert if constant buffer is not present in this instance
		assert(m_effectConstantsCB);
		assert(m_effectDataCB.Get());

		memcpy(&(m_effectConstantsCB[currFrameIndex]), &m_effectConstants, sizeof(SampleEffectConstants));
		size_t cbOffset = sizeof(SampleEffectConstants) * currFrameIndex;
		commandList->SetGraphicsRootConstantBufferView(static_cast<uint32_t>(CommonHeader::RootParameterIndex::ConstantBuffer), m_effectDataCB->GetGPUVirtualAddress() + cbOffset);
		m_dirtyCB = false;
	}
}

void SampleEffect::Impl::SetAllDirtyFlags()
{
	m_dirtyTexture = true;
	m_dirtyCB = true;
}

SampleEffect::SampleEffect(_In_ ID3D12Device * device,
	const _In_ DirectX::EffectPipelineStateDescription * effectPsoDesc,
	const uint32_t backbufferCount, 
	const _In_ D3D12_SHADER_BYTECODE * vertexShaderBlob,
	const _In_ D3D12_SHADER_BYTECODE * pixelShaderBlob,
	bool enableRootSignature, 
	bool enableConstantBuffer) :
    pImpl(new Impl(device, effectPsoDesc, backbufferCount, vertexShaderBlob, pixelShaderBlob, enableRootSignature, enableConstantBuffer))
{
}

SampleEffect::SampleEffect(SampleEffect && moveFrom) noexcept :
	pImpl(std::move(moveFrom.pImpl))
{
}

SampleEffect& SampleEffect::operator=(SampleEffect && moveFrom) noexcept
{
	pImpl = std::move(moveFrom.pImpl);
	return *this;
}

SampleEffect::~SampleEffect()
{
}

void SampleEffect::Apply(_In_ ID3D12GraphicsCommandList * commandList)
{
	pImpl->Apply(commandList);
}

void SampleEffect::UpdateConstants(FXMMATRIX& worldViewProj)
{
	pImpl->UpdateConstants(worldViewProj);
}

void SampleEffect::SetDescriptors(_In_ ID3D12GraphicsCommandList * commandList, uint32_t currFrameIndex, D3D12_GPU_DESCRIPTOR_HANDLE texture)
{
	pImpl->SetDescriptors(commandList, currFrameIndex, texture);
}

ID3D12RootSignature * SampleEffect::GetRootSignature() const
{
	return pImpl->GetRootSignature();
}

void SampleEffect::SetAllDirtyFlags()
{
	pImpl->SetAllDirtyFlags();
}

