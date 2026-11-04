//--------------------------------------------------------------------------------------
// GamepadLighting.cpp
//
// Gamepad lighting UI and color control.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Gamepad.h"

//--------------------------------------------------------------------------------------
// GameInput v4 lighting overview
//
// A device that supports lighting reports GameInputKindLighting from GetSupportedInput().
// Its LEDs are organized into lighting groups (thumbsticks, D-pad, face buttons, center
// light, etc.), reported as flags by GetSupportedLightingGroups(). A group can contain
// more than one LED; GetSupportedLightingGroupSize() returns how many.
//
// Changing colors is a two-step process:
//   1. SetLightingColor() stages a color for one or more groups. The optional
//      groupOrderIndex targets a single LED within a group; the default
//      (GAMEINPUT_LIGHTING_GROUP_ORDER_INDEX_ALL) sets every LED in the group.
//   2. ApplyLightingColors() sends all staged colors to the device at once.
//
// ApplyLightingSystemDefault() returns control of the lights to the system, restoring
// the device's default lighting. Call it when the title no longer wants to own the
// lights, such as on shutdown or suspend.
//--------------------------------------------------------------------------------------
namespace
{
    constexpr const char* c_lightingEffectNames[] = {
        "Manual",
        "Breathing",
        "Color Cycle"
    };

    // Duration of one Breathing or Color Cycle loop.
    constexpr double c_lightingEffectPeriodSeconds = 3.0;
}

//--------------------------------------------------------------------------------------
// Apply the selected lighting effect to the gamepad's supported lighting groups.
//--------------------------------------------------------------------------------------
void Sample::UpdateLighting(GamepadDevice& gamepad)
{
    if (!(gamepad.device->GetSupportedInput() & GameInputKindLighting))
        return;

    auto& lighting = gamepad.lightingEffectState;
    if (!lighting.enabled)
        return;

    float color[3] = { lighting.color[0], lighting.color[1], lighting.color[2] };

    // Position within the current effect loop, in the range [0, 1).
    const float phase = static_cast<float>(std::fmod(ImGui::GetTime(), c_lightingEffectPeriodSeconds) / c_lightingEffectPeriodSeconds);

    switch (lighting.selectedEffect)
    {
        case LightingEffect::Manual:
            break;

        case LightingEffect::ColorCycle:
            // Sweep the hue around the color wheel at full saturation and brightness.
            ImGui::ColorConvertHSVtoRGB(phase, 1.0f, 1.0f, color[0], color[1], color[2]);
            break;

        case LightingEffect::Breathing:
        {
            // Fade the selected color from full brightness to off and back using a cosine wave.
            const float brightness = 0.5f + 0.5f * std::cos(phase * DirectX::XM_2PI);
            for (auto& channel : color)
                channel *= brightness;
            break;
        }

        case LightingEffect::Count:
            return;
    }

    // Convert [0, 1] channels to the rounded [0, 255] range used by GameInputColor.
    GameInputColor giColor;
    giColor.r = static_cast<uint8_t>(color[0] * 255.0f + 0.5f);
    giColor.g = static_cast<uint8_t>(color[1] * 255.0f + 0.5f);
    giColor.b = static_cast<uint8_t>(color[2] * 255.0f + 0.5f);

    // Stage the same color for every supported group, then send it to the device.
    gamepad.device->SetLightingColor(giColor, GameInputLightingGroupAll);
    gamepad.device->ApplyLightingColors();
}

//--------------------------------------------------------------------------------------
// Return lighting control to the system on every connected gamepad without changing
// the selected effects. The selected effect resumes on the next UpdateLighting.
//--------------------------------------------------------------------------------------
void Sample::RestoreSystemLighting()
{
    std::lock_guard<std::mutex> lock(m_gamepadsMutex);

    for (auto& gamepad : m_gamepads)
    {
        if (!(gamepad.device->GetSupportedInput() & GameInputKindLighting))
            continue;

        gamepad.device->ApplyLightingSystemDefault();
    }
}

//--------------------------------------------------------------------------------------
// Lighting effect selection and controls.
//--------------------------------------------------------------------------------------
void Sample::DrawLightingSection(GamepadDevice& gamepad)
{
    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Leaf))
    {
        if (!(gamepad.device->GetSupportedInput() & GameInputKindLighting))
        {
            ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Lighting not supported by this device.");
            return;
        }

        auto& lighting = gamepad.lightingEffectState;

        ImGuiAtg::BeginNavigationGroup("Lighting");

        if (ImGui::Checkbox("Enable lighting effects", &lighting.enabled) && !lighting.enabled)
        {
            // Hand the lights back to the system until effects are re-enabled.
            gamepad.device->ApplyLightingSystemDefault();
        }

        ImGui::BeginDisabled(!lighting.enabled);
        ImGui::Text("Select a lighting effect:");
        ImGui::Spacing();

        int index = static_cast<int>(lighting.selectedEffect);
        if (ImGui::Combo("##LightingEffect", &index, c_lightingEffectNames, static_cast<int>(LightingEffect::Count)))
        {
            lighting.selectedEffect = static_cast<LightingEffect>(index);
        }

        if (lighting.selectedEffect != LightingEffect::ColorCycle)
        {
            ImGui::ColorEdit3("Select Color", lighting.color, ImGuiColorEditFlags_NoInputs);
        }

        ImGui::EndDisabled();
        ImGuiAtg::EndNavigationGroup();
    }
}
