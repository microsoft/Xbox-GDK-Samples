//--------------------------------------------------------------------------------------
// RenderManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "RenderManager.h"
#include "RenderContext.h"

#include "DeviceResources.h"

#include "SpriteBatch.h"
#include "CommonStates.h"

namespace
{
    const float c_backgroundColor[4] = { 0, 0, 32.0f / 255.0f, 1.0f };
}

namespace ThunderRumble
{
    RenderManager::RenderManager()
    {
        m_deviceResources = std::make_unique<DX::DeviceResources>();
        m_deviceResources->RegisterDeviceNotify(this);
    }

    RenderManager::~RenderManager()
    {

    }

    std::shared_ptr<RenderContext> RenderManager::GetRenderContext(BlendMode mode)
    {
        ID3D11BlendState *blendstate = nullptr;

        switch (mode)
        {
        case BlendMode::NonPremultiplied:
            blendstate = m_commonStates->NonPremultiplied();
            break;
        case BlendMode::Additive:
            blendstate = m_commonStates->Additive();
            break;
        default:
            break;
        };

        std::shared_ptr<RenderContext> context;
        context.reset(new RenderContext(m_spriteBatch, blendstate));

        return context;

    }

    void RenderManager::Initialize(HWND window, int width, int height)
    {
        m_deviceResources->SetWindow(window, width, height);

        m_deviceResources->CreateDeviceResources();

        m_spriteBatch = std::make_shared<DirectX::SpriteBatch>(m_deviceResources->GetD3DDeviceContext());
        m_commonStates = std::make_shared<DirectX::CommonStates>(m_deviceResources->GetD3DDevice());

        m_deviceResources->CreateWindowSizeDependentResources();
    }

    void RenderManager::OnWindowSizeChanged(int width, int height)
    {
        m_deviceResources->WindowSizeChanged(width, height);
    }

    void RenderManager::Clear()
    {
        m_deviceResources->PIXBeginEvent(L"Clear");

        // Clear the views.
        auto context = m_deviceResources->GetD3DDeviceContext();
        auto renderTarget = m_deviceResources->GetRenderTargetView();
        auto depthStencil = m_deviceResources->GetDepthStencilView();

        context->ClearRenderTargetView(renderTarget, c_backgroundColor);
        context->ClearDepthStencilView(depthStencil, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        context->OMSetRenderTargets(1, &renderTarget, depthStencil);

        // Set the viewport.
        auto viewport = m_deviceResources->GetScreenViewport();
        context->RSSetViewports(1, &viewport);

        m_deviceResources->PIXEndEvent();
    }

    void RenderManager::Present()
    {
        m_deviceResources->Present();
    }

    void RenderManager::OnDeviceLost()
    {
        
    }

    void RenderManager::OnDeviceRestored()
    {
        // TODO: ContentManager dump reload textures and fonts
    }
}