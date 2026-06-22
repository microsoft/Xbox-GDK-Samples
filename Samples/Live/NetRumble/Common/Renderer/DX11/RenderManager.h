//--------------------------------------------------------------------------------------
// RenderManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "Manager.h"
#include "DeviceResources.h"

#include "RenderContext.h"

#include "CommonStates.h"
#include "SpriteBatch.h"


namespace ThunderRumble
{
    enum class BlendMode
    {
        Default,
        NonPremultiplied,
        Additive
    };

    class RenderManager : public Manager, public DX::IDeviceNotify
    {
    public:
        RenderManager();
        ~RenderManager();

        // IDeviceNotify
        void OnDeviceLost() override;
        void OnDeviceRestored() override;

        void Initialize(HWND window, int width, int height);
        void OnWindowSizeChanged(int width, int height);

        void Clear();
        void Present();


        std::shared_ptr<RenderContext> GetRenderContext(BlendMode mode = BlendMode::Default);

        ID3D11Device1* GetD3DDevice() const { return m_deviceResources->GetD3DDevice(); }

    private:
        std::unique_ptr<DX::DeviceResources>    m_deviceResources;

        std::shared_ptr<DirectX::SpriteBatch>   m_spriteBatch;
        std::shared_ptr<DirectX::CommonStates>  m_commonStates;
    };
}
