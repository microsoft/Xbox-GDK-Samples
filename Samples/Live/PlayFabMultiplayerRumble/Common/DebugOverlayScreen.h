//--------------------------------------------------------------------------------------
// DebugOverlayScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameScreen.h"

namespace PlayFabMultiplayerRumble
{

class DebugOverlayScreen : public GameScreen
{
public:
    DebugOverlayScreen();
    virtual ~DebugOverlayScreen() = default;

    virtual void Draw(float totalTime, float elapsedTime) override;
};

}
