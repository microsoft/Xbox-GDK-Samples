//--------------------------------------------------------------------------------------
// SampleEffect.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Common.h"
#include "SampleEffect.h"
#include "DirectXHelpers.h"

#include "AlignedNew.h"

using namespace DirectX;

#pragma warning(disable : 4324)
// On Xbox One, constant buffers should be aligned to 64 bytes
#define CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT 64
__declspec(align(CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)) struct ConstantBufferBase {};

// Constant buffer layout. Must match the shader!
struct SampleEffectConstants : ConstantBufferBase
{
	DirectX::XMMATRIX worldViewProj;
};

static_assert((sizeof(SampleEffectConstants) % 16) == 0, "CB size not padded correctly");

__declspec(align(CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)) class SampleEffect::Impl :
	public AlignedNew<SampleEffect::Impl>
{
public:
	Impl(_In_ ID3D12Device* device,
		const uint32_t backbufferCount,
		const EffectPipelineStateDescription* effectPsoDesc,
		_In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
		_In_ D3D12_SHADER_BYTECODE* pixelShaderBlob);

	void CreateRootSignature();
	void CreatePSOs(const EffectPipelineStateDescription* effectPsoDesc);
	void Apply(_In_ ID3D12GraphicsCommandList* commandList);

	void SetRootSignature(_In_ ID3D12GraphicsCommandList* commandList);
	ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); };
	void SetDescriptorHeaps(_In_ ID3D12GraphicsCommandList * commandList, uint32_t currFrameIndex);
	void SetCurrentStateArgs(uint32_t currFrameIndex, uint32_t psoID, D3D12_GPU_DESCRIPTOR_HANDLE texture);
	void ResetAllDirtyFlags();

	void XM_CALLCONV InitializeCamera(DirectX::FXMVECTOR camPos,
		DirectX::FXMVECTOR camLookAt,
		float nearPlaneDist,
		float farPlaneDist,
		float aspectWByH);
	void UpdateCamera(DirectX::GamePad::State& gamepad, float elapsedTime);
	void UpdateDrawData(_In_ DrawData* drawData);
	void SetDrawDataBuffer(_In_ ID3D12GraphicsCommandList* commandList);
	void SetVertexBufferAsSRV(_In_ ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS& vertexBufferAsSRVVirtualAddress);

	void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE texture)
	{
		if (!m_dirtyTexture && m_textureGPUHandle.ptr != texture.ptr)
			m_dirtyTexture = true;
		m_textureGPUHandle = texture;
	}

	void* operator new(size_t size)
	{
		return _aligned_malloc(size, CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
	}

	void operator delete(void* ptr)
	{
		return _aligned_free(ptr);
	}

private:
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso[NUM_PSO];
	D3D12_SHADER_BYTECODE                       m_vertexShaderBlob[NUM_PSO];
	D3D12_SHADER_BYTECODE                       m_pixelShaderBlob[NUM_PSO];
	Microsoft::WRL::ComPtr<ID3D12Device>        m_device;
	uint32_t                                    m_backbufferCount;
	SampleCamera                                m_camera;
	D3D12_GPU_DESCRIPTOR_HANDLE                 m_textureGPUHandle;
	SampleEffectConstants                       m_effectConstants;
	Microsoft::WRL::ComPtr<ID3D12Resource>      m_effectDataCB;
	SampleEffectConstants*                      m_effectConstantsCB;
	uint32_t                                    m_currPsoID;
	uint32_t                                    m_backbufferIndex;

	// Draw data
	Microsoft::WRL::ComPtr<ID3D12Resource>      m_drawDataBuffer;
	DrawData*                                   m_drawDataMappedBuffer;
	uint32_t                                    m_drawDataCount;

	// Dirty flags
	bool                                        m_dirtyPso;
	bool                                        m_dirtyTexture;
	bool                                        m_dirtyCB;
};

