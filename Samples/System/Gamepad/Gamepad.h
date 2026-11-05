//--------------------------------------------------------------------------------------
// Gamepad.h
//
// GameInput gamepad sample state and interface.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GamepadHaptics.h"
#include "GamepadLighting.h"
#include "GamepadVibration.h"
#include "HapticsManager/HapticsManager.h"

//--------------------------------------------------------------------------------------
// State associated with one connected gamepad. GameInput device pointers have stable
// identity and can be compared directly.
//--------------------------------------------------------------------------------------
struct GamepadDevice
{
    Microsoft::WRL::ComPtr<IGameInputDevice> device;

    std::string displayName;                      // Cached from IGameInputDevice::GetDisplayName at connect
    std::string pnpPath;                          // Cached from IGameInputDevice::GetPnpPath at connect

    GameInputGamepadInfo  deviceInfo     = {};    // Gamepad-specific info
    GameInputGamepadState gamepadState   = {};    // Buttons, triggers, thumbsticks
    GameInputSensorsState sensorsState   = {};    // Accelerometer, gyroscope, orientation
    GameInputSystemButtons systemButtons = {};    // System buttons (Guide, Share)

    bool hasGamepadState  = false;
    bool hasSensorsState  = false;
    bool hasSystemButtonState = false;

    uint64_t readingTimestamp = 0;                // Microseconds (from IGameInputReading::GetTimestamp)
    size_t selectedMediaIndex = 0;
    VibrationEffectState vibEffectState;
    LightingEffectState lightingEffectState;
};

//--------------------------------------------------------------------------------------
// Sample class
//--------------------------------------------------------------------------------------
class Sample
{
public:
    Sample() = default;
    ~Sample() = default;

    Sample(Sample const&) = delete;
    Sample& operator= (Sample const&) = delete;

    void Initialize(HWND hWnd, ImGuiAtg::DeviceContext* deviceContext);
    void Update();
    void Draw();
    void Shutdown();
    void Activated();
    void Deactivated();
    LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

#ifdef _GAMING_XBOX
    // PLM suspend/resume handling for Xbox console
    void Suspend(ImGuiAtg::DeviceContext* deviceContext);
    void Resume(ImGuiAtg::DeviceContext* deviceContext);
#endif

private:
    // Gamepad.cpp
    void DrawDeviceInfoSection(const GamepadDevice& gamepad);

    static std::string GetDeviceDisplayName(IGameInputDevice* device);
    static std::string GetDevicePnpPath(IGameInputDevice* device);
    static const char* GameInputFamilyToString(GameInputDeviceFamily family);
    static std::string GameInputKindToString(GameInputKind kind);
    static std::string GameInputLightingGroupsToString(GameInputLightingGroups groups);
    void ToggleGamepadNavigation();
    void HandleSampleInput();
    static void CALLBACK OnDeviceChanged(GameInputCallbackToken token, void* context, GameInputDeviceEvent deviceEvent, IGameInputDevice* device) noexcept;

    Microsoft::WRL::ComPtr<IGameInput> m_gameInput;         // Main GameInput interface
    GameInputCallbackToken m_deviceCallbackToken = 0;       // Token for device callback
    std::mutex m_gamepadsMutex;                             // Synchronization for m_gamepads
    std::vector<GamepadDevice> m_gamepads;                  // All tracked gamepads
    int m_selectedGamepad = 0;                              // Currently viewed tab
    int m_requestedGamepadTab = -1;                         // Tab selected by a gamepad shortcut
#ifndef _GAMING_XBOX
    bool m_enableBackgroundInput = false;                   // Receive input while the window is unfocused (PC only)
#endif

    // GamepadReadings.cpp
    void PollGamepadReading(GamepadDevice& gamepad);
    bool RegisterReadingCallbacks();
    void UnregisterReadingCallbacks();
    void DrawButtonsSection(const GamepadDevice& gamepad);
    void DrawAnalogSection(const GameInputGamepadState& state);
    void DrawSensorsSection(const GamepadDevice& gamepad);

    // GamepadLighting.cpp
    void UpdateLighting(GamepadDevice& gamepad);
    void RestoreSystemLighting();
    void DrawLightingSection(GamepadDevice& gamepad);

    static void CALLBACK OnReadingChanged(GameInputCallbackToken token, void* context, IGameInputReading* reading) noexcept;

    GameInputCallbackToken m_readingCallbackToken = 0;      // Token for reading callback
    bool m_useCallbackMode = false;                         // false = polling, true = callback-driven

    // GamepadVibration.cpp
    void InitializeVibrationEffect(GamepadDevice& gamepad);
    void UpdateVibration(GamepadDevice& gamepad);
    void StopVibration();
    void DrawVibrationSection(GamepadDevice& gamepad);

    uint64_t m_perfFrequency = 0;                           // QueryPerformanceFrequency result

    // GamepadHaptics.cpp
    void InitializeHaptics();
    void DrawHapticsSection(GamepadDevice& gamepad);
#ifndef _GAMING_XBOX
    std::wstring OpenFileDialog(const wchar_t* filter);
#endif

    std::unique_ptr<ATG::HapticsManager> m_hapticsManager;  // Manages haptic audio endpoints
    std::vector<MediaItem> m_mediaList;                     // Available WAV effects
    HWND m_hWnd = nullptr;

    // GamepadModel.cpp
    void InitializeModelRenderer(ImGuiAtg::DeviceContext* deviceContext);
    void RenderModel(const GameInputSensorsState& sensors);
    void DrawModelSection(const GamepadDevice& gamepad);

    ImGuiAtg::ModelViewer m_modelViewer;
};
