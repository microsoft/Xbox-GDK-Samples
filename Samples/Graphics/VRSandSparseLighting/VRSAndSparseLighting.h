//--------------------------------------------------------------------------------------
// VRSAndSparseLighting.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "PerformanceTimersXbox.h"
#include "StepTimer.h"

#include "Terrain.h"
#include "DPadUI.h"



// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    ~Sample();

    Sample(Sample&&) = default;
    Sample& operator= (Sample&&) = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    void OnDeviceLost() override;
    void OnDeviceRestored() override;

    // Messages
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

    // Properties
    bool RequestHDRMode() const noexcept { return true; }

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    Terrain										m_terrain;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;


    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::SpriteFont>        m_textFont;
    std::unique_ptr<DirectX::SpriteBatch>       m_fontBatch;
    std::unique_ptr<DirectX::DescriptorHeap>    m_uiDescriptorHeap;

    struct UIDescriptors
    {
        enum Enum
        {
            TextFont,
            Count
        };
    };

    struct SunSpeed
    {
        enum Enum
        {
            Stopped,
            Slow,
            Fast,
            Count
        };
    };


    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // Camera
    DirectX::SimpleMath::Matrix                 m_proj;
    DirectX::SimpleMath::Matrix                 m_view;
    DirectX::SimpleMath::Matrix                 m_viewProj;
    DirectX::SimpleMath::Vector3                m_position;

    float                                       m_pitch;
    float                                       m_yaw;
    SunSpeed::Enum                              m_sunSpeed;
    float                                       m_sunHeight;

    bool                                        m_viewUI;

    float                                       m_shadingRateTolerance;

    DX::GPUCommandListTimer<ID3D12GraphicsCommandList>  m_gpuTimer;

    struct DPADMenu
    {
        enum Enum
        {
            GroundTruth,
            VRS,
            SparseLighting,
            SparseBufferGeneration,
            SparseLightingTileSize,
            DepthDiscontinuityCheck,
            DeblockerOptions,
            RotateLitPixelOptions,
            DebugVisualiation,
            ZoomLevel,
            Count
        };
    };

    DPadUI                                              m_dpadUI;
};
