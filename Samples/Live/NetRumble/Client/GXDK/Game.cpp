//--------------------------------------------------------------------------------------
// Game.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"
#include "Managers.h"
#include "MainMenuScreen.h"
#include "GameLobbyScreen.h"
#include "StarfieldScreen.h"
#include "DebugOverlayScreen.h"
#include "GamePlayScreen.h"
#include <DataBuffer.h>

extern void ExitGame();

using namespace NetRumble;
using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const DWORD WaitTimeoutInMs = 500;
}

Game::Game() noexcept
{
    /*while (!IsDebuggerPresent()) {}*/

    m_signInCallbackToken = 0;
    m_signOutCallbackToken = 0;
}

Game::~Game()
{
    CleanupUser(true);
    MultiplayerShutdown();
    Managers::Shutdown();
    WaitForAndCleanupHandles();
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window)
{
    RECT rc;
    GetClientRect(window, &rc);

    m_outputWidth = rc.right - rc.left;
    m_outputHeight = rc.bottom - rc.top;

#ifdef _DEBUG
    DebugInit();
#endif

    // Lock the game to 60FPS
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);

    Managers::Initialize();

    Managers::Get<RenderManager>()->Initialize(window, m_outputWidth, m_outputHeight);
    m_descriptors = Managers::Get<RenderManager>()->CreateDescriptorPile(64);

    Managers::Get<ContentManager>()->Initialize(m_descriptors);

    Managers::Get<AudioManager>()->Initialize();

    MultiplayerInitialize();

    PrefetchContent();

    auto gameStateMgr = Managers::Get<GameStateManager>();

    gameStateMgr->SwitchToState(GameState::StartMenu);
    m_world = std::make_unique<World>();

    Managers::Get<ScreenManager>()->AddBackgroundScreen(std::make_unique<StarfieldScreen>());
    Managers::Get<ScreenManager>()->AddForegroundScreen(std::make_unique<DebugOverlayScreen>());
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    Managers::Get<AsyncTaskManager>()->Tick();
    Managers::Get<ScreenManager>()->Update(timer);
    Managers::Get<InputManager>()->Update();
    Managers::Get<AudioManager>()->Tick();
    Managers::Get<ParticleEffectManager>()->Update(static_cast<float_t>(timer.GetElapsedSeconds()));
    Managers::Get<GameStateManager>()->Update();

    if (Managers::Get<XboxUserManager>()->IsAnyoneSignedIn())
    {
        Managers::Get<OnlineManager>()->Tick(static_cast<float_t>(timer.GetElapsedSeconds()));
        Managers::Get<PlayFabPartyManager>()->DoWork();
    }
}

// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    auto renderManager = Managers::Get<RenderManager>();
    renderManager->Clear(m_descriptors->Heap());

    Managers::Get<ScreenManager>()->Render(m_timer);

    renderManager->Present();
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    DEBUGLOG("Game::OnSuspending - Start\n");

    // We need to completely cleanup all networking and the user
    CleanupUser(true);

    // wait for all async completion handles
    WaitForAndCleanupHandles();

    Managers::Get<ScreenManager>()->Suspend();
    Managers::Get<RenderManager>()->Suspend();
    Managers::Get<AudioManager>()->Suspend();
    Managers::Get<InputManager>()->Suspend();

    DEBUGLOG("Game::OnSuspending - End\n");
}

void Game::OnResuming()
{
    DEBUGLOG("Game::OnResuming - Start\n");

    Managers::Get<InputManager>()->Resume();
    Managers::Get<AudioManager>()->Resume();
    Managers::Get<RenderManager>()->Resume();
    Managers::Get<ScreenManager>()->Resume();

    m_timer.ResetElapsedTime();

    // We need to go back to the start menu (this already waits for networking connectivity)
    Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);

    DEBUGLOG("Game::OnResuming - End\n");
}

void Game::OnWindowSizeChanged(int width, int height)
{
    auto renderMgr = Managers::Get<RenderManager>();

    if (!renderMgr->OnWindowSizeChanged(width, height))
        return;

    renderMgr->CreateWindowSizeDependentResources();
}

