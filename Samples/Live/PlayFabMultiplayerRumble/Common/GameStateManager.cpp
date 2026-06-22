//--------------------------------------------------------------------------------------
// GameStateManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "GameScreen.h"
#include "PlayerState.h"
#include "World.h"
#include "Managers.h"

#include "UserStartupScreen.h"
#include "MainMenuScreen.h"
#include "GameLobbyScreen.h"
#include "StarfieldScreen.h"

#include "Game.h"

using namespace PlayFabMultiplayerRumble;

std::string GetGameStateString(GameState state)
{
    switch (state)
    {
    case GameState::Initialize:              return STRINGIFY(GameState::Initialize);
    case GameState::StartMenu:               return STRINGIFY(GameState::StartMenu);
    case GameState::MainMenu:                return STRINGIFY(GameState::MainMenu);
    case GameState::MPHostGame:              return STRINGIFY(GameState::MPHostGame);
    case GameState::MPJoinGame:              return STRINGIFY(GameState::MPJoinGame);
    case GameState::MPMatchmaking:           return STRINGIFY(GameState::MPMatchmaking);
    case GameState::MPMatchmakingCancel:     return STRINGIFY(GameState::MPMatchmakingCancel);
    case GameState::Lobby:                   return STRINGIFY(GameState::Lobby);
    case GameState::StartingGame:            return STRINGIFY(GameState::StartingGame);
    case GameState::InGame:                  return STRINGIFY(GameState::InGame);
    case GameState::ExitingGame:             return STRINGIFY(GameState::ExitingGame);
    case GameState::EvaluatingNetwork:       return STRINGIFY(GameState::EvaluatingNetwork);
    case GameState::MigratingNetwork:        return STRINGIFY(GameState::MigratingNetwork);
    case GameState::WaitingForPeerMigration: return STRINGIFY(GameState::WaitingForPeerMigration);
    case GameState::Invalid:                 return STRINGIFY(GameState::Invalid);
    }

    //we should never get here
    assert(false);
    return "Unknown enumeration value";
}

void GameStateManager::Update()
{
    //DEBUGLOG("GameStateManager::Update: current gamestate: %s", GetGameStateString(currentGameState).c_str());

    if (currentGameState != nextGameState)
    {
        DEBUGLOG("GameStateManager::Update: currentGameState: %s nextGameState: %s", GetGameStateString(currentGameState).c_str(), GetGameStateString(nextGameState).c_str());

        currentGameState = nextGameState;

        switch (currentGameState)
        {
            case GameState::StartMenu:
            {
                auto screenMgr = Managers::Get<ScreenManager>();
                screenMgr->ExitAllScreens();
                screenMgr->AddGameScreen<UserStartupScreen>();
                screenMgr->SetBackgroundsVisible(true);
                break;
            }

            case GameState::MainMenu:
            {
                auto screenMgr = Managers::Get<ScreenManager>();
                screenMgr->ExitAllScreens();
                screenMgr->AddGameScreen<MainMenuScreen>();
                screenMgr->SetBackgroundsVisible(true);

                // If invite info exists, join now
                if (Managers::Get<OnlineManager>()->HasPendingInviteSession())
                {
                    Managers::Get<OnlineManager>()->JoinPendingInviteSession();
                }
                break;
            }

            case GameState::MPHostGame:
            {
                auto screenMgr = Managers::Get<ScreenManager>();
                screenMgr->ExitAllScreens();
                screenMgr->AddGameScreen<GameLobbyScreen>();
                Managers::Get<OnlineManager>()->HostMultiplayerGame();
                break;
            }

            case GameState::MPJoinGame:
            {
                auto screenMgr = Managers::Get<ScreenManager>();
                screenMgr->ExitAllScreens();
                screenMgr->AddGameScreen<GameLobbyScreen>();
                break;
            }

            case GameState::MPMatchmaking:
            {
                auto screenMgr = Managers::Get<ScreenManager>();
                screenMgr->ExitAllScreens();
                screenMgr->AddGameScreen<GameLobbyScreen>();
                Managers::Get<OnlineManager>()->StartMatchmaking();
                break;
            }

            case GameState::InGame:
            {
                g_game->StartGame();
                break;
            }

            case GameState::EvaluatingNetwork:
            {
                std::vector<std::vector<std::pair<std::string, uint64_t>>> userRegionLists;

                // Get each user's vector of datacenter latencies
                for (const auto& user : g_game->GetAllPlayerStates())
                {
                    userRegionLists.push_back(user->GetRegionLatencyList());
                }

                Party::PartyNetworkDescriptor descriptor{};
                Managers::Get<PlayFabPartyManager>()->GetPartyNetworkDescriptor(&descriptor);

                // Get the region list sorted by average latency among members
                auto regionList = Managers::Get<PlayFabPartyManager>()->GetBestRegionList(userRegionLists);

                if (!regionList.empty())
                {
                    if (regionList.at(0) == descriptor.regionName)
                    {
                        // We're already in the best region so we can just start
                        DEBUGLOG("GameStateManager::Update: Network rebalancing not requried.");
                        Managers::Get<GameStateManager>()->SwitchToState(GameState::InGame);
                    }
                    else
                    {
                        DEBUGLOG("GameStateManager::Update: Network rebalancing requried!");
                        Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::MigrateRegion, 0));
                        Managers::Get<GameStateManager>()->SwitchToState(GameState::MigratingNetwork);
                        Managers::Get<OnlineManager>()->MigrateToRegion(
                            regionList,
                            [](bool succeeded)
                            {
                                DEBUGLOG("GameStateManager::Update: MigrateToNewNetwork returned: %s", succeeded ? "true" : "false");

                                Managers::Get<GameStateManager>()->SwitchToState(GameState::WaitingForPeerMigration);
                            });
                    }
                }
                else
                {
                    DEBUGLOG("GameStateManager::Update: GetBestRegionList returned no regions!");
                    Managers::Get<GameStateManager>()->SwitchToState(GameState::InGame);
                }
                break;
            }
        }
    }
}
