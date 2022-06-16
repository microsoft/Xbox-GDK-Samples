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
        DrawFrustumEffect();

        void CreateDeviceResources(DX::DeviceResources& deviceResources, DirectX::CommonStates& commonStates);
        void ReleaseResources();

        void Update(DirectX::FXMMATRIX viewProj, DirectX::CXMMATRIX debugViewProj);

        void Draw(ID3D12GraphicsCommandList6* cmdList);

    private:
        struct DrawFrustumData
        {
            DirectX::XMFLOAT4X4 ViewProj;
            DirectX::XMFLOAT4   Planes[6];
            DirectX::XMFLOAT4   LineColor;
            float               Thickness;
            uint32_t            pad[3];
        };

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso;

        Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;

        DrawFrustumData m_data;
    };
}

