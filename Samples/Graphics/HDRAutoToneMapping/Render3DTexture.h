//--------------------------------------------------------------------------------------
// Render3DTexture.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class Render3DTexture
{
public:
    void Initialize(_In_ ID3D12Device* d3dDevice);
    void Draw(_In_ ID3D12GraphicsCommandList* d3dCommandList, _In_ ID3D12PipelineState* d3dPSO);
    void ReleaseDevice();

    ID3D12RootSignature* GetRootSignature() const { return m_d3dRootSignature.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_d3dRootSignature;
};
