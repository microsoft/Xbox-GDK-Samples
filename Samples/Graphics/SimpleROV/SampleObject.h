//--------------------------------------------------------------------------------------
// SampleObject.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class SampleObject
{
public:

    SampleObject() = delete;
    ~SampleObject() = default;

    SampleObject(SampleObject&&) = default;
    SampleObject& operator= (SampleObject&&) = default;

    SampleObject(const SampleObject&) = delete;
    SampleObject& operator=(const SampleObject&) = delete;

    SampleObject(ID3D12Device* device);
    SampleObject(ID3D12Device* device, DirectX::ResourceUploadBatch& upload,
        std::unique_ptr<DirectX::DescriptorPile>& shaderVisiblePile, std::wstring const& modelName);

    // Basic render loop
    void Update();
    void Render(ID3D12GraphicsCommandList* cmdList, std::unique_ptr<DirectX::DescriptorPile> const& shaderVisiblePile);

    void Translate(DirectX::SimpleMath::Vector3 t);
    void SetPosition(DirectX::SimpleMath::Vector3 t);

    void Rotate(DirectX::SimpleMath::Vector3 r);
    void SetRotation(DirectX::SimpleMath::Vector3 r);

    void SetScale(DirectX::SimpleMath::Vector3 h);

    void SetColor(DirectX::SimpleMath::Vector3 color);

    inline DirectX::SimpleMath::Matrix const& GetWorldMatrix() const
    {
        return m_world;
    }

    inline DirectX::SimpleMath::Matrix const& GetNormalMatrix() const
    {
        return m_normalTransform;
    }

    inline DirectX::SimpleMath::Matrix const& GetPreviousFrameWorldMatrix() const
    {
        return m_previousWorld;
    }

    inline DirectX::SimpleMath::Vector3 const& GetPosition() const
    {
        return m_position;
    }

    inline DirectX::SimpleMath::Vector3 const& GetColor() const
    {
        return m_color;
    }

private:
    DirectX::SimpleMath::Matrix     m_world;
    DirectX::SimpleMath::Matrix     m_normalTransform;
    DirectX::SimpleMath::Matrix     m_previousWorld;

    DirectX::SimpleMath::Vector3    m_position;
    DirectX::SimpleMath::Vector3    m_scale;
    DirectX::SimpleMath::Vector3    m_rotation;

    std::unique_ptr<DirectX::GeometricPrimitive> m_mesh;
    std::unique_ptr<DirectX::Model> m_model;
    std::unique_ptr<DirectX::EffectTextureFactory> m_texFactory;

    bool m_needToUpdateMatrices;
    bool m_useModel;
    size_t m_texOffsets;

    DirectX::SimpleMath::Vector3    m_color;
};
