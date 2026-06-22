//--------------------------------------------------------------------------------------
// PixelBinning.h
//
// This sample demonstrates how to re-arrange pixels spatially to improve coherency of
// any compute workload processing them. This is done by
//      1. Assigning an id in a variable range [0 .. 1-2047] to every pixel
//      2. Creating an indirection buffer of pixel coordinates where pixels with the
//         same id are stored adjacently in memory
//
//      Potential applications:
//          1. Tile-based classification approaches where splitting a compute workload
//             into multiple smaller workloads processed by specialized compute shaders
//             might be beneficial
//
//          2. Raytracing techniques where re-arranging rays allows to create
//             wavefronts processing more spatially coherent rays and that could lead to
//             faster BVH traversal
//
// For additional information refer to ReadMe.docx
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

        FragmentIDsUAV,
        FragmentIDsSRV,

        VerificationKeysUAV,

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

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_fullScreenNoiseRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_fullScreenNoisePSO;

    std::unique_ptr<DirectX::DescriptorHeap>    m_csuHeap;

    // Options
    uint32_t                                    m_currentColour;
    uint32_t                                    m_currentResolutionIndex;

    uint32_t                                    m_binningType;
    uint32_t                                    m_dbgviewType;

    uint16_t                                    m_numResolutionIndices;
    uint16_t                                    m_numBinsUsed;
    uint32_t                                    m_numCountersUsed;

    bool                                        m_hideHUD;
};
