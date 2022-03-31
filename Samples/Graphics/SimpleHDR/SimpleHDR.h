//--------------------------------------------------------------------------------------
// SimpleHDR.h
//
// A simple sample to show how to implement HDR on Xbox
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

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
    void OnConstrained();
    void OnUnConstrained();

private:

    void Update(DX::StepTimer const& timer);
    void Render();
    void RenderUI(ID3D12GraphicsCommandList* commandList);

    void FinalHDRShader(ID3D12GraphicsCommandList* commandList);

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void TryEnableHDR();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input device.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;
    std::unique_ptr<DirectX::DescriptorPile>    m_rtvPile;
    std::unique_ptr<DirectX::DescriptorPile>    m_srvPile;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::BasicEffect>       m_effect;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;

    // Direct3D 12 objects
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_finalHDRShaderRS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_finalHDRShaderPSO;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_hdrScene;

    bool                                        m_bIsDisplayInHDRMode;

    uint32_t                                    m_width;
    uint32_t                                    m_height;

    std::unique_ptr<DirectX::DescriptorHeap>    m_rtvDescriptorHeap;

    enum RTVs
    {
        HDRSceneRTV,
        NumRTVs
    };

    enum SRVs
    {
        FontSRV,
        CtrlFontSRV,
        HDRSceneSRV,
        NumSRVs
    };
};
