//--------------------------------------------------------------------------------------
// GameObject.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

// Time (seconds) spent between two control points when moving.
#define TIME_PER_PIECE 1.0f

wchar_t const* const s_folderPaths[4] =
{
    L"Media\\Meshes\\FPSRoom",
    L"Media\\Meshes\\Cactus",
    L"Media\\Meshes\\Dragon",
    nullptr
};

GameObject::GameObject(ID3D12Device* device) :
    m_position{},
    m_scale(1.0f),
    m_rotation{},
    m_dirtyFlag(false),
    m_useModel(false),
    m_currentPoint(0),
    m_elapsedTime(0.0f),
    m_inMotion(false)
{
    m_mesh = GeometricPrimitive::CreateIcosahedron(15.0f, false, device);
}

GameObject::GameObject(ID3D12Device* device, DirectX::ResourceUploadBatch& upload, std::wstring const& modelName) :
    m_position{},
    m_scale(1.0f),
    m_rotation{},
    m_dirtyFlag(false),
    m_useModel(true),
    m_currentPoint(0),
    m_elapsedTime(0.0f),
    m_inMotion(false)
{
    m_descriptorHeap = std::make_unique<DirectX::DescriptorHeap>(device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 16u);
    m_descriptorHeap->Heap()->SetName(modelName.c_str());

    wchar_t meshFilename[1024] = {};
    wchar_t filepath[1024] = {};
    _snwprintf_s(meshFilename, std::size(meshFilename), _TRUNCATE, modelName.c_str());

    DX::FindMediaFile(filepath, static_cast<int>(std::size(filepath)), meshFilename, s_folderPaths);

    // Store the media directory.
    std::wstring pathTemp = filepath;
    std::wstring mediaDirectory = pathTemp.substr(0, pathTemp.find_last_of('\\'));

    auto modelBLOB = DX::ReadData(filepath);
    m_model = Model::CreateFromSDKMESH(device, modelBLOB.data(), modelBLOB.size());

    m_model->LoadStaticBuffers(device, upload);

    m_texFactory = std::make_unique<EffectTextureFactory>(device, upload, m_descriptorHeap->Heap());
    m_texFactory->SetDirectory(mediaDirectory.c_str());

    // Load the model's textures (for scene).
    m_texFactory->EnableForceSRGB(true);
    m_model->LoadTextures(*m_texFactory);
}

// Basic render loop
void GameObject::Update(float dt)
{
    // If in motion, use cubic interpolation to move.
    if (m_inMotion)
    {
        CubicInterpolation(dt);
    }

    if (m_dirtyFlag)
    {
        // These are row-major matrices.

        // Scale.
        auto H = Matrix::CreateScale(m_scale);

        // Rotation.
        auto R = Matrix::CreateRotationX(m_rotation.x) *
            Matrix::CreateRotationX(m_rotation.y) *
            Matrix::CreateRotationX(m_rotation.z);

        // Translation.
        auto T = Matrix::CreateTranslation(m_position);

        m_world = H * R * T;
        m_normalTransform = H * R;

        m_dirtyFlag = false;
    }
}

void GameObject::Render(ID3D12GraphicsCommandList* cmdList)
{
    if (m_useModel)
    {
        // Set descriptor heap for this model.
        ID3D12DescriptorHeap* descriptorHeaps[] = { m_descriptorHeap->Heap() };
        cmdList->SetDescriptorHeaps(1, descriptorHeaps);

        // Drawing all submeshes.
        for (size_t i = 0; i < m_model->meshes.size(); ++i)
        {
            auto& opaqueParts = m_model->meshes[i]->opaqueMeshParts;
            for (size_t j = 0; j < opaqueParts.size(); ++j)
            {
                if (m_model->textureNames.size() > 0)
                {
                    auto material = m_model->materials[opaqueParts[j]->materialIndex];
                    uint32_t textureIndex = (material.diffuseTextureIndex != -1) ? material.diffuseTextureIndex : 0u;
                    cmdList->SetGraphicsRootDescriptorTable(/*ROOT_INDEX*/2, m_descriptorHeap->GetGpuHandle(textureIndex));
                }

                opaqueParts[j]->Draw(cmdList);
            }
        }
    }
    else // Use mesh.
    {
        m_mesh->Draw(cmdList);
    }

    m_previousWorld = m_world;
}

void GameObject::Translate(Vector3 t)
{
    if (t.x == 0.0f && t.y == 0.0f && t.z == 0.0f)
    {
        return;
    }

    m_dirtyFlag = true;
    m_position = m_position + t;
}

void GameObject::SetPosition(Vector3 pos)
{
    m_dirtyFlag = true;
    m_position = pos;
}

void GameObject::Rotate(Vector3 r)
{
    if (r.x == 0.0f && r.y == 0.0f && r.z == 0.0f)
    {
        return;
    }

    m_dirtyFlag = true;
    m_rotation = m_rotation + r;
}

void GameObject::SetRotation(Vector3 r)
{
    m_dirtyFlag = true;
    m_rotation = r;
}

void GameObject::SetScale(Vector3 h)
{
    m_dirtyFlag = true;
    m_scale = h;
}

void GameObject::SetInMotion(bool inMotion)
{
    m_inMotion = inMotion;
}

void GameObject::AddControlPoint(Vector3 const& point)
{
    m_controlPoints.push_back(point);
}

void GameObject::CubicInterpolation(float dt)
{
    // Requisite for moving objects to have at least 4 points.
    if (m_controlPoints.size() < 4)
    {
        return;
    }

    m_elapsedTime += dt;

    uint32_t const previousPoint = (m_currentPoint == 0) ? static_cast<uint32_t>(m_controlPoints.size()) - 1 : m_currentPoint - 1;
    uint32_t const nextPoint = (m_currentPoint == static_cast<uint32_t>(m_controlPoints.size()) - 1) ? 0 : m_currentPoint + 1;
    uint32_t const nextToNextPoint = (nextPoint == static_cast<uint32_t>(m_controlPoints.size()) - 1) ? 0 : nextPoint + 1;

    Vector3 p0 = m_controlPoints[previousPoint];
    Vector3 p1 = m_controlPoints[m_currentPoint];
    Vector3 p2 = m_controlPoints[nextPoint];
    Vector3 p3 = m_controlPoints[nextToNextPoint];

    float t = std::min(m_elapsedTime / TIME_PER_PIECE, 1.0f);

    if (t >= 0.99f)
    {
        Vector3 newPosition = p2;
        SetPosition(newPosition);

        m_elapsedTime = 0.0f;
        m_currentPoint = (m_currentPoint == m_controlPoints.size() - 1) ? 0 : m_currentPoint + 1;
    }
    else
    {
        Vector3 newPosition = Vector3::CatmullRom(p0, p1, p2, p3, t);
        SetPosition(newPosition);
    }
}
