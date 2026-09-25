//--------------------------------------------------------------------------------------
// GamepadVibration.cpp
//
// Vibration UI and preset effect implementation.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Gamepad.h"

//--------------------------------------------------------------------------------------
// Effect patterns use parallel duration (milliseconds) and motor-level arrays.
//--------------------------------------------------------------------------------------
namespace
{
    constexpr const char* g_vibrationEffectNames[] = {
        "Manual",
        "Impulse Test",
        "Flat Tire",
        "Gun with Recoil",
        "Heartbeat",
        "Footsteps"
    };

    // Rhythmic bump on the left trigger.
    constexpr uint32_t g_flatTireLTDurations[] = { 33, 80, 16 };
    constexpr float    g_flatTireLTLevels[]    = { 0.8f, 0.0f, 0.0f };

    // Trigger pulse followed by a full motor kick.
    constexpr uint32_t g_gunRecoilLTDurations[] = { 20, 10, 90, 10000 };
    constexpr float    g_gunRecoilLTLevels[]    = { 1.0f, 0.0f, 0.0f, 0.0f };

    // Alternating trigger pulses at approximately 60 BPM.
    constexpr uint32_t g_heartbeatLTDurations[] = { 25, 200, 25, 10, 745 };
    constexpr float    g_heartbeatLTLevels[]    = { 0.2f, 0.0f, 0.0f, 0.0f, 0.0f };
    constexpr uint32_t g_heartbeatRTDurations[] = { 25, 200, 25, 10, 745 };
    constexpr float    g_heartbeatRTLevels[]    = { 0.0f, 0.0f, 0.2f, 0.02f, 0.0f };

    // Alternating trigger thumps.
    constexpr uint32_t g_footstepsLTDurations[] = { 25, 600, 25, 600 };
    constexpr float    g_footstepsLTLevels[]    = { 0.3f, 0.0f, 0.0f, 0.0f };
    constexpr uint32_t g_footstepsRTDurations[] = { 25, 600, 25, 600 };
    constexpr float    g_footstepsRTLevels[]    = { 0.0f, 0.0f, 0.3f, 0.0f };
}

//--------------------------------------------------------------------------------------
// Reset effect state and schedule its first step.
//--------------------------------------------------------------------------------------
void Sample::InitializeVibrationEffect(GamepadDevice& gamepad)
{
    auto& effectState = gamepad.vibEffect;
    effectState.leftTriggerIndex = 0;
    effectState.rightTriggerIndex = 0;
    effectState.rumble = {};
    effectState.gunState = VibrationEffectState::GunState::WaitForRelease;

    uint64_t counter = 0;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&counter));

    switch (effectState.selectedEffect)
    {
    case VibrationEffect::FlatTire:
        effectState.leftTriggerNextTime = counter + (m_perfFrequency * g_flatTireLTDurations[0]) / 1000;
        break;
    case VibrationEffect::Heartbeat:
        effectState.leftTriggerNextTime = counter + (m_perfFrequency * g_heartbeatLTDurations[0]) / 1000;
        effectState.rightTriggerNextTime = counter + (m_perfFrequency * g_heartbeatRTDurations[0]) / 1000;
        break;
    case VibrationEffect::Footsteps:
        effectState.leftTriggerNextTime = counter + (m_perfFrequency * g_footstepsLTDurations[0]) / 1000;
        effectState.rightTriggerNextTime = counter + (m_perfFrequency * g_footstepsRTDurations[0]) / 1000;
        break;
    case VibrationEffect::Manual:
    case VibrationEffect::ImpulseTest:
    case VibrationEffect::GunWithRecoil:
    case VibrationEffect::Count:
    default:
        break;
    }
}

