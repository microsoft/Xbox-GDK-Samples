//--------------------------------------------------------------------------------------
// PlayFabPartyManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Party.h"

// Suppress warnings from PlayFab Party Xbox Live header
#pragma warning(push)
#pragma warning(disable : 28285) // SAL syntax error in partyxboxlive.h
#include "PartyXboxLive.h"
#pragma warning(pop)

#include "IManager.h"
#include "NetworkMessages.h"

namespace PlayFabMultiplayerRumble
{

enum class NetworkManagerState
{
    Initialize,
    WaitingForNetwork,
    NetworkConnected,
    Migrating,
    Leaving
};

enum class NetworkManagerConnectionType
{
    peerToPeer,
    cloudRelay
};

class PlayFabPartyManager : public IManager
{
public:
    PlayFabPartyManager() = default;
    virtual ~PlayFabPartyManager() = default;

    void Initialize();
    void SetLocalUser(uint64_t xuid, std::function<void(PartyError)> userCreatedCallback = nullptr);
    void ClearLocalUser();
    void AddRemoteUser(uint64_t xuid, const std::string& entityId);
    void RemoveRemoteUser(uint64_t xuid);
    void CreateAndConnectToNetwork(const char* networkId, std::function<void(std::string)> onNetworkCreated = nullptr);
    void ConnectToNetwork(const char* networkId, const char* descriptor, std::function<void(void)> onNetworkConnected = nullptr);
    void SendGameMessage(const GameMessage& message);
    void SetGameMessageHandler(std::function<void(uint64_t, std::shared_ptr<GameMessage>)> onMessageReceived);
    void SetEndpointChangeHandler(std::function<void(uint64_t, bool)> onEndpointChanged);
    void SendTextAsVoice(std::string text);
    void SendTextMessage(std::string text);
    void LeaveNetwork(std::function<void(void)> onNetworkDestroyed = nullptr);
    void MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool, const char*)> onMigrationComplete = nullptr);
    void MigrateToNetwork(const char* descriptor, std::function<void(bool)> callback = nullptr);
    void Shutdown();

    void DoWork();

    void TryEntityTokenRefresh();

    //PartyXblStateChange Functions
    void OnXblCreateLocalChatUserCompleted(const Party::PartyXblStateChange* change);
    void OnXblLocalChatUserDestroyed(const Party::PartyXblStateChange* change);
    void OnXblLoginToPlayFabCompleted(const Party::PartyXblStateChange* change);
    void OnXblRequiredChatPermissionInfoChanged(const Party::PartyXblStateChange* change);
    void OnXblTokenAndSignatureRequested(const Party::PartyXblStateChange* change);
    
    //PartyStateChange Functions
    void OnConnectToNetworkCompleted(const Party::PartyStateChange* change);
    void OnLocalUserRemoved(const Party::PartyStateChange* change);
    void OnCreateEndpointCompleted(const Party::PartyStateChange* change);
    void OnEndpointCreated(const Party::PartyStateChange* change);
    void OnEndpointDestroyed(const Party::PartyStateChange* change);
    void OnLeaveNetworkCompleted(const Party::PartyStateChange* change);
    void OnNetworkDestroyed(const Party::PartyStateChange* change);
    void OnEndpointMessageReceived(const Party::PartyStateChange* change);
    void OnCreateChatControlCompleted(const Party::PartyStateChange* change);
    void OnChatControlCreated(const Party::PartyStateChange* change);
    void OnChatControlDestroyed(const Party::PartyStateChange* change);
    void OnChatTextReceived(const Party::PartyStateChange* change);
    void OnVoiceChatTranscriptionReceived(const Party::PartyStateChange* change);
    void OnSetChatAudioInputCompleted(const Party::PartyStateChange* change);
    void OnSetChatAudioOutputCompleted(const Party::PartyStateChange* change);
    void OnLocalChatAudioInputChanged(const Party::PartyStateChange* change);
    void OnLocalChatAudioOutputChanged(const Party::PartyStateChange* change);
    void OnPopulateAvailableTextToSpeechProfilesCompleted(const Party::PartyStateChange* change);
    void OnRemoteDeviceLeftNetwork(const Party::PartyStateChange* change);
    void OnInvitationDestroyed(const Party::PartyStateChange* change);
    void OnChatControlLeftNetwork(const Party::PartyStateChange* change);

    void SetLanguageCode(const char* lang, const char* name);
    const char* GetLanguageCode() const { return m_languageCode.c_str(); }
    const char* GetLanguageName() const { return m_languageName.c_str(); }
    void SetTextFilteringLevel(Party::PartyTextChatFilterLevel level);
    Party::PartyTextChatFilterLevel GetTextFilteringLevel();
    inline bool IsTextChatFilteringEnabled() { return m_textChatFiltering; }
    void SetTextChatFilteringEnabled(bool bEnabled);

    inline NetworkManagerState State() const { return m_state; }
    inline bool IsPeerConnectionType() const { return (m_connectionType == NetworkManagerConnectionType::peerToPeer); }
    inline bool HasLocalUser() const { return m_localChatUser != nullptr; }
    inline bool IsConnected() const { return m_state == NetworkManagerState::NetworkConnected; }
    inline bool IsCognitiveServicesEnabled() { return m_enableCognitiveServices; }
    void SetCognitiveServicesEnabled(bool bEnabled);
    void SetNetworkType(bool bIsPeerToPeerConnectionType);

    void PopulatePartyRegionLatencies(bool send = true);
    std::map<uint64_t, uint64_t> GetRemotePlayerLatencies();
    uint64_t GetNetworkQueuedSendMessagesCount();
    uint64_t GetNetworkAverageRelayRoundTripTime();
    bool GetPartyNetworkDescriptor(Party::PartyNetworkDescriptor* descriptor);
    std::vector<std::string> GetBestRegionList(const std::vector<std::vector<std::pair<std::string, uint64_t>>>& userRegionLatencyList);

    Party::PartyChatControl* GetChatControl(std::string& peer);

    Party::PartyLocalChatControl* GetLocalChatControl()
    {
        return m_localChatControl;
    }

    const char* GetCurrentUserEntityId()
    {
        return m_localEntityId.c_str();
    }

    const char* GetCurrentUserEntityToken()
    {
        return m_localEntityToken.c_str();
    }

