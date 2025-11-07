//--------------------------------------------------------------------------------------
// GamepadVibration.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"

#include <GameInput.h>
#if GAMEINPUT_API_VERSION == 1
using namespace GameInput::v1;
#elif GAMEINPUT_API_VERSION == 2
using namespace GameInput::v2;
#elif GAMEINPUT_API_VERSION == 3
using namespace GameInput::v3;
#endif

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample final : public DX::IDeviceNotify
{
public:
    static constexpr int TRIGGEREFFECTS_MAX = 5;

    enum class TRIGGEREFFECTS
    {
        TRIGGEREFFECTS_IMPULSETEST = 0,
        TRIGGEREFFECTS_FLATTIRE,
        TRIGGEREFFECTS_GUNWITHRECOIL,
        TRIGGEREFFECTS_HEARTBEAT,
        TRIGGEREFFECTS_FOOTSTEPS
    };

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

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    void InitializeImpulseTriggerEffects();

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

    //Gamepad states
    Microsoft::WRL::ComPtr<IGameInput>                      m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>               m_reading;
    
    bool            m_connected;
    bool            m_dPadPressed;
    float           m_leftMotorSpeed;
    float           m_rightMotorSpeed;
    float           m_leftTriggerLevel;
    float           m_rightTriggerLevel;

    // Variables used to control the Impulse Trigger Effects
    TRIGGEREFFECTS  m_selectedTriggerEffect; // Effect currently selected
    DWORD           m_triggerEffectCounter;  // General purpose counter for use by trigger effects

    size_t          m_leftTriggerArraySize;
    uint32_t*       m_pLeftTriggerDurations;
    float*          m_pLeftTriggerLevels;
    size_t          m_rightTriggerArraySize;
    uint32_t*       m_pRightTriggerDurations;
    float*          m_pRightTriggerLevels;
    uint32_t        m_leftTriggerIndex;
    uint32_t        m_rightTriggerIndex;

    uint64_t        m_frequency;
    uint64_t        m_counter;
    uint64_t        m_leftTriggerIndexUpdateTime;
    uint64_t        m_rightTriggerIndexUpdateTime;
    
    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorHeap>    m_resourceDescriptors;

    // UI
    std::unique_ptr<DirectX::SpriteBatch>       m_batch;
    std::unique_ptr<DirectX::SpriteFont>        m_font;
    std::unique_ptr<DirectX::SpriteFont>        m_smallFont;
    std::unique_ptr<DirectX::SpriteFont>        m_ctrlFont;

    Microsoft::WRL::ComPtr<ID3D12Resource>      m_background;

    enum Descriptors : size_t
    {
        PrintFont,
        TextFont,
        ControllerFont,
        Background,
        Count,
    };
};
