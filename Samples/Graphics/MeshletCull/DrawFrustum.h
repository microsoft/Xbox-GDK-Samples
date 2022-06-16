//--------------------------------------------------------------------------------------
// DrawFrustum.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "DeviceResources.h"

namespace ATG
{
    class DrawFrustumEffect
    {
    public:
        void CreateDeviceResources(DX::DeviceResources& deviceResources);
        void ReleaseResources();

        void Update(DirectX::FXMMATRIX viewProj, DirectX::XMVECTOR (&planes)[6]);
        void Draw(ID3D12GraphicsCommandList6* cmdList);

    private:
        struct DrawFrustumData
        {
            DirectX::XMFLOAT4X4 ViewProj;
            DirectX::XMFLOAT4   Planes[6];
            DirectX::XMFLOAT4   LineColor;
        };

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;
        Microsoft::WRL::ComPtr<ID3D12Resource>      m_constantBuffer;

        DrawFrustumData m_data = {};
    };
}

