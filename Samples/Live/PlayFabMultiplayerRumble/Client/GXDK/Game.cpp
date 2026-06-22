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

extern void ExitGame();

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

using Microsoft::WRL::ComPtr;

namespace
{
    const DWORD WaitTimeoutInMs = 500;
}

Game::Game() noexcept :
    m_outputWidth(0),
    m_outputHeight(0)  
{
    /*while (!IsDebuggerPresent()) {}*/

    m_signInCallbackToken = 0;
    m_signOutCallbackToken = 0;
}

Game::~Game()
{
    CleanupUser();
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
    Managers::Get<OnlineManager>()->Tick(static_cast<float_t>(timer.GetElapsedSeconds()));

    if (Managers::Get<XboxUserManager>()->IsAnyoneSignedIn())
    {
        Managers::Get<PlayFabPartyManager>()->DoWork();
        Managers::Get<FriendsManager>()->DoWork();
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
}

void Game::OnDeactivated()
{   
}

void Game::OnSuspending()
{
    Managers::Get<ScreenManager>()->Suspend();
    Managers::Get<RenderManager>()->Suspend();
    Managers::Get<AudioManager>()->Suspend();
    Managers::Get<InputManager>()->Suspend();

    // We need to completely cleanup all networking and the user
    CleanupUser();

    // wait for all async completion handles
    WaitForAndCleanupHandles();
}

void Game::OnResuming()
{
    Managers::Get<InputManager>()->Resume();
    Managers::Get<AudioManager>()->Resume();
    Managers::Get<RenderManager>()->Resume();
    Managers::Get<ScreenManager>()->Resume();

    m_timer.ResetElapsedTime();

    // We need to go back to the start menu (this already waits for networking connectivity)
    Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
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

    //For debugging purposes
    //DEBUGLOG("Game::ProcessGameNetworkMessage: Receiving game message: %s", message->GetGameMessageTypeString());

    switch (message->GetMessageType())
    {
    case GameMessageType::MPPrivilegeError:
        Managers::Get<ScreenManager>()->ShowError("Your account does not have multiplayer privileges");
        break;

    case GameMessageType::CPPrivilegeError:
        Managers::Get<ScreenManager>()->ShowError("Your account does not have crossplay privileges");
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
            auto region = raw.substr(0, raw.find(':'));
            auto latency = std::strtoull(raw.substr(raw.find(':') + 1).c_str(), nullptr, 10);

            player->SetRegionLatency(region, latency);

            DEBUGLOG("Game::ProcessGameNetworkMessage: Received RegionLatency from %s: %s - %d", player->DisplayName.c_str(), region.c_str(), latency);
        }
        else
        {
            DEBUGLOG("Game::ProcessGameNetworkMessage: Received RegionLatency for a player we don't know");
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
            auto localPlayer = GetPlayerState(localId);
            if (localPlayer->InGame)
            {
                m_world->IsGameWon = true;
                m_world->SetGameInProgress(false);
                m_world->DeserializeGameOver(message->GetData());
            }
            else
            {
                ResetGameplayData();
            }
        }
        break;

    case GameMessageType::GameSettings:
    {
        m_world->DeserializeGameSettings(message->GetData());
        break;
    }

    case GameMessageType::GameStart:
    {
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
            auto playerState = std::make_shared<PlayerState>();
            playerState->IsLocalPlayer = false;
            playerState->ShipColor(0);
            playerState->ShipVariation(0);
            playerState->PeerId = sourceId;

            auto friendsManager = Managers::Get<FriendsManager>();
            friendsManager->ReadUserDisplayNameAsync(sourceId, [playerState, friendsManager](bool bSuccess, uint64_t xuid)
            {
                if (bSuccess)
                {
                    playerState->DisplayName = friendsManager->GetUserDisplayName(xuid);
                }
                else
                {
                    DEBUGLOG("Failed to read display name for xuid: %llu", xuid);
                }
            });

            m_peers[sourceId] = playerState;

            auto onlineManager = Managers::Get<OnlineManager>();
            GameMessage msg = GameMessage(GameMessageType::PlayerState, GetLocalPlayerState()->SerializePlayerStateData());
            onlineManager->SendGameMessage(msg);
        }
        break;
    }

    case GameMessageType::PlayerInfo:
    {
        auto itr = m_peers.find(sourceId);
        if (itr != m_peers.end() && sourceId != localId)
        {
            auto playerState = itr->second;
            playerState->DisplayName = message->StringValue();

            if (Managers::Get<OnlineManager>()->IsHost())
            {
                for (auto& [id, peer] : m_peers)
                {
                    // Don't resend this to the host or player that sent it initially
                    if (id != sourceId && !peer->IsLocalPlayer)
                    {
                        Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::PlayerInfo, playerState->DisplayName));
                    }
                }
            }

            DEBUGLOG("Game::ProcessGameNetworkMessage: Received PlayerInfo for: %ws", playerState->DisplayName.c_str());
        }
        else
        {
            DEBUGLOG("Game::ProcessGameNetworkMessage: Received PlayerInfo for unknown peer %u", sourceId);
        }
        break;
    }

    case GameMessageType::PlayerState:
    {
        DEBUGLOG("Game::ProcessGameNetworkMessage: Received a PlayerState message from %u", sourceId);
        auto playerState = GetPlayerState(sourceId);
        if (playerState != nullptr && sourceId != localId)
        {
            playerState->DeserializePlayerStateData(message->GetData());
        }
        else
        {
            DEBUGLOG("Game::ProcessGameNetworkMessage: No PlayerState exists for peer %u!", sourceId);
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
    {
        if (m_world->IsInitialized())
        {
            m_world->DeserializePowerUpSpawn(message->GetData());
        }
        else
        {
            DEBUGLOG("Game::ProcessGameNetworkMessage: World not initialized");
        }
        break;
    }

    case GameMessageType::ShipData:
    {
        if (m_world->IsInitialized())
        {
            auto peerState = GetPlayerState(sourceId);
            if (peerState != nullptr)
            {
                peerState->GetShip()->Deserialize(message->GetData());
            }
        }
        else
        {
            DEBUGLOG("Game::ProcessGameNetworkMessage: World not initialized");
        }
        break;
    }

    case GameMessageType::ShipDeath:
    {
        DEBUGLOG("Received a ShipDeath message from %u", sourceId);
        if (m_world->IsInitialized())
        {
            auto peerState = GetPlayerState(sourceId);
            if (peerState != nullptr)
            {
                m_world->DeserializeShipDeath(sourceId, message->GetData());
            }
            else
            {
                DEBUGLOG("PlayerState not found for %u", sourceId);
            }
        }
        else
        {
            DEBUGLOG("...World not initialized!");
        }
        break;
    }

    case GameMessageType::ShipInput:
    {
        if (m_world->IsInitialized())
        {
            auto peerState = GetPlayerState(sourceId);
            if (peerState != nullptr)
            {
                peerState->GetShip()->Input.Deserialize(message->GetData());
            }
        }
        else
        {
            DEBUGLOG("Game::ProcessGameNetworkMessage: World not initialized");
        }
        break;
    }

    case GameMessageType::ShipSpawn:
    {
        DEBUGLOG("Received a ShipSpawn message");
        if (m_world->IsInitialized())
        {
            m_world->DeserializeShipSpawn(message->GetData());
        }
        else
        {
            DEBUGLOG("Game::ProcessGameNetworkMessage: World not initialized");
        }
        break;
    }

    case GameMessageType::WorldData:
    {
        if (m_world->IsInitialized())
        {
            m_world->DeserializeWorldData(message->GetData());
        }
        else
        {
            DEBUGLOG("Game::ProcessGameNetworkMessage: World not initialized");
        }
        break;
    }

    case GameMessageType::WorldSetup:
    {
        if (!m_world->IsInitialized())
        {
            m_world->DeserializeWorldSetup(message->GetData());
        }
        else
        {
            DEBUGLOG("Game::ProcessGameNetworkMessage: World already initialized");
        }
        break;
    }

    case GameMessageType::JoiningGame:
    {
        Managers::Get<GameStateManager>()->SwitchToState(GameState::MPJoinGame);
        break;
    }

    case GameMessageType::MatchmakingFailed:
    {
        Managers::Get<ScreenManager>()->ShowError("Matchmaking Failed. Returning to Main Menu.", []() {
            Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
            });
        break;
    }

    case GameMessageType::JoinGameFailed:
    {
        Managers::Get<ScreenManager>()->ShowError("Failed to join game. Returning to Main Menu.", []() {
            Managers::Get<GameStateManager>()->SwitchToState(GameState::MainMenu);
            });
        break;
    }

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
    {
        Managers::Get<GameStateManager>()->SwitchToState(GameState::Lobby);
        break;
    }

    case GameMessageType::MatchmakingCanceled:
    {
        Managers::Get<GameStateManager>()->SwitchToState(GameState::ExitingGame);
        Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
        break;
    }

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
    
    case GameMessageType::GameTurn:
        break;
    case GameMessageType::GameLeft:
        break;

    case GameMessageType::FindLobbiesCompleted:
        break;
    case GameMessageType::FindLobbiesFailed:
        break;

    case GameMessageType::NetworkLost:
    {
        // cleans up the local user networking cache
        CleanupUser();

        // clean up xblcontexthandle
        Managers::Get<OnlineManager>()->OnNetworkLost();

        // wait for all async completion handles
        WaitForAndCleanupHandles();

        Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
        break;
    }
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

    Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::PlayerState, GetLocalPlayerState()->SerializePlayerStateData()));

    Managers::Get<ScreenManager>()->AddGameScreen<GamePlayScreen>();
    Managers::Get<ScreenManager>()->SetBackgroundsVisible(false);

    m_world->SetGameInProgress(true);

    if (Managers::Get<OnlineManager>()->IsHost())
    {
        m_world->GenerateWorld();

        Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::GameStart, 0));

        std::map<uint64_t, std::shared_ptr<Ship>> ships;

        for (const auto& item : m_peers)
        {
            if (item.second && !item.second->IsInactive())
            {
                ships[item.first] = item.second->GetShip();
            }
        }

        Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::WorldSetup, m_world->SerializeWorldSetup(ships)));
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

    // Remove all remote peers
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

    WaitForMultipleObjects(DWORD(m_waitHandles.size()), m_waitHandles.data(), true, WaitTimeoutInMs);

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
    };

    // NOTE: when the user signs out however, we always make sure to immediately put
    // the user back to the initial start screen
    auto userSignedOut = [this](const User &/*user*/)
    {
        CleanupUser();
        WaitForAndCleanupHandles();
        Managers::Get<GameStateManager>()->SwitchToState(GameState::StartMenu);
    };

    m_signInCallbackToken = Managers::Get<XboxUserManager>()->AddUserSignedInCallback(userSignedIn);
    m_signOutCallbackToken = Managers::Get<XboxUserManager>()->AddUserSignedOutCallback(userSignedOut);
}

