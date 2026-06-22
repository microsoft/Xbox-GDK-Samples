//--------------------------------------------------------------------------------------
// PlayerState.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "CollisionManager.h"
#include "PlayerState.h"
#include "Ship.h"
#include "SampleConfig.h"

using namespace PlayFabMultiplayerRumble;
using namespace DirectX;

PlayerState::PlayerState(std::string_view displayName)
{
    m_playerShip = std::make_shared<Ship>();

    if (displayName.empty())
    {
        DisplayName = "Player";
    }
    else
    {
        DisplayName = displayName;
    }

    EnterLobby();
}

void PlayerState::DeactivatePlayer()
{
    m_isInactive = true;
    m_playerShip->Score = 0;
    EnterLobby();
}

void PlayerState::EnterLobby()
{
    if (InGame)
    {
        m_playerShip->Die(nullptr, true);
        m_playerShip->RespawnTimer = 0.0f;
        InGame = false;
    }
 
    LobbyReady = false;
}

void PlayerState::ReactivatePlayer()
{
    m_isInactive = false;
}

void PlayerState::DeserializePlayerStateData(const std::vector<uint8_t> &data)
{
    PlayerStateData rcvdPlayerStateData = PlayerStateData();
    CopyMemory(&rcvdPlayerStateData, data.data(), sizeof(PlayerStateData));

    DEBUGLOG("Received player state data: InGame = %u; LobbyReady = %u; ColorIndex = %u; ColorVariation = %u", 
        rcvdPlayerStateData.inGame, 
        rcvdPlayerStateData.lobbyReady, 
        rcvdPlayerStateData.shipColorIndex, 
        rcvdPlayerStateData.shipVariation);

    InGame = rcvdPlayerStateData.inGame;
    LobbyReady = rcvdPlayerStateData.lobbyReady;
    ShipColor(rcvdPlayerStateData.shipColorIndex);
    ShipVariation(rcvdPlayerStateData.shipVariation);
}

std::vector<uint8_t> PlayerState::SerializePlayerStateData() const
{
    PlayerStateData playerStateData = { InGame, LobbyReady, m_shipColor, m_shipVariation };

    DEBUGLOG("Serializing player state data: InGame = %u; LobbyReady = %u; ColorIndex = %u; ColorVariation = %u", 
        InGame, 
        LobbyReady, 
        m_shipColor, 
        m_shipVariation);

    std::vector<uint8_t> data(sizeof(PlayerStateData));
    CopyMemory(data.data(), &playerStateData, sizeof(PlayerStateData));

    return data;
}

void PlayerState::SetRegionLatency(std::string_view region, uint64_t latency)
{
    auto it = std::find_if(
        std::begin(m_latencyList),
        std::end(m_latencyList),
        [&](const std::pair<std::string, uint64_t>& item)
        {
            return item.first == region;
        });

    if (it != std::end(m_latencyList))
    {
        (*it).second = latency;
    }
    else
    {
        m_latencyList.emplace_back(std::pair<std::string, uint64_t>{ region, latency });
    }
}
