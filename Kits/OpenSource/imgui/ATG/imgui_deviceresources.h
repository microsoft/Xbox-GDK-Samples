//
// DeviceResources.h - A wrapper for the Direct3D 12/12.X device and swapchain
//

#pragma once

#ifndef _GAMING_XBOX
#include "d3dx12.h"
#endif

namespace DX
{
    // Provides an interface for an application that owns DeviceResources to be notified of the device being lost or created.
    interface IDeviceNotify
    {
        virtual void OnDeviceLost() = 0;
        virtual void OnDeviceRestored() = 0;

    protected:
        ~IDeviceNotify() = default;
    };

    // Controls all the DirectX device resources.
    class DeviceResources
    {
    public:
        static constexpr unsigned int c_AllowTearing         = 0x1;     // PC only
        static constexpr unsigned int c_EnableHDR            = 0x2;
        static constexpr unsigned int c_ReverseDepth         = 0x4;
        static constexpr unsigned int c_GeometryShaders      = 0x8;     // Xbox only
        static constexpr unsigned int c_TessellationShaders  = 0x10;    // Xbox only
        static constexpr unsigned int c_AmplificationShaders = 0x20;
        static constexpr unsigned int c_EnableDXR            = 0x40;
        static constexpr unsigned int c_ColorDcc             = 0x80;    // Xbox only
        static constexpr unsigned int c_DepthTcc             = 0x100;   // Xbox only

        DeviceResources(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
                        DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
                        UINT backBufferCount = 2,
                        unsigned int flags = 0) noexcept(false);
        ~DeviceResources();

        DeviceResources(DeviceResources&&) = default;
        DeviceResources& operator= (DeviceResources&&) = default;

        DeviceResources(DeviceResources const&) = delete;
        DeviceResources& operator= (DeviceResources const&) = delete;

#ifdef _GAMING_XBOX_SCARLETT
        void CreateDeviceResources(D3D12XBOX_CREATE_DEVICE_FLAGS createDeviceFlags = D3D12XBOX_CREATE_DEVICE_FLAG_NONE);
#else
        void CreateDeviceResources();
#endif
        void CreateWindowSizeDependentResources();
        void SetWindow(HWND window, int width, int height) noexcept;
        bool WindowSizeChanged(int width, int height);
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify) noexcept { m_deviceNotify = deviceNotify; }
        void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT,
                     D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET);
#ifdef _GAMING_XBOX
        void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET,
                     _In_opt_ const D3D12XBOX_PRESENT_PARAMETERS* params = nullptr);
#else
        void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
#endif
        void Suspend();
        void Resume();
        void WaitForGpu() noexcept;
#ifdef _GAMING_XBOX
        void WaitForOrigin();
#endif
        void UpdateColorSpace();

        // Direct3D Properties.
        void SetClearColor(_In_reads_(4) const float* rgba) noexcept { memcpy(m_clearColor, rgba, sizeof(m_clearColor)); }

        // Device Accessors.
        RECT GetOutputSize() const noexcept { return m_outputSize; }

        // Direct3D Accessors.
        auto                        GetD3DDevice() const noexcept          { return m_d3dDevice.Get(); }
#ifndef _GAMING_XBOX
        auto                        GetSwapChain() const noexcept          { return m_swapChain.Get(); }
        auto                        GetDXGIFactory() const noexcept        { return m_dxgiFactory.Get(); }
