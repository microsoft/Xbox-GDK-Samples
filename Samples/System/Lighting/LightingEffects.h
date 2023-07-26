//--------------------------------------------------------------------------------------
// LightingEffects.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Lighting.h"

class LightingEffects final
{
    public:
        static LampArrayColor GetLampArrayColorForAngle(double angle);
        static LampArrayColor GetRandomColor();
        static void ResetEffects(LampArrayContext* device);

        static void UpdateColorWaveEffect(LampArrayContext* lampArray);
        static void UpdateColorCycleEffect(LampArrayContext* lampArray);
        static void UpdateBlinkEffect(LampArrayContext* lampArray);
        static void UpdateColorWheelEffect(LampArrayContext* lampArray);
        static void UpdateWASDEffect(LampArrayContext* lampArray);
        static void UpdateSolidEffect(LampArrayContext* lampArray);
};
