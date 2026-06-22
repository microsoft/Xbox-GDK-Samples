//--------------------------------------------------------------------------------------
// TextureCompression.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "TextureInfo.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:

    Sample() noexcept(false);
    ~Sample();

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

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectStroage objects.
    Microsoft::WRL::ComPtr<IDStorageStatusArrayX> m_DSStatusArray;
    Microsoft::WRL::ComPtr<IDStorageQueueX>     m_DSSQueue;
    Microsoft::WRL::ComPtr<IDStorageFactoryX>   m_DSFactory;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::SpriteBatch>       m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;
    std::unique_ptr<DirectX::DescriptorHeap>    m_srvHeap;
    std::unique_ptr<DirectX::DescriptorHeap>    m_samplerHeap;

    // Quad shader pipeline
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_simplePSO;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_simpleRootSignature;

    std::vector<TextureInfo>                    m_textureInfos;

    uint32_t                                    m_nextSRVIndex;
    bool                                        m_cacheFlushRequired;

    struct Texture
    {
        uint32_t                                    infoIndex{0};
        uint32_t                                    renderMipIndex{0};
        uint32_t                                    streamMipIndex{0};

        D3D12_GPU_VIRTUAL_ADDRESS                   memory{};
        Microsoft::WRL::ComPtr<ID3D12Resource>      resource{};
        uint32_t                                    srvIndex{0};
        UINT64                                      fenceValue{0};             // Last used fence value
    };

    // These 3 variables hold the running state for the sample. Note that m_texture will
    // stay current while a new texture is requested, streamed and decompressed
    // asynchronously. After that, the current texture is retired and the new texture made
    // current.
    Texture*                                    m_texture;
    std::future<Texture*>                       m_streamRequest;
    std::list<Texture*>                         m_retiredTextures;

    Texture* AllocateTexture(uint32_t textureIndex) const;
    std::future<Texture*> RequestTexture(Texture* texture, uint32_t streamMipIndex) const;
    void PlaceTexture(Texture* texture);

    void PruneRetiredTextures(UINT64 lastRetiredFence);
};
