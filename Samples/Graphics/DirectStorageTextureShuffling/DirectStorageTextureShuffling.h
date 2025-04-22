//--------------------------------------------------------------------------------------
// DirectStorageTextureShuffling.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include "ShuffledTextureMetadata.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final 
{
public:

    Sample() noexcept(false);
    ~Sample() = default;

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
    void OnConstrained() {}
    void OnUnConstrained() {}

private:

    void InitDirectStorage();
    void LoadTextureMetadata();
    void LoadTextureData();
    void UnshuffleTextureData();

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
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::DescriptorHeap>    m_srvUavHeap;

    // DirectStroage objects
    std::vector<Microsoft::WRL::ComPtr<IDStorageFileX>> m_dsFiles;
    Microsoft::WRL::ComPtr<IDStorageQueueX1>            m_dsQueue;
    Microsoft::WRL::ComPtr<IDStorageFactoryX1>          m_dsFactory;

    // Use async compute for unshuffling
    Microsoft::WRL::ComPtr<ID3D12CommandQueue>          m_unshuffleCmdQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator>      m_unshuffleCmdAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6>  m_unshuffleCmdList;

    // Unshuffling PSOs
    Microsoft::WRL::ComPtr<ID3D12RootSignature>                  m_unshuffleRootSignature;
    std::vector<Microsoft::WRL::ComPtr<ID3D12PipelineState>>     m_unshufflePSOs;

    // Synchronization for data loading, processing and rendering
    Microsoft::WRL::ComPtr<ID3D12Fence>                 m_unshuffleTextureFence;
    HANDLE                                              m_unshuffleTextureFenceEvent;
    uint64_t                                            m_unshuffleTextureFenceValue;

    // Texture processing objects
    std::vector<ShuffledTextureMetadata>                m_textureMetadata;
    std::vector<uint32_t>                               m_textureUploadSizes;
    std::vector<D3D12_GPU_VIRTUAL_ADDRESS>              m_textureMemory;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_textures;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_textureDataBuffers;

    // Simple quad rendering
    Microsoft::WRL::ComPtr<ID3D12PipelineState>   m_simplePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>   m_simpleRootSignature;
    Microsoft::WRL::ComPtr<ID3D12Resource>        m_quadVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW                      m_quadVertexBufferView;

};
