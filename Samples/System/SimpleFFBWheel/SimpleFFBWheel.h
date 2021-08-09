//--------------------------------------------------------------------------------------
// SimpleFFBWheel.h
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

    Microsoft::WRL::ComPtr<IGameInputDevice> m_device;
    Microsoft::WRL::ComPtr<IGameInputForceFeedbackEffect> m_feedbackEffect;

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
    GameInputCallbackToken                      m_deviceToken;
    std::wstring			                    m_buttonString;
    float 						                m_wheelValue;
    float                                       m_clutchValue;
    float                                       m_brakeValue;
    float                                       m_throttleValue;
    float                                       m_handbrakeValue;
    int32_t                                     m_patternShifterGearValue;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum Descriptors
    {
        PrintFont,
        ControllerFont,
        Background,
        Count,
    };
};
