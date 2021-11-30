//--------------------------------------------------------------------------------------
// SimpleInstancing.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Shared.h"


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

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void ResetSimulation();
    float FloatRand(float lowerBound = -1.0f, float upperBound = 1.0f);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    enum Descriptors
    {
        TextFont,
        ControllerFont,
        Count
    };

    //--------------------------------------------------------------------------------------
    // Sample Objects.
    //--------------------------------------------------------------------------------------

    // Instance vertex definition
    struct Instance
    {
        DirectX::XMFLOAT4 quaternion;
        DirectX::XMFLOAT4 positionAndScale;
    };

    // Light data structure (maps to constant buffer in pixel shader)
    struct Lights
    {
        DirectX::XMFLOAT4 directional;
        DirectX::XMFLOAT4 pointPositions[c_pointLightCount];
        DirectX::XMFLOAT4 pointColors[c_pointLightCount];
    };

    // Direct3D 12 pipeline objects.
    Microsoft::WRL::ComPtr<ID3D12RootSignature>  m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>  m_pipelineState;

    // Direct3D 12 resources.
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                     m_vertexBufferView[3];
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_indexBuffer;
    D3D12_INDEX_BUFFER_VIEW                      m_indexBufferView;
    Microsoft::WRL::ComPtr<ID3D12Resource>       m_boxColors;

    Microsoft::WRL::ComPtr<ID3D12Resource>       m_instanceData;
    uint8_t*                                     m_mappedInstanceData;
    D3D12_GPU_VIRTUAL_ADDRESS                    m_instanceDataGpuAddr;

    // A synchronization fence and an event. These members will be used
    // to synchronize the CPU with the GPU so that there will be no
    // contention for the instance data. 
    Microsoft::WRL::ComPtr<ID3D12Fence>          m_fence;
    Microsoft::WRL::Wrappers::Event              m_fenceEvent;

    struct aligned_deleter { void operator()(void* p) { _aligned_free(p); } };

    std::unique_ptr<Instance[]>                             m_CPUInstanceData;
    std::unique_ptr<DirectX::XMVECTOR[], aligned_deleter>   m_rotationQuaternions;
    std::unique_ptr<DirectX::XMVECTOR[], aligned_deleter>   m_velocities;
    uint32_t                                                m_usedInstanceCount;

    DirectX::SimpleMath::Matrix                 m_proj;
    DirectX::SimpleMath::Matrix                 m_view;
    Lights                                      m_lights;
    float                                       m_pitch;
    float                                       m_yaw;

    std::default_random_engine                  m_randomEngine;
};
