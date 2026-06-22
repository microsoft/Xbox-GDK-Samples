//-----------------------------------------------------------------------------
// Scene.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#include <GeometricPrimitive.h>
#include "CustomEffect.h"

namespace ATG
{
    // A set of geometric shapes to render shaders
    struct Scene
    {
        static constexpr size_t                         c_width = 3;
        static constexpr size_t                         c_numShapes = c_width * c_width;
        std::unique_ptr<ATG::CustomEffect>              m_effects[c_numShapes];
        std::unique_ptr<DirectX::GeometricPrimitive>    m_mesh;

        DirectX::XMMATRIX                               m_view{}, m_projection{};
        float                                           m_angle = 0.0f;

        void InitializeViewProj(
            DirectX::FXMMATRIX view,
            DirectX::CXMMATRIX projection)
        {
            m_view = view;
            m_projection = projection;
            m_angle = 0;
        }

        void XM_CALLCONV Update(float elapsedSeconds, DirectX::FXMVECTOR basePosition)
        {
            using namespace DirectX;

            // Rotate shapes
            m_angle += elapsedSeconds * 0.9f;

            for (size_t i = 0; i < c_width; i++)
            {
                for (size_t j = 0; j < c_width; j++)
                {
                    auto pos = XMVectorAdd(basePosition, XMVectorSet(j * 20.f, i * 20.f, 0.f, 0.f));

                    auto& be = m_effects[i * c_width + j];
                    be->m_constants.SetWorldViewProj(XMMatrixMultiply(XMMatrixRotationY(m_angle), XMMatrixTranslationFromVector(pos)),
                        m_view, m_projection);

                    be->m_constants.globalTime += elapsedSeconds;
                }
            }
        }

        void Draw(ID3D12GraphicsCommandList* commandList)
        {
            for (size_t i = 0; i < Scene::c_numShapes; i++)
            {
                m_effects[i]->Apply(commandList);
                m_mesh->Draw(commandList);
            }
        }
    };
}
