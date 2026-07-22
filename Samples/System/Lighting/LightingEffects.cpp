//--------------------------------------------------------------------------------------
// LightingEffects.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "LightingEffects.h"
#include "EffectHelpers.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <vector>

// The color and timing utilities the effects are built from live in
// EffectHelpers.h; pulling them in here lets each effect below read as a
// self-contained technique.
using namespace EffectHelpers;

namespace
{
    // Effect metadata, indexed by the Effect enum
    using LightingEffects::EffectInfo;

    constexpr EffectInfo c_Effects[] =
    {
        { Effect::ColorCycle, "Color Cycle", "Uniformly cycles colors across all lamps.",                             { 0, { nullptr, nullptr },    true  } },
        { Effect::ColorWave,  "Color Wave",  "Rainbow that scrolls horizontally across the device.",                  { 0, { nullptr, nullptr },    true  } },
        { Effect::ColorWheel, "Color Wheel", "Rainbow that rotates around the center of the device.",                 { 0, { nullptr, nullptr },    true  } },
        { Effect::Blink,      "Blink",       "Fades a chosen color off and on across all lamps.",                     { 1, { "Color", nullptr },    true  } },
        { Effect::WASD,       "WASD",        "Highlights the W/A/S/D keys against a colored keyboard.",               { 2, { "Base", "Highlight" }, false } },
        { Effect::Purposes,   "Purposes",    "Colors each lamp by its declared purpose (control, accent, etc.).",     { 0, { nullptr, nullptr },    false } },
        { Effect::Solid,      "Solid",       "A single solid color you choose.",                                      { 1, { "Color", nullptr },    false } },
        { Effect::Health,     "Health",      "Green-to-red health bar that pulses when health is low.",               { 0, { nullptr, nullptr },    true  } },  // speed sets the low-health pulse rate
        { Effect::Ripple,     "Ripple",      "Rings of color expand outward from the center of the device.",          { 1, { "Color", nullptr },    true  } },
        { Effect::Reactive,   "Reactive",    "Lights each key as you press it, then fades it out (keyboards only).",  { 1, { "Color", nullptr },    false } },
        { Effect::Manual,     "Manual",      "Set each lamp to its own color.",                                       { 0, { nullptr, nullptr },    false } },
    };

    //==================================================================================
    // Lighting effects
    //
    // Each routine below is one self-contained technique. It reads the device's
    // cached lamp geometry and per-effect state, computes colors, and pushes them to
    // the LampArray. Animated effects use WrapTime/TickDelta; one-shot effects are
    // applied once by Run(). Any tuning constants sit directly above their effect.
    //==================================================================================

    //----------------------------------------------------------------------------------
    // Color Cycle
    //
    // Uniformly cycles a rainbow across every lamp on the device.
    //----------------------------------------------------------------------------------

    constexpr double c_ColorCyclePeriod = 10.0;     // seconds per full rainbow loop

    void ColorCycle(LampArrayDevice& device)
    {
        WrapTime(device, c_ColorCyclePeriod);

        const double angle = c_TwoPi * device.elapsedSeconds / c_ColorCyclePeriod;
        device.lampArray->SetColor(ColorForAngle(angle));
    }

    //----------------------------------------------------------------------------------
    // Color Wave
    //
    // A rainbow that scrolls horizontally: each lamp's hue is offset by its
    // normalized X position, so the colors appear to travel across the device.
    //----------------------------------------------------------------------------------

    constexpr double c_ColorWavePeriod = 2.0;       // seconds per full scroll

    void ColorWave(LampArrayDevice& device)
    {
        WrapTime(device, c_ColorWavePeriod);

        const double angle = c_TwoPi * device.elapsedSeconds / c_ColorWavePeriod;
        for (uint32_t i = 0; i < device.lampCount; ++i)
        {
            const double angleOffset = c_TwoPi * device.normalizedX[i];
            device.lampColors[i] = ColorForAngle(angle + angleOffset);
        }

        device.lampArray->SetColorsForIndices(device.lampCount, device.lampIndices.get(), device.lampColors.get());
    }

    //----------------------------------------------------------------------------------
    // Color Wheel
    //
    // A rainbow that rotates around the center of the device: each lamp's hue is
    // offset by its angle relative to the device center.
    //----------------------------------------------------------------------------------

    constexpr double c_ColorWheelPeriod = 2.0;      // seconds per full rotation

    void ColorWheel(LampArrayDevice& device)
    {
        WrapTime(device, c_ColorWheelPeriod);

        const double angle = c_TwoPi * device.elapsedSeconds / c_ColorWheelPeriod;
        for (uint32_t i = 0; i < device.lampCount; ++i)
        {
            device.lampColors[i] = ColorForAngle(angle + device.wheelAngle[i]);
        }

        device.lampArray->SetColorsForIndices(device.lampCount, device.lampIndices.get(), device.lampColors.get());
    }

