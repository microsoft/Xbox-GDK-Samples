//--------------------------------------------------------------------------------------
// DrawCullData.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "DeviceResources.h"

namespace ATG
{
    class DrawCullDataEffect
    {
    public:
        void CreateDeviceResources(DX::DeviceResources& deviceResources);
        void ReleaseResources();

        void SetConstants(DirectX::FXMVECTOR color, DX::OrbitCamera& camera, const VQS& world);
        void Draw(ID3D12GraphicsCommandList6* cmdList, MeshletSet& meshlet, uint32_t offset, uint32_t count);

    private:
        struct DrawCullData
        {
            DirectX::XMFLOAT4X4 World;
            DirectX::XMFLOAT4X4 ViewProj;
            DirectX::XMFLOAT4 Color;
            DirectX::XMFLOAT4 ViewUp;
            DirectX::XMFLOAT3 ViewForward;
            float Scale;
            uint32_t MeshletOffset;
            uint32_t MeshletCount;
        };

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_boundingSpherePso;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_normalConePso;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_constantBuffer;

        DrawCullData m_data = {};
    };
}
