//--------------------------------------------------------------------------------------
// GameStateManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Manager.h"

namespace NetRumble
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

class GameStateManager : public Manager
{
    public:
        GameStateManager() ;

        void Update();

        inline void SwitchToState(GameState state) { _nextState = state; }
        inline GameState GetState() const { return _state; }

    private:
        GameState _state;
        GameState _nextState;
    };
}
