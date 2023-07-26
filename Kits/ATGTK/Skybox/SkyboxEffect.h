//--------------------------------------------------------------------------------------
// SkyboxEffect.h
//
// A sky box effect for DirectX 12.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include <Effects.h>
#include <SimpleMath.h>


namespace DX
{
    class SkyboxEffect : public DirectX::IEffect, public DirectX::IEffectMatrices
    {
    public:
        explicit SkyboxEffect(_In_ ID3D12Device* device, const DirectX::EffectPipelineStateDescription& pipelineStateDesc, bool reverseZ = false);

        SkyboxEffect(SkyboxEffect&&) = delete;
        SkyboxEffect& operator= (SkyboxEffect&&) = delete;

        SkyboxEffect(SkyboxEffect const&) = delete;
        SkyboxEffect& operator= (SkyboxEffect const&) = delete;

        // IEffect methods.
        void __cdecl Apply(_In_ ID3D12GraphicsCommandList* commandList) override;

        // Camera settings.
        void XM_CALLCONV SetWorld(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetView(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetProjection(DirectX::FXMMATRIX value) override;
        void XM_CALLCONV SetMatrices(DirectX::FXMMATRIX world, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection) override;

        // Texture settings.
        void __cdecl SetTexture(_In_ D3D12_GPU_DESCRIPTOR_HANDLE srvDescriptor,
            _In_ D3D12_GPU_DESCRIPTOR_HANDLE samplerDescriptor);

    private:
        enum Descriptors
        {
            InputSRV,
            InputSampler,
            ConstantBuffer,
            Count
        };

        Microsoft::WRL::ComPtr<ID3D12Device>        m_device;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;

        D3D12_GPU_DESCRIPTOR_HANDLE m_texture;
        D3D12_GPU_DESCRIPTOR_HANDLE m_textureSampler;

        uint32_t m_dirtyFlags;

        DirectX::SimpleMath::Matrix m_view;
        DirectX::SimpleMath::Matrix m_proj;
        DirectX::SimpleMath::Matrix m_worldViewProj;

        DirectX::GraphicsResource   m_constantBuffer;

        bool                        m_reverseZ;
    };
}
