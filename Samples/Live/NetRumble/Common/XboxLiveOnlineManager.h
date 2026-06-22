#pragma once

#include "OnlineManager.h"

#include "XboxHandle.h"
#include <xsapi-c/services_c.h>

namespace NetRumble
{
    class User;
    class PlayFabPartyManager;

    struct XboxLiveOnlineUser : public OnlineUser
    {
        XblMultiplayerActivityDetails mpActivity = {};
    };

    class XboxLiveOnlineManager final : public IOnlineManager
    {
    public:
        XboxLiveOnlineManager() noexcept(false);
        ~XboxLiveOnlineManager() override;

        virtual void Initialize() override;
        virtual void ShutdownAsync(std::function<void()> completionCallback) override;

        virtual bool IsNetworkAvailable() override;

        virtual void SetNetworkAvailableCallback(std::function<void()> callback) override;

        virtual void StartMatchmaking() override;
        virtual bool IsMatchmaking() override { return m_mpState == OnlineState::Matchmaking; }
        virtual void CancelMatchmaking(bool immediate) override;

        virtual void HostMultiplayerGame(bool privateSession = false) override;
        virtual void JoinMultiplayerGame(OnlineUser* host) override;
        virtual void JoinPendingInviteSession() override;
        void JoinMultiplayerGame(const char *session);
        void JoinGameFromInvite(const char *session);
        virtual void LeaveMultiplayerGame(bool immediate) override;

        virtual void SendGameMessage(const GameMessage &message) override;

        virtual bool HasMultiplayerPrivileges() const override;
        virtual bool HasCrossplayPrivileges() const override;
        virtual bool HasPendingInviteSession()  const override { return !m_joiningSession.empty(); }
        virtual bool IsHost() const override;
        virtual bool IsConnected() const override;

        virtual uint64_t GetNetworkId() const override;

        virtual void RegisterOnlineMessageHandler(OnlineMessageHandler handler) override;

        virtual void Tick(float delta) override;

        virtual void OnNetworkLost() override;

        virtual void UpdateStatistic(std::string_view name, int value) override;
        virtual void UpdateStatistic(std::string_view , double) override {}
        virtual void SendTelemetry(std::string_view eventName) override;
        virtual void MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool)> callback = nullptr) override;

        virtual void ShowInviteUI() override;

        virtual HRESULT QueryUserDisplayNameAsync(uint64_t id, XTaskQueueHandle taskQueue, std::function<void(const OnlineUser &)> then) override;
        virtual HRESULT QueryUserDisplayNamesAsync(uint64_t *id, size_t count, XTaskQueueHandle taskQueue, std::function<void(const std::vector<OnlineUser> &)> then) override;
        virtual HRESULT GetInGameFriendsAsync(XTaskQueueHandle taskQueue, std::function<void(std::vector<std::shared_ptr<OnlineUser>> &)> then) override;

    private:
        // Internal Methods
        bool HasForDisplayNameCached(uint64_t id);
        void UpdateDisplayName(uint64_t xuid, std::string_view name); 
        ATG::XboxHandle<XblContextHandle> GetXboxLiveContext() { return m_context; }

        void InitializeXboxLive(ATG::XboxHandle<XUserHandle> user);
        void CheckCrossplayPrivilege(ATG::XboxHandle<XUserHandle> user);

        ATG::XboxHandle<XblUserHandle> GetCurrentUserHandle() const;
        uint64_t GetCurrentUserXuid() const;

        void WriteEntityIdToSession();
        void MigrateToNewNetwork();
        void FindAndConnectToNetwork();
        void FindAndAddRemoteUsers();
        void FindAndRemoveRemoteUsers();
        void MultiplayerTick();
        void SubmitLobbyToMatchmaking();
        void CreateGameSession();
        void CreatePlayFabParty();
        void SetSessionProperty(const char* name, const char* value);
        std::string GetSessionProperty(const char* name);


        void SocialInit();
        void SocialTick();
        void SocialCleanup();
        void UpdateFriendDisplayNames();

        void CheckAndTrySetHost(bool forceLocal = false);

        enum class OnlineState
        {
            Ready,
            Matchmaking,
            Hosting,
            Joining,
            Canceling,
            InGame
        };

        bool m_hasMultiplayerPrivileges;
        bool m_hasCrossplayPrivileges;
        bool m_isMatchFound;

        OnlineState m_mpState;

        OnlineMessageHandler m_messageHandler;
        std::string m_networkId;
        std::string m_networkDescriptor;
        std::string m_joiningSession;
        std::vector<std::string> m_regionList;
        std::vector<uint64_t> m_migratedXuids;
        ATG::XboxHandle<XUserHandle> m_user;
        ATG::XboxHandle<XblContextHandle> m_context;
        std::map<uint64_t, std::string> m_xuidToDisplayNameMap;
        std::map<uint64_t, std::string> m_xuidToEntityIdMap;

        std::function<void()> m_networkAvailableCallback;
        XTaskQueueRegistrationToken m_networkAvailableToken;
        XTaskQueueRegistrationToken m_inviteRegistration;
        uint32_t m_signInCallbackToken;
        uint32_t m_signOutCallbackToken;

        XblSocialManagerUserGroupHandle m_socialGroup;
    };

    extern const char *MessageTypeString(GameMessageType type);
    using OnlineManager = XboxLiveOnlineManager;
}
