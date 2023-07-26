//--------------------------------------------------------------------------------------
// Lighting.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "TextConsole.h"

#include <LampArray.h>
#include <map>

using Microsoft::WRL::ComPtr;

namespace
{
    static constexpr uint32_t NUM_EFFECTS = 6;
    enum LightingEffect
    {
        ColorCycle,
        ColorWave,
        ColorWheel,
        Blink,
        WASD,
        Solid,
    };
};

// A struct that holds information regarding a LampArray device for quick access
typedef struct LampArrayContext
{
    ComPtr<ILampArray> lampArray;
    std::unique_ptr<LampArrayColor> lampColors;
    std::unique_ptr<uint32_t> lampIndices;

    std::map<uint32_t, double> lampXPositions;
    std::map<uint32_t, double> lampWheelAngles;

    LampArrayColor lastRandomColor = {};
    LampArrayPosition centerPoint = {};

    uint32_t frameCount = 0;
} LampArrayContext;

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
    std::unique_ptr<DX::DeviceResources>            m_deviceResources;

    // Rendering loop timer.
    uint64_t                                        m_frame;
    DX::StepTimer                                   m_timer;

    // Input devices.
    std::unique_ptr<DirectX::GamePad>               m_gamePad;
    std::unique_ptr<DirectX::Keyboard>              m_keyboard;

    DirectX::GamePad::ButtonStateTracker            m_gamePadButtons;
    DirectX::Keyboard::KeyboardStateTracker         m_keyboardButtons;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>        m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>        m_resourceDescriptors;

    // UI
    std::unique_ptr<DX::TextConsoleImage>           m_log;
    std::unique_ptr<DirectX::SpriteBatch>           m_spriteBatch;
    std::unique_ptr<DirectX::SpriteFont>            m_ctrlFont;
    std::unique_ptr<DirectX::SpriteFont>            m_font;

    // LampArray
    LampArrayCallbackToken                          m_callbackToken{};
    std::vector<std::shared_ptr<LampArrayContext>>  m_lampArrays;
    LightingEffect                                  m_currentLightingEffect;
    bool                                            m_effectChanged;

    static void LampArrayCallback(void* context, bool isAttached, ILampArray* pLampArray);

    void PreviousEffect()
    {
        uint32_t index = static_cast<uint32_t>(m_currentLightingEffect);
        if(index == 0)
            return;

        m_currentLightingEffect = static_cast<LightingEffect>((index - 1) % NUM_EFFECTS);
        m_effectChanged = true;
    }

    void NextEffect()
    {
        uint32_t index = static_cast<uint32_t>(m_currentLightingEffect);
        if(index == NUM_EFFECTS-1)
            return;

        m_currentLightingEffect = static_cast<LightingEffect>((static_cast<uint32_t>(m_currentLightingEffect) + 1) % NUM_EFFECTS);
        m_effectChanged = true;
    }

    // Resource descriptors
    enum Descriptors
    {
        Font,
        ConsoleFont,
        ControllerFont,
        Background,
        ConsoleBackground,
        Reserve,
        Count = 32,
    };
};
