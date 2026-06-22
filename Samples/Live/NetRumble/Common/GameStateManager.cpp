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

using namespace NetRumble;

GameStateManager::GameStateManager() :
    _state{ GameState::Initialize },
    _nextState{ GameState::Initialize }
{
}

void GameStateManager::Update()
{
    if (_state != _nextState)
    {
        _state = _nextState;

        switch (_state)
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
            case GameState::Lobby:
            {
                break;
            }
            case GameState::StartingGame:
            {
                break;
            }
            case GameState::InGame:
            {
                g_game->StartGame();
                break;
            }
            case GameState::ExitingGame:
            {
                break;
            }
            case GameState::EvaluatingNetwork:
            {
                std::vector<std::vector<std::pair<std::string, uint64_t>>> userRegionLists;

                // Get each user's vector of datacenter latencies
                for (auto user : g_game->GetAllPlayerStates())
                {
                    userRegionLists.push_back(user->GetRegionLatencyList());
                }

                Party::PartyNetworkDescriptor descriptor{};
                Managers::Get<PlayFabPartyManager>()->GetPartyNetworkDescriptor(&descriptor);

                // Get the region list sorted by average latency among members
                auto regionList = Managers::Get<PlayFabPartyManager>()->GetBestRegionList(userRegionLists);

                if (regionList.size() != 0)
                {
                    if (regionList.at(0) == descriptor.regionName)
                    {
                        // We're already in the best region so we can just start
                        DEBUGLOG("Network rebalancing not requried.\n");
                        Managers::Get<GameStateManager>()->SwitchToState(GameState::InGame);
                    }
                    else
                    {
                        DEBUGLOG("Network rebalancing requried!\n");
                        Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::MigrateRegion, 0));
                        Managers::Get<GameStateManager>()->SwitchToState(GameState::MigratingNetwork);
                        Managers::Get<OnlineManager>()->MigrateToRegion(
                            regionList,
                            [](bool succeeded)
                            {
                                DEBUGLOG("MigrateToNewNetwork returned: %s\n", succeeded ? "true" : "false");

                                Managers::Get<GameStateManager>()->SwitchToState(GameState::WaitingForPeerMigration);
                            });
                    }
                }
                else
                {
                    DEBUGLOG("GetBestRegionList returned no regions!\n");
                    Managers::Get<GameStateManager>()->SwitchToState(GameState::InGame);
                }
                break;
            }
            case GameState::MigratingNetwork:
            {
                break;
            }
            case GameState::WaitingForPeerMigration:
            {
                break;
            }
            default:
                break;
        }
    }
}
