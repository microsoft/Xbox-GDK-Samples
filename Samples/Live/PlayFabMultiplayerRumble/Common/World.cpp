//--------------------------------------------------------------------------------------
// World.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "World.h"
#include "Managers.h"
#include "DataBuffer.h"
#include "PlayerState.h"
#include "Weapon.h"
#include "LaserWeapon.h"
#include "MineWeapon.h"
#include "DoubleLaserWeapon.h"
#include "TripleLaserWeapon.h"
#include "RocketWeapon.h"
#include "DoubleLaserPowerUp.h"
#include "TripleLaserPowerUp.h"
#include "RocketPowerUp.h"
#include "RandomMath.h"
#include "IOnlineManager.h"

#include "Game.h"
#include "SampleConfig.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

World::World()
{
    m_isGameInProgress = false;
    WinningScore = 5;

    ResetDefaults();

    m_starfield = std::make_unique<Starfield>(SimpleMath::Vector2());

    // Set outer barrier and world dimensions
    m_outerBarrierCounts = XMINT2(c_BarrierCount, c_BarrierCount);
    m_worldDimensions.left = c_BarrierSize;
    m_worldDimensions.right = m_outerBarrierCounts.x * c_BarrierSize;
    m_worldDimensions.top = c_BarrierSize;
    m_worldDimensions.bottom = m_outerBarrierCounts.y * c_BarrierSize;

    // Load world barrier textures
    auto contentManager = Managers::Get<ContentManager>();
    m_cornerBarrierTexture = contentManager->LoadTexture(L"Assets\\Textures\\barrierEnd.png");
    m_horizontalBarrierTexture = contentManager->LoadTexture(L"Assets\\Textures\\barrierRed.png");
    m_verticalBarrierTexture = contentManager->LoadTexture(L"Assets\\Textures\\barrierPurple.png");

    // Initialize the CollisionManager
    auto collisionManager = Managers::Get<CollisionManager>();
    collisionManager->SetDimensions(m_worldDimensions);
    m_worldDimensions.top = m_worldDimensions.left = 0;

    // Setup the collision version of the world barriers
    auto MakeRect = [](int x, int y, int w, int h)
    {
        RECT r = { x, y, x + w, y + h };
        return r;
    };

    collisionManager->Barriers().clear();
    collisionManager->Barriers().push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.top, m_worldDimensions.right - m_worldDimensions.left, c_BarrierSize)); // top edge
    collisionManager->Barriers().push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.bottom, m_worldDimensions.right - m_worldDimensions.left, c_BarrierSize)); // bottom edge
    collisionManager->Barriers().push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.top, c_BarrierSize, m_worldDimensions.bottom - m_worldDimensions.top)); // left edge
    collisionManager->Barriers().push_back(MakeRect(m_worldDimensions.right, m_worldDimensions.top, c_BarrierSize, m_worldDimensions.bottom - m_worldDimensions.top)); // right edge

    // Setup the rendering version of the world barriers
    m_cornerBarriers.clear();
    m_cornerBarriers.push_back(MakeRect(m_worldDimensions.left  , m_worldDimensions.top    , 4 * c_BarrierSize, 4 * c_BarrierSize)); // top-left corner
    m_cornerBarriers.push_back(MakeRect(m_worldDimensions.right , m_worldDimensions.top    , 4 * c_BarrierSize, 4 * c_BarrierSize)); // top-right corner
	m_cornerBarriers.push_back(MakeRect(m_worldDimensions.right , m_worldDimensions.bottom , 4 * c_BarrierSize, 4 * c_BarrierSize)); // bottom-right corner
    m_cornerBarriers.push_back(MakeRect(m_worldDimensions.left  , m_worldDimensions.bottom , 4 * c_BarrierSize, 4 * c_BarrierSize)); // bottom-left corner

    m_horizontalBarriers.clear();
    for (int i = 2; i < m_outerBarrierCounts.x - 1; i++)
    {
        m_horizontalBarriers.push_back(MakeRect(m_worldDimensions.left + c_BarrierSize * i, m_worldDimensions.top, c_BarrierSize, c_BarrierSize)); // top edge
        m_horizontalBarriers.push_back(MakeRect(m_worldDimensions.left + c_BarrierSize * i, m_worldDimensions.top + m_worldDimensions.right - m_worldDimensions.left, c_BarrierSize, c_BarrierSize)); // bottom edge
    }

    m_verticalBarriers.clear();
    for (int i = 2; i < m_outerBarrierCounts.y - 1; i++)
    {
        m_verticalBarriers.push_back(MakeRect(m_worldDimensions.left, m_worldDimensions.top + c_BarrierSize * i, c_BarrierSize, c_BarrierSize)); // left edge
        m_verticalBarriers.push_back(MakeRect(m_worldDimensions.right, m_worldDimensions.top + c_BarrierSize * i, c_BarrierSize, c_BarrierSize)); // right edge
    }
}

