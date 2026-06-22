//--------------------------------------------------------------------------------------
// Scene.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// Scene representation
struct sceneObject
{
    std::unique_ptr<DirectX::Model> Model;
    Microsoft::WRL::ComPtr<ID3D12Resource>  ConstantBuffer;
    std::unique_ptr<DirectX::DescriptorPile> m_srvPile;
};

struct Scene
{
    Scene(const wchar_t* wstrDisplayName);
    virtual ~Scene() = default;

    virtual void Render(ID3D12GraphicsCommandList* commandList, std::unique_ptr<DirectX::DescriptorPile>& srvPile) = 0;

    wchar_t         m_wstrDisplayName[1024];
    sceneObject     m_obj;
    std::wstring    m_mediaDirectory;
};

struct BakedScene : public Scene
{
	BakedScene(Microsoft::WRL::ComPtr<ID3D12Device> pDev, const wchar_t* wstrDisplayName, const wchar_t* wstrMeshName);
    virtual ~BakedScene() = default;

	virtual void Render(ID3D12GraphicsCommandList* commandList, std::unique_ptr<DirectX::DescriptorPile>& srvPile) override;
};
