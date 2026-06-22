//--------------------------------------------------------------------------------------
// GamePlayScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"
#include "GameStateManager.h"

namespace NetRumble
{

class GamePlayScreen : public GameScreen
{
public:
    GamePlayScreen();
    virtual ~GamePlayScreen() = default;

    virtual void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
    virtual void HandleInput() override;
    virtual void Draw(float totalTime, float elapsedTime) override;

    virtual void ExitScreen(bool immediate = false) override;

private:
    void DrawHud();

    std::shared_ptr<DirectX::SpriteFont> m_playerFont;
	std::shared_ptr<DirectX::SpriteFont> m_scoreFont;
};

}
