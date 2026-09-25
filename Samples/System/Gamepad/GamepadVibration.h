//--------------------------------------------------------------------------------------
// GamepadVibration.h
//
// Vibration effect types and per-gamepad effect state.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <GameInput.h>

// GameInputRumbleParams has four independent motor channels:
//   lowFrequency  - left vibration motor (larger, lower pitch), range 0.0-1.0
//   highFrequency - right vibration motor (smaller, higher pitch), range 0.0-1.0
//   leftTrigger   - left impulse trigger motor (adaptive trigger), range 0.0-1.0
//   rightTrigger  - right impulse trigger motor (adaptive trigger), range 0.0-1.0
enum class VibrationEffect
{
    Manual = 0,     // Direct slider control of all four motors
    ImpulseTest,    // Trigger position directly drives corresponding motors
    FlatTire,       // Rhythmic pulse on left trigger (driving over bumps)
    GunWithRecoil,  // Trigger-activated shot with motor recoil kick
    Heartbeat,      // Alternating left/right trigger pulses (~60 BPM)
    Footsteps,      // Alternating left/right trigger thumps
    Count
};

struct VibrationEffectState
{
    enum class GunState
    {
        WaitForRelease,
        WaitForPress,
        Playing
    };

    VibrationEffect selectedEffect = VibrationEffect::Manual;

    uint32_t leftTriggerIndex = 0;
    uint32_t rightTriggerIndex = 0;

    uint64_t leftTriggerNextTime = 0;
    uint64_t rightTriggerNextTime = 0;

    GunState gunState = GunState::WaitForRelease;

    // Values passed directly to SetRumbleState each frame.
    GameInput::v3::GameInputRumbleParams rumble = {};

    static constexpr int HistorySize = 180;  // ~3 seconds at 60fps
    float lowFreqHistory[HistorySize] = {};
    float highFreqHistory[HistorySize] = {};
    float leftTriggerHistory[HistorySize] = {};
    float rightTriggerHistory[HistorySize] = {};
    int historyOffset = 0;  // Write position in ring buffer
};
