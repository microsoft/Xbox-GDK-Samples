//--------------------------------------------------------------------------------------
// SampleEffect.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Effects.h"

class SampleEffect : public DirectX::IEffect
{
public:
	SampleEffect(_In_ ID3D12Device* device,
		const _In_ DirectX::EffectPipelineStateDescription* effectPsoDesc,
		const uint32_t backbufferCount = 2,
		const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob = nullptr,
		const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob = nullptr,
		bool enableRootSignature = false,
		bool enableConstantBuffer = false);
	SampleEffect(SampleEffect&& moveFrom) noexcept;
	SampleEffect& operator =(SampleEffect&& moveFrom) noexcept;

	SampleEffect(SampleEffect const&) = delete;
	SampleEffect& operator=(SampleEffect const&) = delete;

	virtual ~SampleEffect();

	void Apply(_In_ ID3D12GraphicsCommandList* commandList) override;
	void UpdateConstants(DirectX::FXMMATRIX& worldViewProj); 
	void SetDescriptors(_In_ ID3D12GraphicsCommandList* commandList, uint32_t currFrameIndex, D3D12_GPU_DESCRIPTOR_HANDLE texture);
	ID3D12RootSignature* GetRootSignature() const;
	void SetAllDirtyFlags();

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};
