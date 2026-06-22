//--------------------------------------------------------------------------------------
// SampleObject.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Shaders/SharedDefinitions.h"
#include "SampleObject.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

#define EPSILON     0.000001f

wchar_t const* const s_folderPaths[5] =
{
    L"Media\\Meshes\\AliasSampleCityBlock",
    L"Media\\Meshes\\ATGDragon",
    L"Media\\Meshes\\TankScene",
    L"Media\\Meshes\\Gargoyle",
    nullptr
};

SampleObject::SampleObject(ID3D12Device* device) :
    m_position{},
    m_scale(1.0f),
    m_rotation{},
    m_needToUpdateMatrices(false),
    m_useModel(false),
    m_texOffsets(0),
    m_color(1.0f)
{
    m_mesh = GeometricPrimitive::CreateBox(Vector3(1.0f), true, false, device);
}

SampleObject::SampleObject(ID3D12Device* device, DirectX::ResourceUploadBatch& upload,
    std::unique_ptr<DescriptorPile>& shaderVisiblePile, std::wstring const& modelName) :
    m_position{},
    m_scale(1.0f),
    m_rotation{},
    m_needToUpdateMatrices(false),
    m_useModel(true),
    m_texOffsets(0),
    m_color(1.0f)
{
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

    m_texFactory = std::make_unique<EffectTextureFactory>(device, upload, shaderVisiblePile->Heap());
    m_texFactory->SetDirectory(mediaDirectory.c_str());

    // Load the model's textures (for scene).
    size_t end;
    if (m_model->textureNames.size() > 0)
    {
        shaderVisiblePile->AllocateRange(m_model->textureNames.size(), m_texOffsets, end);
        m_texFactory->EnableForceSRGB(true);
        m_model->LoadTextures(*m_texFactory, INT(m_texOffsets));
    }
}

void SampleObject::Update()
{
    if (m_needToUpdateMatrices)
    {
        // These are row-major matrices.

        // Scale.
        auto H = Matrix::CreateScale(m_scale);

        // Rotation.
        auto R = Matrix::CreateRotationX(m_rotation.x) *
            Matrix::CreateRotationY(m_rotation.y) *
            Matrix::CreateRotationZ(m_rotation.z);

        // Translation.
        auto T = Matrix::CreateTranslation(m_position);

        m_world = H * R * T;
        m_normalTransform = H * R;

        m_needToUpdateMatrices = false;
    }
}

void SampleObject::Render(ID3D12GraphicsCommandList* cmdList, std::unique_ptr<DirectX::DescriptorPile> const& shaderVisiblePile)
{
    PIXBeginEvent(cmdList, PIX_COLOR_DEFAULT, L"Render Model");
    if (m_useModel)
    {
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
                    cmdList->SetGraphicsRootDescriptorTable(/*ROOT_INDEX*/3,
                        shaderVisiblePile->GetGpuHandle((uint32_t)m_texOffsets + textureIndex));
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
    PIXEndEvent(cmdList);
}

void SampleObject::Translate(Vector3 t)
{
    if (t.x == 0.0f && t.y == 0.0f && t.z == 0.0f)
    {
        return;
    }

    m_needToUpdateMatrices = true;
    m_position = m_position + t;
}

void SampleObject::SetPosition(Vector3 pos)
{
    Vector3 delta = pos - m_position;
    if (std::abs(delta.x) < EPSILON && std::abs(delta.y) < EPSILON && std::abs(delta.z) < EPSILON)
    {
        return;
    }

    m_needToUpdateMatrices = true;
    m_position = pos;
}

void SampleObject::Rotate(Vector3 r)
{
    if (r.x == 0.0f && r.y == 0.0f && r.z == 0.0f)
    {
        return;
    }

    m_needToUpdateMatrices = true;
    m_rotation = m_rotation + r;
}

void SampleObject::SetRotation(Vector3 r)
{
    Vector3 delta = r - m_rotation;
    if (std::abs(delta.x) < EPSILON && std::abs(delta.y) < EPSILON && std::abs(delta.z) < EPSILON)
    {
        return;
    }

    m_needToUpdateMatrices = true;
    m_rotation = r;
}

void SampleObject::SetScale(Vector3 h)
{
    Vector3 delta = h - m_scale;
    if (std::abs(delta.x) < EPSILON && std::abs(delta.y) < EPSILON && std::abs(delta.z) < EPSILON)
    {
        return;
    }

    m_needToUpdateMatrices = true;
    m_scale = h;
}

void SampleObject::SetColor(Vector3 color)
{
    m_color = color;
}