private:
    Party::PartyInvitationConfiguration GetPartyInvitationConfiguration(const char *networkId);
    Party::PartyNetworkConfiguration GetPartyNetworkConfiguration();
    bool InternalConnectToNetwork(const char* networkId, Party::PartyNetworkDescriptor& descriptor, Party::PartyNetwork** network, Party::PartyLocalEndpoint** endpoint);
    void CreateLocalUser();
    void CreateLocalChatControl();
    void SetTextChatAccessibilityOptions();
    std::string DisplayNameFromChatControl(Party::PartyChatControl* control);
    void SetTextChatTranslationOptions(bool bTranslateToLocalLanguage, bool bEnableTextFiltering);
    void SetVoiceChatTranscriptionOptions(bool bTranscribeSelf, bool bTranscribeOtherChatControlsWithMatchingLanguages, bool bTranscribeOtherChatControlsWithNonMatchingLanguages, bool bTranslateToLocalLanguage);

    std::function<void(std::string)> m_onNetworkCreated;
    std::function<void(void)> m_onNetworkConnected;
    std::function<void(void)> m_onNetworkDestroyed;
    std::function<void(bool)> m_onNetworkMigrated;
    std::function<void(bool, const char*)> m_onRegionMigrated;
    std::function<void(uint64_t, std::shared_ptr<GameMessage>)> m_onMessageReceived;
    std::function<void(uint64_t, bool)> m_onEndpointChanged;
    NetworkManagerState m_state = NetworkManagerState::Initialize;
    NetworkManagerConnectionType m_connectionType = NetworkManagerConnectionType::peerToPeer;
    std::map<std::string, Party::PartyChatControl*> m_chatControls;
    Party::PartyLocalEndpoint* m_localEndpoint = nullptr;
    Party::PartyNetwork* m_network = nullptr;
    Party::PartyLocalEndpoint* m_newLocalEndpoint = nullptr;
    Party::PartyNetwork* m_newNetwork = nullptr;
    Party::PartyLocalUser* m_localUser = nullptr;
    Party::PartyLocalChatControl* m_localChatControl = nullptr;
    Party::PartyXblLocalChatUser* m_localChatUser = nullptr;
    bool m_partyInitialized = false;
    bool m_partyXblInitialized = false;
    bool m_localUserReady = false;
    bool m_playfabLoginComplete = false;
    bool m_refreshingEntityToken = false;
    bool m_enableCognitiveServices = false;
    bool m_textChatFiltering = false;
    std::string m_languageCode = "en-US";
    std::string m_languageName = "English (United States)";
    std::string m_localEntityId;
    std::string m_localEntityToken;
    time_t m_localEntityTokenExpirationTime = 0;
    std::function<void(PartyError)> m_userCreatedCallback;
    Party::PartyNetworkDescriptor m_networkDescriptor = {};
    std::string m_networkId;
};

}
