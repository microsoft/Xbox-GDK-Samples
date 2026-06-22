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
    DXGI_FORMAT indexFormat{};
    uint32_t indexCount{0};
    uint32_t vertexStride{0};
    uint32_t vertexCount{0};
};

struct ModelData
{
    std::vector<MeshBuffers> meshBuffers;
    uint32_t m_meshInfoOffset{0};
    Microsoft::WRL::ComPtr<ID3D12Resource> m_BLAS;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_BLASScratch;
};

struct ModelInstance
{
    int modelIndex{-1};
    DirectX::SimpleMath::Matrix world;
    bool renderable{false};
};

struct Scene
{
    std::vector<std::unique_ptr<ModelData>> m_uniqueModels;
    std::vector<std::unique_ptr<ModelInstance>> m_instances;
};

enum class MaterialType
{
    LitShadowCasting,
    UnlitNonShadowCasting
};

struct SceneInstance
{
    uint32_t contentIndex;
    DirectX::SimpleMath::Matrix transform;
    MaterialType materialType;
};

struct SceneCollection
{
    std::vector<std::unique_ptr<SceneInstance>> m_scenes;
};
