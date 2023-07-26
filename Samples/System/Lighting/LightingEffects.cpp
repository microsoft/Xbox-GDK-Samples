//--------------------------------------------------------------------------------------
// LightingEffects.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Lighting.h"
#include "LightingEffects.h"

#include <LampArray.h>

namespace
{
    // Color Cycle effect parameters
    static constexpr uint32_t COLOR_CYCLE_EFFECT_FRAME_COUNT = 180;
    static constexpr double M_TWOPI = 2 * M_PI;
    static constexpr double M_TWOPI_THREE = M_TWOPI / 3;
    static constexpr double M_FOURPI_THREE = 2 * M_TWOPI / 3;

    // Blink effect parameters
    static constexpr uint32_t BLINK_EFFECT_FRAME_COUNT = 60;
    static constexpr uint32_t BLINK_EFFECT_RAMP_UP_MAX = BLINK_EFFECT_FRAME_COUNT / 4;
    static constexpr uint32_t BLINK_EFFECT_SOLID_MAX = BLINK_EFFECT_FRAME_COUNT / 2;
    static constexpr uint32_t BLINK_EFFECT_RAMP_DOWN_MAX = 3 * BLINK_EFFECT_FRAME_COUNT / 4;

    // Color Wave effect parameters
    static constexpr uint32_t COLOR_WAVE_EFFECT_FRAME_COUNT = 60;

    // WASD effect parameters
    static constexpr uint8_t SC_A = 0x1E;
    static constexpr uint8_t SC_D = 0x20;
    static constexpr uint8_t SC_S = 0x1F;
    static constexpr uint8_t SC_W = 0x11;

    // Color Wheel effect parameters
    static constexpr uint32_t COLOR_WHEEL_EFFECT_FRAME_COUNT = 90;
}

void LightingEffects::UpdateColorCycleEffect(LampArrayContext* device)
{
    // Reset our effect if we've gone past the repeat threshold
    if (device->frameCount >= COLOR_CYCLE_EFFECT_FRAME_COUNT)
    {
        device->frameCount = 0;
    }

    // We can create an RGB cycle effect using a sine wave.
    // First, figure out how far into our effect we are, and find a corresponding angle
    // for which we will compute our color based on its sine value.
    double angle = 2 * M_PI * (float)device->frameCount / (float)COLOR_CYCLE_EFFECT_FRAME_COUNT;

    device->lampArray->SetColor(GetLampArrayColorForAngle(angle));
    device->frameCount++;
}

void LightingEffects::UpdateBlinkEffect(LampArrayContext* device)
{
    // We can construct a blink effect by repeating these four sequences:
    // 1. Attack (ramp up from black to desired color)
    // 2. Sustain (retain full desired color)
    // 3. Decay (fade from desired color to black)
    // 4. Delay (all lamps off prior to repeating)
    //
    // Steps 1 and 3 will be done with a linear scale.
    LampArrayColor finalColor = {};
    finalColor.a = 0xFF;

    // Reset our effect if we've gone past the repeat threshold
    if (device->frameCount >= BLINK_EFFECT_FRAME_COUNT)
    {
        device->frameCount = 0;
    }

    // 1. Attack: scale our color based on how far into the first set of frames we are.
    if (device->frameCount < BLINK_EFFECT_RAMP_UP_MAX)
    {
        float multiplier = (float)(device->frameCount) / (float)BLINK_EFFECT_RAMP_UP_MAX;

        finalColor.r = (uint8_t)((float)device->lastRandomColor.r * multiplier);
        finalColor.g = (uint8_t)((float)device->lastRandomColor.g * multiplier);
        finalColor.b = (uint8_t)((float)device->lastRandomColor.b * multiplier);

        device->lampArray->SetColor(finalColor);
    }

    // 2. Sustain: use the full color
    else if (device->frameCount < BLINK_EFFECT_SOLID_MAX)
    {
        device->lampArray->SetColor(device->lastRandomColor);
    }

    // 3. Decay: scale our color based on how many frames are left in this set of frames. 
    else if (device->frameCount < BLINK_EFFECT_RAMP_DOWN_MAX)
    {
        float multiplier = (float)(BLINK_EFFECT_RAMP_DOWN_MAX - device->frameCount) /
                            (float)(BLINK_EFFECT_RAMP_DOWN_MAX - BLINK_EFFECT_SOLID_MAX);

        finalColor.r = (uint8_t)((float)device->lastRandomColor.r * multiplier);
        finalColor.g = (uint8_t)((float)device->lastRandomColor.g * multiplier);
        finalColor.b = (uint8_t)((float)device->lastRandomColor.b * multiplier);

        device->lampArray->SetColor(finalColor);
    }

    // 4. Delay: use black
    else
    {
        device->lampArray->SetColor(finalColor);
    }
}