void Game::PrefetchContent()
{
    auto contentManager = Managers::Get<ContentManager>();
    std::wstring texturePath = L"Assets\\Textures\\";

    // Ship
    contentManager->LoadTexture(texturePath + L"ship0.png");
    contentManager->LoadTexture(texturePath + L"ship1.png");
    contentManager->LoadTexture(texturePath + L"ship2.png");
    contentManager->LoadTexture(texturePath + L"ship3.png");
    contentManager->LoadTexture(texturePath + L"ship0Overlay.png");
    contentManager->LoadTexture(texturePath + L"ship1Overlay.png");
    contentManager->LoadTexture(texturePath + L"ship2Overlay.png");
    contentManager->LoadTexture(texturePath + L"ship3Overlay.png");
    contentManager->LoadTexture(texturePath + L"shipShields.png");

    // Asteroids
    contentManager->LoadTexture(texturePath + L"asteroid0.png");
    contentManager->LoadTexture(texturePath + L"asteroid1.png");
    contentManager->LoadTexture(texturePath + L"asteroid2.png");

    // Laser
    contentManager->LoadTexture(texturePath + L"laser.png");
    contentManager->LoadTexture(texturePath + L"powerupDoubleLaser.png");
    contentManager->LoadTexture(texturePath + L"powerupTripleLaser.png");

    // Mine
    contentManager->LoadTexture(texturePath + L"mine.png");

    // Rocket
    contentManager->LoadTexture(texturePath + L"rocket.png");
    contentManager->LoadTexture(texturePath + L"powerupRocket.png");


    auto audioManager = Managers::Get<AudioManager>();
    std::wstring audioPath = L"Assets\\Audio\\";

    audioManager->LoadSound(L"asteroid_touch", audioPath + L"asteroid_touch.wav");
    audioManager->LoadSound(L"explosion_large", audioPath + L"explosion_large.wav");
    audioManager->LoadSound(L"explosion_medium", audioPath + L"explosion_medium.wav");
    audioManager->LoadSound(L"explosion_shockwave", audioPath + L"explosion_shockwave.wav");
    audioManager->LoadSound(L"fire_laser1", audioPath + L"fire_laser1.wav");
    audioManager->LoadSound(L"fire_laser2", audioPath + L"fire_laser2.wav");
    audioManager->LoadSound(L"fire_laser3", audioPath + L"fire_laser3.wav");
    audioManager->LoadSound(L"fire_rocket1", audioPath + L"fire_rocket1.wav");
    audioManager->LoadSound(L"fire_rocket2", audioPath + L"fire_rocket2.wav");
    audioManager->LoadSound(L"menu_scroll", audioPath + L"menu_scroll.wav");
    audioManager->LoadSound(L"menu_select", audioPath + L"menu_select.wav");
    audioManager->LoadSound(L"player_spawn", audioPath + L"player_spawn.wav");
    audioManager->LoadSound(L"powerup_spawn", audioPath + L"powerup_spawn.wav");
    audioManager->LoadSound(L"powerup_touch", audioPath + L"powerup_touch.wav");
    audioManager->LoadSound(L"rocket", audioPath + L"rocket.wav");
}

