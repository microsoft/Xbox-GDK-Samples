//
// DeviceResources.h - A wrapper for the Direct3D 12/12.X device and swapchain
//

#pragma once

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
        DeviceResources(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
                        DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT,
                        UINT backBufferCount = 2) noexcept(false);
        ~DeviceResources();

        DeviceResources(DeviceResources&&) = default;
        DeviceResources& operator= (DeviceResources&&) = default;

        DeviceResources(DeviceResources const&) = delete;
        DeviceResources& operator= (DeviceResources const&) = delete;

        void CreateDeviceResources();
        void CreateWindowSizeDependentResources();
        void SetWindow(HWND window, int width, int height) noexcept;
        bool WindowSizeChanged(int width, int height);
        void HandleDeviceLost();
        void RegisterDeviceNotify(IDeviceNotify* deviceNotify) noexcept { m_deviceNotify = deviceNotify; }
        void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT,
                     D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void Suspend();
        void Resume();
        void WaitForGpu() noexcept;

        // Device Accessors.
        RECT GetOutputSize() const noexcept { return m_outputSize; }

        // Direct3D Accessors.
        auto                        GetD3DDevice() const noexcept          { return m_d3dDevice.Get(); }
#ifdef _GAMING_DESKTOP
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

        CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView() const noexcept
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(m_backBufferIndex), m_rtvDescriptorSize);
        }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const noexcept
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        }

    private:
        void MoveToNextFrame();
#ifdef _GAMING_XBOX
        void RegisterFrameEvents();
#else
        void GetAdapter(IDXGIAdapter1** ppAdapter);
#endif

        static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

        UINT                                                m_backBufferIndex;

        // Direct3D objects.
        Microsoft::WRL::ComPtr<ID3D12Device>                m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocators[MAX_BACK_BUFFER_COUNT];

        // Swap chain objects.
#ifdef _GAMING_DESKTOP
        Microsoft::WRL::ComPtr<IDXGIFactory6>               m_dxgiFactory;
        Microsoft::WRL::ComPtr<IDXGISwapChain3>             m_swapChain;
#endif
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargets[MAX_BACK_BUFFER_COUNT];
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_depthStencil;

        // Presentation fence objects.
        Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
        UINT64                                              m_fenceValues[MAX_BACK_BUFFER_COUNT];
        Microsoft::WRL::Wrappers::Event                     m_fenceEvent;
#ifdef _GAMING_XBOX
        D3D12XBOX_FRAME_PIPELINE_TOKEN                      m_framePipelineToken;
#endif

        // Direct3D rendering objects.
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
        UINT                                                m_rtvDescriptorSize;
        D3D12_VIEWPORT                                      m_screenViewport;
        D3D12_RECT                                          m_scissorRect;

        // Direct3D properties.
        DXGI_FORMAT                                         m_backBufferFormat;
        DXGI_FORMAT                                         m_depthBufferFormat;
        UINT                                                m_backBufferCount;

        // Cached device properties.
        HWND                                                m_window;
        D3D_FEATURE_LEVEL                                   m_d3dFeatureLevel;
        RECT                                                m_outputSize;

        // The IDeviceNotify can be held directly as it owns the DeviceResources.
        IDeviceNotify*                                      m_deviceNotify;
    };
}
