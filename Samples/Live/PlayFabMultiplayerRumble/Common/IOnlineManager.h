//--------------------------------------------------------------------------------------
// IOnlineManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "IManager.h"
#include "NetworkMessages.h"
#include "Json.h"

namespace PlayFabMultiplayerRumble
{
    class GameMessage;
    using OnlineMessageHandler = std::function<void(uint64_t, GameMessage*)>;

    struct OnlineUser
    {
        std::string Name;
        uint64_t Id = 0;
    };

    enum class OnlineState
    {
        Ready,
        Matchmaking,
        Hosting,
        Joining,
        Canceling,
        InGame
    };

    class IOnlineManager : public IManager
    {
    protected:
        OnlineMessageHandler m_messageHandler = nullptr;
        OnlineState m_mpState = OnlineState::Ready;

        bool m_isMatchFound = false;
        bool m_hasMultiplayerPrivileges = false;
        bool m_hasCrossplayPrivileges = false;

        std::string m_networkId{};
        std::string m_networkDescriptor{};
        std::string m_joiningSession{};

    public:
        //abstract functions
        virtual ~IOnlineManager() noexcept = default;
        virtual void Initialize() noexcept = 0;
        virtual void ShutdownAsync(std::function<void()> completionCallback) = 0;
        virtual void StartMatchmaking() noexcept = 0;
        virtual void CancelMatchmaking() noexcept = 0;
        virtual void FindLobbies() noexcept = 0;
        virtual void HostMultiplayerGame(bool privateSession = false) noexcept = 0;
        virtual void JoinMultiplayerGame(OnlineUser* host) noexcept = 0;
        virtual void JoinPendingInviteSession() noexcept = 0;
        virtual void LeaveMultiplayerGame() noexcept = 0;
        virtual bool IsNetworkAvailable() noexcept = 0;
        virtual void SetNetworkAvailableCallback(std::function<void()> callback) noexcept = 0;
        virtual bool IsHost() noexcept = 0;

        virtual void Tick(float delta) noexcept = 0;
        virtual void MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool)> callback) noexcept = 0;
        virtual void ShowInviteUI() = 0;
        virtual uint64_t GetNetworkId() const noexcept = 0;

        //non abstract functions
        virtual bool IsMatchmaking() const noexcept;
        virtual bool HasMultiplayerPrivileges() const noexcept;
        virtual bool HasCrossplayPrivileges() const noexcept;
        virtual bool HasPendingInviteSession() const noexcept;
        virtual bool IsConnected() const;
        virtual void RegisterOnlineMessageHandler(OnlineMessageHandler handler) noexcept;
        virtual void SendGameMessage(const GameMessage& message) noexcept;
    };
}
