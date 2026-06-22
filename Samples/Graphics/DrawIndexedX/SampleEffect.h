//--------------------------------------------------------------------------------------
// SampleEffect.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "Effects.h"

// class to store values for each individual draw call in a constant buffer
struct DrawData
{
	uint32_t materialID;
	uint32_t baseVertexID;
	uint32_t drawID;
	uint32_t padding2;
};

__declspec(align(16)) class SampleCamera
{
public:
	void XM_CALLCONV InitializeCamera(DirectX::FXMVECTOR camPos,
		DirectX::FXMVECTOR camLookAt,
		float nearPlaneDist,
		float farPlaneDist,
		float aspectWByH);
	void UpdateCamera(DirectX::GamePad::State& gamepad, float elapsedTime);

	// May have alignment issues when ported to 32-bit
	DirectX::XMVECTOR m_camPos;
	DirectX::XMVECTOR m_camLookAt;
	DirectX::XMMATRIX m_camView;
	DirectX::XMMATRIX m_camProj;
	DirectX::XMMATRIX m_worldViewProj;
	float             m_camLookAtDist;
	float             m_Near;
	float             m_Far;
};

class SampleEffect : public DirectX::IEffect
{
public:
	SampleEffect(_In_ ID3D12Device* device, 
		const uint32_t backbufferCount,
		const DirectX::EffectPipelineStateDescription* effectPsoDesc,
		_In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
		_In_ D3D12_SHADER_BYTECODE* pixelShaderBlob);
	SampleEffect(SampleEffect&& moveFrom) noexcept;
	SampleEffect& operator= (SampleEffect&& moveFrom) noexcept;

	SampleEffect(SampleEffect const&) = delete;
	SampleEffect& operator= (SampleEffect const&) = delete;

	virtual ~SampleEffect();

	void XM_CALLCONV InitializeCamera(DirectX::FXMVECTOR camPos,
		DirectX::FXMVECTOR camLookAt,
		float nearPlaneDist,
		float farPlaneDist,
		float aspectWByH);
	void UpdateCamera(DirectX::GamePad::State& gamepad, float elapsedTime);
	void UpdateDrawData(_In_ DrawData* drawData);
	void SetDrawDataBuffer(_In_ ID3D12GraphicsCommandList* commandList);
	void SetVertexBufferAsSRV(_In_ ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS vertexBufferAsSRVVirtualAddress);

	// IEffect methods.
	void Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

	void SetRootSignature(_In_ ID3D12GraphicsCommandList* commandList);
	ID3D12RootSignature* GetRootSignature() const;
	void SetCurrentStateArgs(uint32_t currFrameIndex, uint32_t psoID, D3D12_GPU_DESCRIPTOR_HANDLE texture);
	void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE texture);
	void ResetAllDirtyFlags();

	enum class DescriptorIndex
	{
		Texture,
		DescriptorCount
	};

	enum class UAVDescriptorIndex
	{
		AppendUAV,
		DescriptorCount
	};

	enum class RootParameterIndex
	{
		DescriptorTable0,
		ConstantBuffer0,
		SRV0,
		SRV1,
		UAV0,
		RootParameterCount
	};

private:
	// Private implementation.
	class Impl;

	std::unique_ptr<Impl> pImpl;
};