SampleEffect::Impl::Impl(_In_ ID3D12Device* device,
	const uint32_t backbufferCount,
	const EffectPipelineStateDescription* effectPsoDesc,
	_In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
	_In_ D3D12_SHADER_BYTECODE* pixelShaderBlob) :
    m_vertexShaderBlob{},
    m_pixelShaderBlob{},
	m_device(device),
	m_backbufferCount(backbufferCount),
    m_textureGPUHandle{},
    m_effectConstants{},
    m_effectConstantsCB(nullptr),
	m_currPsoID(0),
    m_backbufferIndex(0),
    m_drawDataMappedBuffer(nullptr),
    m_drawDataCount(0),
	m_dirtyPso(true),
	m_dirtyTexture(true),
	m_dirtyCB(true)
{
	for (uint32_t i = 0; i < NUM_PSO; ++i)
	{
		m_vertexShaderBlob[i] = vertexShaderBlob[i];
		if (pixelShaderBlob)
			m_pixelShaderBlob[i] = pixelShaderBlob[i];
		else
		{
			m_pixelShaderBlob[i].BytecodeLength = 0;
			m_pixelShaderBlob[i].pShaderBytecode = nullptr;
		}
	}
	CreateRootSignature();

	CreatePSOs(effectPsoDesc);

	memset(&m_effectConstants, 0, sizeof(SampleEffectConstants));

	// Create Constant Buffer
	D3D12_HEAP_PROPERTIES uploadHeapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC meshDataBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		sizeof(SampleEffectConstants) * m_backbufferCount                   // UINT64 width,
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

	// Create Buffer
	D3D12_RESOURCE_DESC drawDataBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
		sizeof(DrawData) * MAX_DRAWS * m_backbufferCount                    // UINT64 width,
																			// D3D12_RESOURCE_FLAGS miscFlags = D3D12_RESOURCE_FLAG_NONE,
	);													                    // UINT64 alignment = 0 )

	DX::ThrowIfFailed(device->CreateCommittedResource(
		&uploadHeapProp, 											        // _In_ const D3D12_HEAP_PROPERTIES* pHeapProperties,
		D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,					    // D3D12_HEAP_FLAGS HeapFlags,
		&drawDataBufferDesc,										        // _In_ const D3D12_RESOURCE_DESC* pResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,		                            // D3D12_RESOURCE_STATES InitialState,
		nullptr,											                // _In_opt_  const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		IID_GRAPHICS_PPV_ARGS(m_drawDataBuffer.ReleaseAndGetAddressOf())	    // REFIID riidResource,
																			// _Outptr_opt_ void** ppvResource)
	));
	m_drawDataBuffer->SetName(L"Draw Data Buffer");
	m_drawDataBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_drawDataMappedBuffer));
}

