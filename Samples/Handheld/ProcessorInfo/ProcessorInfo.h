//--------------------------------------------------------------------------------------
// ProcessorInfo.h
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
    void OnActivated() {}
    void OnDeactivated() {}
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnDisplayChange();
    void OnWindowSizeChanged(int32_t width, int32_t height);

    // Properties
    void GetDefaultSize(int32_t& width, int32_t& height) const noexcept;
    void GetCurrentSize(int32_t& width, int32_t& height) const noexcept;

private:

    enum class Page
    {
        firstPage,
        generalInfoPage = firstPage,
        efficiencyPage,
        cachePage,
        powerPage,
        lastPage = powerPage,
    };

    Page m_displayPage;
    void IncrementPage();
    void DecrementPage();
    void DisplayGeneralInfoPage();
    void DisplayEfficiencyPage();
    void DisplayCachePage();
    void DisplayPowerPage();

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>   m_spriteBatch;
    Microsoft::WRL::ComPtr<ID3D12Resource>	m_background;
    std::unique_ptr<DirectX::SpriteFont>    m_regularFont;
    std::unique_ptr<DirectX::SpriteFont>    m_largeFont;
    std::unique_ptr<DirectX::SpriteFont>    m_ctrlFont;

    enum Descriptors
    {
        Background,
        RegularFont,
        LargeFont,
        CtrlFont,
        Count
    };
};
