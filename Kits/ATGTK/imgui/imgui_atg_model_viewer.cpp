//--------------------------------------------------------------------------------------
// imgui_atg_model_viewer.cpp
//
// Implementation of the high-level 3D model viewer for ImGui applications.
// See imgui_atg_model_viewer.h for usage documentation.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "imgui_atg_model_viewer.h"

using namespace DirectX;

namespace ImGuiAtg
{
    void ModelViewer::Initialize(DeviceContext* deviceContext, UINT width, UINT height)
    {
        m_renderer.Initialize(deviceContext, width, height, m_clearColor);

        auto* device = m_renderer.GetDevice();

        m_graphicsMemory = std::make_unique<GraphicsMemory>(device);
        m_states = std::make_unique<CommonStates>(device);

        // Set default camera (LH, looking at origin)
        m_view = XMMatrixLookAtLH(
            XMVectorSet(0.0f, 0.5f, -2.0f, 0.0f),
            XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f),
            XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
        m_proj = XMMatrixPerspectiveFovLH(
            XM_PIDIV4,
            static_cast<float>(width) / static_cast<float>(height),
            0.1f, 100.0f);
    }

    void ModelViewer::LoadModel(const wchar_t* sdkmeshPath, const wchar_t* textureDir)
    {
        if (!m_renderer.IsInitialized())
            return;

        auto* device = m_renderer.GetDevice();
        auto* commandQueue = m_renderer.GetCommandQueue();

        // Load geometry
        m_model = Model::CreateFromSDKMESH(device, sdkmeshPath);

        // Upload static buffers and load textures
        ResourceUploadBatch upload(device);
        upload.Begin();

        m_model->LoadStaticBuffers(device, upload);
        m_texFactory = m_model->LoadTextures(device, upload, textureDir);

        auto finish = upload.End(commandQueue);
        finish.wait();

        // Create effects with default lighting
        RenderTargetState rtState(m_renderer.GetRTFormat(), m_renderer.GetDSFormat());

        EffectPipelineStateDescription opaquePd(
            nullptr,
            CommonStates::Opaque,
            CommonStates::DepthDefault,
            CommonStates::CullNone,
            rtState);

        EffectPipelineStateDescription alphaPd(
            nullptr,
            CommonStates::AlphaBlend,
            CommonStates::DepthRead,
            CommonStates::CullNone,
            rtState);

        // If the model has textures, create effects bound to the texture heap.
        // If not (m_texFactory is null), use the default overload without textures.
        if (m_texFactory)
        {
            m_modelEffects = m_model->CreateEffects(opaquePd, alphaPd, m_texFactory->Heap(), m_states->Heap());
        }
        else
        {
            EffectFactory fxFactory(device);
            m_modelEffects = m_model->CreateEffects(fxFactory, alphaPd, opaquePd);
        }

        for (auto& effect : m_modelEffects)
        {
            auto basic = dynamic_cast<BasicEffect*>(effect.get());
            if (basic)
            {
                basic->EnableDefaultLighting();
            }
        }
    }

    void ModelViewer::SetCamera(FXMVECTOR eye, FXMVECTOR target, FXMVECTOR up,
                                float fovY, float nearZ, float farZ)
    {
        m_view = XMMatrixLookAtLH(eye, target, up);
        m_proj = XMMatrixPerspectiveFovLH(fovY,
            static_cast<float>(m_renderer.GetWidth()) / static_cast<float>(m_renderer.GetHeight()),
            nearZ, farZ);
    }

    void ModelViewer::SetClearColor(float r, float g, float b, float a)
    {
        m_clearColor[0] = r;
        m_clearColor[1] = g;
        m_clearColor[2] = b;
        m_clearColor[3] = a;
    }

    void ModelViewer::Render(FXMMATRIX world)
    {
        if (!m_renderer.IsInitialized() || !m_model)
            return;

        m_renderer.Begin(m_clearColor);
        auto* cmdList = m_renderer.GetCommandList();

        // Set descriptor heaps for model textures and samplers
        if (m_texFactory)
        {
            ID3D12DescriptorHeap* heaps[] = { m_texFactory->Heap(), m_states->Heap() };
            cmdList->SetDescriptorHeaps(static_cast<UINT>(std::size(heaps)), heaps);
        }

        Model::UpdateEffectMatrices(m_modelEffects, world, m_view, m_proj);
        m_model->Draw(cmdList, m_modelEffects.cbegin());

        m_renderer.End();

        m_graphicsMemory->Commit(m_renderer.GetCommandQueue());
    }

    void ModelViewer::DrawImage(float width, float height) const
    {
        m_renderer.DrawImage(width, height);
    }
}