    //----------------------------------------------------------------------------------
    // Blink
    //
    // Fades the chosen color in and out across all lamps using a linear
    // attack/sustain/decay/delay envelope across c_BlinkPeriod seconds.
    //----------------------------------------------------------------------------------

    constexpr double c_BlinkPeriod = 1.0;
    constexpr double c_BlinkAttackEnd = c_BlinkPeriod / 4.0;         // fade in until here
    constexpr double c_BlinkSustainEnd = c_BlinkPeriod / 2.0;        // hold full color until here
    constexpr double c_BlinkDecayEnd = 3.0 * c_BlinkPeriod / 4.0;    // fade out until here, then off

    void Blink(LampArrayDevice& device)
    {
        WrapTime(device, c_BlinkPeriod);

        const double elapsed = device.elapsedSeconds;
        const LampArrayColor base = ToColor(device.colors[static_cast<size_t>(Effect::Blink)][0]);
        LampArrayColor color = {};
        color.a = 0xFF;

        if (elapsed < c_BlinkAttackEnd)
        {
            // Attack: scale up from black to the full color.
            const float t = static_cast<float>(elapsed / c_BlinkAttackEnd);
            color.r = static_cast<uint8_t>(base.r * t);
            color.g = static_cast<uint8_t>(base.g * t);
            color.b = static_cast<uint8_t>(base.b * t);
            device.lampArray->SetColor(color);
        }
        else if (elapsed < c_BlinkSustainEnd)
        {
            // Sustain: hold the full color.
            device.lampArray->SetColor(base);
        }
        else if (elapsed < c_BlinkDecayEnd)
        {
            // Decay: scale back down to black.
            const float t = static_cast<float>((c_BlinkDecayEnd - elapsed) /
                            (c_BlinkDecayEnd - c_BlinkSustainEnd));
            color.r = static_cast<uint8_t>(base.r * t);
            color.g = static_cast<uint8_t>(base.g * t);
            color.b = static_cast<uint8_t>(base.b * t);
            device.lampArray->SetColor(color);
        }
        else
        {
            // Delay: lamps off until the envelope repeats.
            device.lampArray->SetColor(color);
        }
    }

    //----------------------------------------------------------------------------------
    // WASD
    //
    // Lights the whole keyboard with the base color and the W/A/S/D keys with the
    // highlight color. Only meaningful for devices that expose scan codes, which
    // the caller has already verified.
    //----------------------------------------------------------------------------------

    constexpr uint32_t c_ScanCodeW = 0x11;
    constexpr uint32_t c_ScanCodeA = 0x1E;
    constexpr uint32_t c_ScanCodeS = 0x1F;
    constexpr uint32_t c_ScanCodeD = 0x20;

    void WASD(LampArrayDevice& device)
    {
        const LampArrayColor base = ToColor(device.colors[static_cast<size_t>(Effect::WASD)][0]);
        device.lampArray->SetColor(base);

        const LampArrayColor highlight = ToColor(device.colors[static_cast<size_t>(Effect::WASD)][1]);
        const uint32_t scanCodes[] = { c_ScanCodeW, c_ScanCodeA, c_ScanCodeS, c_ScanCodeD };
        const LampArrayColor colors[] = { highlight, highlight, highlight, highlight };

        device.lampArray->SetColorsForScanCodes(static_cast<uint32_t>(std::size(scanCodes)), scanCodes, colors);
    }

    //----------------------------------------------------------------------------------
    // Solid
    //
    // Lights every lamp with a single chosen color.
    //----------------------------------------------------------------------------------

    void Solid(LampArrayDevice& device)
    {
        device.lampArray->SetColor(ToColor(device.colors[static_cast<size_t>(Effect::Solid)][0]));
    }

    //----------------------------------------------------------------------------------
    // Purposes
    //
    // Colors each lamp by its declared role. A lamp can be tagged with one or more
    // LampPurposes (control keys, accent lighting, status indicators, etc.); this
    // asks the device for the lamps serving each purpose and tints them, addressing
    // lamps by role instead of by raw index.
    //----------------------------------------------------------------------------------

