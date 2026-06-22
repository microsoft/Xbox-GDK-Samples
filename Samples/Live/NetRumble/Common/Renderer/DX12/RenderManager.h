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
#include "DescriptorHeap.h"
#include "SpriteBatch.h"


namespace NetRumble
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
        RenderManager() noexcept;
        ~RenderManager() noexcept;

        void OnDeviceLost() override;
        void OnDeviceRestored() override;

        void Initialize(HWND window, int width, int height);
        bool OnWindowSizeChanged(int width, int height);

        void Clear(ID3D12DescriptorHeap* descriptorHeap);
        void Present();
        void Suspend();
        void Resume();

        std::unique_ptr<RenderContext> GetRenderContext(BlendMode mode = BlendMode::Default) const;

        //ID3D12Device1* GetD3DDevice() const { return m_deviceResources->GetD3DDevice(); }

        std::shared_ptr<DX::Texture> LoadTexture(const wchar_t *path, D3D12_CPU_DESCRIPTOR_HANDLE srvDescriptor);
        std::shared_ptr<DirectX::SpriteFont> LoadFont(const wchar_t *path, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor, D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptor);
        std::shared_ptr<DirectX::DescriptorPile> CreateDescriptorPile(size_t descriptorCount);

        void CreateDeviceDependentResources();
        void CreateWindowSizeDependentResources();

    private:
        int m_windowWidth{ 0 };
        int m_windowHeight{ 0 };

        std::unique_ptr<DX::DeviceResources>        m_deviceResources;
        std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

        std::shared_ptr<DirectX::SpriteBatch>   m_defaultSpriteBatch;
        std::shared_ptr<DirectX::SpriteBatch>   m_nonPreMultipliedspriteBatch;
        std::shared_ptr<DirectX::SpriteBatch>   m_additiveSpriteBatch;
    };
}
