//--------------------------------------------------------------------------------------
// GameLobbyScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"
#include "GameStateManager.h"

namespace NetRumble
{

class GameLobbyScreen : public MenuScreen
{
public:
    GameLobbyScreen() noexcept;
    virtual ~GameLobbyScreen() = default;

    virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
    virtual void HandleInput() override;
    virtual void Draw(float totalTime, float elapsedTime) override;

protected:
    virtual void OnCancel() override;

private:
    bool m_ready;
    bool m_exiting;
    float m_countdownTimer;

    GameState m_lastState;
    TextureHandle m_inGameTexture;
    TextureHandle m_readyTexture;
};

}
