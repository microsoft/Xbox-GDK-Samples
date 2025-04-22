//--------------------------------------------------------------------------------------
// Simple120Hz.h
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
    void DrawHUD(ID3D12GraphicsCommandList* commandList);
    void ConvertToHDR10(ID3D12GraphicsCommandList* commandList);

    void Clear();
    
    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void RefreshSupportedIntervals();
    void UpdateFrameInterval();
    void TryEnableHDR();

public:
    enum FrameInterval : uint32_t
    {
        FRAME_INTERVAL_30HZ = 0,
        FRAME_INTERVAL_40HZ,
        FRAME_INTERVAL_60HZ,
        FRAME_INTERVAL_120HZ,
#if _GXDK_VER >= 0x55F00C58 /* GDK Edition 220300 */
        FRAME_INTERVAL_24HZ,
#endif
        FRAME_INTERVAL_COUNT
    };

private:
    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;
    uint32_t                                    m_displayWidth;
    uint32_t                                    m_displayHeight;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;

    // HUD
    std::unique_ptr<DirectX::DescriptorPile>    m_rtvPile;
    std::unique_ptr<DirectX::DescriptorPile>    m_srvPile;
    std::unique_ptr<DirectX::SpriteBatch>       m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;

    // Direct3D 12 objects
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_d3dConvertToHDR10RS;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_d3dConvertToHDR10PSO;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_renderTexture;

    // HDR State and Settings
    bool                                        m_preferHdr;
    bool                                        m_isDisplayInHDRMode;

    // Refresh Rate State and Settings
    FrameInterval                               m_frameInterval;
    bool                                        m_refreshRateSupport[FRAME_INTERVAL_COUNT];
    bool                                        m_vrrSupported;
};
