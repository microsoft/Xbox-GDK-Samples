//--------------------------------------------------------------------------------------
// SimpleFFBWheel.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "DeviceResources.h"
#include "StepTimer.h"
#include "SampleGUI.h"
#include "TextConsole.h"

#include <GameInput.h>

namespace
{
    const int c_sampleUIPanel = 2000;
    const int c_valuesHeader = 2003;
    const int c_valuesLabel = 2004;
    const int c_descBox = 2006;
    const int c_speedLabel = 2007;
    const int c_StartBtn = 2101;

    static constexpr int NUM_EFFECTS = 3;

    // [SAMPLE] Our own enumeration to quickly map effect types to strings and
    // simplify some sample logic
    enum class EffectType
    {
        Spring,
        Damper,
        Gravel,
    };
    const wchar_t* g_EffectText[] = { L"Spring", L"Damper", L"SawtoothUp" };

    // [SAMPLE] A wrapper around a set of params and the resulting effect
    typedef struct SampleEffect
    {
        GameInputForceFeedbackParams params{};
        Microsoft::WRL::ComPtr<IGameInputForceFeedbackEffect> effect{};
        std::wstring name;
    } SampleEffect;

    // [SAMPLE] Used to map inputs to the state of the simulation
    typedef struct SimulationInputState
    {
        bool  Gravel;
        bool  Exit;
        float Accelerator;
        float Brake;
    } SimulationInputState;

    const wchar_t* g_DescText =
        L"Use the pedals or dpad to accelerate or brake. The wheel's force feedback (Spring and Damper) will change based on the speed of the vehicle.\n\n"
        L"Press the A button to simulate the effect of driving over a rough road or gravel (SawtoothUp).";
}

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

    // FFB
    void StartEffect(EffectType type);
    void UpdateEffect(EffectType type);
    void StopEffect(EffectType type);
    void UpdateForceFeedbackFrame();

    SampleEffect& GetSampleEffect(EffectType type)
    {
        return m_effects[static_cast<uint32_t>(type)];
    }

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

    void SetupUI();

    // GameInput methods
    static void CALLBACK OnGameInputDeviceAddedRemoved(
        _In_ GameInputCallbackToken,
        _In_ void * context,
        _In_ IGameInputDevice * device,
        _In_ uint64_t,
        _In_ GameInputDeviceStatus currentStatus,
        _In_ GameInputDeviceStatus) noexcept;
    bool GetRacingWheelButtonPressed(GameInputRacingWheelButtons button);

    // GameInput vars
    Microsoft::WRL::ComPtr<IGameInput>                      m_gameInput;
    GameInputCallbackToken                                  m_callbackToken = 0;
    Microsoft::WRL::ComPtr<IGameInputDevice>                m_wheelDevice;
    Microsoft::WRL::ComPtr<IGameInputReading>               m_reading;
    SampleEffect                                            m_effects[NUM_EFFECTS]{};
    GameInputRacingWheelState                               m_wheelState{};
    GameInputRacingWheelButtons                             m_lastRacingWheelButtons{};
    SimulationInputState                                    m_inputState{};
    float                                                   m_speed = 0;

    // Device resources.
    std::unique_ptr<DX::DeviceResources>        m_deviceResources;

    // Rendering loop timer.
    uint64_t                                    m_frame = 0;
    DX::StepTimer                               m_timer;

    // DirectXTK objects.
    std::unique_ptr<DirectX::GraphicsMemory>    m_graphicsMemory;
    std::unique_ptr<DirectX::DescriptorPile>    m_resourceDescriptors;

    // UI
    std::unique_ptr<ATG::UIManager>             m_ui;
    std::unique_ptr<DX::TextConsoleImage>       m_log;
    ATG::TextLabel*                             m_valuesHeader;
    ATG::TextLabel*                             m_valuesLabel;
    ATG::TextLabel*                             m_speedLabel;
    ATG::TextBox*                               m_descBox;
    std::unique_ptr<DirectX::SpriteBatch>       m_spriteBatch;
    Microsoft::WRL::ComPtr<ID3D12Resource>      m_wheel;
    DirectX::XMFLOAT2                           m_wheelOrigin;

    enum Descriptors
    {
        Font,
        ConsoleFont,
        Background,
        ConsoleBackground,
        SteeringWheel,
        Reserve,
        Count = 32,
    };
};
