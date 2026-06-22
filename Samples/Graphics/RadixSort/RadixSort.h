//--------------------------------------------------------------------------------------------------
// RadixSort.h
//
// This sample demonstrates how to sort 32-bit data elements on GPU with a radix sort
//
// For additional information refer to ReadMe.docx
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

namespace SRVUAVDescriptors
{
    enum Descriptor
    {
        OutputUAV,
        BinIdsUAV,

        FontSRVSmall,
        FontSRVBig,
        ControllerFontSRV,

        EnumCount
    };
};

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
    void OnConstrained() {}
    void OnUnConstrained() {}

    // Properties
    bool RequestHDRMode() const noexcept { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableHDR) != 0 : false; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void RenderHUD(ID3D12GraphicsCommandList* cl);

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    DX::GPUTimer                                m_gpuTimer;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>       m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_bigFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;


    // Sample resources
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_outputTex;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_binIdsTex;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_fullScreenNoiseRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_fullScreenNoisePSO;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_bufferAsTexRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_bufferAsTexPSO;

    std::unique_ptr<DirectX::DescriptorHeap>    m_csuHeap;

    // Options
    uint32_t                                    m_currentColour;
    uint32_t                                    m_currentResolutionIndex;

    uint16_t                                    m_numResolutionIndices;

    bool                                        m_hideHUD;
};
