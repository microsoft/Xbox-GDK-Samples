//-----------------------------------------------------------------------------
// CustomEffect.h
//
// This is a customization of a DirectXTK Tool Kit IEffect. Can be constructed
// with only a root signature and pipeline state object. However, the root
// signature and constants must match the shaders shipped with the AdvancedPSO
// sample.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include <unordered_map>
#include <Effects.h>
#include <DirectXMath.h>
#include <GraphicsMemory.h>

// Constant buffer layout. Must match the shader!
namespace ATG
{
    struct Constants
    {
        static constexpr int MaxDirectionalLights = 3;

        DirectX::XMVECTOR diffuseColor;
        DirectX::XMVECTOR emissiveColor;
        DirectX::XMVECTOR specularColorAndPower;

        DirectX::XMVECTOR lightDirection[MaxDirectionalLights];
        DirectX::XMVECTOR lightDiffuseColor[MaxDirectionalLights];
        DirectX::XMVECTOR lightSpecularColor[MaxDirectionalLights];

        DirectX::XMVECTOR eyePosition;

        DirectX::XMVECTOR fogColor;
        DirectX::XMVECTOR fogVector;

        DirectX::XMMATRIX world;
        DirectX::XMVECTOR worldInverseTranspose[3];
        DirectX::XMMATRIX worldViewProj;

        float             globalTime;

        void SetLightingConstants()
        {
            static const DirectX::XMVECTORF32 defaultDirections[MaxDirectionalLights] =
            {
                { -0.5265408f, -0.5735765f, -0.6275069f },
                { 0.7198464f,  0.3420201f,  0.6040227f },
                { 0.4545195f, -0.7660444f,  0.4545195f },
            };

            static const DirectX::XMVECTORF32 defaultDiffuse[MaxDirectionalLights] =
            {
                { 1.0000000f, 0.9607844f, 0.8078432f },
                { 0.9647059f, 0.7607844f, 0.4078432f },
                { 0.3231373f, 0.3607844f, 0.3937255f },
            };

            static const DirectX::XMVECTORF32 defaultSpecular[MaxDirectionalLights] =
            {
                { 1.0000000f, 0.9607844f, 0.8078432f },
                { 0.0000000f, 0.0000000f, 0.0000000f },
                { 0.3231373f, 0.3607844f, 0.3937255f },
            };

            // Default colors
            diffuseColor = DirectX::XMVectorSet(1.f, 1.f, 1.f, 1.0f);
            emissiveColor = DirectX::XMVectorSet(0.05333332f, 0.09882354f, 0.1819608f, 1.f);
            specularColorAndPower = DirectX::XMVectorSet(1.f, 1.f, 1.f, 16);

            for (int i = 0; i < MaxDirectionalLights; i++)
            {
                lightDirection[i] = defaultDirections[i];
                lightDiffuseColor[i] = defaultDiffuse[i];
                lightSpecularColor[i] = defaultSpecular[i];
            }
        }

        void XM_CALLCONV SetWorldViewProj(DirectX::FXMMATRIX w, DirectX::CXMMATRIX view, DirectX::CXMMATRIX proj)
        {
            using namespace DirectX;

            world = XMMatrixTranspose(w);

            XMMATRIX worldInverse = XMMatrixInverse(nullptr, w);
            worldInverseTranspose[0] = worldInverse.r[0];
            worldInverseTranspose[1] = worldInverse.r[1];
            worldInverseTranspose[2] = worldInverse.r[2];

            XMMATRIX viewInverse = XMMatrixInverse(nullptr, view);
            eyePosition = viewInverse.r[3];

            XMMATRIX worldView = XMMatrixMultiply(w, view);
            worldViewProj = XMMatrixTranspose(XMMatrixMultiply(worldView, proj));
        }

    };

    class CustomEffect : public DirectX::IEffect
    {
    public:

        // This custom effect has some hard-coded root signature entries
        enum DescriptorIndex
        {
            Texture,
            DescriptorCount
        };

        enum RootParameterIndex
        {
            TextureSRV,
            ConstantBuffer,
            RootParameterCount
        };

        // Load a PSO from a de-duplicated set, given base path and vertex and pixel shader combination.
        CustomEffect(Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature,
            Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState)
            : m_constants{}
            , m_texture{}
        {
            using namespace Microsoft::WRL;

            m_pipelineState = pipelineState;
            m_rootSignature = rootSignature;
        }

        virtual void __cdecl Apply(_In_ ID3D12GraphicsCommandList* cmdList) override // IEffect
        {
            // Update constants
            m_constantBuffer = DirectX::GraphicsMemory::Get().AllocateConstant(m_constants);

            // Set root signature
            cmdList->SetGraphicsRootSignature(m_rootSignature.Get());
            cmdList->SetGraphicsRootDescriptorTable(RootParameterIndex::TextureSRV, m_texture);
            cmdList->SetGraphicsRootConstantBufferView(RootParameterIndex::ConstantBuffer, m_constantBuffer.GpuAddress());

            // Set pipeline state.
            cmdList->SetPipelineState(m_pipelineState.Get());
        }

        Constants                                       m_constants;
        D3D12_GPU_DESCRIPTOR_HANDLE                     m_texture;

    private:
        DirectX::GraphicsResource                       m_constantBuffer;
        Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineState;
    };
} // namespace ATG
