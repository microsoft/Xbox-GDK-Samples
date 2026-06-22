//--------------------------------------------------------------------------------------
// StarfieldScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "GameScreen.h"

namespace NetRumble
{

class Starfield;

class StarfieldScreen : public GameScreen
{
public:
    StarfieldScreen();

    virtual void LoadContent() override;
    virtual void Reset() override;
    virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
    virtual void Draw(float totalTime, float elapsedTime) override;

private:
    std::unique_ptr<Starfield> m_starfield;
    TextureHandle m_title;
    float m_movement;
};

}