void Game::ProcessGameNetworkMessage(uint64_t sourceId, GameMessage *message)
{
    auto localId = Managers::Get<OnlineManager>()->GetNetworkId();
    auto asyncManager = Managers::Get<AsyncTaskManager>();

    switch (message->MessageType())
    {
    case GameMessageType::MPPrivilegeError:
        Managers::Get<ScreenManager>()->ShowError("Your account does not have multiplayer privileges");
        break;

    case GameMessageType::GameCountdown:
        Managers::Get<GameStateManager>()->SwitchToState(GameState::StartingGame);

        // When a client receives the GameCountdown message it means the host is about to start the game
        // and we need to send our latency information
        Managers::Get<PlayFabPartyManager>()->PopulatePartyRegionLatencies();
        break;

    case GameMessageType::RegionLatency:
    {
        auto player = GetPlayerState(sourceId);
        if (player != nullptr)
        {
            auto raw = message->StringValue();
            auto region = raw.substr(0, raw.find(":"));
            auto latency = std::strtoull(raw.substr(raw.find(":") + 1).c_str(), nullptr, 10);

            player->SetRegionLatency(region, latency);

            DEBUGLOG("Received RegionLatency from %s: %s - %d\n", player->DisplayName.c_str(), region.c_str(), latency);
        }
        else
        {
            DEBUGLOG("Received RegionLatency for a player we don't know\n");
        }
        break;
    }
    case GameMessageType::MigrateRegion:
    {
        Managers::Get<GameStateManager>()->SwitchToState(GameState::MigratingNetwork);
        break;
    }

    case GameMessageType::GameOver:
        if (m_world->IsInitialized())
        {
            DEBUGLOG("Received a GameOver message\n");

            auto localPlayer = GetPlayerState(localId);
            if (localPlayer->InGame)
            {
                m_world->IsGameWon = true;
                m_world->SetGameInProgress(false);
                m_world->DeserializeGameOver(message->RawData());
            }
            else
            {
                ResetGameplayData();
            }
        }
        break;

    case GameMessageType::GameSettings:
        DEBUGLOG("Received a GameSettings message\n");
        m_world->DeserializeGameSettings(message->RawData());
        break;

    case GameMessageType::GameStart:
    {
        DEBUGLOG("Received a GameStart message\n");

        auto localPlayer = GetPlayerState(localId);
        if (!localPlayer->InGame)
        {
            m_world->SetGameInProgress(true);

            if (localPlayer->LobbyReady)
            {
                Managers::Get<GameStateManager>()->SwitchToState(GameState::InGame);
            }
        }
        break;
    }
    case GameMessageType::PlayerJoined:
    {
        auto itr = m_peers.find(sourceId);
        if (itr == m_peers.end() && sourceId != localId)
        {
            auto onlineManager = Managers::Get<OnlineManager>();

            auto playerState = std::make_shared<PlayerState>();
            playerState->IsLocalPlayer = false;
            playerState->ShipColor(0);
            playerState->ShipVariation(0);
            playerState->PeerId = sourceId;
            
            onlineManager->QueryUserDisplayNameAsync(sourceId, asyncManager->GetDefaultQueue().get(), [playerState](const OnlineUser &user)
            {
                playerState->DisplayName = user.Name;
            });

            m_peers[sourceId] = playerState;

            Managers::Get<OnlineManager>()->SendGameMessage(
                GameMessage(
                    GameMessageType::PlayerState,
                    GetLocalPlayerState()->SerializePlayerStateData()
                )
            );
        }
        break;
    }
    case GameMessageType::PlayerInfo:
    {
        auto itr = m_peers.find(sourceId);
        if (itr != m_peers.end() && sourceId != localId)
        {
            auto playerState = itr->second;
            playerState->DisplayName = message->StringValue().c_str();

            if (Managers::Get<OnlineManager>()->IsHost())
            {
                for (auto& [id, peer] : m_peers)
                {
                    // Don't resend this to the host or player that sent it initially
                    if (id != sourceId && !peer->IsLocalPlayer)
                    {
                        Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(
                            GameMessageType::PlayerInfo,
                            playerState->DisplayName
                        ));
                    }
                }
            }

            DEBUGLOG("Received PlayerInfo for: %ws\n", playerState->DisplayName.c_str());
        }
        else
        {
            DEBUGLOG("Received PlayerInfo for unknown peer %u\n", sourceId);
        }
        break;
    }
    case GameMessageType::PlayerState:
    {
        DEBUGLOG("Received a PlayerState message from %u\n", sourceId);
        auto playerState = GetPlayerState(sourceId);
        if (playerState != nullptr && sourceId != localId)
        {
            playerState->DeserializePlayerStateData(message->RawData());
        }
        else
        {
            DEBUGLOG("No PlayerState exists for peer %u!\n", sourceId);
        }
        break;
    }
    case GameMessageType::PlayerLeft:
    {
        auto playerState = GetPlayerState(sourceId);
        playerState->DeactivatePlayer();
        SetPlayerState(sourceId, nullptr);
        break;
    }
    case GameMessageType::PowerUpSpawn:
        DEBUGLOG("Received a PowerUpSpawn message\n");
        if (m_world->IsInitialized())
        {
            m_world->DeserializePowerUpSpawn(message->RawData());
        }
        else
        {
            DEBUGLOG("...World not initialized!\n");
        }
        break;

    case GameMessageType::ShipData:
        if (m_world->IsInitialized())
        {
            auto peerState = GetPlayerState(sourceId);
            if (peerState != nullptr)
            {
                peerState->GetShip()->Deserialize(message->RawData());
            }
        }
        else
        {
            DEBUGLOG("Received a ShipData message\n");
            DEBUGLOG("...World not initialized!\n");
        }
        break;

    case GameMessageType::ShipDeath:
        DEBUGLOG("Received a ShipDeath message from %u\n", sourceId);
        if (m_world->IsInitialized())
        {
            auto peerState = GetPlayerState(sourceId);
            if (peerState != nullptr)
            {
                m_world->DeserializeShipDeath(sourceId, message->RawData());
            }
            else
            {
                DEBUGLOG("PlayerState not found for %u\n", sourceId);
            }
        }
        else
        {
            DEBUGLOG("...World not initialized!\n");
        }
        break;

    case GameMessageType::ShipInput:
        if (m_world->IsInitialized())
        {
            auto peerState = GetPlayerState(sourceId);
            if (peerState != nullptr)
            {
                peerState->GetShip()->Input.Deserialize(message->RawData());
            }
        }
        else
        {
            DEBUGLOG("...World not initialized!\n");
        }
        break;

    case GameMessageType::ShipSpawn:
        DEBUGLOG("Received a ShipSpawn message\n");
        if (m_world->IsInitialized())
        {
            m_world->DeserializeShipSpawn(message->RawData());
        }
        else
        {
            DEBUGLOG("...World not initialized!\n");
        }
        break;

    case GameMessageType::WorldData:
        if (m_world->IsInitialized())
        {
            m_world->DeserializeWorldData(message->RawData());
        }
        else
        {
            DEBUGLOG("...World not initialized!\n");
        }
        break;

    case GameMessageType::WorldSetup:
        DEBUGLOG("Received a WorldSetup message\n");
        if (!m_world->IsInitialized())
        {
            m_world->DeserializeWorldSetup(message->RawData());
        }
        else
        {
            DEBUGLOG("...World is already initialized!\n");
        }
        break;
    case GameMessageType::JoiningGame:
        Managers::Get<GameStateManager>()->SwitchToState(GameState::MPJoinGame);
        break;
    case GameMessageType::MatchmakingFailed:
        Managers::Get<ScreenManager>()->ShowError("Matchmaking Failed. Returning to Main Menu.", []() {
            Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
        });
        break;
    case GameMessageType::JoinGameFailed:
        Managers::Get<ScreenManager>()->ShowError("Failed to join game. Returning to Main Menu.", []() {
            Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
        });
        break;
    case GameMessageType::OnlineDisconnect:
    {
        auto state = Managers::Get<GameStateManager>()->GetState();
        if (state != GameState::ExitingGame && state != GameState::MainMenu)
        {
            Managers::Get<ScreenManager>()->ShowError("Lost online connections. Returning to Main Menu.", []() {
                Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
                });
        }
        else if (state == GameState::ExitingGame)
        {
            Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
        }
        break;
    }
    case GameMessageType::JoinedGameComplete:
        Managers::Get<GameStateManager>()->SwitchToState(GameState::Lobby);
        break;
    case GameMessageType::MatchmakingCanceled:
        Managers::Get<GameStateManager>()->SwitchToState(GameState::ExitingGame);
        Managers::Get<OnlineManager>()->LeaveMultiplayerGame(false);
        break;
    case GameMessageType::LeaveGameComplete:
    {
        // Remove all remote peers
        std::vector<uint64_t> remotePlayers;
        for (auto &&[id, playerState] : m_peers)
        {
            if (!playerState->IsLocalPlayer)
            {
                remotePlayers.push_back(id);
            }
        }
        for (auto &id : remotePlayers)
        {
            m_peers.erase(id);
        }

        Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
        break;
    }
    case GameMessageType::NetworkLost:
    {
        DEBUGLOG("Network connection has been lost...\n");

        // cleans up the local user networking cache
        CleanupUser(true);

        // clean up xblcontexthandle
        Managers::Get<OnlineManager>()->OnNetworkLost();

        // wait for all async completion handles
        WaitForAndCleanupHandles();

        Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
        break;
    }
    default:
        break;
    }
}

