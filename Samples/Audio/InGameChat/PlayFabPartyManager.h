//--------------------------------------------------------------------------------------
// PlayFabPartyManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include "Party.h"
#include "PartyXboxLive.h"

enum class NetworkManagerState
{
    Initialize,
    WaitingForNetwork,
    NetworkConnected,
    Leaving
};

struct XuidLookupContext
{
    std::function<void(bool)> callback;
};

class PlayFabPartyManager
{
public:
    PlayFabPartyManager() = default;
    ~PlayFabPartyManager();

    PlayFabPartyManager(PlayFabPartyManager&&) = delete;
    PlayFabPartyManager& operator= (PlayFabPartyManager&&) = delete;

    PlayFabPartyManager(PlayFabPartyManager const&) = delete;
    PlayFabPartyManager& operator=(PlayFabPartyManager const&) = delete;

    void Initialize();
    void SetLocalUser(uint64_t xuid, std::function<void(PartyError)> userCreatedCallback = nullptr);
    void CreateAndConnectToNetwork(const char* networkId, std::function<void(std::string)> onNetworkCreated = nullptr);
    void ConnectToNetwork(const char* networkId, const char* descriptor, std::function<void(void)> onNetworkConnected = nullptr);
    void SendNetworkMessage(uint64_t xuid, std::vector<uint8_t>& message, bool reliable);
    void SetNetworkMessageHandler(std::function<void(uint64_t, std::vector<uint8_t>&)> onMessageReceived);
    void SetEndpointChangeHandler(std::function<void(uint64_t, bool)> onEndpointChanged);
    void SetXuidToEntityIdHandler(std::function<void(uint64_t, std::string)> onXuidMapped);
    void LeaveNetwork(std::function<void(void)> onNetworkDestroyed = nullptr);
    void LookupEntityIdsForXuids(size_t count, uint64_t* xuids, std::function<void(bool)> callback = nullptr);
    void Shutdown();

    void DoWork();

    void TryEntityTokenRefresh();

    // PartyXblStateChange Functions
    void OnCreateLocalChatUserCompleted(const Party::PartyXblStateChange* change);
    void OnLoginToPlayFabCompleted(const Party::PartyXblStateChange* change);
    void OnGetEntityIdsFromXboxLiveUserIdsCompleted(const Party::PartyXblStateChange* change);

    // PartyStateChange Functions
    void OnCreateNewNetworkCompleted(const Party::PartyStateChange* change);
    void OnConnectToNetworkCompleted(const Party::PartyStateChange* change);
    void OnLocalUserRemoved(const Party::PartyStateChange* change);
    void OnCreateEndpointCompleted(const Party::PartyStateChange* change);
    void OnEndpointCreated(const Party::PartyStateChange* change);
    void OnEndpointDestroyed(const Party::PartyStateChange* change);
    void OnLeaveNetworkCompleted(const Party::PartyStateChange* change);
    void OnNetworkDestroyed(const Party::PartyStateChange* change);
    void OnEndpointMessageReceived(const Party::PartyStateChange* change);

    const char* GetLocalUserEntityId() const { return m_localEntityId.c_str(); }

    inline NetworkManagerState State() const { return m_state; }
    inline bool IsConnected() const { return m_state == NetworkManagerState::NetworkConnected; }
    inline bool IsHost() const { return m_host == true; }
    inline void SetHost(bool isHost) { m_host = isHost; }
    inline bool IsEndpointXuid(uint64_t xuid) { return m_partyEndpoints.find(xuid) != m_partyEndpoints.end(); }
    uint64_t GetXuidFromEntityId(const char* entityId);
    const char* GetEntityIdFromXuid(uint64_t xuid);

private:
    bool InternalConnectToNetwork(const char* networkId, Party::PartyNetworkDescriptor& descriptor);
    void CreateLocalUser();

    std::function<void(std::string)> m_onNetworkCreated;
    std::function<void(void)> m_onNetworkConnected;
    std::function<void(void)> m_onNetworkDestroyed;
    std::function<void(uint64_t, std::vector<uint8_t>&)> m_onMessageReceived;
    std::function<void(uint64_t, bool)> m_onEndpointChanged;
    std::function<void(uint64_t, std::string)> m_onXuidMapped;
    NetworkManagerState m_state = NetworkManagerState::Initialize;
    std::map<uint64_t, Party::PartyEndpoint*> m_partyEndpoints;
    Party::PartyLocalEndpoint* m_localEndpoint = nullptr;
    Party::PartyNetwork* m_network = nullptr;
    Party::PartyLocalUser* m_localUser = nullptr;
    Party::PartyXblLocalChatUser* m_localChatUser = nullptr;
    bool m_partyInitialized = false;
    bool m_partyXblInitialized = false;
    bool m_host = false;
    bool m_localUserReady = false;
    bool m_playfabLoginComplete = false;
    bool m_refreshingEntityToken = false;
    std::string m_localEntityId;
    std::string m_localEntityToken;
    time_t m_localEntityTokenExpirationTime = 0;
    std::function<void(PartyError)> m_userCreatedCallback;
    std::map<std::string, uint64_t> m_entityIdToXuid;
    std::map<uint64_t, std::string> m_xuidToEntityId;
    std::list<Party::PartyEndpoint*> m_pendingEndpoints;

private:
    const char* c_pfTitleId = "86E2";
};

