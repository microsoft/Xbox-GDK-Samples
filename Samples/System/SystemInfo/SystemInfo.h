//--------------------------------------------------------------------------------------
// SystemInfo.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"


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
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const noexcept;

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
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_largeFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;
    float                                       m_scale;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum class InfoPage
    {
        SYSTEMINFO = 0,
        GLOBALMEMORYSTATUS,
        GETDEVICETYPE,
        ANALYTICSINFO,
        CONSOLEINFO,
        GLPI,
        GLPIEX,
#ifdef _GAMING_DESKTOP
        GETPROCESSINFO,
        CPUSETS,
        DIRECT3D,
        DIRECT3D_OPT1,
        DIRECT3D_OPT2,
        DIRECT3D_OPT3,
        DIRECT3D_OPT4,
        DXGI,
#else
        DIRECT3D,
#endif
        MAX,
    };

    enum Descriptors
    {
        SmallFont,
        LargeFont,
        ControllerFont,
        Background,
        Count,
    };

    int m_current;
    bool m_gamepadPresent;
};
