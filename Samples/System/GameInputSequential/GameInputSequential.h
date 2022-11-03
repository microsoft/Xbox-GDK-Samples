//--------------------------------------------------------------------------------------
// GameInputSequential.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include <GameInput.h>

constexpr size_t COMBOCOUNT = 4;

struct InputMove
{
    std::wstring Name;
    GameInputGamepadButtons Buttons[COMBOCOUNT];
    uint64_t Timing[COMBOCOUNT];
    int lastIndex;
    uint64_t lastTime;

    InputMove(const std::wstring& name, const GameInputGamepadButtons* buttons, const uint64_t* timing) :
        Name(name)
    {
        Reset();

        for (size_t i = 0; i < COMBOCOUNT; i++)
        {
            Buttons[i] = buttons[i];
            Timing[i] = timing[i];
        }
    }

    void Reset()
    {
        lastIndex = 0;
        lastTime = 0;
    }
};

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
    void OnConstrained() {}
    void OnUnConstrained() {}

    Microsoft::WRL::ComPtr<IGameInputDevice>    m_device;
    Microsoft::WRL::ComPtr<IGameInput>			m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>   m_lastReading;

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
    std::wstring			                    m_buttonString;
    double						                m_leftTrigger;
    double                                      m_rightTrigger;
    double                                      m_leftStickX;
    double                                      m_leftStickY;
    double                                      m_rightStickX;
    double                                      m_rightStickY;
    GameInputCallbackToken                      m_deviceToken;

    //Move states
    std::vector<InputMove>                      m_moves;
    std::wstring			                    m_moveString;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;
    float                                       m_scale;

    enum Descriptors
    {
        PrintFont,
        ControllerFont,
        Count,
    };
};
