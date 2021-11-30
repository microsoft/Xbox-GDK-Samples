//--------------------------------------------------------------------------------------
// SimpleDeviceAndSwapChain.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept;
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void WaitForGpu();
    void MoveToNextFrame();
    void RegisterFrameEvents();

    // Application state
    HWND                                                m_window;
    int                                                 m_outputWidth;
    int                                                 m_outputHeight;

    // Direct3D Objects
    static constexpr UINT                               c_swapBufferCount = 2;
    UINT                                                m_backBufferIndex;
    Microsoft::WRL::ComPtr<ID3D12Device>                m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_rtvDescriptorHeap;
    UINT                                                m_rtvDescriptorSize;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>        m_dsvDescriptorHeap;
    UINT                                                m_dsvDescriptorSize;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_commandAllocators[c_swapBufferCount];
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>   m_commandList;
    Microsoft::WRL::ComPtr<ID3D12Fence>                 m_fence;
    UINT64                                              m_fenceValues[c_swapBufferCount];
    Microsoft::WRL::Wrappers::Event                     m_fenceEvent;
    D3D12XBOX_FRAME_PIPELINE_TOKEN                      m_framePipelineToken;

    // Rendering resources
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_renderTargets[c_swapBufferCount];
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_depthStencil;

    // Rendering loop timer.
    uint64_t                                            m_frame;
    DX::StepTimer                                       m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>                   m_gamePad;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>            m_graphicsMemory;

    std::unique_ptr<DirectX::DescriptorHeap>            m_resourceDescriptors;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_background;

    enum Descriptors
    {
        Background,
        Count
    };

    std::unique_ptr<DirectX::SpriteBatch>               m_batch;
};
