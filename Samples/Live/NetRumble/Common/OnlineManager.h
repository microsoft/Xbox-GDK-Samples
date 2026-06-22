#pragma once

#include "Manager.h"

#include "Json.h"

namespace NetRumble
{
    class GameMessage;
    using OnlineMessageHandler = std::function<void(uint64_t, GameMessage*)>;

    struct OnlineUser
    {
        std::string Name;
        uint64_t Id = 0;
    };

    class IOnlineManager : public Manager
    {
    public:
        virtual ~IOnlineManager() noexcept = default;

        virtual void Initialize() = 0;
        virtual void ShutdownAsync(std::function<void()> completionCallback) = 0;

        virtual void StartMatchmaking() = 0;
        virtual bool IsMatchmaking() = 0;
        virtual void CancelMatchmaking(bool immediate) = 0;

        virtual void HostMultiplayerGame(bool privateSession = false) = 0;
        virtual void JoinMultiplayerGame(OnlineUser* host) = 0;
        virtual void JoinPendingInviteSession() = 0;
        virtual void LeaveMultiplayerGame(bool immediate) = 0;

        virtual void SendGameMessage(const GameMessage &) = 0;

        virtual bool IsNetworkAvailable() = 0;

        virtual void SetNetworkAvailableCallback(std::function<void()> callback) = 0;

        virtual bool HasMultiplayerPrivileges() const = 0;
        virtual bool HasCrossplayPrivileges() const = 0;
        virtual bool HasPendingInviteSession() const = 0;
        virtual bool IsHost() const = 0;
        virtual bool IsConnected() const = 0;

        virtual void RegisterOnlineMessageHandler(OnlineMessageHandler handler) = 0;
    
        virtual void Tick(float delta) = 0;

        virtual void OnNetworkLost() = 0;

        virtual void UpdateStatistic(std::string_view name, int value) = 0;
        virtual void UpdateStatistic(std::string_view name, double value) = 0;
        virtual void SendTelemetry(std::string_view eventName) = 0;
        virtual void MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool)> callback) = 0;
        virtual void ShowInviteUI() = 0;

        virtual uint64_t GetNetworkId() const = 0;
        virtual HRESULT QueryUserDisplayNameAsync(uint64_t id, XTaskQueueHandle taskQueue, std::function<void(const OnlineUser &)> then) = 0;
        virtual HRESULT QueryUserDisplayNamesAsync(uint64_t *id, size_t count, XTaskQueueHandle taskQueue, std::function<void(const std::vector<OnlineUser> &)> then) = 0;
        virtual HRESULT GetInGameFriendsAsync(XTaskQueueHandle taskQueue, std::function<void(std::vector<std::shared_ptr<OnlineUser>> &)> then) = 0;
    };

}
