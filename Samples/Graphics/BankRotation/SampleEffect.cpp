//--------------------------------------------------------------------------------------
// SampleEffect.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SampleEffect.h"
#include "DirectXHelpers.h"

#include "AlignedNew.h"

using namespace DirectX;

using Microsoft::WRL::ComPtr;

#pragma warning(disable : 4324)
// On Xbox One, constant buffers should be aligned to 64 bytes
#define CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT 64
__declspec(align(CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)) struct ConstantBufferBase {};

// Constant buffer layout. Must match the shader!
struct SampleEffectConstants : ConstantBufferBase
{
    XMMATRIX worldViewProj;
};

static_assert((sizeof(SampleEffectConstants) % 16) == 0, "CB size not padded correctly");

__declspec(align(CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT)) class SampleEffect::Impl :
    public AlignedNew<SampleEffect::Impl>
{
public:
    Impl(_In_ ID3D12Device* device,
        const EffectPipelineStateDescription* effectPsoDesc,
        const uint32_t backbufferCount,
        const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
        const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob,
        bool enableRootSignature,
        bool enableConstantBuffer);

    void CreateRootSignature(const _In_ D3D12_SHADER_BYTECODE* m_vertexShaderBlob);
    void CreatePSOs(const EffectPipelineStateDescription* effectPsoDesc,
        const _In_ D3D12_SHADER_BYTECODE* m_vertexShaderBlob,
        const _In_ D3D12_SHADER_BYTECODE* m_pixelShaderBlob);
    void Apply(_In_ ID3D12GraphicsCommandList* commandList);

    void SetRootSignature(_In_ ID3D12GraphicsCommandList* commandList);
    ID3D12RootSignature* GetRootSignature() const { return m_rootSignature.Get(); };
    void SetDescriptorHeaps(_In_ ID3D12GraphicsCommandList * commandList, _In_ uint32_t currFrameIndex);
    void SetCurrentStateArgs(uint32_t currFrameIndex, _In_ uint32_t psoID, _In_ D3D12_GPU_DESCRIPTOR_HANDLE texture);
    void SetAllDirtyFlags();

    void UpdateConstants(FXMMATRIX worldViewProj);

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
    ComPtr<ID3D12RootSignature>                 m_rootSignature;
    ComPtr<ID3D12PipelineState>                 m_pso;
    ComPtr<ID3D12Device>                        m_device;
    D3D12_GPU_DESCRIPTOR_HANDLE                 m_textureGPUHandle;
    SampleEffectConstants                       m_effectConstants;
    ComPtr<ID3D12Resource>                      m_effectDataCB;
    SampleEffectConstants*                      m_effectConstantsCB;
    uint32_t                                    m_currPsoID;
    uint32_t                                    m_backbufferIndex;

    // Dirty flags
    bool                                        m_dirtyPso;
    bool                                        m_dirtyTexture;
    bool                                        m_dirtyCB;
};

