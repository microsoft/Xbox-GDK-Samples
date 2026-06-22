//--------------------------------------------------------------------------------------
// NetworkMessages.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "NetworkMessages.h"

using namespace PlayFabMultiplayerRumble;

GameMessage::GameMessage(GameMessageType type, uint32_t data) :
    m_type{type}
{
    m_data.resize(sizeof(unsigned));
    memcpy(m_data.data(), &data, sizeof(data));
}

GameMessage::GameMessage(GameMessageType type, std::string_view data) :
    m_type{type}
{
    m_data.resize(data.length() * sizeof(char));
    memcpy(m_data.data(), reinterpret_cast<const uint8_t*>(data.data()), m_data.size());
}

GameMessage::GameMessage(GameMessageType type, const std::vector<uint8_t> &data) :
    m_type{type},
    m_data{data}
{
}

GameMessage::GameMessage(const std::vector<uint8_t> &data)
{
    if (data.size() < (sizeof(GameMessageType) + sizeof(uint8_t)))
    {
        // Invalid message
        return;
    }

    memcpy(&m_type, data.data(), sizeof(GameMessageType));

    m_data.resize(data.size() - sizeof(GameMessageType));
    memcpy(m_data.data(), data.data() + sizeof(GameMessageType), m_data.size());
}

std::vector<uint8_t> GameMessage::Serialize() const
{
    if (m_type == GameMessageType::Unknown || m_data.empty())
    {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> packet(sizeof(GameMessageType) + m_data.size());

    memcpy(packet.data(), &m_type, sizeof(GameMessageType));
    memcpy(packet.data() + sizeof(GameMessageType), m_data.data(), m_data.size());

    return packet;
}

std::string GameMessage::StringValue() const
{
    if (!m_data.empty())
    {
        auto stringData = reinterpret_cast<const char *>(m_data.data());
        return std::string(stringData, stringData + (m_data.size() / sizeof(char)));
    }

    return "";
}

uint32_t GameMessage::UnsignedValue() const
{
    if (!m_data.empty())
    {
        return *(reinterpret_cast<const uint32_t *>(m_data.data()));
    }

    return 0;
}

const char* GameMessage::GetGameMessageTypeString() const noexcept
{
    static_assert((int)GameMessageType::MAX == 32);

    switch (m_type)
    {
    case GameMessageType::Unknown:              return STRINGIFY(GameMessageType::Unknown);
    case GameMessageType::GameCountdown:        return STRINGIFY(GameMessageType::GameCountdown);
    case GameMessageType::GameStart:            return STRINGIFY(GameMessageType::GameStart);
    case GameMessageType::GameTurn:             return STRINGIFY(GameMessageType::GameTurn);
    case GameMessageType::GameOver:             return STRINGIFY(GameMessageType::GameOver);
    case GameMessageType::GameLeft:             return STRINGIFY(GameMessageType::GameLeft);
    case GameMessageType::GameSettings:         return STRINGIFY(GameMessageType::GameSettings);
    case GameMessageType::PlayerJoined:         return STRINGIFY(GameMessageType::PlayerJoined);
    case GameMessageType::PlayerInfo:           return STRINGIFY(GameMessageType::PlayerInfo);
    case GameMessageType::PlayerState:          return STRINGIFY(GameMessageType::PlayerState);
    case GameMessageType::PlayerLeft:           return STRINGIFY(GameMessageType::PlayerLeft);
    case GameMessageType::PowerUpSpawn:         return STRINGIFY(GameMessageType::PowerUpSpawn);
    case GameMessageType::WorldSetup:           return STRINGIFY(GameMessageType::WorldSetup);
    case GameMessageType::WorldData:            return STRINGIFY(GameMessageType::WorldData);
    case GameMessageType::ShipSpawn:            return STRINGIFY(GameMessageType::ShipSpawn);
    case GameMessageType::ShipInput:            return STRINGIFY(GameMessageType::ShipInput);
    case GameMessageType::ShipData:             return STRINGIFY(GameMessageType::ShipData);
    case GameMessageType::ShipDeath:            return STRINGIFY(GameMessageType::ShipDeath);
    case GameMessageType::JoiningGame:          return STRINGIFY(GameMessageType::JoiningGame);
    case GameMessageType::OnlineDisconnect:     return STRINGIFY(GameMessageType::OnlineDisconnect);
    case GameMessageType::MatchmakingFailed:    return STRINGIFY(GameMessageType::MatchmakingFailed);
    case GameMessageType::MatchmakingCanceled:  return STRINGIFY(GameMessageType::MatchmakingCanceled);
    case GameMessageType::JoinGameFailed:       return STRINGIFY(GameMessageType::JoinGameFailed);
    case GameMessageType::JoinedGameComplete:   return STRINGIFY(GameMessageType::JoinedGameComplete);
    case GameMessageType::LeaveGameComplete:    return STRINGIFY(GameMessageType::LeaveGameComplete);
    case GameMessageType::MPPrivilegeError:     return STRINGIFY(GameMessageType::MPPrivilegeError);
    case GameMessageType::CPPrivilegeError:     return STRINGIFY(GameMessageType::CPPrivilegeError);
    case GameMessageType::RegionLatency:        return STRINGIFY(GameMessageType::RegionLatency);
    case GameMessageType::MigrateRegion:        return STRINGIFY(GameMessageType::MigrateRegion);
    case GameMessageType::FindLobbiesCompleted: return STRINGIFY(GameMessageType::FindLobbiesCompleted);
    case GameMessageType::FindLobbiesFailed:    return STRINGIFY(GameMessageType::FindLobbiesFailed);
    case GameMessageType::NetworkLost:          return STRINGIFY(GameMessageType::NetworkLost);
    }

    //we should never get here
    assert(false);
    return STRINGIFY(GameMessageType::Unknown);
}
