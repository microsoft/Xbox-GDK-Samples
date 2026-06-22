//--------------------------------------------------------------------------------------
// Render3DTexture.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Render3DTexture.h"
#include "DirectXHelpers.h"

using Microsoft::WRL::ComPtr;
using namespace DX;

// Initialize
void Render3DTexture::Initialize(_In_ ID3D12Device* d3dDevice)
{
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, rootSignatureFlags);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    DX::ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    DX::ThrowIfFailed(d3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(m_d3dRootSignature.ReleaseAndGetAddressOf())));
}

// Draw the 3D texture
_Use_decl_annotations_
void Render3DTexture::Draw(
    ID3D12GraphicsCommandList* d3dCommandList,
    ID3D12PipelineState* d3dPSO)
{
    d3dCommandList->SetGraphicsRootSignature(m_d3dRootSignature.Get());
    d3dCommandList->SetPipelineState(d3dPSO);
    d3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    d3dCommandList->DrawInstanced(4, 32, 0, 0);
}

void Render3DTexture::ReleaseDevice()
{
    m_d3dRootSignature.Reset();
}