SampleEffect::Impl::Impl(_In_ ID3D12Device* device,
    const EffectPipelineStateDescription* effectPsoDesc,
    const uint32_t backbufferCount,
    const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
    const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob,
    bool enableRootSignature,
    bool enableConstantBuffer) :
    m_device(device),
    m_currPsoID(0),
    m_dirtyPso(true),
    m_dirtyTexture(true),
    m_dirtyCB(true)
{
    if (enableRootSignature)
    {
        CreateRootSignature(vertexShaderBlob);
    }

    CreatePSOs(effectPsoDesc, vertexShaderBlob, pixelShaderBlob);

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

void SampleEffect::Impl::CreateRootSignature(const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob)
{
    if (vertexShaderBlob->pShaderBytecode)
    {
        DX::ThrowIfFailed(
            m_device.Get()->CreateRootSignature(0, vertexShaderBlob->pShaderBytecode, vertexShaderBlob->BytecodeLength,
                IID_GRAPHICS_PPV_ARGS(m_rootSignature.ReleaseAndGetAddressOf())));
        m_rootSignature->SetName(L"Mesh Root Signature");
    }
    else
    {
        throw std::exception("Vertex Shader blob is invalid");
    }
}

void SampleEffect::Impl::CreatePSOs(
    const EffectPipelineStateDescription* effectPsoDesc,
    const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
    const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob)
{
    assert(vertexShaderBlob);

    D3D12_SHADER_BYTECODE pixelShaderBlobForPSO;
    
    if (pixelShaderBlob)
        pixelShaderBlobForPSO = *pixelShaderBlob;
    else
    {
        pixelShaderBlobForPSO.BytecodeLength = 0;
        pixelShaderBlobForPSO.pShaderBytecode = nullptr;
    }
    effectPsoDesc->CreatePipelineState(m_device.Get(), m_rootSignature.Get(), *vertexShaderBlob, pixelShaderBlobForPSO, m_pso.ReleaseAndGetAddressOf());
}

void SampleEffect::Impl::SetRootSignature(_In_ ID3D12GraphicsCommandList* commandList)
{
    assert(m_rootSignature.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
}

void SampleEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Set the PSO and descriptor heaps
    if (m_dirtyPso)
    {
        commandList->SetPipelineState(m_pso.Get());
        m_dirtyPso = false;;
    }
    
    SetDescriptorHeaps(commandList, m_backbufferIndex);
}

void SampleEffect::Impl::SetAllDirtyFlags()
{
    m_dirtyPso = true;
    m_dirtyTexture = true;
    m_dirtyCB = true;
}

void SampleEffect::Impl::SetDescriptorHeaps(_In_ ID3D12GraphicsCommandList* commandList, _In_ uint32_t currFrameIndex)
{
    // Set the texture descriptors
    if (m_dirtyTexture)
    {
        commandList->SetGraphicsRootDescriptorTable(static_cast<uint32_t>(RootParameterIndex::DescriptorTableSRV), m_textureGPUHandle);
        m_dirtyTexture = false;
    }
    
    // Set constants
    if (m_dirtyCB)
    {
        // Assert if constant buffer is not present in this instance
        assert(m_effectConstantsCB);
        assert(m_effectDataCB.Get());

        memcpy(&(m_effectConstantsCB[currFrameIndex]), &m_effectConstants, sizeof(SampleEffectConstants));
        size_t cbOffset = sizeof(SampleEffectConstants) * currFrameIndex;
        commandList->SetGraphicsRootConstantBufferView(static_cast<uint32_t>(RootParameterIndex::ConstantBuffer0), m_effectDataCB->GetGPUVirtualAddress() + cbOffset);
        m_dirtyCB = false;
    }
}

void SampleEffect::Impl::SetCurrentStateArgs(uint32_t currFrameIndex, _In_ uint32_t psoID, _In_ D3D12_GPU_DESCRIPTOR_HANDLE texture)
{
    if (!m_dirtyPso && m_currPsoID != psoID)
        m_dirtyPso = true;
    m_currPsoID = psoID;
    m_backbufferIndex = currFrameIndex;
    if (!m_dirtyTexture && m_textureGPUHandle.ptr != texture.ptr)
        m_dirtyTexture = true;
    m_textureGPUHandle = texture;
}

void SampleEffect::Impl::UpdateConstants(FXMMATRIX worldViewProj)
{
    m_effectConstants.worldViewProj = XMMatrixTranspose(worldViewProj);
    m_dirtyCB = true;
}

// SampleEffect members
SampleEffect::SampleEffect(_In_ ID3D12Device* device,
    const EffectPipelineStateDescription* effectPsoDesc,
    const uint32_t backbufferCount,
    const _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
    const _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob,
    bool enableRootSignature,
    bool enableConstantBuffer) noexcept :
    pImpl(new Impl(device, effectPsoDesc, backbufferCount, vertexShaderBlob, pixelShaderBlob, enableRootSignature, enableConstantBuffer))
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

void SampleEffect::UpdateConstants(FXMMATRIX worldViewProj)
{
    pImpl->UpdateConstants(worldViewProj);
}

void SampleEffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->Apply(commandList);
}

void SampleEffect::SetCurrentStateArgs(uint32_t currFrameIndex, _In_ uint32_t psoID, _In_ D3D12_GPU_DESCRIPTOR_HANDLE texture)
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

void SampleEffect::SetAllDirtyFlags()
{
    pImpl->SetAllDirtyFlags();
}

// MeshPartEffect
class MeshPartEffect::Impl
{
public:
    Impl(D3D12_GPU_DESCRIPTOR_HANDLE textureDescriptor,
        int textureDescriptorOffset,
        int textureOffset,
        int textureIncrementSize,
        D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor = {},
        int samplerDescriptorOffset = 0,
        int samplerOffset = 0,
        int samplerIncrementSize = 0)
        : m_textureDescriptor(textureDescriptor),
        m_samplerDescriptor(samplerDescriptor)
    {
        m_textureDescriptor.ptr += (textureDescriptorOffset + textureOffset) * textureIncrementSize;
        m_samplerDescriptor.ptr += (samplerDescriptorOffset + samplerOffset) * samplerIncrementSize;
    }

    void Apply(_In_ ID3D12GraphicsCommandList* commandList);

private:
    D3D12_GPU_DESCRIPTOR_HANDLE m_textureDescriptor;
    D3D12_GPU_DESCRIPTOR_HANDLE m_samplerDescriptor;
};

void MeshPartEffect::Impl::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    commandList->SetGraphicsRootDescriptorTable(static_cast<uint32_t>(RootParameterIndex::DescriptorTableSRV), m_textureDescriptor);
    // Sampler Descriptor is unused in this sample. Set the sampler descriptor table if it is being used
}

MeshPartEffect::MeshPartEffect(D3D12_GPU_DESCRIPTOR_HANDLE textureDescriptor,
    int textureDescriptorOffset,
    int textureOffset,
    int textureIncrementSize,
    D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor,
    int samplerDescriptorOffset,
    int samplerOffset,
    int samplerIncrementSize) noexcept(false)
{
    pImpl = std::make_unique<Impl>(textureDescriptor,
        textureDescriptorOffset,
        textureOffset,
        textureIncrementSize,
        samplerDescriptor,
        samplerDescriptorOffset,
        samplerOffset,
        samplerIncrementSize);
}

MeshPartEffect::~MeshPartEffect()
{
}

// MeshPartEffect methods
void MeshPartEffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    pImpl->Apply(commandList);
}