void Game::MultiplayerShutdown()
{
    Managers::Get<XboxUserManager>()->RemoveUserSignedInCallback(m_signInCallbackToken);
    m_signInCallbackToken = 0;

    Managers::Get<XboxUserManager>()->RemoveUserSignedOutCallback(m_signOutCallbackToken);
    m_signOutCallbackToken = 0;

    auto cleanupEventHandle = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    AddHandleToWaitSet(cleanupEventHandle);

    Managers::Get<OnlineManager>()->ShutdownAsync([cleanupEventHandle]()
    {
        SetEvent(cleanupEventHandle);
        CloseHandle(cleanupEventHandle);
    });

    Managers::Get<XboxUserManager>()->Shutdown();
}

void Game::CleanupUser()
{
    // What we need to do:

    ResetGameplayData();

    // - stop matchmaking?
    if (Managers::Get<OnlineManager>()->IsMatchmaking())
    {
        Managers::Get<OnlineManager>()->CancelMatchmaking();
    }

    // - stop playing and leave the session?
    else if (Managers::Get<OnlineManager>()->IsConnected())
    {
        Managers::Get<OnlineManager>()->LeaveMultiplayerGame();
    }

    // - clean up PlayFabParty user
    if (Managers::Get<PlayFabPartyManager>()->HasLocalUser())
    {
        Managers::Get<PlayFabPartyManager>()->ClearLocalUser();
    }

    // - clean up local user state?
    if (0 != Managers::Get<OnlineManager>()->GetNetworkId())
    {
        m_peers.erase(Managers::Get<OnlineManager>()->GetNetworkId());
        m_localPlayerName.clear();
    }

    // - clean up waiting for the network
    Managers::Get<OnlineManager>()->SetNetworkAvailableCallback(nullptr);

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

void Game::PartyShutdownAsync(std::function<void()> callback)
{
    auto asyncManager = Managers::Get<AsyncTaskManager>();
    auto asyncQueue = asyncManager->GetDefaultQueue();
    auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [callback = std::move(callback)](XAsyncBlock*)
    {
        if (callback)
        {
            callback();
        }
    });

    HRESULT hr = XAsyncRun(&asyncHelper->asyncBlock, [](XAsyncBlock*)->HRESULT
    {
        Managers::Get<PlayFabPartyManager>()->Shutdown();
        return S_OK;
    });

    if (FAILED(hr))
    {
        LogError_HRESULT("XAsyncRun", hr);
        delete asyncHelper;
    }
}
