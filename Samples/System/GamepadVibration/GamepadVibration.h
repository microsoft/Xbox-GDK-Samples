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

// A basic sample implementation that creates a D3D12 device and
// provides a render loop.
class Sample
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

    // Initialization and management
    void Initialize(HWND window);

    // Basic render loop
    void Tick();

    // Messages
    void OnSuspending();
    void OnResuming();

    void InitializeImpulseTriggerEffects();

private:

    void Update(DX::StepTimer const& timer);
    void Render();

    void Clear();

    void CreateDeviceDependentResources();
    void CreateWindowSizeDependentResources();

    //Gamepad states
    Microsoft::WRL::ComPtr<IGameInput>					m_gameInput;
    Microsoft::WRL::ComPtr<IGameInputReading>           m_reading;
    std::vector<APP_LOCAL_DEVICE_ID>					m_deviceIds;

    bool                m_connected;
    float               m_leftMotorSpeed;
    float               m_rightMotorSpeed;
    float               m_leftTriggerLevel;
    float               m_rightTriggerLevel;
    bool                m_dPadPressed;

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

    
    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame;
    DX::StepTimer                               m_timer;

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