    void Purposes(LampArrayDevice& device)
    {
        // Start from all-off so lamps with no matching purpose stay dark.
        device.lampArray->SetColor(LampArrayColor{});

        struct PurposeColor
        {
            LampPurposes purpose;
            LampArrayColor color;
        };

        const PurposeColor purposeColors[] =
        {
            { LampPurposes::Control,      { 0x00, 0xFF, 0x00, 0xFF } },   // green
            { LampPurposes::Accent,       { 0xFF, 0x00, 0xFF, 0xFF } },   // magenta
            { LampPurposes::Branding,     { 0xFF, 0x80, 0x00, 0xFF } },   // orange
            { LampPurposes::Status,       { 0xFF, 0xFF, 0x00, 0xFF } },   // yellow
            { LampPurposes::Illumination, { 0xFF, 0xFF, 0xFF, 0xFF } },   // white
            { LampPurposes::Presentation, { 0x00, 0x80, 0xFF, 0xFF } },   // blue
        };

        for (const PurposeColor& entry : purposeColors)
        {
            // GetIndicesCountForPurposes / GetIndicesForPurposes return the lamps
            // serving a purpose; tint them with SetColorsForIndices.
            const uint32_t count = device.lampArray->GetIndicesCountForPurposes(entry.purpose);
            if (count == 0)
            {
                continue;
            }

            std::vector<uint32_t> indices(count);
            device.lampArray->GetIndicesForPurposes(entry.purpose, count, indices.data());

            std::vector<LampArrayColor> colors(count, entry.color);
            device.lampArray->SetColorsForIndices(count, indices.data(), colors.data());
        }
    }

    //----------------------------------------------------------------------------------
    // Health
    //
    // Visualizes simulated health on the device. Above the critical threshold, a
    // multi-lamp device shows a bar that fills in proportion to health (green at
    // full, red as it drains) and a single-lamp device shows that color; at or below
    // the threshold the whole device pulses red as a low-health warning.
    //----------------------------------------------------------------------------------

    // Health at or below c_LowHealthThreshold is critical; c_HealthPulsePeriod is
    // one full dim-to-bright-to-dim pulse, scaled by the device's speed setting.
    constexpr double c_LowHealthThreshold = 0.25;
    constexpr double c_HealthPulsePeriod = 0.8;

    void Health(LampArrayDevice& device)
    {
        WrapTime(device, c_HealthPulsePeriod);

        // Critical health: pulse every lamp red as a heartbeat warning.
        if (device.health <= static_cast<float>(c_LowHealthThreshold))
        {
            const double phase = c_TwoPi * device.elapsedSeconds / c_HealthPulsePeriod;
            const double intensity = 0.3 + 0.7 * (0.5 + 0.5 * std::sin(phase));
            device.lampArray->SetColor(ScaleColor(LampArrayColor{ 0xFF, 0x00, 0x00, 0xFF }, intensity));
            return;
        }

        const LampArrayColor color = HealthColor(device.health);

        if (device.lampCount <= 1)
        {
            // One lamp can only show an aggregate status color, not a bar.
            device.lampArray->SetColor(color);
            return;
        }

        // Light the first round(health * lampCount) lamps in left-to-right order
        // and leave the rest dark, so the bar degrades one lamp at a time and reads
        // as a coarse bar even on a two-lamp device.
        const uint32_t litCount = static_cast<uint32_t>(std::lround(device.health * device.lampCount));
        const LampArrayColor off = {};
        for (uint32_t k = 0; k < device.lampCount; ++k)
        {
            device.lampColors[device.xOrder[k]] = (k < litCount) ? color : off;
        }

        device.lampArray->SetColorsForIndices(device.lampCount, device.lampIndices.get(), device.lampColors.get());
    }

    //----------------------------------------------------------------------------------
    // Manual
    //
    // Applies the per-lamp colors chosen in the device panel, driving each lamp
    // independently with SetColorsForIndices.
    //----------------------------------------------------------------------------------

    void Manual(LampArrayDevice& device)
    {
        if (device.manualColors.size() < device.lampCount)
        {
            return;
        }

        for (uint32_t i = 0; i < device.lampCount; ++i)
        {
            device.lampColors[i] = ToColor(device.manualColors[i].data());
        }

        device.lampArray->SetColorsForIndices(device.lampCount, device.lampIndices.get(), device.lampColors.get());
    }

    //----------------------------------------------------------------------------------
    // Ripple
    //
    // Expands a ring of the chosen color outward from the device center: a lamp
    // brightens as the ring front passes its radius and dims behind it. The radial
    // counterpart to Color Wave (linear) and Color Wheel (angular), using each
    // lamp's cached normalized distance from center.
    //----------------------------------------------------------------------------------

    // The ring sweeps center-to-edge once per c_RipplePeriod seconds;
    // c_RippleBandWidth is the bright ring's half-width in normalized radius.
    constexpr double c_RipplePeriod = 1.5;
    constexpr double c_RippleBandWidth = 0.30;

