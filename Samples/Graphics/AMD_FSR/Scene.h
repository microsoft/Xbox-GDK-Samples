//--------------------------------------------------------------------------------------
// Scene.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

struct MeshBuffers
{
    Microsoft::WRL::ComPtr<ID3D12Resource> vb;
    Microsoft::WRL::ComPtr<ID3D12Resource> ib;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView = {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
    uint32_t indexCount = 0;
    uint32_t materialIndex = 0;
};

struct MaterialRenderData
{
    uint32_t albedoDescriptor = UINT32_MAX;
    uint32_t normalDescriptor = UINT32_MAX;
    uint32_t metallicRoughnessDescriptor = UINT32_MAX;
    DirectX::XMFLOAT4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    bool alphaBlend = false;
};

struct ModelData
{
    std::vector<MeshBuffers> meshBuffers;

    // Object-space bounding box merged across all primitives of this model, used for
    // per-instance frustum culling. Computed once at load time from vertex positions.
    DirectX::BoundingBox localBounds = DirectX::BoundingBox(DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3(0, 0, 0));
    bool hasLocalBounds = false;
};

struct ModelInstance
{
    int modelIndex = 0;
    DirectX::SimpleMath::Matrix world;
};

struct Scene
{
    std::vector<std::unique_ptr<ModelData>> m_uniqueModels;
    std::vector<std::unique_ptr<ModelInstance>> m_instances;
    std::vector<MaterialRenderData> m_materials;
};
