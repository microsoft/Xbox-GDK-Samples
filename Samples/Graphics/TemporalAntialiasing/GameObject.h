//--------------------------------------------------------------------------------------
// GameObject.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class GameObject
{
public:

    GameObject() = delete;
    ~GameObject() = default;

    GameObject(GameObject&&) = default;
    GameObject& operator= (GameObject&&) = default;

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;

    GameObject(ID3D12Device* device);
    GameObject(ID3D12Device* device, DirectX::ResourceUploadBatch& upload, std::wstring const& modelName);

    // Basic render loop
    void Update(float dt);
    void Render(ID3D12GraphicsCommandList* cmdList);

    void Translate(DirectX::SimpleMath::Vector3 t);
    void SetPosition(DirectX::SimpleMath::Vector3 t);

    void Rotate(DirectX::SimpleMath::Vector3 r);
    void SetRotation(DirectX::SimpleMath::Vector3 r);

    void SetScale(DirectX::SimpleMath::Vector3 h);

    void AddControlPoint(DirectX::SimpleMath::Vector3 const& point);

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

    inline bool InMotion()
    {
        return m_inMotion;
    }

    void SetInMotion(bool inMotion);

private:
    void CubicInterpolation(float dt);

private:
    DirectX::SimpleMath::Matrix     m_world;
    DirectX::SimpleMath::Matrix     m_normalTransform;
    DirectX::SimpleMath::Matrix     m_previousWorld;

    DirectX::SimpleMath::Vector3    m_position;
    DirectX::SimpleMath::Vector3    m_scale;
    DirectX::SimpleMath::Vector3    m_rotation;

    std::unique_ptr<DirectX::GeometricPrimitive> m_mesh;
    std::unique_ptr<DirectX::Model> m_model;
    std::unique_ptr<DirectX::DescriptorHeap> m_descriptorHeap;
    std::unique_ptr<DirectX::EffectTextureFactory> m_texFactory;

    bool m_dirtyFlag;
    bool m_useModel;

    // Related to motion between points
    std::vector<DirectX::SimpleMath::Vector3> m_controlPoints;
    uint32_t m_currentPoint;
    float m_elapsedTime;
    bool m_inMotion;
};