    void Ripple(LampArrayDevice& device)
    {
        WrapTime(device, c_RipplePeriod);

        const LampArrayColor color = ToColor(device.colors[static_cast<size_t>(Effect::Ripple)][0]);
        const double front = device.elapsedSeconds / c_RipplePeriod;    // 0 (center) to 1 (edge)

        for (uint32_t i = 0; i < device.lampCount; ++i)
        {
            const double distance = std::abs(device.radius[i] - front);
            const double intensity = (distance < c_RippleBandWidth)
                ? (1.0 - distance / c_RippleBandWidth) : 0.0;
            device.lampColors[i] = ScaleColor(color, intensity);
        }

        device.lampArray->SetColorsForIndices(device.lampCount, device.lampIndices.get(), device.lampColors.get());
    }

    //----------------------------------------------------------------------------------
    // Reactive
    //
    // Lights each key as it is pressed and fades it out, the trail effect common on
    // gaming keyboards. WndProcHandler sets a pressed key's intensity to 1.0; here
    // each intensity fades toward 0 and the still-glowing keys are lit by scan code
    // over an unlit base.
    //----------------------------------------------------------------------------------

    constexpr double c_ReactiveFadeSeconds = 0.6;   // full brightness to off

    void Reactive(LampArrayDevice& device, double deltaSeconds)
    {
        // Start from an unlit keyboard so only recently pressed keys glow.
        device.lampArray->SetColor(LampArrayColor{});

        const LampArrayColor trail = ToColor(device.colors[static_cast<size_t>(Effect::Reactive)][0]);
        const float fade = static_cast<float>(deltaSeconds / c_ReactiveFadeSeconds);

        uint32_t scanCodes[256] = {};
        LampArrayColor colors[256] = {};
        uint32_t litCount = 0;

        for (uint32_t scanCode = 0; scanCode < std::size(device.keyIntensity); ++scanCode)
        {
            float intensity = device.keyIntensity[scanCode];
            if (intensity <= 0.0f)
            {
                continue;
            }

            intensity = (std::max)(0.0f, intensity - fade);
            device.keyIntensity[scanCode] = intensity;

            if (intensity > 0.0f)
            {
                scanCodes[litCount] = scanCode;
                colors[litCount] = ScaleColor(trail, intensity);
                ++litCount;
            }
        }

        if (litCount > 0)
        {
            device.lampArray->SetColorsForScanCodes(litCount, scanCodes, colors);
        }
    }
}

//==================================================================================
// Public interface: effect metadata accessors and the per-frame dispatch.
//==================================================================================

const char* LightingEffects::Name(Effect effect)
{
    return c_Effects[static_cast<size_t>(effect)].name;
}

const char* LightingEffects::Description(Effect effect)
{
    return c_Effects[static_cast<size_t>(effect)].description;
}

const LightingEffects::EffectControls& LightingEffects::Controls(Effect effect)
{
    return c_Effects[static_cast<size_t>(effect)].controls;
}

void LightingEffects::Run(LampArrayDevice& device)
{
    const bool justChanged = device.effectChanged;
    if (justChanged)
    {
        Reset(device);
    }

    // Real time elapsed since this effect last ran. Reset zeroes lastCounter, so a
    // freshly assigned effect reports zero and renders its starting phase.
    const double deltaSeconds = TickDelta(device);

    // Animated effects render at the current elapsed time, then advance it scaled by
    // the device's speed. The static effects (WASD, Solid, Purposes) only need to be
    // applied once, when first assigned (or when their color changes).
    const double scaledDelta = deltaSeconds * device.speedScale;
    switch (device.effect)
    {
    case Effect::ColorCycle:
        ColorCycle(device);
        device.elapsedSeconds += scaledDelta;
        break;

    case Effect::ColorWave:
        ColorWave(device);
        device.elapsedSeconds += scaledDelta;
        break;

    case Effect::ColorWheel:
        ColorWheel(device);
        device.elapsedSeconds += scaledDelta;
        break;

    case Effect::Blink:
        Blink(device);
        device.elapsedSeconds += scaledDelta;
        break;

    case Effect::WASD:
        if (justChanged)
        {
            WASD(device);
        }
        break;

    case Effect::Purposes:
        if (justChanged)
        {
            Purposes(device);
        }
        break;

    case Effect::Solid:
        if (justChanged)
        {
            Solid(device);
        }
        break;

    case Effect::Health:
        Health(device);
        device.elapsedSeconds += scaledDelta;
        break;

    case Effect::Manual:
        // Re-applied every frame so color-picker edits show at once.
        Manual(device);
        break;

    case Effect::Ripple:
        Ripple(device);
        device.elapsedSeconds += scaledDelta;
        break;

    case Effect::Reactive:
        // Re-applied every frame so trails fade and new presses appear; the fade
        // uses real (unscaled) time, independent of the speed setting.
        Reactive(device, deltaSeconds);
        break;

    case Effect::Count:
    default:
        break;
    }
}