// Clear the world of objects and reset default values
void World::ResetDefaults()
{
    m_asteroids.clear();

    m_isInitialized = false;
    m_updatesSinceWorldDataSent = 0;
    m_powerUp = nullptr;
    m_powerUpTimer = c_MaximumPowerUpTimer;

    IsGameWon = false;
    WinnerName = "";
    WinningColor = Colors::White;

    if (g_game)
    {
        g_game->ClearPlayerScores();
    }

    Managers::Get<CollisionManager>()->Collection().ApplyPendingRemovals();
    Managers::Get<CollisionManager>()->Collection().clear();
}

// Generate the world, placing asteroids and all ships
void World::GenerateWorld()
{
    // first reset world defaults from any prior game
    ResetDefaults();

    auto collisionMgr = Managers::Get<CollisionManager>();

    // initialize the ships, finding spawn points and resetting score
    for (const auto& playerState : g_game->GetAllPlayerStates())
    {
        if (playerState && playerState->LobbyReady)
        {
            auto ship = playerState->GetShip();
            ship->Initialize(c_Asteroids);
            ship->Position = collisionMgr->FindSpawnPoint(ship.get(), ship->Radius);
        }
    }

    // place the asteroids
    for (size_t i = 0; i < c_Asteroids; ++i)
    {
        // choose one of three radii and texture variations
        float radius = 32.0f;
        switch (RandomMath::RandomBetween(0, 2))
        {
        case 0:
            radius = 32.0f;
            break;
        case 1:
            radius = 60.0f;
            break;
        case 2:
            radius = 96.0f;
            break;
        }
        int variation = RandomMath::RandomBetween(0, Asteroid::c_Variations - 1);

        // create the asteroid
        auto asteroid = std::make_shared<Asteroid>(radius, variation);
        asteroid->Initialize();
        asteroid->Position = collisionMgr->FindSpawnPoint(asteroid.get(), radius);
        m_asteroids.push_back(asteroid);
        
    }

    auto localState = g_game->GetLocalPlayerState();
    if (localState)
    {
        m_starfield->Reset(localState->GetShip()->Position);
    }

    m_isInitialized = true;
}

std::vector<uint8_t> World::SerializeShipSpawn(uint64_t peerid) const
{
    auto playerState = g_game->GetPlayerState(peerid);
    if (playerState != nullptr)
    {
        auto ship = playerState->GetShip();
        if (ship != nullptr)
        {
            SimpleMath::Vector2 spawnPt = Managers::Get<CollisionManager>()->FindSpawnPoint(ship.get(), ship->Radius);
            DataBufferWriter dataWriter;

            dataWriter.WriteUInt64(peerid);
            dataWriter.WriteStruct(spawnPt);

            return dataWriter.GetBuffer();
        }
    }

    return std::vector<uint8_t>();
}

void World::DeserializeShipSpawn(const std::vector<uint8_t>& data)
{
    DataBufferReader dataReader(data);

    auto peerid = dataReader.ReadUInt64();
    auto x = dataReader.ReadSingle();
    auto y = dataReader.ReadSingle();
    SimpleMath::Vector2 position = SimpleMath::Vector2(x, y);

    DEBUGLOG("DeserializeShipSpawn() received Peerid %u at (%f, %f)", peerid, position.x, position.y);

    auto playerState = g_game->GetPlayerState(peerid);
    if (playerState != nullptr)
    {
        auto ship = playerState->GetShip();
        if (ship != nullptr)
        {
            ship->Position = position;
            ship->Initialize(playerState->IsLocalPlayer);
        }
    }
}