std::shared_ptr<PlayerState> Game::GetPlayerState(uint64_t peer)
{
    auto itr = m_peers.find(peer);
    if (itr != m_peers.end())
    {
        return (*itr).second;
    }
    return nullptr;
}

std::shared_ptr<PlayerState> Game::GetLocalPlayerState()
{
    for (const auto& peer : m_peers)
    {
        if (peer.second->IsLocalPlayer)
        {
            return peer.second;
        }
    }

    return nullptr;
}

void Game::SetPlayerState(uint64_t id, std::shared_ptr<PlayerState> state)
{
    auto peer = GetPlayerState(id);

    if (peer != nullptr && state == nullptr)
    {
        peer->DeactivatePlayer();
        m_peers.erase(id);
    }
    else
    {
        m_peers[id] = state;
    }
}

void Game::ClearPlayerScores()
{
    for (auto& pair : m_peers)
    {
        pair.second->GetShip()->Score = 0;
    }
}

void Game::StartGame()
{
    GetLocalPlayerState()->InGame = true;
    Managers::Get<OnlineManager>()->SendGameMessage(
        GameMessage(
            GameMessageType::PlayerState,
            GetLocalPlayerState()->SerializePlayerStateData()
        )
    );
    Managers::Get<ScreenManager>()->AddGameScreen<GamePlayScreen>();
    Managers::Get<ScreenManager>()->SetBackgroundsVisible(false);

    m_world->SetGameInProgress(true);

    if (Managers::Get<OnlineManager>()->IsHost())
    {
        m_world->GenerateWorld();

        Managers::Get<OnlineManager>()->SendGameMessage(
            GameMessage(
                GameMessageType::GameStart,
                0
            )
        );

        std::map<uint64_t, std::shared_ptr<Ship>> ships;

        for (auto item : m_peers)
        {
            if (item.second && !item.second->IsInactive())
            {
                ships[item.first] = item.second->GetShip();
            }
        }

        Managers::Get<OnlineManager>()->SendGameMessage(
            GameMessage(
                GameMessageType::WorldSetup,
                m_world->SerializeWorldSetup(ships)
            )
        );
    }
}

