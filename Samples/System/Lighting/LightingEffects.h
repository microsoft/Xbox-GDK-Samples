//--------------------------------------------------------------------------------------
// LightingEffects.h
//
// The lighting effects, implemented as pure color-generation routines that
// operate on a single LampArrayDevice. Each routine is one self-contained
// technique; the shared color and timing utilities they build on live in
// EffectHelpers.h. The interesting LampArray API usage lives in Lighting.cpp.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Lighting.h"

namespace LightingEffects
{
    // Per-effect UI metadata describing which adjustable controls an effect
    // exposes. The device panel renders only these, so the visible controls change
    // with the selected effect: colorCount color pickers (labeled by colorLabels)
    // and the speed slider when usesSpeed is true.
    struct EffectControls
    {
        uint32_t    colorCount;
        const char* colorLabels[c_MaxEffectColors];
        bool        usesSpeed;
    };

    // Per-effect metadata row: an effect's display name, one-line description, and
    // its EffectControls. The implementation keeps one row per Effect (see the
    // table in LightingEffects.cpp), which drives the Name/Description/Controls
    // accessors below.
    struct EffectInfo
    {
        Effect          id;             // which effect this row describes
        const char*     name;
        const char*     description;
        EffectControls  controls;       // colorCount, color labels, usesSpeed
    };

    // Display name and one-line description for an effect, shown in the device
    // panel's effect selector.
    const char* Name(Effect effect);
    const char* Description(Effect effect);

    // The adjustable controls the effect exposes (color pickers and/or speed).
    const EffectControls& Controls(Effect effect);

    // Runs the device's currently assigned effect for one frame. The effect
    // measures its own real elapsed time (via QueryPerformanceCounter) to advance
    // its animation, and performs the one-time reset first if the effect was just
    // (re)assigned.
    void Run(LampArrayDevice& device);
}
