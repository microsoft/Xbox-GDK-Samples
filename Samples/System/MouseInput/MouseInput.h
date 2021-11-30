//--------------------------------------------------------------------------------------
// MouseInput.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "CompressedTextureFactory.h"

#include <GameInput.h>

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:
    Sample() noexcept(false);

    enum MouseMode { ABSOLUTE_MOUSE, RELATIVE_MOUSE, EDGECURSOR_MOUSE };

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Mouse cursor camera and target updates
    void UpdateCamera(DirectX::SimpleMath::Vector3 movement);
    void MoveForward(float amount);
    void MoveRight(float amount);

    // Sample logic
    void SetMode(MouseMode newMode);

    // Messages
    void OnSuspending();
    void OnResuming();

    bool IsRunning4k() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_Enable4K_UHD) != 0 : false; }
    bool IsRunning1440p() const { return m_deviceResources ? (m_deviceResources->GetDeviceOptions() & DX::DeviceResources::c_EnableQHD) != 0 : false; }

    int m_screenLocation_x;
    int m_screenLocation_y;
    bool m_leftButton;
    bool m_rightButton;

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

    // MouseCursor sample
    void SetView();

    bool m_isAbsolute;
    bool m_isRelative;
    bool m_isEdgeMove;
    bool m_highlightFPS;
    bool m_highlightRTS;
    bool m_mouseDown;

    // FPS
    DirectX::SimpleMath::Vector3 m_eyeFPS;
    DirectX::SimpleMath::Vector3 m_targetFPS;
    // RTS
    DirectX::SimpleMath::Vector3 m_eyeRTS;
    DirectX::SimpleMath::Vector3 m_targetRTS;

    DirectX::SimpleMath::Vector3 m_eye;
    DirectX::SimpleMath::Vector3 m_target;

    float m_pitch, m_yaw;

    DirectX::SimpleMath::Matrix m_world;
    DirectX::SimpleMath::Matrix m_view;
    DirectX::SimpleMath::Matrix m_proj;

    // Input devices.
    Microsoft::WRL::ComPtr<IGameInput>			m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>   m_reading;
    GameInputMouseState                         m_lastMouseState;

    // DirectXTK rendering
    std::unique_ptr<DirectX::SpriteFont> m_font;
    std::unique_ptr<DirectX::SpriteFont> m_font64;
    std::unique_ptr<DirectX::SpriteFont> m_font32;
    std::unique_ptr<DirectX::SpriteFont> m_font28;
    DirectX::SimpleMath::Vector2 m_fontPos;
    DirectX::SimpleMath::Vector2 m_fontPosTitle;
    DirectX::SimpleMath::Vector2 m_fontPosSubtitle;
    DirectX::SimpleMath::Vector2 m_fontPosFPS;
    DirectX::SimpleMath::Vector2 m_fontPosRTS;
    std::unique_ptr<DirectX::CommonStates> m_states;
    std::unique_ptr<DirectX::EffectFactory> m_FPSfxFactory;
    std::unique_ptr<DirectX::EffectFactory> m_RTSfxFactory;
    std::unique_ptr<DirectX::EffectTextureFactory>  m_modelFPSResources;
    std::unique_ptr<DX::CompressedTextureFactory>   m_modelRTSResources;
    std::unique_ptr<DirectX::Model> m_modelFPS;
    std::unique_ptr<DirectX::Model> m_modelRTS;
    DirectX::Model::EffectCollection m_modelFPSEffect;
    DirectX::Model::EffectCollection m_modelRTSEffect;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture_background;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture_tile;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_texture_tile_border;
    RECT m_fullscreenRect;
    RECT m_FPStile;
    RECT m_RTStile;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;

    enum Descriptors
    {
        PrintFont36,
        PrintFont34,
        PrintFont24,
        PrintFont22,
        MenuHighlight,
        Menu,
        Background,
        Count,
    };
};