void Game::DrawWorld(float elapsedTime)
{
    if (m_world->IsInitialized())
    {
        m_world->Draw(elapsedTime);
    }
}

void Game::UpdateWorld(float totalTime, float elapsedTime)
{
    if (m_world->IsInitialized())
    {
        m_world->Update(totalTime, elapsedTime);
    }
}

std::vector<std::shared_ptr<PlayerState>> Game::GetAllPlayerStates()
{
    std::vector<std::shared_ptr<PlayerState>> players;

    for (auto& [id, peer] : m_peers)
    {
        players.push_back(peer);
    }

    return players;
}

void Game::ResetGameplayData()
{
    // Reset any existing game world to its defaults
    if (m_world != nullptr)
    {
        m_world->ResetDefaults();
        m_world->SetGameInProgress(false);
    }

    // remove remote players
    std::vector<uint64_t> remotePlayers;
    for (auto&& [id, player] : m_peers)
    {
        if (!player->IsLocalPlayer)
        {
            remotePlayers.push_back(id);
        }
        else
        {
            player->InGame = false;
            player->LobbyReady = false;
        }
    }
    for (auto& id : remotePlayers)
    {
        m_peers.erase(id);
    }
}

void Game::AddHandleToWaitSet(HANDLE handle)
{
    HANDLE duplicate = nullptr;

    DuplicateHandle(
        GetCurrentProcess(),
        handle,
        GetCurrentProcess(),
        &duplicate,
        0,
        FALSE,
        DUPLICATE_SAME_ACCESS);

    m_waitHandles.push_back(duplicate);
}

void Game::WaitForAndCleanupHandles()
{
    if (m_waitHandles.empty())
    {
        return;
    }

    DWORD result = WaitForMultipleObjects(DWORD(m_waitHandles.size()), m_waitHandles.data(), true, WaitTimeoutInMs);

    if (result == WAIT_TIMEOUT)
    {
        DEBUGLOG("WaitForAndCleanupHandles Timed out waiting for handles\n");
    }

    for (auto handle : m_waitHandles)
    {
        CloseHandle(handle);
    }

    m_waitHandles.clear();
}

void Game::MultiplayerInitialize()
{
    Managers::Get<XboxUserManager>()->Initialize();
    Managers::Get<OnlineManager>()->Initialize();
    Managers::Get<OnlineManager>()->RegisterOnlineMessageHandler([this](uint64_t source, GameMessage *message)
    {
        ProcessGameNetworkMessage(source, message);
    });

    // NOTE: when the user has completed their Live sign-in, we do not go straight to
    // the main menu since we need to update the user for PlayFab Party first
    auto userSignedIn = [this](const User &user)
    {
        m_localPlayerName = user.Name();

        auto localPlayer = std::make_shared<PlayerState>(m_localPlayerName);
        localPlayer->IsLocalPlayer = true;
        localPlayer->ShipColor(0);
        localPlayer->ShipVariation(0);
        localPlayer->PeerId = Managers::Get<OnlineManager>()->GetNetworkId();

        m_peers[localPlayer->PeerId] = localPlayer;

        auto userHandle = *Managers::Get<XboxUserManager>()->GetCurrentUser();

        Managers::Get<SpeechManager>()->LoadAccessibiltySettingsAsync(userHandle, [](HRESULT hr)
        {
            if (SUCCEEDED(hr))
            {
                auto screen = Managers::Get<ScreenManager>()->GetCurrentGameScreen();
                if (screen != nullptr)
                {
                    screen->ReadCurrentEntry();
                }
            }
        });
    };

    // NOTE: when the user signs out however, we always make sure to immediately put
    // the user back to the initial start screen
    auto userSignedOut = [this](const User &/*user*/)
    {
        CleanupUser(true);
        WaitForAndCleanupHandles();
        Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
    };

    m_signInCallbackToken = Managers::Get<XboxUserManager>()->AddUserSignedInCallback(userSignedIn);
    m_signOutCallbackToken = Managers::Get<XboxUserManager>()->AddUserSignedOutCallback(userSignedOut);
}

