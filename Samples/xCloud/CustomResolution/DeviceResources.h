//
// DeviceResources.h - A wrapper for the Direct3D 12.X device and swapchain
//
// NOTE: This is a customized version of DeviceResources.h that includes support for dynamic resolution.
//

#pragma once

namespace DX
{
    // Controls all the DirectX device resources.
    class DeviceResources
    {
    public:
        static constexpr unsigned int c_Enable4K_UHD = 0x1;
        static constexpr unsigned int c_EnableHDR    = 0x2;

        DeviceResources(DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM,
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
        void SetWindow(HWND window) noexcept { m_window = window; }
        void Prepare(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT);
        void Present(D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET);
        void Suspend();
        void Resume();
        void WaitForGpu() noexcept;

        // Device Accessors.
        RECT GetOutputSize() const noexcept { return m_outputSize; }

        // Direct3D Accessors.
        auto                        GetD3DDevice() const noexcept           { return m_d3dDevice.Get(); }
        D3D_FEATURE_LEVEL           GetDeviceFeatureLevel() const noexcept  { return m_d3dFeatureLevel; }
        ID3D12Resource*             GetRenderTarget() const noexcept        { return m_renderTargets[m_backBufferIndex].Get(); }
        ID3D12Resource*             GetDepthStencil() const noexcept        { return m_depthStencil.Get(); }
        ID3D12CommandQueue*         GetCommandQueue() const noexcept        { return m_commandQueue.Get(); }
        ID3D12CommandAllocator*     GetCommandAllocator() const noexcept    { return m_commandAllocators[m_backBufferIndex].Get(); }
        auto                        GetCommandList() const noexcept         { return m_commandList.Get(); }
        DXGI_FORMAT                 GetBackBufferFormat() const noexcept    { return m_backBufferFormat; }
        DXGI_FORMAT                 GetDepthBufferFormat() const noexcept   { return m_depthBufferFormat; }
        D3D12_VIEWPORT              GetScreenViewport() const noexcept      { return m_screenViewport; }
        D3D12_RECT                  GetScissorRect() const noexcept         { return m_scissorRect; }
        UINT                        GetCurrentFrameIndex() const noexcept   { return m_backBufferIndex; }
        UINT                        GetBackBufferCount() const noexcept     { return m_backBufferCount; }
        unsigned int                GetDeviceOptions() const noexcept       { return m_options; }
        D3D12XBOX_FRAME_PIPELINE_TOKEN GetFrameToken() const noexcept       { return m_framePipelineToken; }

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

        // Second display plane for UI
        ID3D12Resource* GetUIRenderTarget() const noexcept { return m_renderTargetsUI[m_backBufferIndex].Get(); }
        CD3DX12_CPU_DESCRIPTOR_HANDLE GetUIRenderTargetView() const noexcept
        {
            return CD3DX12_CPU_DESCRIPTOR_HANDLE(
                m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
                static_cast<INT>(m_backBufferCount + m_backBufferIndex), m_rtvDescriptorSize);
        }

        void SetPresentationThreshold(float threshold)  noexcept { m_presentationThreshold = threshold; }
        void SetSceneRenderSize(long width, long height)  noexcept { m_sceneRenderSize = { 0, 0, width, height }; }

    private:
        void MoveToNextFrame();
        void RegisterFrameEvents();

        static constexpr size_t MAX_BACK_BUFFER_COUNT = 3;

        UINT                                                m_backBufferIndex;

        // Direct3D objects.
#ifdef _GAMING_XBOX_SCARLETT
        Microsoft::WRL::ComPtr<ID3D12Device8>               m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5>  m_commandList;
#else
        Microsoft::WRL::ComPtr<ID3D12Device2>               m_d3dDevice;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
#endif
        Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocators[MAX_BACK_BUFFER_COUNT];

        // Swap chain objects.
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargets[MAX_BACK_BUFFER_COUNT];
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_depthStencil;
        
        // Second display plane for UI at fixed resolution
        Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargetsUI[MAX_BACK_BUFFER_COUNT];

        // Presentation fence objects.
        Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
        UINT64                                              m_fenceValues[MAX_BACK_BUFFER_COUNT];
        Microsoft::WRL::Wrappers::Event                     m_fenceEvent;
        D3D12XBOX_FRAME_PIPELINE_TOKEN                      m_framePipelineToken;

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

        // DeviceResources options (see flags above)
        unsigned int                                        m_options;

        // Presentation parameters
        float                                               m_presentationThreshold;
        RECT                                                m_sceneRenderSize;
    };
}
