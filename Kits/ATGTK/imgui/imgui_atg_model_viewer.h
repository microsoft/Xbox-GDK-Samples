//--------------------------------------------------------------------------------------
// imgui_atg_model_viewer.h
//
// High-level 3D model viewer for ImGui applications.
//
// Wraps RenderTarget + DirectXTK12 to provide a simple API for loading and displaying
// 3D models in ImGui windows. Handles all DirectXTK12 boilerplate: model loading,
// texture loading, effect creation, lighting, and per-frame rendering.
//
// OPTIONAL: This file depends on DirectXTK12 (via vcpkg). Only include it in projects
// that need 3D model display. Samples that are purely 2D/ImGui do not need this.
//
// Usage:
//   // During initialization:
//   ImGuiAtg::ModelViewer viewer;
//   viewer.Initialize(deviceContext, 512, 512);
//   viewer.LoadModel(L"media\\Model\\Controller.sdkmesh", L"media\\Model\\");
//
//   // Each frame:
//   XMMATRIX world = XMMatrixRotationQuaternion(orientation);
//   viewer.Render(world);
//
//   // During ImGui drawing:
//   viewer.DrawImage(width, height);
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "imgui_atg_render_target.h"

#include <SimpleMath.h>
#include <GraphicsMemory.h>
#include <DescriptorHeap.h>
#include <Model.h>
#include <Effects.h>
#include <CommonStates.h>
#include <RenderTargetState.h>
#include <ResourceUploadBatch.h>

namespace ImGuiAtg
{
    class ModelViewer
    {
    public:
        ModelViewer() = default;
        ~ModelViewer() = default;

        ModelViewer(ModelViewer&&) = delete;
        ModelViewer& operator=(ModelViewer&&) = delete;
        ModelViewer(const ModelViewer&) = delete;
        ModelViewer& operator=(const ModelViewer&) = delete;

        // Initialize the rendering pipeline. Must be called before LoadModel.
        void Initialize(DeviceContext* deviceContext, UINT width, UINT height);

        // Load a .sdkmesh model with textures from the given directory.
        // Creates effects with default lighting. Call after Initialize.
        void LoadModel(const wchar_t* sdkmeshPath, const wchar_t* textureDir);

        // Set the camera. Call before Render() if you want a non-default view.
        // Defaults to a LH camera at (0, 0.5, -2) looking at the origin.
        void SetCamera(DirectX::FXMVECTOR eye, DirectX::FXMVECTOR target, DirectX::FXMVECTOR up,
                       float fovY = DirectX::XM_PIDIV4, float nearZ = 0.1f, float farZ = 100.0f);

        // Set the background clear color. Default is dark grey (0.15, 0.15, 0.15).
        void SetClearColor(float r, float g, float b, float a = 1.0f);

        // Render the model with the given world matrix to the offscreen texture.
        // Call once per frame, before ImGui rendering.
        void Render(DirectX::FXMMATRIX world);

        // Display the rendered image as an ImGui widget.
        void DrawImage(float width, float height) const;

        // Access the underlying RenderTarget for advanced use
        RenderTarget&       GetRenderer() { return m_renderer; }
        const RenderTarget& GetRenderer() const { return m_renderer; }

        bool IsInitialized() const { return m_renderer.IsInitialized(); }
        bool IsModelLoaded() const { return m_model != nullptr; }

    private:
        RenderTarget m_renderer;

        std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
        std::unique_ptr<DirectX::CommonStates> m_states;
        std::unique_ptr<DirectX::EffectTextureFactory> m_texFactory;

        // Loaded model
        std::shared_ptr<DirectX::Model> m_model;
        DirectX::Model::EffectCollection m_modelEffects;

        // Camera
        DirectX::XMMATRIX m_view = DirectX::XMMatrixIdentity();
        DirectX::XMMATRIX m_proj = DirectX::XMMatrixIdentity();

        // Clear color
        float m_clearColor[4] = { 0.15f, 0.15f, 0.15f, 1.0f };
    };
}
