//--------------------------------------------------------------------------------------
// ScatteringFp16.h
//
// This sample compares performance difference when using 16-bit math in VALU heavy shader
// that simulates atmospheric single scattering to the same shader without using 16-bit math
//
// Additionally, the sample compares various approaches of enabling 16-bit math:
//      a. The main function is called twice per compute thread to give the compiler the 
//         opportunity to merge duplicated instruction into packed instructions and pack 
//         16-bit data into VGPRs so that the data can be fed into 16-bit packed instructions
//
//      b. The data is duplicated for every operation (SIMD-like approach) to help the compiler
//         to figure what operations could be translated to 16-bit packed instructions and what
//         16-bit data could be packed together into VGPRs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

namespace SRVUAVDescriptors
{
    enum Descriptor
    {
        OutputUAV,

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

    Sample(Sample&&) = delete;
    Sample& operator= (Sample&&) = delete;

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

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_scatteringRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_scattering32Bit1pptPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_scattering32Bit2pptNoPackPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_scattering32Bit2pptWithPackPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_scattering16Bit1pptPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_scattering16Bit2pptNoPackPSO;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_scattering16Bit2pptWithPackPSO;

    std::unique_ptr<DirectX::DescriptorHeap>    m_csuHeap;

    // Options
    uint32_t                                    m_currentColour;
    uint32_t                                    m_currentResolutionIndex;

    uint32_t                                    m_scatteringMode;
    uint16_t                                    m_flags;
    uint16_t                                    m_numResolutionIndices;
    float                                       m_leftStickX;
    float                                       m_leftStickY;

    bool                                        m_hideHUD;
};
