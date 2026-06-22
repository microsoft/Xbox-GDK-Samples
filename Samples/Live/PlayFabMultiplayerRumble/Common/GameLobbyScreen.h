//--------------------------------------------------------------------------------------
// GameLobbyScreen.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "MenuScreen.h"
#include "GameStateManager.h"
#include "SampleConfig.h"

namespace PlayFabMultiplayerRumble
{
    class GameLobbyScreen : public MenuScreen
    {
    public:
        GameLobbyScreen() noexcept;
        virtual ~GameLobbyScreen() = default;

        void Update(float totalTime, float elapsedTime, bool otherScreenHasFocus, bool coveredByOtherScreen) override;
        void HandleInput() override;
        void Draw(float totalTime, float elapsedTime) override;

    protected:
        void OnCancel() override;

    private:
        bool m_ready = false;
        bool m_exiting = false;
        float m_countdownTimer = SampleConfig::c_LobbyCountdownSeconds;

        GameState m_lastState = GameState::Initialize;
        TextureHandle m_inGameTexture;
        TextureHandle m_readyTexture;

        void ChangeShipDesign();
        void ChangeShipColor();

        void SendPlayerStateMessage();

        void HandleStartingGameGameState(float elapsedTime);
        void HandleLobbyGameState(GameState state);

        void SetPlayerReady(bool bPlayerReady);
    };
}
