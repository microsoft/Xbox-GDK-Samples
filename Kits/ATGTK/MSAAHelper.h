//--------------------------------------------------------------------------------------
// File: MSAAHelper.h
//
// Helper for managing MSAA render targets
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//-------------------------------------------------------------------------------------

#pragma once

#include <DirectXMath.h>

#include <wrl/client.h>

namespace DX
{
    class MSAAHelper
    {
    public:
        explicit MSAAHelper(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
            unsigned int sampleCount = 4) noexcept(false);

        MSAAHelper(MSAAHelper&&) = default;
        MSAAHelper& operator= (MSAAHelper&&) = default;

        MSAAHelper(MSAAHelper const&) = delete;
        MSAAHelper& operator= (MSAAHelper const&) = delete;

#if defined(__d3d12_h__) || defined(__d3d12_x_h__) || defined(__XBOX_D3D12_X__)

        void SetDevice(_In_ ID3D12Device* device);

        void SizeResources(size_t width, size_t height);

        void ReleaseDevice();

        void Prepare(_In_ ID3D12GraphicsCommandList* commandList,
            D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RESOLVE_SOURCE);

        void Resolve(_In_ ID3D12GraphicsCommandList* commandList, _In_ ID3D12Resource* backBuffer,
            D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_PRESENT);

        void ResolveDepth(_In_ ID3D12GraphicsCommandList* commandList, _In_ ID3D12Resource* depthBuffer,
            D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_DEPTH_WRITE);

        ID3D12Resource* GetMSAARenderTarget() const noexcept { return m_msaaRenderTarget.Get(); }
        ID3D12Resource* GetMSAADepthStencil() const noexcept { return m_msaaDepthStencil.Get(); }

        D3D12_CPU_DESCRIPTOR_HANDLE GetMSAARenderTargetView() const noexcept
        {
            return m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        }
        D3D12_CPU_DESCRIPTOR_HANDLE GetMSAADepthStencilView() const noexcept
        {
            return m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        }

        void SetClearColor(DirectX::FXMVECTOR color)
        {
            DirectX::XMStoreFloat4(reinterpret_cast<DirectX::XMFLOAT4*>(m_clearColor), color);
        }

    private:
        Microsoft::WRL::ComPtr<ID3D12Device>                m_device;
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_msaaRenderTarget;
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_msaaDepthStencil;
        float                                               m_clearColor[4];

        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;

#if defined(_XBOX_ONE) && defined(_TITLE) && (_XDK_VER < 0x3F6803F3 /* XDK Edition 170600 */)
        DXGI_FORMAT                                         m_resolveFormat;
#endif

#elif defined(__d3d11_h__) || defined(__d3d11_x_h__)

        void SetDevice(_In_ ID3D11Device* device);

        void SizeResources(size_t width, size_t height);

        void ReleaseDevice();

#if defined(_XBOX_ONE) && defined(_TITLE)
        void Resolve(_In_ ID3D11DeviceContextX* context, _In_ ID3D11Texture2D* backBuffer);
        void ResolveDepth(_In_ ID3D11DeviceContextX* context, _In_ ID3D11Texture2D* depthBuffer);
#else
        void Resolve(_In_ ID3D11DeviceContext* context, _In_ ID3D11Texture2D* backBuffer);
        void ResolveDepth(_In_ ID3D11DeviceContext* context, _In_ ID3D11Texture2D* depthBuffer);
#endif

        ID3D11Texture2D* GetMSAARenderTarget() const noexcept { return m_msaaRenderTarget.Get(); }
        ID3D11Texture2D* GetMSAADepthStencil() const noexcept { return m_msaaDepthStencil.Get(); }

        ID3D11RenderTargetView* GetMSAARenderTargetView() const noexcept { return m_renderTargetView.Get(); }
        ID3D11DepthStencilView* GetMSAADepthStencilView() const noexcept { return m_depthStencilView.Get(); }

    private:
        Microsoft::WRL::ComPtr<ID3D11Device>                m_device;

        Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_msaaRenderTarget;
        Microsoft::WRL::ComPtr<ID3D11Texture2D>             m_msaaDepthStencil;
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView>      m_renderTargetView;
        Microsoft::WRL::ComPtr<ID3D11DepthStencilView>      m_depthStencilView;

#if defined(_XBOX_ONE) && defined(_TITLE)
        bool                                                m_fastSemantics;
#endif

#else
#   error Please #include <d3d11.h> or <d3d12.h>
#endif

    public:
        DXGI_FORMAT GetBackBufferFormat() const noexcept { return m_backBufferFormat; }
        DXGI_FORMAT GetDepthBufferFormat() const noexcept { return m_depthBufferFormat; }
        unsigned int GetSampleCount() const noexcept { return m_sampleCount; }

        void SetWindow(const RECT& rect);

    private:
        DXGI_FORMAT                                         m_backBufferFormat;
        DXGI_FORMAT                                         m_depthBufferFormat;
        unsigned int                                        m_sampleCount;
        unsigned int                                        m_targetSampleCount;

        size_t                                              m_width;
        size_t                                              m_height;
    };
}
