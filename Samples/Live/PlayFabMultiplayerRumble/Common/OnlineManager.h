//--------------------------------------------------------------------------------------
// OnlineManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "IOnlineManager.h"
#include "XboxHandle.h"
#include <xsapi-c/services_c.h>

#include "PFEntityKey.h"
#include "PFMultiplayer.h"
#include "PFLobby.h"
#include "PFMatchmaking.h"

#include "PlayFabUtils.h"

namespace PlayFabMultiplayerRumble
{
    class User;
    class PlayFabPartyManager;

    class OnlineManager final : public IOnlineManager
    {
    public:
        OnlineManager() = default;
        ~OnlineManager() = default;

        void Initialize() noexcept override;
        void ShutdownAsync(std::function<void()> completionCallback) noexcept override;
        bool IsNetworkAvailable() noexcept override;
        void SetNetworkAvailableCallback(std::function<void()> callback) noexcept override;
        void StartMatchmaking() noexcept override;
        void CancelMatchmaking() noexcept override;
        void FindLobbies() noexcept override;
        void HostMultiplayerGame(bool privateSession = false) noexcept override;
        void JoinMultiplayerGame(OnlineUser* host) noexcept override;
        void JoinPendingInviteSession() noexcept override;
        void JoinMultiplayerGame(const std::string& connectionString);
        void JoinGameFromInvite(const std::string& connectionString);
        void LeaveMultiplayerGame() noexcept override;
        bool IsHost() noexcept override;
        uint64_t GetNetworkId() const noexcept override;
        void Tick(float delta) noexcept override;
        void MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool)> callback = nullptr) noexcept override;
        void ShowInviteUI() noexcept override;

        void OnNetworkLost();

        void SetEntityToken();
        void SetLobbySearchCallback(std::function<void(uint32_t, const PFLobbySearchResult*)> callback) noexcept;

        uint32_t GetLobbyMemberCount();
        const char* GetLobbyConnectionString();

        ATG::XboxHandle<XblContextHandle> GetXboxLiveContext() { return m_xblContext; }

        uint64_t GetXuidFromEntityId(const char* entityId)
        {
            auto it = m_entityIdToXuidMap.find(entityId);
            if (it != m_entityIdToXuidMap.end())
            {
                return it->second;
            }

            return 0;
        }

        const char* GetEntityIdFromXuid(uint64_t xuid)
        {
            auto it = m_xuidToEntityIdMap.find(xuid);
            if (it != m_xuidToEntityIdMap.end())
            {
                return it->second.c_str();
            }

            return nullptr;
        }

    private:
        // Internal Methods
        void InitializeXboxLive(ATG::XboxHandle<XUserHandle> user);
        void CheckCrossPlayPrivilege();
        bool CheckPrivilege(ATG::XboxHandle<XUserHandle> user, XUserPrivilegeOptions option, XUserPrivilege privilege);
        void ResolvePrivilege(ATG::XboxHandle<XUserHandle> user, XUserPrivilegeOptions option, XUserPrivilege privilege, std::function<void(bool)> callback);

        uint64_t GetCurrentUserXuid() const;
        void MigrateToNewNetwork();
        void FindAndConnectToNetwork();
        void CreatePlayFabParty();
        void SetLobbyProperties(PropertyHelper& properties);
        std::string GetLobbyProperty(const char* name);

        std::vector<uint64_t> m_migratedXuids;
        ATG::XboxHandle<XUserHandle> m_user;
        ATG::XboxHandle<XblContextHandle> m_xblContext;
        
        std::map<uint64_t, std::string> m_xuidToEntityIdMap;
        std::map<std::string, uint64_t> m_entityIdToXuidMap;

        std::function<void(uint32_t, const PFLobbySearchResult*)> m_lobbySearchCallback;
        std::function<void()> m_networkAvailableCallback;
        XTaskQueueRegistrationToken m_networkAvailableToken{};
        XTaskQueueRegistrationToken m_inviteRegistration{};
        uint32_t m_signInCallbackToken = 0;
        uint32_t m_signOutCallbackToken = 0;
        XblSocialManagerUserGroupHandle m_socialGroup = nullptr;

        //PlayFabMultiplayer specific
        PFMultiplayerHandle m_pfmHandle = nullptr;
        PFLobbyHandle m_myLobby = nullptr;
        PFMatchmakingTicketHandle m_activeMatchmakingTicket = nullptr;

        void InitializePlayFabMultiplayer();
        void UninitializePlayFabMultiplayer();

        void ProcessLobbyStateChanges();
        void OnCreateAndJoinLobbyCompleted(const PFLobbyStateChange& stateChange);
        void OnJoinLobbyCompleted(const PFLobbyStateChange& stateChange);
        void OnMemberAdded(const PFLobbyStateChange& stateChange);
        void OnMemberRemoved(const PFLobbyStateChange& stateChange);
        void OnLeaveLobbyCompleted(const PFLobbyStateChange& stateChange);
        void OnUpdated(const PFLobbyStateChange& stateChange);
        void OnPostUpdateCompleted(const PFLobbyStateChange& stateChange);
        void OnDisconnecting(const PFLobbyStateChange& stateChange);
        void OnJoinArrangedLobbyCompleted(const PFLobbyStateChange& stateChange);
        void OnFindLobbiesCompleted(const PFLobbyStateChange& stateChange);

        void ProcessMatchmakingStateChanges();
        void OnTicketStatusChanged(const PFMatchmakingStateChange& stateChange);

        void OnTicketCompleted(const PFMatchmakingStateChange& stateChange);
        PFEntityKey GetCurrentUserEntityKey() const;

        void SetMPAActivity();
        void DeleteMPAActivity();
    };
}
