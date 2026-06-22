//--------------------------------------------------------------------------------------
// MipMapCS.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "FullScreenQuad\FullScreenQuad.h"
#include "Shared.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final
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

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;
    std::unique_ptr<DirectX::SpriteFont>        m_colorCtrlFont;

    std::unique_ptr<DX::FullScreenQuad>         m_fullScreenQuad;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_quadPSO;

    enum CSGenMips
    {
        CSGenerateMips = 0,
        CSGenerateMips_OddX,
        CSGenerateMips_OddY,
        CSGenerateMips_OddXY,
        CSGenerateMips_Count,
    };

    enum FfxSpdPipelineStates
    {
        FfxSpdFp32,
        FfxSpdFp16,
        FfxSpdPipelineStates_Count
    };

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_csRootSig;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_spdRootSig;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_generateMips[CSGenerateMips_Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_generateMipsFp16[CSGenerateMips_Count];
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_ffxSpd[FfxSpdPipelineStates_Count];

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_texture;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_sourceTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_atomicBuffer;

    bool                                        m_allLevels;
    bool                                        m_multiPass;
    bool                                        m_fp16;

    uint32_t                                    m_mipLevel;
    uint32_t                                    m_numLevels;
    uint32_t                                    m_topMip;

    uint32_t                                    m_texWidth;
    uint32_t                                    m_texHeight;

    float                                       m_zoom;
    float                                       m_offsetX;
    float                                       m_offsetY;

    enum Descriptors : size_t
    {
        TextFont,
        ControllerFont,
        ColorControllerFont,
        Texture,
        UAV_MipBase,
        UAV_MipMax = UAV_MipBase + D3D12_REQ_MIP_LEVELS + MIPS_IN_ONE_SHADER,
        Count,
    };
};
