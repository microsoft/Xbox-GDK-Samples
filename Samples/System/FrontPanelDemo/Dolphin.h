//--------------------------------------------------------------------------------------
// Dolphin.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "DeviceResources.h"

class Dolphin
{
public:

    Dolphin();

    void Load(ID3D12Device *device, DirectX::RenderTargetState rtState, unsigned int backbufferCount);

    void Update(float totalTime, float elapsedTime);

    ATG::VS_CONSTANT_BUFFER* MapVSConstants(unsigned int bufferIndex);
    void UnmapAndSetVSConstants(
        ID3D12GraphicsCommandList* commandList, 
        unsigned int bufferIndex, 
        bool wireframe,
        ID3D12DescriptorHeap *Heap, 
        D3D12_GPU_DESCRIPTOR_HANDLE dolphinResourceView, 
        D3D12_GPU_DESCRIPTOR_HANDLE causticResourceView);

    void Render(ID3D12GraphicsCommandList * commandList);

    void Translate(DirectX::XMVECTOR t);
    DirectX::XMMATRIX GetWorld();
    float GetBlendWeight();

private:

    DirectX::SimpleMath::Vector3                    m_translation;
    DirectX::SimpleMath::Matrix                     m_world;
    float                                           m_animationTime;
    float                                           m_blendWeight;

    D3D12_PRIMITIVE_TOPOLOGY                        m_primitiveType;
    unsigned int                                    m_indexCount;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>     m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>     m_pipelineStateWireframe;
    D3D12_VERTEX_BUFFER_VIEW                        m_vertexBufferView[3];
    D3D12_INDEX_BUFFER_VIEW                         m_indexBufferView;

    Microsoft::WRL::ComPtr<ID3D12Resource>          m_VSConstants;
    ATG::VS_CONSTANT_BUFFER*                        m_mappedVSConstantData;
    D3D12_GPU_VIRTUAL_ADDRESS                       m_VSConstantDataGpuAddr;

    std::unique_ptr<DirectX::Model>                 m_dolphinModel1;
    std::unique_ptr<DirectX::Model>                 m_dolphinModel2;
    std::unique_ptr<DirectX::Model>                 m_dolphinModel3;
};