void LightingEffects::UpdateColorWaveEffect(LampArrayContext* device)
{
    LampArrayColor finalColor = {};
    finalColor.a = 0xFF;

    // Reset our effect if we've gone past the repeat threshold
    if (device->frameCount >= COLOR_WAVE_EFFECT_FRAME_COUNT)
    {
        device->frameCount = 0;
    }

    // This effect will use the LampArray's size and lamp positions
    // to create a color wave effect. To optimize performance, we have cached each lamp's
    // X position when the device was attached.
    LampArrayPosition lampArrayBoundingBox = {};
    device->lampArray->GetBoundingBox(&lampArrayBoundingBox);

    // First, figure out how far into our effect we are, and find a corresponding angle
    // for which we will compute our color based on its sine value.
    double angle = 2 * M_PI * (float)device->frameCount / (float)COLOR_WAVE_EFFECT_FRAME_COUNT;

    // To produce the wave effect, for each lamp we'll add an offset to the angle,
    // based on the lamp's X position relative to the width of the device.
    // The offset will be in the range [0, 2pi] inclusive.
    for (uint32_t i = 0; i < device->lampArray->GetLampCount(); i++)
    {
        double angleOffset = 2 * M_PI * device->lampXPositions[i];
        device->lampColors.get()[i] = GetLampArrayColorForAngle(angle + angleOffset);
    }

    device->lampArray->SetColorsForIndices(
        device->lampArray->GetLampCount(),
        device->lampIndices.get(),
        device->lampColors.get());
}

void LightingEffects::UpdateColorWheelEffect(LampArrayContext* device)
{
    LampArrayColor finalColor = {};
    finalColor.a = 0xFF;

    // Reset our effect if we've gone past the repeat threshold
    if (device->frameCount >= COLOR_WHEEL_EFFECT_FRAME_COUNT)
    {
        device->frameCount = 0;
    }

    // This effect will use the LampArray's size and lamp positions
    // to create a color wheel effect. Each lamp's angle for the color wheel is based on its
    // X and Y position relative to the center point of the device, and these were cached
    // when the device was attached.

    // First, figure out how far into our effect we are, and find a corresponding angle
    // for which we will compute our color based on its sine value.
    double angle = 2 * M_PI * (float)device->frameCount / (float)COLOR_WHEEL_EFFECT_FRAME_COUNT;

    // To produce the color wheel effect, for each lamp we'll add an offset to the angle,
    // based on the lamp's angle relative to the center point of the device.
    for (uint32_t i = 0; i < device->lampArray->GetLampCount(); i++)
    {
        device->lampColors.get()[i] = GetLampArrayColorForAngle(angle + device->lampWheelAngles[i]);
    }

    device->lampArray->SetColorsForIndices(
        device->lampArray->GetLampCount(),
        device->lampIndices.get(),
        device->lampColors.get());

    device->frameCount++;
}

void LightingEffects::UpdateWASDEffect(LampArrayContext* device)
{
    // Set all lamps to blue, except for WASD to yellow, if supported
    LampArrayColor blueColor = { 0x00, 0x00, 0xFF, 0xFF }; // RGBA
    device->lampArray->SetColor(blueColor);

    if (device->lampArray->SupportsScanCodes())
    {
        LampArrayColor yellowColor = { 0xFF, 0xFF, 0x00, 0xFF }; // RGBA

        std::vector<uint32_t> scanCodesForWASD = { SC_W, SC_A, SC_S, SC_D };
        std::vector<LampArrayColor> colors;

        for (uint32_t i = 0; i < scanCodesForWASD.size(); i++)
        {
            colors.push_back(yellowColor);
        }

        device->lampArray->SetColorsForScanCodes(
            static_cast<uint32_t>(scanCodesForWASD.size()),
            scanCodesForWASD.data(),
            colors.data());
    }  
}

void LightingEffects::UpdateSolidEffect(LampArrayContext* device)
{
    // Set all lamps on the device to a random color.
    device->lampArray->SetColor(device->lastRandomColor);
}

void LightingEffects::ResetEffects(LampArrayContext* device)
{
    const LampArrayColor emptyColor = {};

    // Reset all lamp colors to black in preparation for new effect
    for (uint32_t i = 0; i < device->lampArray->GetLampCount(); i++)
    {
        device->lampColors.get()[i] = emptyColor;
    }

    device->lastRandomColor = GetRandomColor();
    device->frameCount = 0;
}

// This helper function is used to help make color cycles based on sine waves.
LampArrayColor LightingEffects::GetLampArrayColorForAngle(double angle)
{
    LampArrayColor color = {};
    color.a = 0xFF;

    // The sine value will be in the range of [-1, 1] inclusive.
    // Normalize that value so it falls between 0 and 1, and use it for the color's R value.
    color.r = (uint8_t)(0xFF * ((sin(angle) + 1) / 2));

    // The color's G and B values will come from angles with offsets of 2pi/3 and 4pi/3, respectively.
    color.g = (uint8_t)(0xFF * ((sin(angle + M_TWOPI_THREE) + 1) / 2));
    color.b = (uint8_t)(0xFF * ((sin(angle + M_FOURPI_THREE) + 1) / 2));
    return color;
}

LampArrayColor LightingEffects::GetRandomColor()
{
    LampArrayColor color = {};
    color.a = 0xFF;
    color.r = static_cast<uint8_t>(rand() % 0xFF);
    color.g = static_cast<uint8_t>(rand() % 0xFF);
    color.b = static_cast<uint8_t>(rand() % 0xFF);
    return color;
}