std::vector<unsigned char> World::SerializePowerUpSpawn() const
{
    DataBufferWriter dataWriter;

    dataWriter.WriteByte(static_cast<byte>(PowerUp::ChooseNextPowerUpType()));

    XMFLOAT2 spawnPt = Managers::Get<CollisionManager>()->FindSpawnPoint(nullptr, 50.0f);
    dataWriter.WriteStruct(spawnPt);

    return dataWriter.GetBuffer();
}

void World::DeserializePowerUpSpawn(const std::vector<uint8_t> &data)
{
    DataBufferReader dataReader(data);

    PowerUpType powerUpType = static_cast<PowerUpType>(dataReader.ReadByte());
    SimpleMath::Vector2 position;
    dataReader.ReadStruct(position);

    DEBUGLOG("DeserializePowerUpSpawn() received powerUp type %u at (%f, %f)", powerUpType, position.x, position.y);

    SpawnPowerUp(powerUpType, position);
}

// Prepare the world data for the WorldData packet
std::vector<unsigned char> World::SerializeWorldData() const
{
    DataBufferWriter dataWriter;

    // write the asteroid data
    for (size_t i = 0; i < c_Asteroids; ++i)
    {
        dataWriter.WriteStruct(m_asteroids[i]->Position);
        dataWriter.WriteStruct(m_asteroids[i]->Velocity);
    }

    return dataWriter.GetBuffer();
}

void World::DeserializeWorldData(const std::vector<uint8_t> &data)
{
    DataBufferReader dataReader(data);

    // update the asteroid data
    for (size_t i = 0; i < c_Asteroids; ++i)
    {
        dataReader.ReadStruct(m_asteroids[i]->Position);
        dataReader.ReadStruct(m_asteroids[i]->Velocity);
    }
}

void World::DeserializeGameSettings(const std::vector<uint8_t> &data)
{
    DataBufferReader dataReader(data);
    WinningScore = dataReader.ReadInt32();
}

// Prepare the member ships and world data for the WorldSetup packet
std::vector<uint8_t> World::SerializeWorldSetup(std::map<uint64_t, std::shared_ptr<Ship>>& ships) const
{
    DataBufferWriter dataWriter;

    dataWriter.WriteUInt32(static_cast<uint32_t>(ships.size()));

    // write active ship data
    for (auto& activeShipPair : ships)
    {
        auto peerid = activeShipPair.first;
        auto ship = activeShipPair.second;

        dataWriter.WriteUInt64(peerid);
        dataWriter.WriteStruct(ship->Position);

        WeaponType currentWeaponType = WeaponType::Unknown;
        if (ship->PrimaryWeapon != nullptr)
        {
            currentWeaponType = ship->PrimaryWeapon->GetWeaponType();
        }
        dataWriter.WriteByte(static_cast<uint8_t>(currentWeaponType));

        DEBUGLOG("SerializeWorldSetup() sending Peer %llu at (%f, %f) with weapon %u", peerid, ship->Position.x, ship->Position.y, currentWeaponType);
    }

    // write the asteroid data
    for (size_t i = 0; i < c_Asteroids; ++i)
    {
        dataWriter.WriteSingle(m_asteroids[i]->Radius);
        dataWriter.WriteByte(static_cast<uint8_t>(m_asteroids[i]->Variation));
        dataWriter.WriteStruct(m_asteroids[i]->Position);
        dataWriter.WriteStruct(m_asteroids[i]->Velocity);
    }

    // write the powerUp data
    if (m_powerUp != nullptr)
    {
        dataWriter.WriteByte(static_cast<uint8_t>(m_powerUp->GetPowerUpType()));
        dataWriter.WriteStruct(m_powerUp->Position);
    }
    else
    {
        dataWriter.WriteByte(static_cast<uint8_t>(PowerUpType::Unknown));
        dataWriter.WriteSingle(0.0f);
        dataWriter.WriteSingle(0.0f);
    }

    // write the game mode and winning score (redundant with the GameSettings packet, but that packet isn't sent to those who join in progress)
    dataWriter.WriteInt32(WinningScore);

    return dataWriter.GetBuffer();
}

