//--------------------------------------------------------------------------------------
// IOnlineManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "IOnlineManager.h"
#include "Managers.h"

using namespace PlayFabMultiplayerRumble;

bool IOnlineManager::IsMatchmaking() const noexcept
{
    return m_mpState == OnlineState::Matchmaking;
}

bool IOnlineManager::HasMultiplayerPrivileges() const noexcept
{
    return m_hasMultiplayerPrivileges;
}

bool IOnlineManager::HasCrossplayPrivileges() const noexcept
{
    return m_hasCrossplayPrivileges;
}

bool IOnlineManager::HasPendingInviteSession() const noexcept
{
    return !m_joiningSession.empty();
}

bool IOnlineManager::IsConnected() const
{
    return Managers::Get<PlayFabPartyManager>()->IsConnected();
}

void IOnlineManager::RegisterOnlineMessageHandler(OnlineMessageHandler handler) noexcept
{
    m_messageHandler = handler;

    Managers::Get<PlayFabPartyManager>()->SetGameMessageHandler(
        [this](uint64_t xuid, std::shared_ptr<GameMessage> message)
        {
            if (message->GetMessageType() != GameMessageType::WorldData)
            {
                DEBUGLOG("Receiving message: %s", message->GetGameMessageTypeString());
            }
            
            m_messageHandler(xuid, message.get());
        });
}

void IOnlineManager::SendGameMessage(const GameMessage& message) noexcept
{
    GameMessageType msgType = message.GetMessageType();

    if (msgType != GameMessageType::WorldData &&
        msgType != GameMessageType::ShipInput)
    {
        DEBUGLOG("Sending message: %s ", message.GetGameMessageTypeString());
    }
    
    auto packet = message.Serialize();
    Managers::Get<PlayFabPartyManager>()->SendGameMessage(packet);
}
