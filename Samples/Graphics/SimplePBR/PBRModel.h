//--------------------------------------------------------------------------------------
// PBRModel.h
//
// A wrapper for SDKMesh models that use PBR materials.
//
// This implies the follwing folder structure and naming convention for source assets:
//
// modelPath\modelName.sdkmesh
// modelPath\materalName\modelname_BaseColor.png
// modelPath\materalName\modelname_Normal.png
// modelPath\materalName\modelname_Roughness.png
// modelPath\materalName\modelname_Metallic.png
// 
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include <Model.h>

namespace ATG
{
    class PBRModel
    {
    public:
        PBRModel(const wchar_t* modelPath)
            : m_modelFullPath(modelPath)
        {
            // Remove extension and path
            auto lastSlash = m_modelFullPath.find_last_of(L"\\");
            if (lastSlash != size_t(-1))
                m_modelBasePath = m_modelFullPath.substr(0, lastSlash);
            else
                m_modelBasePath = L".";

            auto lastDot = m_modelFullPath.find_last_of(L".");
            m_modelName = m_modelFullPath.substr( lastSlash + 1, lastDot - lastSlash - 1);
        }

        void Create(
            _In_ ID3D12Device* device,
            const DirectX::RenderTargetState& rtState,
            const DirectX::CommonStates* commonStates,
            DirectX::ResourceUploadBatch& resourceUpload,
            _In_ DirectX::DescriptorPile* pile)
        {
            using namespace DirectX;
            using namespace DirectX::SimpleMath;

            // Generate paths for resources
            enum Textures
            {
                Albedo = 0,
                Normal,
                RMA,
                MaxTextures
            };

            wchar_t fullTexturePath[MaxTextures][_MAX_PATH] = {};

            swprintf_s(fullTexturePath[Albedo], L"%s\\%s_BaseColor.dds", m_modelBasePath.c_str(),  m_modelName.c_str());
            swprintf_s(fullTexturePath[Normal], L"%s\\%s_Normal.dds",    m_modelBasePath.c_str(),  m_modelName.c_str());
            swprintf_s(fullTexturePath[RMA], L"%s\\%s_RMA.dds",          m_modelBasePath.c_str(),  m_modelName.c_str());
         
            // PBR Model
            m_model = Model::CreateFromSDKMESH(device, m_modelFullPath.c_str());

            // Optimize model for rendering
            m_model->LoadStaticBuffers(device, resourceUpload);

            // PBR Textures
            // Force SRGB on albedo texture
            DX::ThrowIfFailed(
                CreateDDSTextureFromFileEx(device, resourceUpload, fullTexturePath[Albedo], 0,
                    D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_FORCE_SRGB,
                    m_textureResources[Albedo].ReleaseAndGetAddressOf()));

            // Reload others as linear
            for (size_t i = Normal; i < MaxTextures; i++)
            {
                DX::ThrowIfFailed(CreateDDSTextureFromFile(device, resourceUpload, fullTexturePath[i],
                    m_textureResources[i].ReleaseAndGetAddressOf()));
            }

            // Allocate a range of descriptors from pile
            DescriptorPile::IndexType start, end;
            pile->AllocateRange(MaxTextures, start, end);

            for (size_t i = 0; i < MaxTextures; i++)
            {
                CreateShaderResourceView(device, m_textureResources[i].Get(), pile->GetCpuHandle(start + i));
            }

            // Create PBR Effect
            EffectPipelineStateDescription pbrEffectPipelineState(
                &GeometricPrimitive::VertexType::InputLayout,
                CommonStates::Opaque,
                CommonStates::DepthReverseZ,
                CommonStates::CullClockwise,
                rtState,
                D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
            m_effect = std::make_unique<DirectX::PBREffect>(device, EffectFlags::Texture, pbrEffectPipelineState);

             // Set surface textures
            m_effect->SetSurfaceTextures(pile->GetGpuHandle(start + Albedo),
                                         pile->GetGpuHandle(start + Normal),
                                         pile->GetGpuHandle(start + RMA),
                                         commonStates->AnisotropicClamp());
        }

        const DirectX::Model* GetModel() const { return m_model.get(); }
        DirectX::PBREffect* GetEffect() const { return m_effect.get(); }

    private:
        std::wstring                            m_modelFullPath;
        std::wstring                            m_modelBasePath;
        std::wstring                            m_modelName;

        std::unique_ptr<DirectX::Model>         m_model;
        std::unique_ptr<DirectX::PBREffect>     m_effect;
        Microsoft::WRL::ComPtr<ID3D12Resource>  m_textureResources[4];
    };
}
