//
// DeviceResources.h - A wrapper for the Direct3D 12/12.X device and swapchain
//
// Modified to support HDR on Xbox.
//

#pragma once

#include <FidelityFX/host/ffx_types.h>

#ifdef _GAMING_DESKTOP
#include <ffx_api/ffx_api.hpp>
#include <ffx_api/ffx_upscale.hpp>
#include <ffx_api/ffx_framegeneration.hpp>
#else
#include <FidelityFX/host/backends/gdk/ffx_gdk.h>
#endif // #ifdef _GAMING_DESKTOP

namespace
{
    inline DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt) noexcept
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
        default:                                return fmt;
        }
    }
}

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
        static constexpr unsigned int c_EnableHDR            = 0x1;
        static constexpr unsigned int c_ReverseDepth         = 0x2;
        static constexpr unsigned int c_AmplificationShaders = 0x4;

        enum PresentUiCompositionMode
        {
            kNoUiBackBuffer = 0,
            kUseUiBackBuffer = 1,
        };

        DeviceResources(DXGI_FORMAT frameBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
                        DXGI_FORMAT uiBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
                        DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
                        UINT backBufferCount = 2,
                        unsigned int flags = 0) noexcept(false);
        ~DeviceResources();

        DeviceResources(DeviceResources&&) = default;
        DeviceResources& operator= (DeviceResources&&) = default;

        DeviceResources(DeviceResources const&) = delete;
        DeviceResources& operator= (DeviceResources const&) = delete;

        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();
        void SetWindow(HWND window, int width, int height) noexcept;
        bool WindowSizeChanged(int width, int height);
        void UpdatePrimaryOutputHighRefreshRateSupported();
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify) noexcept { m_deviceNotify = deviceNotify; }
        void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT,
                     D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void Present(PresentUiCompositionMode uiCompositionMode, D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void Suspend();
        void Resume();
        void WaitForGpu() noexcept;
#ifdef _GAMING_XBOX
        static void PresentXCallback(struct FfxPresentXParams *);
        void WaitForOrigin();
#endif

        bool SupportsHighRereshRate() const { return m_hasHighRefreshRate; }

        // Device Accessors.
        RECT GetOutputSize() const noexcept { return m_outputSize; }

        // Direct3D Accessors.
        auto                        GetD3DDevice() const noexcept          { return m_d3dDevice.Get(); }
#ifdef _GAMING_DESKTOP
        auto                        GetSwapChain() const noexcept          { return m_swapChain.Get(); }
        auto                        GetDXGIFactory() const noexcept        { return m_dxgiFactory.Get(); }
#endif
        // FidelityFX Super Resolution 3 swap chain related
#ifdef _GAMING_XBOX
        auto                        GetFFXSwapChain() const noexcept	   { return m_ffxSwapChain; }
#else // _GAMIND_DESKTOP
        ffx::Context&               GetFFXSwapChainContext() noexcept   { return m_swapChainContext; }
        void                        DestroySwapChain() noexcept;
#endif // _GAMING_XBOX

        HWND                        GetWindow() const noexcept             { return m_window; }
        D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const noexcept { return m_d3dFeatureLevel; }
        ID3D12Resource*             GetFrameRenderTarget() const noexcept  { return m_frameRenderTargets[m_backBufferIndex].Get(); }
#ifdef _GAMING_XBOX
        ID3D12Resource*             GetInterpolatedRenderTarget() const noexcept { return m_interpolatedRenderTargets[m_backBufferIndex].Get(); }
#endif
        ID3D12Resource*             GetDepthStencil() const noexcept       { return m_depthStencil.Get(); }
        ID3D12CommandQueue*         GetCommandQueue() const noexcept       { return m_commandQueue.Get(); }
        ID3D12CommandAllocator*     GetCommandAllocator() const noexcept   { return m_commandAllocators[m_backBufferIndex].Get(); }
        auto                        GetCommandList() const noexcept        { return m_commandList.Get(); }
        DXGI_FORMAT                 GetFrameBackBufferFormat() const noexcept   { return m_frameBackBufferFormat; }
        DXGI_FORMAT                 GetUIBackBufferFormat() const noexcept { return m_uiBackBufferFormat; }
        DXGI_FORMAT                 GetDepthBufferFormat() const noexcept  { return m_depthBufferFormat; }
        D3D12_VIEWPORT              GetScreenViewport() const noexcept     { return m_screenViewport; }
        D3D12_RECT                  GetScissorRect() const noexcept        { return m_scissorRect; }
        UINT                        GetCurrentFrameIndex() const noexcept  { return m_backBufferIndex; }
        UINT                        GetBackBufferCount() const noexcept    { return m_backBufferCount; }
        unsigned int                GetDeviceOptions() const noexcept      { return m_options; }

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const noexcept
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(m_backBufferIndex), m_rtvDescriptorSize);
        }
        CD3DX12_CPU_DESCRIPTOR_HANDLE FindFrameRenderTargetView(ID3D12Resource *frameRenderTarget) const noexcept
        {
            for (UINT i = 0; i < m_backBufferCount; ++i)
            {
                if (frameRenderTarget == m_frameRenderTargets[i].Get())
                {
                    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                        m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                        static_cast<INT>(i), m_rtvDescriptorSize);
                }
            }
            return {};
        }