void World::DeserializeWorldSetup(const std::vector<uint8_t> &data)
{
    DataBufferReader dataReader(data);

    // first reset world defaults from any prior game
    ResetDefaults();

    // read the members' ship data
    auto memberSize = dataReader.ReadUInt32();
    for (uint32_t i = 0; i < memberSize; ++i)
    {
        auto peerid = dataReader.ReadUInt64();

        SimpleMath::Vector2 position;
        dataReader.ReadStruct(position);
        
        WeaponType currentWeaponType = static_cast<WeaponType>(dataReader.ReadByte());

        DEBUGLOG("World::DeserializeWorldSetup: Received Peerid %u at (%f, %f) with weapon %u", peerid, position.x, position.y, currentWeaponType);

        auto playerState = g_game->GetPlayerState(peerid);
        if (playerState != nullptr)
        {
            auto ship = playerState->GetShip();

            ship->Initialize(playerState->IsLocalPlayer);
            ship->Position = position;

            switch (currentWeaponType)
            {
                case WeaponType::Laser:       ship->PrimaryWeapon = std::make_shared<LaserWeapon>(ship.get());       break;
                case WeaponType::DoubleLaser: ship->PrimaryWeapon = std::make_shared<DoubleLaserWeapon>(ship.get()); break;
                case WeaponType::TripleLaser: ship->PrimaryWeapon = std::make_shared<TripleLaserWeapon>(ship.get()); break;
                case WeaponType::Rocket:      ship->PrimaryWeapon = std::make_shared<RocketWeapon>(ship.get());      break;
                case WeaponType::Mine:        ship->PrimaryWeapon = std::make_shared<MineWeapon>(ship.get());        break;
            }
        }
    }

    // read the asteroid data
    for (size_t i = 0; i < c_Asteroids; ++i)
    {
        float radius = dataReader.ReadSingle();
        m_asteroids.push_back(std::make_shared<Asteroid>(radius, dataReader.ReadByte()));
        m_asteroids[i]->Initialize();
        dataReader.ReadStruct(m_asteroids[i]->Position);
        dataReader.ReadStruct(m_asteroids[i]->Velocity);
    }

    // read the powerUp data
    PowerUpType powerUpType = static_cast<PowerUpType>(dataReader.ReadByte());
    SimpleMath::Vector2 position;
    dataReader.ReadStruct(position);
    SpawnPowerUp(powerUpType, position);

    // read the game mode and winning score
    WinningScore = dataReader.ReadInt32();

    auto localstate = g_game->GetLocalPlayerState();
    if (localstate && localstate->GetShip())
    {
        m_starfield->Reset(localstate->GetShip()->Position);
    }

    m_isInitialized = true;
}

std::vector<uint8_t> World::SerializeShipDeath(std::shared_ptr<Ship> localShip) const
{
    if (localShip)
    {
        DataBufferWriter dataWriter;

        auto lastDamagedBy = localShip->LastDamagedBy;
        if (lastDamagedBy != nullptr &&
            lastDamagedBy->GetType() == GameplayObjectType::Ship &&
            lastDamagedBy != localShip.get())
        {
            uint64_t killer = 0;

            for (const auto& playerState : g_game->GetAllPlayerStates())
            {
                if (playerState && playerState->InGame)
                {
                    auto ship = playerState->GetShip();
                    if (ship && ship.get() == lastDamagedBy)
                    {
                        killer = playerState->PeerId;
                        break;
                    }
                }
            }

            dataWriter.WriteUInt64(killer);
        }
        else
        {
            dataWriter.WriteUInt64(0);
        }

        return dataWriter.GetBuffer();
    }

    return std::vector<unsigned char>();
}

void World::DeserializeShipDeath(uint64_t peerid, const std::vector<uint8_t> &data)
{
    DEBUGLOG("DeserializeShipDeath() received for peer %u", peerid);

    auto playerState = g_game->GetPlayerState(peerid);
    if (playerState == nullptr)
    {
        return;
    }

    auto shipKilled = playerState->GetShip();
    if (shipKilled == nullptr)
    {
        return;
    }

    DataBufferReader dataReader(data);

    std::shared_ptr<Ship> killerShip = nullptr;
    auto killerid = dataReader.ReadUInt64();
    if (killerid != 0)
    {
        auto killerState = g_game->GetPlayerState(killerid);
        if (killerState)
        {
            killerShip = killerState->GetShip();
        }
    }

    shipKilled->Die(killerShip.get(), false);
}

