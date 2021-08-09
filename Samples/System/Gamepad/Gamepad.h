//--------------------------------------------------------------------------------------
// Gamepad.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include <GameInput.h>

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
{
public:
    Sample() noexcept(false);

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

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

    //Gamepad states
    Microsoft::WRL::ComPtr<IGameInput>			m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>   m_reading;
    std::vector<APP_LOCAL_DEVICE_ID>			m_deviceIds;
    wchar_t							            m_deviceString[20];
    std::wstring			                    m_buttonString;
    double						                m_leftTrigger;
    double                                      m_rightTrigger;
    double                                      m_leftStickX;
    double                                      m_leftStickY;
    double                                      m_rightStickX;
    double                                      m_rightStickY;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;
    float                                       m_scale;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum Descriptors
    {
        PrintFont,
        ControllerFont,
        Background,
        Count,
    };
};