void SampleEffect::Impl::CreateRootSignature()
{
	if (m_vertexShaderBlob[0].pShaderBytecode)
	{
		DX::ThrowIfFailed(
			m_device.Get()->CreateRootSignature(0, m_vertexShaderBlob[0].pShaderBytecode, m_vertexShaderBlob[0].BytecodeLength,
				IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
		m_rootSignature->SetName(L"Mesh Root Signature");
	}
	else
	{
		throw std::exception("Vertex Shader blob is invalid");
	}
}

void SampleEffect::Impl::CreatePSOs(const EffectPipelineStateDescription* effectPsoDesc)
{
	for (uint32_t i = 0; i < NUM_PSO; ++i)
	{
		effectPsoDesc[i].CreatePipelineState(m_device.Get(), m_rootSignature.Get(), m_vertexShaderBlob[i], m_pixelShaderBlob[i], m_pso[i].ReleaseAndGetAddressOf());
	}
}

void SampleEffect::Impl::SetRootSignature(_In_ ID3D12GraphicsCommandList* commandList)
{
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
}

void SampleEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
	// Set the PSO and descriptor heaps
	if (m_dirtyPso)
	{
		commandList->SetPipelineState(m_pso[m_currPsoID].Get());
		m_dirtyPso = false;;
	}

	SetDescriptorHeaps(commandList, m_backbufferIndex);
}

void SampleEffect::Impl::ResetAllDirtyFlags()
{
	m_dirtyPso = true;
	m_dirtyTexture = true;
	m_dirtyCB = true;
}

void SampleEffect::Impl::SetDescriptorHeaps(_In_ ID3D12GraphicsCommandList* commandList, uint32_t currFrameIndex)
{
	// Set the texture descriptors
	if (m_dirtyTexture)
	{
		commandList->SetGraphicsRootDescriptorTable(static_cast<uint32_t>(RootParameterIndex::DescriptorTable0), m_textureGPUHandle);
		m_dirtyTexture = false;
	}

	// Set constants
	if (m_dirtyCB)
	{
		memcpy(&(m_effectConstantsCB[currFrameIndex]), &m_effectConstants, sizeof(SampleEffectConstants));
		size_t cbOffset = sizeof(SampleEffectConstants) * currFrameIndex;
		commandList->SetGraphicsRootConstantBufferView(static_cast<uint32_t>(RootParameterIndex::ConstantBuffer0), m_effectDataCB->GetGPUVirtualAddress() + cbOffset);
		m_dirtyCB = false;
	}
}

void SampleEffect::Impl::SetCurrentStateArgs(uint32_t currFrameIndex, uint32_t psoID, D3D12_GPU_DESCRIPTOR_HANDLE texture)
{
	if (!m_dirtyPso && m_currPsoID != psoID)
		m_dirtyPso = true;
	m_currPsoID = psoID;
	m_backbufferIndex = currFrameIndex;
	if (!m_dirtyTexture && m_textureGPUHandle.ptr != texture.ptr)
		m_dirtyTexture = true;
	m_textureGPUHandle = texture;
}

void XM_CALLCONV SampleEffect::Impl::InitializeCamera(DirectX::FXMVECTOR camPos, DirectX::FXMVECTOR camLookAt, float nearPlaneDist, float farPlaneDist, float aspectWByH)
{
	m_camera.InitializeCamera(camPos, camLookAt, nearPlaneDist, farPlaneDist, aspectWByH);
	m_effectConstants.worldViewProj = XMMatrixTranspose(m_camera.m_worldViewProj);
	m_dirtyCB = true;
}

void SampleEffect::Impl::UpdateCamera(DirectX::GamePad::State & gamepad, float elapsedTime)
{
	m_camera.UpdateCamera(gamepad, elapsedTime);
	m_effectConstants.worldViewProj = XMMatrixTranspose(m_camera.m_worldViewProj);
	m_dirtyCB = true;
}

void SampleEffect::Impl::UpdateDrawData(_In_ DrawData* drawData)
{
	assert(m_drawDataCount < MAX_DRAWS);
	memcpy(m_drawDataMappedBuffer + m_drawDataCount, drawData, sizeof(DrawData));
	m_drawDataMappedBuffer[m_drawDataCount].drawID = m_drawDataCount;
	++m_drawDataCount;
}

void SampleEffect::Impl::SetDrawDataBuffer(_In_ ID3D12GraphicsCommandList* commandList)
{
	commandList->SetGraphicsRootShaderResourceView(static_cast<uint32_t>(RootParameterIndex::SRV0), m_drawDataBuffer->GetGPUVirtualAddress());
}

void SampleEffect::Impl::SetVertexBufferAsSRV(_In_ ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS& vertexBufferAsSRVVirtualAddress)
{
	commandList->SetGraphicsRootShaderResourceView(static_cast<uint32_t>(RootParameterIndex::SRV1), vertexBufferAsSRVVirtualAddress);
}

// SampleEffect members
SampleEffect::SampleEffect(_In_ ID3D12Device* device,
	const uint32_t backbufferCount,
	const EffectPipelineStateDescription* effectPsoDesc,
	_In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
	_In_ D3D12_SHADER_BYTECODE* pixelShaderBlob)
	: pImpl(new Impl(device, backbufferCount, effectPsoDesc, vertexShaderBlob, pixelShaderBlob))
{
}

SampleEffect::SampleEffect(SampleEffect&& moveFrom) noexcept
	: pImpl(std::move(moveFrom.pImpl))
{
}

SampleEffect& SampleEffect::operator=(SampleEffect&& moveFrom) noexcept
{
	pImpl = std::move(moveFrom.pImpl);
	return *this;
}

SampleEffect::~SampleEffect()
{
}

void XM_CALLCONV SampleEffect::InitializeCamera(DirectX::FXMVECTOR camPos, DirectX::FXMVECTOR camLookAt, float nearPlaneDist, float farPlaneDist, float aspectWByH)
{
	pImpl->InitializeCamera(camPos, camLookAt, nearPlaneDist, farPlaneDist, aspectWByH);
}

void SampleEffect::UpdateCamera(DirectX::GamePad::State & gamepad, float elapsedTime)
{
	pImpl->UpdateCamera(gamepad, elapsedTime);
}

void SampleEffect::UpdateDrawData(_In_ DrawData* drawData)
{
	pImpl->UpdateDrawData(drawData);
}

void SampleEffect::SetDrawDataBuffer(_In_ ID3D12GraphicsCommandList* commandList)
{
	pImpl->SetDrawDataBuffer(commandList);
}

void SampleEffect::SetVertexBufferAsSRV(_In_ ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS vertexBufferAsSRVVirtualAddress)
{
	pImpl->SetVertexBufferAsSRV(commandList, vertexBufferAsSRVVirtualAddress);
}

void SampleEffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
	pImpl->Apply(commandList);
}