void Game::MultiplayerShutdown()
{
    DEBUGLOG("Game::MultiplayerShutdown\n");

    Managers::Get<XboxUserManager>()->RemoveUserSignedInCallback(m_signInCallbackToken);
    m_signInCallbackToken = 0;

    Managers::Get<XboxUserManager>()->RemoveUserSignedOutCallback(m_signOutCallbackToken);
    m_signOutCallbackToken = 0;

    auto cleanupEventHandle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    AddHandleToWaitSet(cleanupEventHandle);

    Managers::Get<OnlineManager>()->ShutdownAsync([cleanupEventHandle]()
    {
        DEBUGLOG("ShutdownAsync callback\n");
        SetEvent(cleanupEventHandle);
        CloseHandle(cleanupEventHandle);
    });

    Managers::Get<XboxUserManager>()->Shutdown();
}

void Game::CleanupUser(bool immediate)
{
    // What we need to do:

    ResetGameplayData();

    auto onlineManager = Managers::Get<OnlineManager>();

    // - stop matchmaking?
    if (onlineManager->IsMatchmaking())
    {
        onlineManager->CancelMatchmaking(immediate);
    }
    // - stop playing and leave the session?
    else if (onlineManager->IsConnected())
    {
        // sned host disconnect message to rest of players
        if (onlineManager->IsHost())
        {
            DataBufferWriter dataWriter;

            dataWriter.WriteStruct(DirectX::Colors::DarkOrange);
            dataWriter.WriteString("Host Disconnected");

            onlineManager->SendGameMessage(
                GameMessage(
                    GameMessageType::GameOver,
                    dataWriter.GetBuffer()
                )
            );
        }

        onlineManager->LeaveMultiplayerGame(immediate);
    }

    // - clean up PlayFabParty user
    if (Managers::Get<PlayFabPartyManager>()->HasLocalUser())
    {
        Managers::Get<PlayFabPartyManager>()->ClearLocalUser();
    }

    // - clean up local user state?
    if (0 != onlineManager->GetNetworkId())
    {
        m_peers.erase(onlineManager->GetNetworkId());
        m_localPlayerName.clear();
    }

    // - clean up waiting for the network
    onlineManager->SetNetworkAvailableCallback(nullptr);

    // - asynchronously shutdown the PlayFabPartyManager
    // NOTE: the reason we are wrapping this synchronous method into an asynchronous
    // task is so that we can do various asynchronous related cleanup simultaneously.
    // care must be taken when doing to this to ensure that no other Party APIs
    // are called once this has begun -- which this sample does not do.

    HANDLE shutdownEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    AddHandleToWaitSet(shutdownEvent);

    PartyShutdownAsync([shutdownEvent]()
    {
        SetEvent(shutdownEvent);
        CloseHandle(shutdownEvent);
    });

    Managers::Get<XboxUserManager>()->ClearCurrentUser();
}

void Game::PartyShutdownAsync(std::function<void()> completionCallback)
{
    auto asyncManager = Managers::Get<AsyncTaskManager>();
    auto asyncQueue = asyncManager->GetDefaultQueue();

    XAsyncBlock* asyncShutdownBlock = new XAsyncBlock
    {
        asyncQueue.get(),
        new std::function<void()>(completionCallback),
        [](XAsyncBlock* asyncBlock)
        {
            auto callbackMethod = reinterpret_cast<std::function<void()>*>(asyncBlock->context);
            (*callbackMethod)();
            delete callbackMethod;
            delete asyncBlock;
        }
    };

    XAsyncRun(
        asyncShutdownBlock,
        [](XAsyncBlock*)->HRESULT
    {
        Managers::Get<PlayFabPartyManager>()->Shutdown();
        return S_OK;
    });
}
