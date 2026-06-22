//--------------------------------------------------------------------------------------
// SampleEffect.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "pch.h"
#include "Effects.h"

namespace
{
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
        DescriptorTableSRV,
        ConstantBuffer0,
        DescriptorTableUAV,
        RootParameterCount
    };

    enum class SamplerIndex
    {
        StaticSampler0,
    };
};

class SampleEffect : public DirectX::IEffect
{
public:
    SampleEffect(_In_ ID3D12Device* device,
        const DirectX::EffectPipelineStateDescription* effectPsoDesc,
        const uint32_t backbufferCount = 2,
        const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob = nullptr,
        const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob = nullptr,
        bool enableRootSignature = false,
        bool enableConstantBuffer = false) noexcept;

    SampleEffect(SampleEffect&& moveFrom) noexcept;
    SampleEffect& operator= (SampleEffect&& moveFrom) noexcept;

    SampleEffect(SampleEffect const&) = delete;
    SampleEffect& operator= (SampleEffect const&) = delete;

    ~SampleEffect() override;

    void UpdateConstants(DirectX::FXMMATRIX worldViewProj);

    // IEffect methods
    void Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

    void SetRootSignature(_In_ ID3D12GraphicsCommandList* commandList);
    ID3D12RootSignature* GetRootSignature() const;
    void SetCurrentStateArgs(uint32_t currFrameIndex, _In_ uint32_t psoID, _In_ D3D12_GPU_DESCRIPTOR_HANDLE texture);
    void SetTexture(D3D12_GPU_DESCRIPTOR_HANDLE texture);
    void SetAllDirtyFlags();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class MeshPartEffect : public DirectX::IEffect
{
public:    
    MeshPartEffect(D3D12_GPU_DESCRIPTOR_HANDLE textureDescriptor,
        int textureDescriptorOffset,
        int textureOffset,
        int textureIncrementSize,
        D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor,
        int samplerDescriptorOffset,
        int samplerOffset,
        int samplerIncrementSize) noexcept(false);

    ~MeshPartEffect() override;

    // Override IEffect methods
    void Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

class SampleEffectFactory : public DirectX::IEffectFactory
{
public:
    SampleEffectFactory(
        _In_ ID3D12Device* device,
        D3D12_GPU_DESCRIPTOR_HANDLE textureDescriptor = {},
        D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor = {}) noexcept(false);

    ~SampleEffectFactory() override;

    // Overide IEffectFactory methods
    std::shared_ptr<DirectX::IEffect> CreateEffect(
        const EffectInfo& info,
        const DirectX::EffectPipelineStateDescription& opaquePipelineState,
        const DirectX::EffectPipelineStateDescription& alphaPipelineState,
        const D3D12_INPUT_LAYOUT_DESC& inputLayout,
        int textureDescriptorOffset = 0,
        int samplerDescriptorOffset = 0) override;

    std::shared_ptr<DirectX::IEffect> CreateSampleEffect(
        const DirectX::EffectPipelineStateDescription* effectPsoDesc,
        const uint32_t backbufferCount = 2,
        const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob = nullptr,
        const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob = nullptr);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
