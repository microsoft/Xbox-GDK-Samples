//--------------------------------------------------------------------------------------
// ModelEffect.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// ModelEffect class used to apply effects to a loaded model
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Effects.h"
#include "VertexTypes.h"

#include "DirectXHelpers.h"
#include "DescriptorHeap.h"

#include "ModelEffect.h"
#include "WriteOwnBundlesHelper.h"
#include "DDSTextureLoader.h"
#include "CommonStates.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Create a default root signature if none supplied
_Use_decl_annotations_
void ModelLoader::ModelEffect::CreateRootSignature(ID3D12Device* device, ID3D12RootSignature** outRootSignature)
{
    ID3D12RootSignature* rootSignature = nullptr;

    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // Only the input assembler stage needs access to the constant buffer.
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
    CD3DX12_STATIC_SAMPLER_DESC sampler(0);

    CD3DX12_DESCRIPTOR_RANGE descriptorRanges[static_cast<uint32_t>(DescriptorIndex::DescriptorCount)] = {};
    descriptorRanges[static_cast<uint32_t>(DescriptorIndex::Texture)].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    descriptorRanges[static_cast<uint32_t>(DescriptorIndex::Buffer)].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
    descriptorRanges[static_cast<uint32_t>(DescriptorIndex::UAV)].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0);

    CD3DX12_ROOT_PARAMETER rootParameters[static_cast<uint32_t>(RootParameterIndex::RootParameterCount)] = {};
    rootParameters[static_cast<uint32_t>(RootParameterIndex::DescriptorTable0)].InitAsDescriptorTable(
        _countof(descriptorRanges),
        descriptorRanges);
    rootParameters[static_cast<uint32_t>(RootParameterIndex::ConstantBuffer0)].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[static_cast<uint32_t>(RootParameterIndex::ConstantBuffer1)].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);
    rootParameters[static_cast<uint32_t>(RootParameterIndex::ConstantBuffer2)].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rsigDesc;
    rsigDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

    DX::ThrowIfFailed(DirectX::CreateRootSignature(device, &rsigDesc, &rootSignature));

    rootSignature->SetName(L"Mesh Root Signature");

    *outRootSignature = rootSignature;
}

void ModelLoader::ModelEffect::CreatePSOs()
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_rootSignature;
    psoDesc.BlendState = m_effectPSODesc.blendDesc;
    psoDesc.DepthStencilState = m_effectPSODesc.depthStencilDesc;
    psoDesc.RasterizerState = m_effectPSODesc.rasterizerDesc;
    psoDesc.DSVFormat = m_effectPSODesc.renderTargetState.dsvFormat;
    psoDesc.NodeMask = m_effectPSODesc.renderTargetState.nodeMask;
    psoDesc.NumRenderTargets = m_effectPSODesc.renderTargetState.numRenderTargets;
    memcpy(psoDesc.RTVFormats, m_effectPSODesc.renderTargetState.rtvFormats, sizeof(psoDesc.RTVFormats));
    psoDesc.SampleDesc = m_effectPSODesc.renderTargetState.sampleDesc;
    psoDesc.SampleMask = m_effectPSODesc.renderTargetState.sampleMask;
    psoDesc.InputLayout = m_effectPSODesc.inputLayout;
    psoDesc.IBStripCutValue = m_effectPSODesc.stripCutValue;
    psoDesc.PrimitiveTopologyType = m_effectPSODesc.primitiveTopology;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

    for (uint32_t i = 0; i < NUM_PSO; ++i)
    {
        psoDesc.VS = (m_vertexShaderBlob[i]);
        psoDesc.PS = (m_pixelShaderBlob[i]);

        HRESULT hr = m_device->CreateGraphicsPipelineState(
            &psoDesc,
            IID_GRAPHICS_PPV_ARGS(m_pso[i].ReleaseAndGetAddressOf()));


        if (FAILED(hr))
        {
            throw std::exception(
                "CreatePipelineState failed to create a PSO. "
                "Enable the Direct3D Debug Layer for more information.");
        }

        m_pso[i]->GetDescriptorX(m_psoDescriptor + i);
    }
}

