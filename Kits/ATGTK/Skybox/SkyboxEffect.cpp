//--------------------------------------------------------------------------------------
// SkyboxEffect.cpp
//
// A sky box effect for DirectX 12.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "SkyboxEffect.h"

#include "DirectXHelpers.h"
#include "GraphicsMemory.h"

#include "ReadData.h"

#include <stdexcept>


using namespace DirectX;
using namespace DX;

namespace
{
    struct __declspec(align(16)) SkyboxEffectConstants
    {
        XMMATRIX worldViewProj;
    };

    static_assert((sizeof(SkyboxEffectConstants) % 16) == 0, "CB size alignment");

    constexpr uint32_t DirtyConstantBuffer = 0x1;
    constexpr uint32_t DirtyWVPMatrix = 0x2;
}

SkyboxEffect::SkyboxEffect(
    _In_ ID3D12Device* device,
    const EffectPipelineStateDescription& pipelineStateDesc) :
        m_device(device),
        m_texture{},
        m_textureSampler{},
        m_dirtyFlags(uint32_t(-1))
{
    // Create root signature
    constexpr D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

    const CD3DX12_DESCRIPTOR_RANGE textureSRVs(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    const CD3DX12_DESCRIPTOR_RANGE textureSamplers(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[Count] = {};
    rootParameters[InputSRV].InitAsDescriptorTable(1, &textureSRVs);
    rootParameters[InputSampler].InitAsDescriptorTable(1, &textureSamplers);
    rootParameters[ConstantBuffer].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rsigDesc = {};
    rsigDesc.Init(static_cast<UINT>(std::size(rootParameters)), rootParameters,
        0, nullptr, rootSignatureFlags);

    DX::ThrowIfFailed(
        CreateRootSignature(device, &rsigDesc, m_rootSig.ReleaseAndGetAddressOf())
    );

    // Get shaders
    auto vsBlob = DX::ReadData(L"SkyboxEffect_VS.cso");
    D3D12_SHADER_BYTECODE vs = { vsBlob.data(), vsBlob.size() };

    auto psBlob = DX::ReadData(L"SkyboxEffect_PS.cso");
    D3D12_SHADER_BYTECODE ps = { psBlob.data(), psBlob.size() };

    pipelineStateDesc.CreatePipelineState(device, m_rootSig.Get(), vs, ps,
        m_pso.ReleaseAndGetAddressOf());
}


// IEffect methods.
void SkyboxEffect::Apply(_In_ ID3D12GraphicsCommandList* commandList)
{
    if (m_dirtyFlags & DirtyWVPMatrix)
    {
        // Skybox ignores m_world matrix and the translation of m_view
        XMMATRIX view = m_view;
        view.r[3] = g_XMIdentityR3;
        m_worldViewProj = XMMatrixMultiply(view, m_proj);

        m_dirtyFlags &= ~DirtyWVPMatrix;
        m_dirtyFlags |= DirtyConstantBuffer;
    }

    if (m_dirtyFlags & DirtyConstantBuffer)
    {
        auto cb = GraphicsMemory::Get(m_device.Get()).AllocateConstant<SkyboxEffectConstants>();

        const XMMATRIX transpose = XMMatrixTranspose(m_worldViewProj);
        memcpy(cb.Memory(), &transpose, cb.Size());
        std::swap(m_constantBuffer, cb);

        m_dirtyFlags &= ~DirtyConstantBuffer;
    }

    // Set the root signature & parameters
    commandList->SetGraphicsRootSignature(m_rootSig.Get());

    if (!m_texture.ptr || !m_textureSampler.ptr)
    {
        throw std::runtime_error("SkyboxEffect");
    }

    commandList->SetGraphicsRootDescriptorTable(InputSRV, m_texture);
    commandList->SetGraphicsRootDescriptorTable(InputSampler, m_textureSampler);
    commandList->SetGraphicsRootConstantBufferView(ConstantBuffer, m_constantBuffer.GpuAddress());

    // Set the pipeline state
    commandList->SetPipelineState(m_pso.Get());
}


// Camera settings.
void SkyboxEffect::SetWorld(FXMMATRIX /*value*/)
{
}

void SkyboxEffect::SetView(FXMMATRIX value)
{
    m_view = value;
    m_dirtyFlags |= DirtyWVPMatrix;
}

void SkyboxEffect::SetProjection(FXMMATRIX value)
{
    m_proj = value;
    m_dirtyFlags |= DirtyWVPMatrix;
}

void SkyboxEffect::SetMatrices(FXMMATRIX /*world*/, CXMMATRIX view, CXMMATRIX projection)
{
    m_view = view;
    m_proj = projection;
    m_dirtyFlags |= DirtyWVPMatrix;
}


// Texture settings.
void SkyboxEffect::SetTexture(
    _In_ D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor,
    _In_ D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor)
{
    m_texture = srvDescriptor;
    m_textureSampler = samplerDescriptor;
}
