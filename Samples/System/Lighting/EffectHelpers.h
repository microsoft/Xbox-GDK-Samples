//--------------------------------------------------------------------------------------
// EffectHelpers.h
//
// Color math and per-device animation timing shared by the lighting effects. The
// effects themselves live in LightingEffects.cpp and are written in terms of these
// helpers.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Lighting.h"
#include "imgui.h"

#include <cmath>

namespace EffectHelpers
{
    constexpr double c_Pi = 3.14159265358979323846;
    constexpr double c_TwoPi = 2.0 * c_Pi;

    //----------------------------------------------------------------------------------
    // Color helpers
    //----------------------------------------------------------------------------------

    // Converts a normalized RGB triplet (as edited by the ImGui color pickers)
    // into a LampArrayColor with full alpha.
    inline LampArrayColor ToColor(const float rgb[3])
    {
        LampArrayColor color = {};
        color.a = 0xFF;
        color.r = static_cast<uint8_t>(rgb[0] * 255.0f + 0.5f);
        color.g = static_cast<uint8_t>(rgb[1] * 255.0f + 0.5f);
        color.b = static_cast<uint8_t>(rgb[2] * 255.0f + 0.5f);
        return color;
    }

    // Converts hue to a fully saturated color. Wrap spatial offsets, including
    // negative wheel angles, into [0, 1) before converting.
    inline LampArrayColor ColorForHue(double hue)
    {
        float rgb[3];
        ImGui::ColorConvertHSVtoRGB(static_cast<float>(hue - std::floor(hue)),
            1.0f, 1.0f, rgb[0], rgb[1], rgb[2]);
        return ToColor(rgb);
    }

    // Scales an RGB color by a [0, 1] intensity, preserving alpha.
    inline LampArrayColor ScaleColor(LampArrayColor color, double intensity)
    {
        color.r = static_cast<uint8_t>(color.r * intensity);
        color.g = static_cast<uint8_t>(color.g * intensity);
        color.b = static_cast<uint8_t>(color.b * intensity);
        return color;
    }

    //----------------------------------------------------------------------------------
    // Timing and state helpers
    //----------------------------------------------------------------------------------

    // Wraps the elapsed-time accumulator back into [0, period) once a full loop
    // has completed, keeping the phase bounded without losing the fractional
    // remainder.
    inline void WrapTime(LampArrayDevice& device, double period)
    {
        if (device.elapsedSeconds >= period)
        {
            device.elapsedSeconds = std::fmod(device.elapsedSeconds, period);
        }
    }

    // Clears all cached lamp colors and refreshes the per-effect animation state.
    // Called once each time an effect is (re)assigned to a device.
    inline void Reset(LampArrayDevice& device)
    {
        for (uint32_t i = 0; i < device.lampCount; ++i)
        {
            device.lampColors[i] = LampArrayColor{};
        }

        device.keyIntensity.fill(0.0f);
        device.elapsedSeconds = 0.0;
        device.effectChanged = false;
    }
}