void ModelLoader::ModelEffect::CreateDefaultLighting(_In_ const IEffectFactory::EffectInfo& info)
{
    static_assert(NUM_LIGHTS <= MAX_LIGHTS, "The number of lights should be less than the MAX_LIGHTS allowed");

    // If light required
    {
        static const XMVECTORF32 defaultDirections[NUM_LIGHTS] =
        {
            { -1.f, 2.f, 1.5f },
            { 1.0f, -2.0f, 1.5F },
        };

        static const XMVECTORF32 defaultDiffuse[NUM_LIGHTS] =
        {
            { 1.0f, 1.0f, 1.0f },
            { 1.0f, 1.0f, 1.0f },
        };

        static const XMVECTORF32 defaultSpecular[NUM_LIGHTS] =
        {
            { 1.0000000f, 0.9607844f, 0.8078432f },
            { 0.2000000f, 0.2000000f, 0.2000000f },
        };

        for (int i = 0; i < NUM_LIGHTS; i++)
        {
            m_modelEffectConstants.lightDirection[i] = defaultDirections[i];
            m_modelEffectConstants.lightDiffuseColor[i] = defaultDiffuse[i];
            m_modelEffectConstants.lightSpecularColor[i] = defaultSpecular[i];
        }
    }

    m_modelEffectConstants.diffuseColor = XMLoadFloat3(&info.diffuseColor);

    if (info.specularColor.x != 0 || info.specularColor.y != 0 || info.specularColor.z != 0)
    {
        m_modelEffectConstants.specularColorAndPower = XMLoadFloat3(&info.specularColor);
        m_modelEffectConstants.specularColorAndPower = XMVectorSetW(m_modelEffectConstants.specularColorAndPower, info.specularPower);
    }
    else
    {
        m_modelEffectConstants.specularColorAndPower = g_XMIdentityR3;
    }

    m_modelEffectConstants.emissiveColor = XMLoadFloat3(&info.emissiveColor);

    static const XMVECTORF32 defaultAmbient = { 0.f, 0.0f, 0.f };
    m_modelEffectConstants.ambientColor = defaultAmbient;

}


void ModelLoader::ModelEffect::GetRootSignature(ID3D12RootSignature ** rootSignature)
{
    *rootSignature = m_rootSignature;
}

void ModelLoader::ModelEffect::GetPSO(ID3D12PipelineState ** pso, _In_ uint32_t psoID)
{
    *pso = m_pso[psoID].Get();
}

void ModelLoader::ModelEffect::UpdateConstantBufferResource(uint32_t currFrameIndex)
{
    // Update instance data
    D3D12_RANGE cbUpdateRange =
    {
        sizeof(ModelEffectConstants) * currFrameIndex,
        sizeof(ModelEffectConstants) * (currFrameIndex + 1)
    };
    ModelEffectConstants* modelEffectConstantsCB;
    m_effectDataCB->Map(0, &cbUpdateRange, reinterpret_cast<void**>(&modelEffectConstantsCB));
    memcpy(&(modelEffectConstantsCB[currFrameIndex]), &m_modelEffectConstants, sizeof(ModelEffectConstants));
    m_effectDataCB->Unmap(0, &cbUpdateRange);
}

void ModelLoader::ModelEffect::SetRootSignature(_In_ ID3D12GraphicsCommandList* commandList)
{
    // Set the root signature
    commandList->SetGraphicsRootSignature(m_rootSignature);
}

void ModelLoader::ModelEffect::SetPSO(_In_ ID3D12GraphicsCommandList* commandList, uint32_t psoID)
{
    // Set the pipeline state
    commandList->SetPipelineState(m_pso[psoID].Get());
}

void ModelLoader::ModelEffect::SetPSOBYOB(uint32_t** writeAddress, uint32_t psoID)
{
    WriteOwnBundle::SetPipelineStateBYOB(writeAddress, m_psoDescriptor[psoID]);
}

void ModelLoader::ModelEffect::SetDescriptorHeaps(_In_ ID3D12GraphicsCommandList* commandList, _In_ uint64_t cbOffset)
{
    // Set the texture descriptors.
    commandList->SetGraphicsRootDescriptorTable(static_cast<uint32_t>(RootParameterIndex::DescriptorTable0), m_textureGPUHandle);
    // Set constants
    commandList->SetGraphicsRootConstantBufferView(static_cast<uint32_t>(RootParameterIndex::ConstantBuffer0), m_effectDataCB->GetGPUVirtualAddress() + cbOffset);
}

void ModelLoader::ModelEffect::SetTexture(_In_ D3D12_GPU_DESCRIPTOR_HANDLE texGPUHandle)
{
    m_textureGPUHandle = texGPUHandle;
}

// Set SRV into the root descriptor by writing GPU packets
void ModelLoader::ModelEffect::SetRootDescriptorTableBYOB(uint32_t** writeAddress, _In_ uint32_t* rootPacketHeader)
{
    WriteOwnBundle::SetGraphicsRootDescriptorTableBYOB(writeAddress, rootPacketHeader, static_cast<uint32_t>(RootParameterIndex::DescriptorTable0), m_textureGPUHandle);
}

// Set constant buffer using GPU packets
void ModelLoader::ModelEffect::UpdateConstantsBYOB(uint32_t** writeAddress, _In_ uint32_t* rootPacketHeader, _In_ uint64_t cbOffset)
{
    WriteOwnBundle::SetGraphicsRootConstantBufferViewBYOB(writeAddress, rootPacketHeader, static_cast<uint32_t>(RootParameterIndex::ConstantBuffer0), m_effectDataCB->GetGPUVirtualAddress() + cbOffset);
}

void ModelLoader::ModelEffect::SetStates(_In_ ID3D12GraphicsCommandList * commandList, _In_ uint32_t psoID, _In_ uint64_t cbOffset)
{
    // Set the root signature, PSO and descriptor heaps
    SetRootSignature(commandList);
    SetPSO(commandList, psoID);
    SetDescriptorHeaps(commandList, cbOffset);
}