std::vector<unsigned char> World::SerializeGameOver() const
{
    DataBufferWriter dataWriter;

    dataWriter.WriteStruct(WinningColor);
    dataWriter.WriteString(WinnerName);

    return dataWriter.GetBuffer();
}

void World::DeserializeGameOver(const std::vector<uint8_t> &data)
{
    DataBufferReader dataReader(data);

    dataReader.ReadStruct(WinningColor);

    WinnerName = dataReader.ReadString();

    DEBUGLOG("DeserializeGameOver() received with winner %ws and color (%f, %f, %f, %f)", WinnerName.c_str(), WinningColor.f[0], WinningColor.f[1], WinningColor.f[2], WinningColor.f[3]);
}

void World::Update(float totalTime, float elapsedTime)
{
    UNREFERENCED_PARAMETER(totalTime);

    if (!IsGameWon && Managers::Get<OnlineManager>()->IsHost())
    {
        int highScore = MININT;
        std::string highScoreName = "";
        DirectX::XMVECTORF32 highScoreColor = Colors::White;

        auto playerStates = g_game->GetAllPlayerStates();

        if (playerStates.size() == 1 && SampleConfig::c_AllowSinglePlayerMatch == false)
        {
            highScore = WinningScore;
            highScoreName = "Insufficient Players";
            highScoreColor = DirectX::Colors::DarkOrange;
        }
        else
        {
            for (const auto& playerState : playerStates)
            {
                if (playerState && playerState->InGame)
                {
                    auto ship = playerState->GetShip();

                    // get current high score (and scorer)
                    if (ship->Score > highScore)
                    {
                        highScore = ship->Score;
                        highScoreName = playerState->DisplayName;
                        highScoreColor = ship->Color;
                    }

                    // respawn players
                    if (ship && !ship->Active() && ship->RespawnTimer <= 0.0f)
                    {
                        // send ship spawn message and immediately process locally
                        auto messageData = SerializeShipSpawn(playerState->PeerId);
                        Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::ShipSpawn, messageData));
                        DeserializeShipSpawn(messageData);
                    }
                }
            }
        }

        // respawn the power-up
        if (m_powerUp == nullptr)
        {
            m_powerUpTimer -= elapsedTime;
            if (m_powerUpTimer <= 0.0f)
            {
                // send the power-up-spawn packet and immediately process locally
                auto messageData = SerializePowerUpSpawn();
                Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::PowerUpSpawn, messageData));
                DeserializePowerUpSpawn(messageData);
            }
        }
        else
        {
            m_powerUpTimer = c_MaximumPowerUpTimer;
        }

        // check if game has been won
        if (highScore >= WinningScore)
        {
            DEBUGLOG("GAME OVER");

            WinnerName = highScoreName;
            WinningColor = highScoreColor;

            Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::GameOver, SerializeGameOver()));

            SetGameInProgress(false);
            IsGameWon = true;
        }
    }
    // End Host

    // update all player ships based on last input
    for (const auto& playerState : g_game->GetAllPlayerStates())
    {
        if (playerState && playerState->InGame)
        {
            auto ship = playerState->GetShip();
            if (ship)
            {
                if (ship->Active())
                {
                    ship->Update(elapsedTime);

                    // check for ship death
                    //   -- server is authority on death of own ship
                    if (playerState->IsLocalPlayer && ship->Life < 0 && !IsGameWon)
                    {
                        // send local ship death message and immediately process locally
                        auto messageData = SerializeShipDeath(ship);
                        Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::ShipDeath, messageData));
                        DeserializeShipDeath(playerState->PeerId, messageData);
                    }
                }
                else if (ship->RespawnTimer > 0.0f)
                {
                    // everyone (not just the host) updates all respawn timers so we can display respawn countdown in HUD
                    ship->RespawnTimer -= elapsedTime;
                }
            }
        }
    }

    // update all asteroids
    for(auto &asteroid : m_asteroids)
    {
        if (asteroid->Active())
        {
            asteroid->Update(elapsedTime);
        }
    }

    // update the power-up
    if (m_powerUp != nullptr)
    {
        if (m_powerUp->Active())
        {
            m_powerUp->Update(elapsedTime);
        }
        else
        {
            m_powerUp = nullptr;
        }
    }

    // update collision manager to apply all physics
    Managers::Get<CollisionManager>()->Update(elapsedTime);

    // update the particle-effect manager
    Managers::Get<ParticleEffectManager>()->Update(elapsedTime);

    // final host duties
    // send everyone an update on the latest state of the world
    if (Managers::Get<OnlineManager>()->IsHost())
    {
        if (m_updatesSinceWorldDataSent >= c_UpdatesBetweenWorldDataPackets)
        {
            Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::WorldData, SerializeWorldData()));

            //TheGame->SendShipUpdates();

            m_updatesSinceWorldDataSent = 0;
        }
        else
        {
            m_updatesSinceWorldDataSent++;
        }
    }
}