//--------------------------------------------------------------------------------------
// Advance the selected effect and apply its rumble values to the device.
//
// Rumble values persist until changed; GameInput handles focus muting.
//--------------------------------------------------------------------------------------
void Sample::UpdateVibration(GamepadDevice& gamepad)
{
    auto& effectState = gamepad.vibEffect;

    if (gamepad.hasGamepadState)
    {
        auto& state = gamepad.gamepadState;
        uint64_t counter = 0;
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&counter));

        switch (effectState.selectedEffect)
        {
        case VibrationEffect::Manual:
            // Sliders set these values directly.
            break;

        case VibrationEffect::ImpulseTest:
            // Trigger positions directly drive all four channels.
            effectState.rumble.leftTrigger = state.leftTrigger;
            effectState.rumble.rightTrigger = state.rightTrigger;
            effectState.rumble.lowFrequency = state.leftTrigger;
            effectState.rumble.highFrequency = state.rightTrigger;
            break;

        case VibrationEffect::FlatTire:
            effectState.rumble.leftTrigger = g_flatTireLTLevels[effectState.leftTriggerIndex];
            if (counter > effectState.leftTriggerNextTime)
            {
                effectState.leftTriggerIndex = (effectState.leftTriggerIndex + 1) % _countof(g_flatTireLTDurations);
                effectState.leftTriggerNextTime = counter + (m_perfFrequency * g_flatTireLTDurations[effectState.leftTriggerIndex]) / 1000;
            }
            break;

        case VibrationEffect::GunWithRecoil:
            // Wait for release, wait for press, then play the recoil sequence.
            switch (effectState.gunState)
            {
            case VibrationEffectState::GunState::WaitForRelease:
                if (state.leftTrigger <= 1.0f / 255.0f)
                    effectState.gunState = VibrationEffectState::GunState::WaitForPress;
                break;
            case VibrationEffectState::GunState::WaitForPress:
                if (state.leftTrigger >= 32.0f / 255.0f)
                {
                    effectState.leftTriggerIndex = 0;
                    effectState.leftTriggerNextTime = counter + (m_perfFrequency * g_gunRecoilLTDurations[0]) / 1000;
                    effectState.gunState = VibrationEffectState::GunState::Playing;
                }
                break;
            case VibrationEffectState::GunState::Playing:
                effectState.rumble.leftTrigger = g_gunRecoilLTLevels[effectState.leftTriggerIndex];

                // Step 2 adds the recoil kick.
                if (effectState.leftTriggerIndex == 2)
                {
                    effectState.rumble.lowFrequency = 1.0f;
                    effectState.rumble.highFrequency = 1.0f;
                }
                else
                {
                    effectState.rumble.lowFrequency = 0.0f;
                    effectState.rumble.highFrequency = 0.0f;
                }

                if (effectState.leftTriggerIndex == 3)
                {
                    effectState.leftTriggerIndex = 0;
                    effectState.gunState = VibrationEffectState::GunState::WaitForRelease;
                    effectState.rumble.lowFrequency = 0;
                    effectState.rumble.highFrequency = 0;
                    effectState.rumble.leftTrigger = 0;
                    break;
                }

                if (counter > effectState.leftTriggerNextTime)
                {
                    effectState.leftTriggerIndex = (effectState.leftTriggerIndex + 1) % _countof(g_gunRecoilLTDurations);
                    effectState.leftTriggerNextTime = counter + (m_perfFrequency * g_gunRecoilLTDurations[effectState.leftTriggerIndex]) / 1000;
                }
                break;
            }
            break;

        case VibrationEffect::Heartbeat:
            // Left and right patterns advance independently.
            effectState.rumble.leftTrigger = g_heartbeatLTLevels[effectState.leftTriggerIndex];
            effectState.rumble.rightTrigger = g_heartbeatRTLevels[effectState.rightTriggerIndex];

            if (counter > effectState.leftTriggerNextTime)
            {
                effectState.leftTriggerIndex = (effectState.leftTriggerIndex + 1) % _countof(g_heartbeatLTDurations);
                effectState.leftTriggerNextTime = counter + (m_perfFrequency * g_heartbeatLTDurations[effectState.leftTriggerIndex]) / 1000;
            }
            if (counter > effectState.rightTriggerNextTime)
            {
                effectState.rightTriggerIndex = (effectState.rightTriggerIndex + 1) % _countof(g_heartbeatRTDurations);
                effectState.rightTriggerNextTime = counter + (m_perfFrequency * g_heartbeatRTDurations[effectState.rightTriggerIndex]) / 1000;
            }
            break;

        case VibrationEffect::Footsteps:
            effectState.rumble.leftTrigger = g_footstepsLTLevels[effectState.leftTriggerIndex];
            effectState.rumble.rightTrigger = g_footstepsRTLevels[effectState.rightTriggerIndex];

            if (counter > effectState.leftTriggerNextTime)
            {
                effectState.leftTriggerIndex = (effectState.leftTriggerIndex + 1) % _countof(g_footstepsLTDurations);
                effectState.leftTriggerNextTime = counter + (m_perfFrequency * g_footstepsLTDurations[effectState.leftTriggerIndex]) / 1000;
            }
            if (counter > effectState.rightTriggerNextTime)
            {
                effectState.rightTriggerIndex = (effectState.rightTriggerIndex + 1) % _countof(g_footstepsRTDurations);
                effectState.rightTriggerNextTime = counter + (m_perfFrequency * g_footstepsRTDurations[effectState.rightTriggerIndex]) / 1000;
            }
            break;

        case VibrationEffect::Count:
        default:
            break;
        }

        int historyIndex = effectState.historyOffset;
        effectState.lowFreqHistory[historyIndex] = effectState.rumble.lowFrequency;
        effectState.highFreqHistory[historyIndex] = effectState.rumble.highFrequency;
        effectState.leftTriggerHistory[historyIndex] = effectState.rumble.leftTrigger;
        effectState.rightTriggerHistory[historyIndex] = effectState.rumble.rightTrigger;
        effectState.historyOffset = (historyIndex + 1) % VibrationEffectState::HistorySize;
    }

    gamepad.device->SetRumbleState(&effectState.rumble);
}

