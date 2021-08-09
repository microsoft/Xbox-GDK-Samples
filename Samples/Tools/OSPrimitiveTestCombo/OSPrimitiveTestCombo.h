//--------------------------------------------------------------------------------------
// OSPrimitiveTestCombo.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample : public DX::IDeviceNotify
{
public:

    Sample() noexcept(false);
    virtual ~Sample();

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic render loop
    void Tick();

    // IDeviceNotify
    virtual void OnDeviceLost() override;
    virtual void OnDeviceRestored() override;

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowMoved();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const;

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void PerformTests();

    std::atomic<bool> m_shutdownThread;
    std::atomic<bool> m_finishedTestRun;
    std::atomic<uint32_t> m_runWorking;
    std::atomic<uint32_t> m_firstProcessorCore;
    std::atomic<uint32_t> m_secondProcessorCore;
    std::thread* m_workerThread;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>           m_gamePad;
    std::unique_ptr<DirectX::Keyboard>          m_keyboard;
    std::unique_ptr<DirectX::Mouse>             m_mouse;

    DirectX::GamePad::ButtonStateTracker        m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker     m_keyboardButtons;

    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap> m_resourceDescriptors;
    std::unique_ptr<DirectX::SpriteBatch>   m_spriteBatch;
    Microsoft::WRL::ComPtr<ID3D12Resource>	m_background;
    std::unique_ptr<DirectX::SpriteFont>    m_regularFont;

    enum Descriptors
    {
        Background,
        RegularFont,
        Count
    };
};
