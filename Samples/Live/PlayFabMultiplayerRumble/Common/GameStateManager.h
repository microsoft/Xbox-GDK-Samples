//--------------------------------------------------------------------------------------
// GameStateManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "IManager.h"

namespace PlayFabMultiplayerRumble
{

class PlayerState;
class World;


enum class GameState : int
{
    Initialize,
    StartMenu,
    MainMenu,
    MPHostGame,
    MPJoinGame,
    MPMatchmaking,
    MPMatchmakingCancel,
    Lobby,
    StartingGame,
    InGame,
    ExitingGame,
    EvaluatingNetwork,
    MigratingNetwork,
    WaitingForPeerMigration,
    Invalid
};

class GameStateManager : public IManager
{
    public:
        GameStateManager() = default;

        void Update();

        inline void SwitchToState(GameState state) { nextGameState = state; }
        inline GameState GetState() const { return currentGameState; }

    private:
        GameState currentGameState = GameState::Initialize;
        GameState nextGameState = GameState::Initialize;
    };
}
