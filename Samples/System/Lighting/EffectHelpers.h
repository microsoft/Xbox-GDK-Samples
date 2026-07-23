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

#include <cmath>

namespace EffectHelpers
{
    constexpr double c_Pi = 3.14159265358979323846;
    constexpr double c_TwoPi = 2.0 * c_Pi;
    constexpr double c_TwoPiOverThree = c_TwoPi / 3.0;
    constexpr double c_FourPiOverThree = 2.0 * c_TwoPi / 3.0;

    //----------------------------------------------------------------------------------
    // Color helpers
    //----------------------------------------------------------------------------------

    // Maps an angle to an RGB color by sampling a sine wave on each channel with
    // a 120-degree phase offset, producing a smooth rainbow as the angle sweeps.
    inline LampArrayColor ColorForAngle(double angle)
    {
        LampArrayColor color = {};
        color.a = 0xFF;

        // sin() is in [-1, 1]; remap to [0, 255] for each channel.
        color.r = static_cast<uint8_t>(0xFF * ((std::sin(angle) + 1.0) / 2.0));
        color.g = static_cast<uint8_t>(0xFF * ((std::sin(angle + c_TwoPiOverThree) + 1.0) / 2.0));
        color.b = static_cast<uint8_t>(0xFF * ((std::sin(angle + c_FourPiOverThree) + 1.0) / 2.0));
        return color;
    }

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

    // Maps a health fraction [0, 1] to a status color: green at full health,
    // through yellow at the midpoint, to red as it approaches zero.
    inline LampArrayColor HealthColor(float health)
    {
        LampArrayColor color = {};
        color.a = 0xFF;

        if (health >= 0.5f)
        {
            // Green to yellow over the upper half: drop red in as health falls.
            const float t = (health - 0.5f) * 2.0f;     // 1 at full health, 0 at the midpoint
            color.r = static_cast<uint8_t>(0xFF * (1.0f - t));
            color.g = 0xFF;
        }
        else
        {
            // Yellow to red over the lower half: drain green out as health falls.
            const float t = health * 2.0f;              // 1 at the midpoint, 0 at empty
            color.r = 0xFF;
            color.g = static_cast<uint8_t>(0xFF * t);
        }
        return color;
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

    // Maximum time an effect will advance in a single call. Clamping keeps a long
    // stall (a breakpoint, a window drag, or a device resuming after being
    // unavailable) from making an animation jump by a large, visible amount.
    constexpr double c_MaxDeltaSeconds = 0.1;

    // Real wall-clock time elapsed since this device's effect last ran, so each
    // effect times itself independent of the frame rate. The first call for a
    // device -- and the first after a reset, which zeroes lastCounter -- reports no
    // elapsed time and just seeds the counter.
    inline double TickDelta(LampArrayDevice& device)
    {
        static const double secondsPerCount = []()
        {
            LARGE_INTEGER frequency = {};
            QueryPerformanceFrequency(&frequency);
            return 1.0 / static_cast<double>(frequency.QuadPart);
        }();

        LARGE_INTEGER counter = {};
        QueryPerformanceCounter(&counter);
        const uint64_t now = static_cast<uint64_t>(counter.QuadPart);

        double deltaSeconds = 0.0;
        if (device.lastCounter != 0)
        {
            deltaSeconds = static_cast<double>(now - device.lastCounter) * secondsPerCount;
            if (deltaSeconds > c_MaxDeltaSeconds)
            {
                deltaSeconds = c_MaxDeltaSeconds;
            }
        }
        device.lastCounter = now;
        return deltaSeconds;
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
        device.lastCounter = 0;
        device.effectChanged = false;
    }
}
