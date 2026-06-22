//--------------------------------------------------------------------------------------
// SampleEffectFactory.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SampleEffect.h"

using namespace DirectX;

using Microsoft::WRL::ComPtr;

class SampleEffectFactory::Impl
{
public:
    Impl(_In_ ID3D12Device* device,
        D3D12_GPU_DESCRIPTOR_HANDLE textureDescriptor,
        D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor) :
        m_device(device),
        m_textureDescriptor(textureDescriptor),
        m_samplerDescriptor(samplerDescriptor)
    {
    }
    
    std::shared_ptr<IEffect> CreateSampleEffect(
        const DirectX::EffectPipelineStateDescription* effectPsoDesc,
        const uint32_t backbufferCount,
        const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
        const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob);

    std::shared_ptr<IEffect> CreateEffect(
        const EffectInfo & info,
        const EffectPipelineStateDescription & opaquePipelineState,
        const EffectPipelineStateDescription & alphaPipelineState,
        const D3D12_INPUT_LAYOUT_DESC & inputLayout,
        int textureDescriptorOffset,
        int samplerDescriptorOffset);

private:
    ComPtr<ID3D12Device>            m_device;
    D3D12_GPU_DESCRIPTOR_HANDLE     m_textureDescriptor;
    D3D12_GPU_DESCRIPTOR_HANDLE     m_samplerDescriptor;
    std::shared_ptr<SampleEffect>   m_sampleEffect;
};

std::shared_ptr<IEffect> SampleEffectFactory::Impl::CreateSampleEffect(
    const EffectPipelineStateDescription* effectPsoDesc,
    const uint32_t backbufferCount,
    const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
    const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob)
{
    m_sampleEffect = std::make_unique<SampleEffect>(
        m_device.Get(),
        effectPsoDesc,
        backbufferCount,
        vertexShaderBlob,
        pixelShaderBlob,
        true,
        true);

    return static_cast<std::shared_ptr<IEffect>>(m_sampleEffect);
}

// Need to be called when creating effect for each mesh part
std::shared_ptr<IEffect> SampleEffectFactory::Impl::CreateEffect(
    const EffectInfo & info,
    const EffectPipelineStateDescription & opaquePipelineState,
    const EffectPipelineStateDescription & alphaPipelineState,
    const D3D12_INPUT_LAYOUT_DESC & inputLayout,
    int textureDescriptorOffset,
    int samplerDescriptorOffset)
{
    UNREFERENCED_PARAMETER(opaquePipelineState);
    UNREFERENCED_PARAMETER(alphaPipelineState);
    UNREFERENCED_PARAMETER(inputLayout);

    std::shared_ptr<MeshPartEffect> meshPartEffect =
        std::make_shared<MeshPartEffect>(m_textureDescriptor,
            textureDescriptorOffset,
            info.diffuseTextureIndex,
            m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
            m_samplerDescriptor,
            samplerDescriptorOffset,
            info.samplerIndex,
            m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));
    return static_cast<std::shared_ptr<IEffect>>(meshPartEffect);
}

// SampleEffectFactory methods implementation
SampleEffectFactory::SampleEffectFactory(
    _In_ ID3D12Device* device,
    D3D12_GPU_DESCRIPTOR_HANDLE textureDescriptor,
    D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor) noexcept(false)
{
    pImpl = std::make_unique<Impl>(device, textureDescriptor, samplerDescriptor);
}

SampleEffectFactory::~SampleEffectFactory()
{
}

std::shared_ptr<IEffect> SampleEffectFactory::CreateSampleEffect(
    const DirectX::EffectPipelineStateDescription* effectPsoDesc,
    const uint32_t backbufferCount,
    const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
    const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob)
{
    return pImpl->CreateSampleEffect(effectPsoDesc, backbufferCount, vertexShaderBlob, pixelShaderBlob);
}

std::shared_ptr<IEffect> SampleEffectFactory::CreateEffect(
    const EffectInfo & info,
    const EffectPipelineStateDescription & opaquePipelineState,
    const EffectPipelineStateDescription & alphaPipelineState,
    const D3D12_INPUT_LAYOUT_DESC & inputLayout,
    int textureDescriptorOffset,
    int samplerDescriptorOffset)
{
    return pImpl->CreateEffect(info,
        opaquePipelineState,
        alphaPipelineState,
        inputLayout,
        textureDescriptorOffset,
        samplerDescriptorOffset);
}
