//--------------------------------------------------------------------------------------
// NetworkMessages.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

namespace NetRumble
{

enum class GameMessageType
{
    Unknown,

    GameCountdown,
    GameStart,
    GameTurn,
    GameOver,
    GameLeft,
    GameSettings,

    PlayerJoined,
    PlayerInfo,
    PlayerState,
    PlayerLeft,

    PowerUpSpawn,

    WorldSetup,
    WorldData,

    ShipSpawn,
    ShipInput,
    ShipData,
    ShipDeath,

    JoiningGame,

    OnlineDisconnect,
    MatchmakingFailed,
    MatchmakingCanceled,
    JoinGameFailed,
    JoinedGameComplete,
    LeaveGameComplete,

    MPPrivilegeError,
    CPPrivilegeError,

    RegionLatency,
    MigrateRegion,

    NetworkLost
};

class GameMessage final
{
public:
    GameMessage() = default;
    GameMessage(GameMessageType type, uint32_t data);
    GameMessage(GameMessageType type, std::string_view data);
    GameMessage(GameMessageType type, const std::vector<uint8_t> &data);
    GameMessage(const std::vector<uint8_t> &data);

    inline GameMessageType MessageType() const { return m_type; }
    inline void MessageType(GameMessageType type) { m_type = type; }

    inline const std::vector<uint8_t> &RawData() const { return m_data; }
    inline void RawData(const std::vector<uint8_t> &data) { m_data = data; }

    std::string StringValue() const;
    uint32_t UnsignedValue() const;

    std::vector<uint8_t> Serialize() const;

private:
    GameMessageType m_type = GameMessageType::Unknown;
    std::vector<uint8_t> m_data;
};

}