#endif
        HWND                        GetWindow() const noexcept             { return m_window; }
        D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const noexcept { return m_d3dFeatureLevel; }
        ID3D12Resource*             GetRenderTarget() const noexcept       { return m_renderTargets[m_backBufferIndex].Get(); }
        ID3D12Resource*             GetDepthStencil() const noexcept       { return m_depthStencil.Get(); }
        ID3D12CommandQueue*         GetCommandQueue() const noexcept       { return m_commandQueue.Get(); }
        ID3D12CommandAllocator*     GetCommandAllocator() const noexcept   { return m_commandAllocators[m_backBufferIndex].Get(); }
        auto                        GetCommandList() const noexcept        { return m_commandList.Get(); }
        DXGI_FORMAT                 GetBackBufferFormat() const noexcept   { return m_backBufferFormat; }
        DXGI_FORMAT                 GetDepthBufferFormat() const noexcept  { return m_depthBufferFormat; }
        D3D12_VIEWPORT              GetScreenViewport() const noexcept     { return m_screenViewport; }
        D3D12_RECT                  GetScissorRect() const noexcept        { return m_scissorRect; }
        UINT                        GetCurrentFrameIndex() const noexcept  { return m_backBufferIndex; }
        UINT                        GetBackBufferCount() const noexcept    { return m_backBufferCount; }
        DXGI_COLOR_SPACE_TYPE       GetColorSpace() const noexcept         { return m_colorSpace; }
        unsigned int                GetDeviceOptions() const noexcept      { return m_options; }

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const noexcept
        {
            const auto cpuHandle = m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuHandle, static_cast<INT>(m_backBufferIndex), m_rtvDescriptorSize);
        }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const noexcept
        {
            const auto cpuHandle = m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(cpuHandle);
        }

        ID3D12DescriptorHeap* GetSRVHeap() const { return m_srvDescriptorHeap.Get(); }
        D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHeapCPUHandle() const { return m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(); }
        D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHeapGPUHandle() const { return m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(); }

    private:
#ifdef _GAMING_XBOX
        void RegisterFrameEvents();
#else
        void MoveToNextFrame();
        void GetAdapter(IDXGIAdapter1** ppAdapter);
#endif

        static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

        UINT                                                m_backBufferIndex;

        // Direct3D objects.
#ifdef _GAMING_XBOX_SCARLETT
        Microsoft::WRL::ComPtr<ID3D12Device8>               m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>  m_commandList;
#elif defined(_GAMING_XBOX_XBOXONE)
        Microsoft::WRL::ComPtr<ID3D12Device2>               m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
#else
        Microsoft::WRL::ComPtr<ID3D12Device>                m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
#endif
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocators[MAX_BACK_BUFFER_COUNT];

        // Swap chain objects.
#ifndef _GAMING_XBOX
        Microsoft::WRL::ComPtr<IDXGIFactory6>               m_dxgiFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain3>             m_swapChain;
#endif
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargets[MAX_BACK_BUFFER_COUNT];
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_depthStencil;

        // Presentation fence objects.
        Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
#ifdef _GAMING_XBOX
        UINT64                                              m_fenceValue;
#else
        UINT64                                              m_fenceValues[MAX_BACK_BUFFER_COUNT];
#endif
        Microsoft::WRL::Wrappers::Event                     m_fenceEvent;
#ifdef _GAMING_XBOX
        D3D12XBOX_FRAME_PIPELINE_TOKEN                      m_framePipelineToken;
#endif

        // Direct3D rendering objects.
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
        UINT                                                m_rtvDescriptorSize;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_srvDescriptorHeap;
        UINT                                                m_srvDescriptorSize;
        D3D12_VIEWPORT                                      m_screenViewport;
        D3D12_RECT                                          m_scissorRect;

        // Direct3D properties.
        DXGI_FORMAT                                         m_backBufferFormat;
        DXGI_FORMAT                                         m_depthBufferFormat;
        UINT                                                m_backBufferCount;
        float                                               m_clearColor[4];

        // Cached device properties.
        HWND                                                m_window;
        D3D_FEATURE_LEVEL                                   m_d3dFeatureLevel;
#ifndef _GAMING_XBOX
        DWORD                                               m_dxgiFactoryFlags;
#endif
        RECT                                                m_outputSize;

        // HDR Support
        DXGI_COLOR_SPACE_TYPE                               m_colorSpace;

        // DeviceResources options (see flags above)
        unsigned int                                        m_options;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify*                                      m_deviceNotify;
    };
}