void World::Draw(float elapsedTime) const
{
    float viewportWidth = static_cast<float>(g_game->GetWindowWidth());
    float viewportHeight = static_cast<float>(g_game->GetWindowHeight());
    auto localShip = g_game->GetLocalPlayerState()->GetShip();

    XMFLOAT2 center = XMFLOAT2(localShip->Position.x - viewportWidth / 2.0f, localShip->Position.y - viewportHeight / 2.0f);

    // pull the center inwards so that it doesn't show a ton of the space outside the game
    center.x = std::min(std::max(center.x, m_worldDimensions.left - c_VisualPadding), m_worldDimensions.right - viewportWidth + c_VisualPadding);
    center.y = std::min(std::max(center.y, m_worldDimensions.top - c_VisualPadding), m_worldDimensions.bottom - viewportHeight + c_VisualPadding);

    // draw the starfield
    m_starfield->Draw(center);

    if (m_isInitialized)
    {
        auto renderContext = Managers::Get<RenderManager>()->GetRenderContext(BlendMode::NonPremultiplied);
        XMMATRIX transform = XMMatrixTranslation(-center.x, -center.y, 0);

        renderContext->Begin(transform);

        // draw the barriers
		float rotation = 0;
		for (auto &corner : m_cornerBarriers)
		{
			renderContext->Draw(m_cornerBarrierTexture, corner, DirectX::Colors::White, rotation);
			rotation += DirectX::XM_PIDIV2;
		}
        std::for_each(m_horizontalBarriers.begin(), m_horizontalBarriers.end(), [&](RECT r) { renderContext->Draw(m_horizontalBarrierTexture, r); });
        std::for_each(m_verticalBarriers.begin(), m_verticalBarriers.end(), [&](RECT r) { renderContext->Draw(m_verticalBarrierTexture, r); });

        // draw the powerup
        if (m_powerUp != nullptr && m_powerUp->Active())
        {
            m_powerUp->Draw(elapsedTime, renderContext.get());
        }

        // draw the asteroids
        for(auto &asteroid : m_asteroids)
        {
            if (asteroid->Active())
            {
                asteroid->Draw(elapsedTime, renderContext.get());
            }
        }

        // draw each ship
        for (const auto& playerState : g_game->GetAllPlayerStates())
        {
            if (playerState && playerState->InGame)
            {
                auto ship = playerState->GetShip();
                if (ship && ship->Active())
                {
                    ship->Draw(elapsedTime, renderContext.get(), false);
                }
            }
        }

        // draw the alpha-blended particles
        Managers::Get<ParticleEffectManager>()->Draw(renderContext.get(), SpriteBlendMode::Alpha);

        renderContext->End();

        renderContext = Managers::Get<RenderManager>()->GetRenderContext(BlendMode::Additive);

        // draw the additive particles
        renderContext->Begin(transform, SpriteSortMode::SpriteSortMode_Texture);

        Managers::Get<ParticleEffectManager>()->Draw(renderContext.get(), SpriteBlendMode::Additive);

        renderContext->End();
    }
}

void World::SpawnPowerUp(PowerUpType type, const DirectX::SimpleMath::Vector2 & position)
{
    switch (type)
    {
    case PowerUpType::DoubleLaser:
        m_powerUp = std::make_shared<DoubleLaserPowerUp>();
        break;
    case PowerUpType::TripleLaser:
        m_powerUp = std::make_shared<TripleLaserPowerUp>();
        break;
    case PowerUpType::Rocket:
        m_powerUp = std::make_shared<RocketPowerUp>();
        break;
    default:
        m_powerUp = nullptr;
        break;
    }

    if (m_powerUp != nullptr)
    {
        m_powerUp->Position = position;
        m_powerUp->Initialize();
    }
}