#ifdef _GAMING_XBOX

        CD3DX12_CPU_DESCRIPTOR_HANDLE FindInterpolatedRenderTargetView(ID3D12Resource* interpolatedRenderTarget) const noexcept
        {
            for (UINT i = 0; i < m_backBufferCount; ++i)
            {
                if (interpolatedRenderTarget == m_interpolatedRenderTargets[i].Get())
                {
                    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                        m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                        static_cast<INT>(i + 1 * m_backBufferCount), m_rtvDescriptorSize);
                }
            }
            return {};
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetUIRenderTargetView() const noexcept
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(m_backBufferIndex + 2 * m_backBufferCount), m_rtvDescriptorSize);
        }
#endif
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const noexcept
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        }

#ifdef _GAMING_XBOX
        void RegisterFrameEvents(bool frameInterpolationEnabled);
    private:
#else
    private:
        void MoveToNextFrame();
        void GetAdapter(IDXGIAdapter1** ppAdapter);
#endif

        static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

        UINT                                                m_backBufferIndex;

        // Direct3D objects.
#ifdef _GAMING_DESKTOP
        Microsoft::WRL::ComPtr<IDXGIAdapter1>               m_adapter;
#endif // _GAMING_DESKTOP
        Microsoft::WRL::ComPtr<ID3D12Device1>               m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
#ifdef _GAMING_XBOX
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_asyncComputeQueue;
#endif // _GAMING_XBOX
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocators[MAX_BACK_BUFFER_COUNT];

        // FidelityFX Super Resolution 3 swap chain object
        // - Overrides the swapchain for Desktop
        // - Overrides the command queue for Scarlett
#ifdef _GAMING_XBOX
        FfxSwapchain                                        m_ffxSwapChain;
#else // _GAMING_DESKTOP
        ffx::Context                                        m_swapChainContext;
#endif // _GAMING_XBOX

        // Swap chain objects.
#ifdef _GAMING_DESKTOP
        Microsoft::WRL::ComPtr<IDXGIFactory6>               m_dxgiFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain4>             m_swapChain;
#endif

        Microsoft::WRL::ComPtr<ID3D12Resource>              m_frameRenderTargets[MAX_BACK_BUFFER_COUNT];
#ifdef _GAMING_XBOX
        // On Xbox we pre-allocate back buffers for Interpolated frame by FidelityFX Super Resolution 3
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_interpolatedRenderTargets[MAX_BACK_BUFFER_COUNT];

        // On Xbox we pre-allocate back buffers for UI to allow composition directly in Present
        // NOTE: depending on FidelityFX Super Resolution 3 UI composition mode, they could be unused
        //       current they are used only for Texture Composition
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_uiRenderTargets[MAX_BACK_BUFFER_COUNT];
#endif
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_depthStencil;

        // Presentation fence objects.
        Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
#ifdef _GAMING_XBOX
        Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fenceAsync;
#endif
        UINT64                                              m_fenceValue;
        Microsoft::WRL::Wrappers::Event                     m_fenceEvent;
#ifdef _GAMING_XBOX
        D3D12XBOX_FRAME_PIPELINE_TOKEN                      m_framePipelineToken;
#endif
        bool                                                m_hasHighRefreshRate; // Support for 120 HZ?
        //bool                                                m_frameInterpolationEnabled = true;

        // Direct3D rendering objects.
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
        UINT                                                m_rtvDescriptorSize;
        D3D12_VIEWPORT                                      m_screenViewport;
        D3D12_RECT                                          m_scissorRect;

        // Direct3D properties.
        DXGI_FORMAT                                         m_frameBackBufferFormat;
        DXGI_FORMAT                                         m_uiBackBufferFormat;
        DXGI_FORMAT                                         m_depthBufferFormat;
        UINT                                                m_backBufferCount;

        // Cached device properties.
        HWND                                                m_window;
        D3D_FEATURE_LEVEL                                   m_d3dFeatureLevel;
        RECT                                                m_outputSize;

        // DeviceResources options (see flags above)
        unsigned int                                        m_options;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify*                                      m_deviceNotify;
    };
}
