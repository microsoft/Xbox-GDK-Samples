//--------------------------------------------------------------------------------------
// OptionsPopUpScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"
#include "GameStateManager.h"

namespace PlayFabMultiplayerRumble
{
class OptionsPopUpScreen : public MenuScreen
{
public:
    OptionsPopUpScreen();
    virtual ~OptionsPopUpScreen();

    virtual void HandleInput() override;
    virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
    virtual void Draw(float totalTime, float elapsedTime) override;
    virtual void LoadContent() override;

protected:
    virtual void OnCancel() override;
    virtual void ComputeMenuBounds(float viewportWidth, float viewportHeight) override;

private:
    TextureHandle m_backgroundTexture;
    int m_currentIndex;
};
}