//--------------------------------------------------------------------------------------
// Stop vibration on every connected gamepad without changing the selected effects.
//--------------------------------------------------------------------------------------
void Sample::StopVibration()
{
    std::lock_guard<std::mutex> lock(m_gamepadsMutex);

    GameInputRumbleParams stop = {};
    for (auto& gamepad : m_gamepads)
    {
        gamepad.device->SetRumbleState(&stop);
    }
}

//--------------------------------------------------------------------------------------
// Vibration effect selection and controls.
//--------------------------------------------------------------------------------------
void Sample::DrawVibrationSection(GamepadDevice& gamepad)
{
    if (ImGui::CollapsingHeader("Vibration", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
    {
        if (!gamepad.deviceInfo->supportedRumbleMotors)
        {
            ImGui::TextWrapped("No rumble motors supported by this device.");
            return;
        }

        ImGuiAtg::BeginNavigationGroup("Vibration");

        ImGui::TextWrapped("Select a vibration effect to play:");
        ImGui::Spacing();

        int effectIndex = static_cast<int>(gamepad.vibEffect.selectedEffect);
        if (ImGui::Combo("##Vibration", &effectIndex, g_vibrationEffectNames, static_cast<int>(VibrationEffect::Count)))
        {
            gamepad.vibEffect.selectedEffect = static_cast<VibrationEffect>(effectIndex);
            InitializeVibrationEffect(gamepad);
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            gamepad.vibEffect.selectedEffect = VibrationEffect::Manual;
            gamepad.vibEffect.rumble = {};
        }

        ImGui::Spacing();
        float width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        switch (gamepad.vibEffect.selectedEffect)
        {
            case VibrationEffect::Manual:
            {
                ImGui::PushItemWidth(width);
                bool hasLowFrequencyMotor = (gamepad.deviceInfo->supportedRumbleMotors & GameInputRumbleLowFrequency) != 0;
                bool hasHighFrequencyMotor = (gamepad.deviceInfo->supportedRumbleMotors & GameInputRumbleHighFrequency) != 0;
                bool hasLeftTriggerMotor = (gamepad.deviceInfo->supportedRumbleMotors & GameInputRumbleLeftTrigger) != 0;
                bool hasRightTriggerMotor = (gamepad.deviceInfo->supportedRumbleMotors & GameInputRumbleRightTrigger) != 0;

                if (hasLowFrequencyMotor)
                    ImGui::SliderFloat("##lfm", &gamepad.vibEffect.rumble.lowFrequency, 0.0f, 1.0f, "Low Freq Motor %.3f");
                if (hasLowFrequencyMotor && hasHighFrequencyMotor)
                    ImGui::SameLine();
                if (hasHighFrequencyMotor)
                    ImGui::SliderFloat("##hfm", &gamepad.vibEffect.rumble.highFrequency, 0.0f, 1.0f, "High Freq Motor %.3f");
                if (hasLeftTriggerMotor)
                    ImGui::SliderFloat("##ltvib", &gamepad.vibEffect.rumble.leftTrigger, 0.0f, 1.0f, "Left Trigger %.3f");
                if (hasLeftTriggerMotor && hasRightTriggerMotor)
                    ImGui::SameLine();
                if (hasRightTriggerMotor)
                    ImGui::SliderFloat("##rtvib", &gamepad.vibEffect.rumble.rightTrigger, 0.0f, 1.0f, "Right Trigger %.3f");
                ImGui::PopItemWidth();
                break;
            }
            case VibrationEffect::ImpulseTest:
                ImGui::TextWrapped("Pull the triggers to feel proportional feedback on both triggers and motors.");
                break;
            case VibrationEffect::FlatTire:
                ImGui::TextWrapped("Simulates a flat tire with a rhythmic bump on the left trigger.");
                break;
            case VibrationEffect::GunWithRecoil:
                ImGui::TextWrapped("Pull the left trigger to fire. Trigger pulse + motor recoil kick.");
                break;
            case VibrationEffect::Heartbeat:
                ImGui::TextWrapped("Alternating left/right trigger pulses simulating a heartbeat (~60 BPM).");
                break;
            case VibrationEffect::Footsteps:
                ImGui::TextWrapped("Alternating left/right trigger thumps simulating nearby footsteps.");
                break;
            case VibrationEffect::Count:
            default:
                break;
        }

        ImGui::Spacing();
        float graphHeight = ImGuiAtg::Scaled(30);
        int offset = gamepad.vibEffect.historyOffset;
        int count = VibrationEffectState::HistorySize;
        ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, 1.0f);
        ImGui::BeginDisabled(true);
        ImGui::PlotLines("##LowFreq",   gamepad.vibEffect.lowFreqHistory,      count, offset, "Low Frequency",  0.0f, 1.0f, ImVec2(width, graphHeight));
        ImGui::SameLine();
        ImGui::PlotLines("##HighFreq",  gamepad.vibEffect.highFreqHistory,     count, offset, "High Frequency", 0.0f, 1.0f, ImVec2(width, graphHeight));
        ImGui::PlotLines("##LeftTrig",  gamepad.vibEffect.leftTriggerHistory,  count, offset, "Left Trigger",   0.0f, 1.0f, ImVec2(width, graphHeight));
        ImGui::SameLine();
        ImGui::PlotLines("##RightTrig", gamepad.vibEffect.rightTriggerHistory, count, offset, "Right Trigger",  0.0f, 1.0f, ImVec2(width, graphHeight));
        ImGui::EndDisabled();
        ImGui::PopStyleVar();

        ImGuiAtg::EndNavigationGroup();
    }
}
