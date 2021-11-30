//--------------------------------------------------------------------------------------
// SimpleLighting.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
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

    struct ConstantBuffer
    {
        DirectX::XMMATRIX worldMatrix;
        DirectX::XMMATRIX viewMatrix;
        DirectX::XMMATRIX projectionMatrix;
        DirectX::XMVECTOR lightDir[2];
        DirectX::XMVECTOR lightColor[2];
        DirectX::XMVECTOR outputColor;
    };

    struct PaddedConstantBuffer
    {
        ConstantBuffer constants;
        uint8_t bytes[240];
    };

    // Check the exact size of the PaddedConstantBuffer to make sure it will align properly
    static_assert((sizeof(PaddedConstantBuffer) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) == 0, "PaddedConstantBuffer is not aligned properly");

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    Microsoft::WRL::ComPtr<ID3D12RootSignature>  m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>  m_lambertPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>  m_solidColorPipelineState;
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_indexBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_perFrameConstants;
    PaddedConstantBuffer*                        m_mappedConstantData;
    D3D12_GPU_VIRTUAL_ADDRESS                    m_constantDataGpuAddr;
    D3D12_VERTEX_BUFFER_VIEW                     m_vertexBufferView;
    D3D12_INDEX_BUFFER_VIEW                      m_indexBufferView;

    // In this simple sample, we know that there are three draw calls
    // and we will update the scene constants for each draw call.
    static constexpr unsigned int                    c_numDrawCalls = 3;

    // A synchronization fence and an event. These members will be used
    // to synchronize the CPU with the GPU so that there will be no
    // contention for the constant buffers. 
    Microsoft::WRL::ComPtr<ID3D12Fence>          m_fence;
    Microsoft::WRL::Wrappers::Event              m_fenceEvent;

    // Index in the root parameter table
    static constexpr UINT                        c_rootParameterCB = 0;

    // Scene constants, updated per-frame
    float                                        m_curRotationAngleRad;

    // These computed values will be loaded into a ConstantBuffer
    // during Render
    DirectX::SimpleMath::Matrix                  m_worldMatrix;
    DirectX::SimpleMath::Matrix                  m_viewMatrix;
    DirectX::SimpleMath::Matrix                  m_projectionMatrix;
    DirectX::XMFLOAT4                            m_lightDirs[2];
    DirectX::XMFLOAT4                            m_lightColors[2];
    DirectX::XMFLOAT4                            m_outputColor;
};
