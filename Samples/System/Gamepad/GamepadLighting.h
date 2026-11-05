//--------------------------------------------------------------------------------------
// GamepadLighting.h
//
// Lighting effect types and per-gamepad effect state.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

enum class LightingEffect
{
    Manual = 0,
    Breathing,
    ColorCycle,
    Count
};

struct LightingEffectState
{
    LightingEffect selectedEffect = LightingEffect::Manual;
    float color[3] = { 0.0f, 1.0f, 0.0f };
    bool enabled = true;
};