void SampleEffect::SetCurrentStateArgs(uint32_t currFrameIndex, uint32_t psoID, D3D12_GPU_DESCRIPTOR_HANDLE texture)
{
	pImpl->SetCurrentStateArgs(currFrameIndex, psoID, texture);
}

void SampleEffect::SetRootSignature(_In_ ID3D12GraphicsCommandList* commandList)
{
	pImpl->SetRootSignature(commandList);
}

void SampleEffect::SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE texture)
{
	pImpl->SetTexture(texture);
}

ID3D12RootSignature* SampleEffect::GetRootSignature() const
{
	return pImpl->GetRootSignature();
};

void SampleEffect::ResetAllDirtyFlags()
{
	pImpl->ResetAllDirtyFlags();
}

// SampleCamera members
void XM_CALLCONV SampleCamera::InitializeCamera(DirectX::FXMVECTOR camPos,
	DirectX::FXMVECTOR camLookAt,
	float nearPlaneDist,
	float farPlaneDist,
	float aspectWByH)
{
	m_camPos = camPos;
	m_camLookAt = camLookAt;
	m_camLookAtDist = XMVectorGetX(XMVector4Length(XMVectorSubtract(m_camLookAt, m_camPos)));
	m_Near = nearPlaneDist;
	m_Far = farPlaneDist;

	// World-View-Projection matrix
	m_camView = XMMatrixLookAtLH(m_camPos, m_camLookAt, XMVectorSet(0, 1, 0, 0));
	m_camProj = XMMatrixPerspectiveFovLH(XM_PIDIV4, aspectWByH, m_Near, m_Far);
	m_worldViewProj = m_camView * m_camProj;
}

void SampleCamera::UpdateCamera(DirectX::GamePad::State& gamepad, float elapsedTime)
{
	// Update camera movement
	XMVECTOR forward = XMVector4Normalize(XMVectorSubtract(m_camLookAt, m_camPos));
	XMVECTOR right = XMVector4Normalize(XMVector3Cross(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), forward));
	XMVECTOR up = XMVector4Normalize(XMVector3Cross(forward, right));

	m_camPos = XMVectorAdd(m_camPos, XMVectorAdd(XMVectorScale(forward, gamepad.thumbSticks.leftY * elapsedTime * 100.f),
		XMVectorScale(right, gamepad.thumbSticks.leftX * elapsedTime * 100.f)));

	float rightThumbstickY = gamepad.thumbSticks.rightY;
	if (((XMVectorGetY(forward) >= 0.98f && rightThumbstickY > 0) ||
		(XMVectorGetY(forward) <= -0.98f && rightThumbstickY < 0)) &&
		(XMVectorGetX(forward) <= 0.01f || XMVectorGetX(forward) >= -0.01f) &&
		(XMVectorGetZ(forward) <= 0.01f || XMVectorGetZ(forward) >= -0.01f))
	{
		// Don't update the right thumbstick Y movement when the camera is pointing almost stright up or straight down
	}
	else
	{
		forward = XMVector3Rotate(forward, XMQuaternionRotationAxis(right, -1 * elapsedTime * rightThumbstickY));
	}
	forward = XMVector3Rotate(forward, XMQuaternionRotationAxis(up, elapsedTime * gamepad.thumbSticks.rightX));
	m_camLookAt = XMVectorAdd(m_camPos, XMVectorScale(forward, m_camLookAtDist));

	m_camView = XMMatrixLookAtLH(m_camPos, m_camLookAt, up);
	m_worldViewProj = XMMatrixMultiply(m_camView, m_camProj);
}
