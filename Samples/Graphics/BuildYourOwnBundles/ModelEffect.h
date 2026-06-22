//--------------------------------------------------------------------------------------
// ModelEffect.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
// ModelEffect class used to apply effects to a loaded model
//--------------------------------------------------------------------------------------

#pragma once

// As Common.h file is shared by both hlsl and cpp files,
// adding few #defines here when compiling the cpp files
#define uint uint32_t
#define uint2 uint64_t 
#define PipelineStateObject D3D12XBOX_DESCRIPTOR_PIPELINE_STATE
#define NOT_HLSL 1

#include "Common.h"
#include "Effects.h"
#include "Model.h"
#include "GraphicsMemory.h"

#include <DirectXMath.h>
#include <DirectXCollision.h>

namespace ModelLoader
{
    struct ModelEffectConstants
    {
        DirectX::XMVECTOR diffuseColor;
        DirectX::XMVECTOR emissiveColor;
        DirectX::XMVECTOR ambientColor;
        DirectX::XMVECTOR specularColorAndPower;

        DirectX::XMVECTOR lightDirection[MAX_LIGHTS];
        DirectX::XMVECTOR lightDiffuseColor[MAX_LIGHTS];
        DirectX::XMVECTOR lightSpecularColor[MAX_LIGHTS];

        DirectX::XMVECTOR eyePosition;

        DirectX::XMMATRIX world;
        DirectX::XMVECTOR worldInverseTranspose[3];
        DirectX::XMMATRIX worldViewProj;
    };

    enum class DescriptorIndex
    {
        Texture,
        Buffer,
        UAV,
        DescriptorCount
    };

    enum class RootParameterIndex
    {
        DescriptorTable0,
        ConstantBuffer0,
        ConstantBuffer1,
        ConstantBuffer2,
        RootParameterCount
    };

    class ModelEffect
    {
    public:
        explicit ModelEffect(ID3D12Device* device,
            _In_ const DirectX::EffectPipelineStateDescription& pipelineStateDesc,
            _In_ D3D12_SHADER_BYTECODE* vertexShaderBlob,
            _In_ D3D12_SHADER_BYTECODE* pixelShaderBlob,
            _In_ ID3D12RootSignature* rootSignature,
            _In_ ID3D12Resource* effectDataCB) :
            m_device(device),
            m_effectPSODesc(pipelineStateDesc),
            m_rootSignature(rootSignature),
            m_effectDataCB(effectDataCB)
        {
            for (uint32_t i = 0; i < NUM_PSO; ++i)
            {
                m_vertexShaderBlob[i] = vertexShaderBlob[i];
                m_pixelShaderBlob[i] = pixelShaderBlob[i];
            }

            CreatePSOs();
        }

        void GetRootSignature(ID3D12RootSignature** rootSignature);
        void GetPSO(ID3D12PipelineState** pso, _In_ uint32_t psoID);
        void UpdateConstantBufferResource(uint32_t currFrameIndex);
        void SetRootSignature(_In_ ID3D12GraphicsCommandList* commandList);
        void SetPSO(_In_ ID3D12GraphicsCommandList* commandList, uint32_t psoID);
        void SetDescriptorHeaps(_In_ ID3D12GraphicsCommandList* commandList, _In_ uint64_t cbOffset = 0);
        void SetTexture(_In_ D3D12_GPU_DESCRIPTOR_HANDLE texGPUHandle);
        void SetStates(_In_ ID3D12GraphicsCommandList* commandList, _In_ uint32_t psoID, _In_ uint64_t cbOffset = 0);

        // For building own bundles
        void SetPSOBYOB(uint32_t** writeAddress, uint32_t psoID);
        void SetRootDescriptorTableBYOB(uint32_t** writeAddress, _In_ uint32_t* rootPacketHeader);
        void UpdateConstantsBYOB(uint32_t** writeAddress, _In_ uint32_t* rootPacketHeader, _In_ uint64_t cbOffset);
        
        ModelEffectConstants           m_modelEffectConstants;

        virtual ~ModelEffect() {}

        void CreatePSOs();
        void CreateDefaultLighting(_In_ const DirectX::IEffectFactory::EffectInfo&  info);
        D3D12_GPU_VIRTUAL_ADDRESS GetEffectDataGPUVirtualAddress() { return m_effectDataCB->GetGPUVirtualAddress(); }
        uint64_t GetTextureGPUHandle() { return m_textureGPUHandle.ptr; }
        D3D12XBOX_DESCRIPTOR_PIPELINE_STATE GetPSODescriptors(uint32_t psoID) 
        { 
            assert(psoID < NUM_PSO);
            return m_psoDescriptor[psoID]; 
        }

        static void CreateRootSignature(_In_ ID3D12Device* device, _Out_ ID3D12RootSignature** rootSignature);

    private:
        ID3D12Device*                                  m_device;
        DirectX::EffectPipelineStateDescription        m_effectPSODesc;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>    m_pso[NUM_PSO];
        D3D12XBOX_DESCRIPTOR_PIPELINE_STATE            m_psoDescriptor[NUM_PSO];
        D3D12_GPU_DESCRIPTOR_HANDLE                    m_textureGPUHandle;
        D3D12_SHADER_BYTECODE                          m_vertexShaderBlob[NUM_PSO];
        D3D12_SHADER_BYTECODE                          m_pixelShaderBlob[NUM_PSO];
        ID3D12RootSignature*                           m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12Resource>         m_effectDataCB;
    };
};
