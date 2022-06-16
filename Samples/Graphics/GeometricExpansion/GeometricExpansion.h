//--------------------------------------------------------------------------------------
// GeometricExpansion.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "Particles.h"


// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:
    Sample() noexcept(false);
    ~Sample();

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
    void GetDefaultSize(int& width, int& height) const;

private:
    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();
    void DrawHUD(ID3D12GraphicsCommandList* commandList);

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();


private:
    // Device resources.
    std::unique_ptr<DX::DeviceResources>     m_deviceResources;
    int                                      m_displayWidth;
    int                                      m_displayHeight;

    // Rendering loop timer.
    uint64_t                                 m_frame;
    DX::StepTimer                            m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>        m_gamePad;
    std::unique_ptr<DirectX::Keyboard>       m_keyboard;
    std::unique_ptr<DirectX::Mouse>          m_mouse;

    DirectX::GamePad::ButtonStateTracker     m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker  m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory> m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile> m_srvPile;

    // HUD
    std::unique_ptr<DirectX::SpriteBatch>    m_hudBatch;
    std::unique_ptr<DirectX::SpriteFont>     m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>     m_ctrlFont;

    // App state variables
    float                                    m_spawnRate; // Particles per Second
    float                                    m_springCoeff;
    float                                    m_dragFactor;
    float                                    m_initialSpeed;

    DX::OrbitCamera                          m_camera;
    ATG::ParticleSystem                      m_particleSystem;
};
