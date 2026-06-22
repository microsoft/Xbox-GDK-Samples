#include "pch.h"
#include "XboxLiveOnlineManager.h"
#include <XPackage.h>
using namespace NetRumble;

#define STRINGIFY(x) #x

const char *NetRumble::MessageTypeString(GameMessageType type)
{
    switch (type)
    {
    case GameMessageType::Unknown:            return STRINGIFY(GameMessageType::Unknown);
    case GameMessageType::GameCountdown:      return STRINGIFY(GameMessageType::GameCountdown);
    case GameMessageType::GameStart:          return STRINGIFY(GameMessageType::GameStart);
    case GameMessageType::GameTurn:           return STRINGIFY(GameMessageType::GameTurn);
    case GameMessageType::GameOver:           return STRINGIFY(GameMessageType::GameOver);
    case GameMessageType::GameLeft:           return STRINGIFY(GameMessageType::GameLeft);
    case GameMessageType::GameSettings:       return STRINGIFY(GameMessageType::GameSettings);
    case GameMessageType::PlayerJoined:       return STRINGIFY(GameMessageType::PlayerJoined);
    case GameMessageType::PlayerInfo:         return STRINGIFY(GameMessageType::PlayerInfo);
    case GameMessageType::PlayerState:        return STRINGIFY(GameMessageType::PlayerState);
    case GameMessageType::PlayerLeft:         return STRINGIFY(GameMessageType::PlayerLeft);
    case GameMessageType::PowerUpSpawn:       return STRINGIFY(GameMessageType::PowerUpSpawn);
    case GameMessageType::WorldSetup:         return STRINGIFY(GameMessageType::WorldSetup);
    case GameMessageType::WorldData:          return STRINGIFY(GameMessageType::WorldData);
    case GameMessageType::ShipSpawn:          return STRINGIFY(GameMessageType::ShipSpawn);
    case GameMessageType::ShipInput:          return STRINGIFY(GameMessageType::ShipInput);
    case GameMessageType::ShipData:           return STRINGIFY(GameMessageType::ShipData);
    case GameMessageType::ShipDeath:          return STRINGIFY(GameMessageType::ShipDeath);
    case GameMessageType::OnlineDisconnect:   return STRINGIFY(GameMessageType::OnlineDisconnect);
    case GameMessageType::MatchmakingFailed:  return STRINGIFY(GameMessageType::MatchmakingFailed);
    case GameMessageType::JoinGameFailed:     return STRINGIFY(GameMessageType::JoinGameFailed);
    case GameMessageType::JoinedGameComplete: return STRINGIFY(GameMessageType::JoinedGameComplete);
    case GameMessageType::LeaveGameComplete:  return STRINGIFY(GameMessageType::LeaveGameComplete);
    case GameMessageType::MPPrivilegeError:   return STRINGIFY(GameMessageType::MPPrivilegeError);
    case GameMessageType::RegionLatency:      return STRINGIFY(GameMessageType::RegionLatency);
    case GameMessageType::MigrateRegion:      return STRINGIFY(GameMessageType::MigrateRegion);
    default:                                  return STRINGIFY(GameMessageType::Unknown);
    }
}

void XboxLiveOnlineManager::SendGameMessage(const GameMessage &message)
{
    DEBUGLOG("Sending message: %s\n", MessageTypeString(message.MessageType()));
    auto packet = message.Serialize();
    Managers::Get<PlayFabPartyManager>()->SendGameMessage(packet);
}

bool XboxLiveOnlineManager::IsHost() const
{
    return Managers::Get<PlayFabPartyManager>()->IsHost();
}

bool XboxLiveOnlineManager::IsConnected() const
{
    return Managers::Get<PlayFabPartyManager>()->IsConnected();
}

uint64_t XboxLiveOnlineManager::GetNetworkId() const
{
    return GetCurrentUserXuid();
}

void XboxLiveOnlineManager::RegisterOnlineMessageHandler(OnlineMessageHandler handler)
{
    m_messageHandler = handler;

    Managers::Get<PlayFabPartyManager>()->SetGameMessageHandler(
        [this](uint64_t xuid, std::shared_ptr<GameMessage> message)
        {
            DEBUGLOG("Receiving message: %s\n", MessageTypeString(message->MessageType()));
            m_messageHandler(xuid, message.get());
        });
}
