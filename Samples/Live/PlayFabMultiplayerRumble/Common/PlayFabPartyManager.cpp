//--------------------------------------------------------------------------------------
// PlayFabPartyManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Managers.h"
#include "Game.h"
#include "PartyImpl.h"
#include "PartyXboxLiveImpl.h"
#include "OnlineManager.h"
#include "Managers.h"
#include "NetworkMessages.h"
#include "STTOverlayScreen.h"
#include "PlayerState.h"
#include "SampleConfig.h"

#include <ctime>

using namespace PlayFabMultiplayerRumble;
using namespace Party;

namespace
{
    const char* GetPartyErrorMessage(PartyError error)
    {
        const char* errorMessage = nullptr;

        PartyError err = PartyManager::GetErrorMessage(error, &errorMessage);
        if (PARTY_FAILED(err))
        {
            DEBUGLOG("GetPartyErrorMessage: Failed to get error message %lu.", error);
            return "[ERROR]";
        }

        return errorMessage;
    }

    void LogError_PartyErrorWithMessage(const char* functionName, PartyError error)
    {
        if (const char* errorMessage = GetPartyErrorMessage(error))
        {
            LogError_ErrorWithMessage(functionName, error, errorMessage);
        }
    }

    const char* GetXblErrorMessage(PartyError error)
    {
        const char* errorMessage = nullptr;

        PartyError err = PartyXblManager::GetErrorMessage(error, &errorMessage);
        if (PARTY_FAILED(err))
        {
            DEBUGLOG("Failed to get error message %lu.", error);
            return "[ERROR]";
        }

        return errorMessage;
    }

    void LogError_XblErrorWithMessage(const char* functionName, PartyError error)
    {
        if (const char* errorMessage = GetXblErrorMessage(error))
        {
            LogError_ErrorWithMessage(functionName, error, errorMessage);
        }
    }

    const char* GetPartyLocalUserRemovedReasonString(PartyLocalUserRemovedReason reason)
    {
        switch (reason)
        {
        case PartyLocalUserRemovedReason::AuthenticationFailed: return STRINGIFY(PartyLocalUserRemovedReason::AuthenticationFailed);
        case PartyLocalUserRemovedReason::RemoveLocalUser:      return STRINGIFY(PartyLocalUserRemovedReason::RemoveLocalUser);
        case PartyLocalUserRemovedReason::DestroyLocalUser:     return STRINGIFY(PartyLocalUserRemovedReason::DestroyLocalUser);
        case PartyLocalUserRemovedReason::DestroyNetwork:       return STRINGIFY(PartyLocalUserRemovedReason::DestroyNetwork);
        }

        //we should never get here
        assert(false);
        return "Unknown enumeration value";
    };

    const char* GetPartyDestroyedReasonString(PartyDestroyedReason reason)
    {
        switch (reason)
        {
        case PartyDestroyedReason::Requested:                return STRINGIFY(PartyDestroyedReason::Requested);
        case PartyDestroyedReason::Disconnected:             return STRINGIFY(PartyDestroyedReason::Disconnected);
        case PartyDestroyedReason::Kicked:                   return STRINGIFY(PartyDestroyedReason::Kicked);
        case PartyDestroyedReason::DeviceLostAuthentication: return STRINGIFY(PartyDestroyedReason::DeviceLostAuthentication);
        case PartyDestroyedReason::CreationFailed:           return STRINGIFY(PartyDestroyedReason::CreationFailed);
        }

        //we should never get here
        assert(false);
        return "Unknown enumeration value";
    };

    const char* GetPartyStateChangeReasonString(PartyStateChangeResult result)
    {
        switch (result)
        {
        case PartyStateChangeResult::Succeeded: return "Succeeded";
        case PartyStateChangeResult::UnknownError: return "An unknown error occurred";
        case PartyStateChangeResult::InternetConnectivityError: return "The local device has internet connectivity issues which caused the operation to fail";
        case PartyStateChangeResult::PartyServiceError: return "The CommunicationFabric service is unable to create a new network at this time";
        case PartyStateChangeResult::NoServersAvailable: return "There are no available servers in the regions specified by the call to PartyManager::CreateNewNetwork()";
        case PartyStateChangeResult::CanceledByTitle: return "Operation canceled by title.";
        case PartyStateChangeResult::UserCreateNetworkThrottled: return "The PartyLocalUser specified in the call to PartyManager::CreateNewNetwork() has created too many networks and cannot create new networks at this time";
        case PartyStateChangeResult::TitleNotEnabledForParty: return "The title has not been configured properly in the Party portal";
        case PartyStateChangeResult::NetworkLimitReached: return "The network is full and is not allowing new devices or users to join";
        case PartyStateChangeResult::NetworkNoLongerExists: return "The network no longer exists";
        case PartyStateChangeResult::NetworkNotJoinable: return "The network is not currently allowing new devices or users to join";
        case PartyStateChangeResult::VersionMismatch: return "The network uses a version of the CommunicationFabric library that is incompatible with this library";
        case PartyStateChangeResult::UserNotAuthorized: return "The specified user was not authorized";
        case PartyStateChangeResult::LeaveNetworkCalled: return "The network was gracefully exited by the local device";
        }

        //we should never get here
        assert(false);
        return "Unknown enumeration value";
    }

    const char* GetPartyStateChangeTypeString(const PartyStateChange* stateChange)
    {
        if (stateChange == nullptr)
        {
            DEBUGLOG("GetPartyStateChangeTypeString: stateChange was null");
            return "[ERROR]";
        }

        switch (stateChange->stateChangeType)
        {
        case PartyStateChangeType::RegionsChanged:                                    return STRINGIFY(PartyStateChangeType::RegionsChanged);
        case PartyStateChangeType::DestroyLocalUserCompleted:                         return STRINGIFY(PartyStateChangeType::DestroyLocalUserCompleted);
        case PartyStateChangeType::CreateNewNetworkCompleted:                         return STRINGIFY(PartyStateChangeType::CreateNewNetworkCompleted);
        case PartyStateChangeType::ConnectToNetworkCompleted:                         return STRINGIFY(PartyStateChangeType::ConnectToNetworkCompleted);
        case PartyStateChangeType::AuthenticateLocalUserCompleted:                    return STRINGIFY(PartyStateChangeType::AuthenticateLocalUserCompleted);
        case PartyStateChangeType::NetworkConfigurationMadeAvailable:                 return STRINGIFY(PartyStateChangeType::NetworkConfigurationMadeAvailable);
        case PartyStateChangeType::NetworkDescriptorChanged:                          return STRINGIFY(PartyStateChangeType::NetworkDescriptorChanged);
        case PartyStateChangeType::LocalUserRemoved:                                  return STRINGIFY(PartyStateChangeType::LocalUserRemoved);
        case PartyStateChangeType::RemoveLocalUserCompleted:                          return STRINGIFY(PartyStateChangeType::RemoveLocalUserCompleted);
        case PartyStateChangeType::LocalUserKicked:                                   return STRINGIFY(PartyStateChangeType::LocalUserKicked);
        case PartyStateChangeType::CreateEndpointCompleted:                           return STRINGIFY(PartyStateChangeType::CreateEndpointCompleted);
        case PartyStateChangeType::DestroyEndpointCompleted:                          return STRINGIFY(PartyStateChangeType::DestroyEndpointCompleted);
        case PartyStateChangeType::EndpointCreated:                                   return STRINGIFY(PartyStateChangeType::EndpointCreated);
        case PartyStateChangeType::EndpointDestroyed:                                 return STRINGIFY(PartyStateChangeType::EndpointDestroyed);
        case PartyStateChangeType::RemoteDeviceCreated:                               return STRINGIFY(PartyStateChangeType::RemoteDeviceCreated);
        case PartyStateChangeType::RemoteDeviceDestroyed:                             return STRINGIFY(PartyStateChangeType::RemoteDeviceDestroyed);
        case PartyStateChangeType::RemoteDeviceJoinedNetwork:                         return STRINGIFY(PartyStateChangeType::RemoteDeviceJoinedNetwork);
        case PartyStateChangeType::RemoteDeviceLeftNetwork:                           return STRINGIFY(PartyStateChangeType::RemoteDeviceLeftNetwork);
        case PartyStateChangeType::DevicePropertiesChanged:                           return STRINGIFY(PartyStateChangeType::DevicePropertiesChanged);
        case PartyStateChangeType::LeaveNetworkCompleted:                             return STRINGIFY(PartyStateChangeType::LeaveNetworkCompleted);
        case PartyStateChangeType::NetworkDestroyed:                                  return STRINGIFY(PartyStateChangeType::NetworkDestroyed);
        case PartyStateChangeType::EndpointMessageReceived:                           return STRINGIFY(PartyStateChangeType::EndpointMessageReceived);
        case PartyStateChangeType::DataBuffersReturned:                               return STRINGIFY(PartyStateChangeType::DataBuffersReturned);
        case PartyStateChangeType::EndpointPropertiesChanged:                         return STRINGIFY(PartyStateChangeType::EndpointPropertiesChanged);
        case PartyStateChangeType::SynchronizeMessagesBetweenEndpointsCompleted:      return STRINGIFY(PartyStateChangeType::SynchronizeMessagesBetweenEndpointsCompleted);
        case PartyStateChangeType::NetworkPropertiesChanged:                          return STRINGIFY(PartyStateChangeType::NetworkPropertiesChanged);
        case PartyStateChangeType::KickDeviceCompleted:                               return STRINGIFY(PartyStateChangeType::KickDeviceCompleted);
        case PartyStateChangeType::KickUserCompleted:                                 return STRINGIFY(PartyStateChangeType::KickUserCompleted);
        case PartyStateChangeType::CreateChatControlCompleted:                        return STRINGIFY(PartyStateChangeType::CreateChatControlCompleted);
        case PartyStateChangeType::DestroyChatControlCompleted:                       return STRINGIFY(PartyStateChangeType::DestroyChatControlCompleted);
        case PartyStateChangeType::ChatControlCreated:                                return STRINGIFY(PartyStateChangeType::ChatControlCreated);
        case PartyStateChangeType::ChatControlDestroyed:                              return STRINGIFY(PartyStateChangeType::ChatControlDestroyed);
        case PartyStateChangeType::SetChatAudioEncoderBitrateCompleted:               return STRINGIFY(PartyStateChangeType::SetChatAudioEncoderBitrateCompleted);
        case PartyStateChangeType::ChatTextReceived:                                  return STRINGIFY(PartyStateChangeType::ChatTextReceived);
        case PartyStateChangeType::VoiceChatTranscriptionReceived:                    return STRINGIFY(PartyStateChangeType::VoiceChatTranscriptionReceived);
        case PartyStateChangeType::SetChatAudioInputCompleted:                        return STRINGIFY(PartyStateChangeType::SetChatAudioInputCompleted);
        case PartyStateChangeType::SetChatAudioOutputCompleted:                       return STRINGIFY(PartyStateChangeType::SetChatAudioOutputCompleted);
        case PartyStateChangeType::LocalChatAudioInputChanged:                        return STRINGIFY(PartyStateChangeType::LocalChatAudioInputChanged);
        case PartyStateChangeType::LocalChatAudioOutputChanged:                       return STRINGIFY(PartyStateChangeType::LocalChatAudioOutputChanged);
        case PartyStateChangeType::SetTextToSpeechProfileCompleted:                   return STRINGIFY(PartyStateChangeType::SetTextToSpeechProfileCompleted);
        case PartyStateChangeType::SynthesizeTextToSpeechCompleted:                   return STRINGIFY(PartyStateChangeType::SynthesizeTextToSpeechCompleted);
        case PartyStateChangeType::ChatControlPropertiesChanged:                      return STRINGIFY(PartyStateChangeType::ChatControlPropertiesChanged);
        case PartyStateChangeType::ChatControlJoinedNetwork:                          return STRINGIFY(PartyStateChangeType::ChatControlJoinedNetwork);
        case PartyStateChangeType::ChatControlLeftNetwork:                            return STRINGIFY(PartyStateChangeType::ChatControlLeftNetwork);
        case PartyStateChangeType::ConnectChatControlCompleted:                       return STRINGIFY(PartyStateChangeType::ConnectChatControlCompleted);
        case PartyStateChangeType::DisconnectChatControlCompleted:                    return STRINGIFY(PartyStateChangeType::DisconnectChatControlCompleted);
        case PartyStateChangeType::PopulateAvailableTextToSpeechProfilesCompleted:    return STRINGIFY(PartyStateChangeType::PopulateAvailableTextToSpeechProfilesCompleted);
        case PartyStateChangeType::CreateInvitationCompleted:                         return STRINGIFY(PartyStateChangeType::CreateInvitationCompleted);
        case PartyStateChangeType::RevokeInvitationCompleted:                         return STRINGIFY(PartyStateChangeType::RevokeInvitationCompleted);
        case PartyStateChangeType::InvitationCreated:                                 return STRINGIFY(PartyStateChangeType::InvitationCreated);
        case PartyStateChangeType::InvitationDestroyed:                               return STRINGIFY(PartyStateChangeType::InvitationDestroyed);
        case PartyStateChangeType::SetLanguageCompleted:                              return STRINGIFY(PartyStateChangeType::SetLanguageCompleted);
        case PartyStateChangeType::SetTranscriptionOptionsCompleted:                  return STRINGIFY(PartyStateChangeType::SetTranscriptionOptionsCompleted);
        case PartyStateChangeType::SetTextChatOptionsCompleted:                       return STRINGIFY(PartyStateChangeType::SetTextChatOptionsCompleted);
        case PartyStateChangeType::ConfigureAudioManipulationVoiceStreamCompleted:    return STRINGIFY(PartyStateChangeType::ConfigureAudioManipulationVoiceStreamCompleted);
        case PartyStateChangeType::ConfigureAudioManipulationCaptureStreamCompleted:  return STRINGIFY(PartyStateChangeType::ConfigureAudioManipulationCaptureStreamCompleted);
        case PartyStateChangeType::ConfigureAudioManipulationRenderStreamCompleted:   return STRINGIFY(PartyStateChangeType::ConfigureAudioManipulationRenderStreamCompleted);
        }

        //we should never get here
        assert(false);
        return "Unknown enumeration value";
    }

    template <typename result_type>
    void LogPartyResult(const result_type& result)
    {
        const char* typeStr = GetPartyStateChangeTypeString(result);
        const char* resultStr = GetPartyStateChangeReasonString(result->result);

        if (result->result != PartyStateChangeResult::Succeeded)
        {
            const char* errorDetailStr = GetPartyErrorMessage(result->errorDetail);

            DEBUGLOG("%s PartyStateChangeResult: %s Error Detail: %s", typeStr, resultStr, errorDetailStr);
        }
        else
        {
            DEBUGLOG("%s PartyStateChangeResult: %s", typeStr, resultStr);
        }
    }
}

void PlayFabPartyManager::SetLanguageCode(const char* lang, const char* name)
{
    m_languageCode = lang;
    m_languageName = name;
}

void PlayFabMultiplayerRumble::PlayFabPartyManager::SetTextFilteringLevel(PartyTextChatFilterLevel level)
{
    PartyError err = PartyManager::SetOption(nullptr, PartyOption::TextChatFilterLevel, &level);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("SetOption", err);
    }
}

PartyTextChatFilterLevel PlayFabPartyManager::GetTextFilteringLevel()
{
    PartyTextChatFilterLevel value = PartyTextChatFilterLevel::FamilyFriendly;

    PartyError err = PartyManager::GetOption(nullptr, PartyOption::TextChatFilterLevel, &value);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("GetOption", err);
    }

    return value;
}

void PlayFabPartyManager::Initialize()
{
    DEBUGLOG("PlayFabPartyManager::Initialize()");

    PartyManager& partyManager = PartyManager::GetSingleton();
    PartyError err;

    if (m_partyInitialized == false)
    {
        // Initialize PlayFab Party
        PartyInitializationConfiguration initConfig = {};
        initConfig.titleId = SampleConfig::c_pfTitleId;
        
        err = partyManager.Initialize(&initConfig);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("Initialize", err);
            return;
        }

        m_partyInitialized = true;
    }

    if (m_partyXblInitialized == false)
    {
        // Initialize XboxLive Plugin
        err = PartyXblManager::GetSingleton().Initialize(SampleConfig::c_pfTitleId);
        if (PARTY_FAILED(err))
        {
            LogError_XblErrorWithMessage("PartyXblManager::Initialize", err);
            return;
        }

        m_partyXblInitialized = true;
    }
}

void PlayFabPartyManager::CreateLocalUser()
{
    PartyManager& partyManager = PartyManager::GetSingleton();
    PartyError err;

    if (m_localUser == nullptr)
    {
        DEBUGLOG("CreateLocalUser with entityId %s", m_localEntityId.c_str());

        // Create a local user object
        err = partyManager.CreateLocalUser(
            reinterpret_cast<PFEntityHandle>(m_localChatUser),  // Entity handle
            &m_localUser                                // OUT local user object
            );
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("CreateLocalUser", err);
            return;
        }
    }
}

void PlayFabPartyManager::CreateLocalChatControl()
{
    if (m_localChatControl == nullptr)
    {
        PartyManager& partyManager = PartyManager::GetSingleton();
        PartyLocalDevice* localDevice = nullptr;

        // Retrieve the local device
        PartyError err = partyManager.GetLocalDevice(&localDevice);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("GetLocalDevice", err);
            return;
        }

        // Create a chat control for the local user on the local device
        err = localDevice->CreateChatControl(m_localUser, m_languageCode.c_str(), nullptr, &m_localChatControl);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("CreateChatControl", err);
            return;
        }

        uint64_t xuid = 0;
        m_localChatUser->GetXboxUserId(&xuid);
        auto xuidString = std::to_string(xuid);

        // Use automatic settings for the audio input device
        err = m_localChatControl->SetAudioInput(PartyAudioDeviceSelectionType::PlatformUserDefault, xuidString.c_str(), nullptr);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("SetAudioInput", err);
            return;
        }

        // Use automatic settings for the audio output device
        err = m_localChatControl->SetAudioOutput(PartyAudioDeviceSelectionType::PlatformUserDefault, xuidString.c_str(), nullptr);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("SetAudioOutput", err);
        }

        // Get the available list of text to speech profiles
        err = m_localChatControl->PopulateAvailableTextToSpeechProfiles(nullptr);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("PopulateAvailableTextToSpeechProfiles", err);
        }
    }
}

void PlayFabPartyManager::SetTextChatAccessibilityOptions()
{
    DEBUGLOG("SetTextChatAccessibilityOptions()");

    PartyXblAccessibilitySettings accessibilitySettings{};
    PartyError err = m_localChatUser->GetAccessibilitySettings(&accessibilitySettings);
    if (PARTY_SUCCEEDED(err))
    {
        // For the purposes of the sample, enable or disable all possible features
        bool bEnabled = accessibilitySettings.speechToTextEnabled || m_enableCognitiveServices;
        SetVoiceChatTranscriptionOptions(bEnabled, bEnabled, bEnabled, bEnabled);

        bool bTranslationsEnabled = accessibilitySettings.textToSpeechEnabled || m_enableCognitiveServices;
        SetTextChatTranslationOptions(bTranslationsEnabled, m_textChatFiltering);
    }
    else
    {
        LogError_PartyErrorWithMessage("GetAccessibilitySettings", err);
    }
}

void PlayFabPartyManager::ClearLocalUser()
{
    PartyError err;

    if (m_localUser != nullptr)
    {
        err = PartyManager::GetSingleton().DestroyLocalUser(m_localUser, nullptr);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("PartyManager::DestroyLocalUser()", err);
        }
    }

    if (m_localChatUser != nullptr)
    {
        err = PartyXblManager::GetSingleton().DestroyChatUser(m_localChatUser);
        if (PARTY_FAILED(err))
        {
            LogError_XblErrorWithMessage("PartyXblManager::DestroyChatUser", err);
        }
    }

    m_localUser = nullptr;
    m_localChatUser = nullptr;
    m_userCreatedCallback = nullptr;
}

void PlayFabPartyManager::SetLocalUser(uint64_t xuid, std::function<void(PartyError)> callback)
{
    DEBUGLOG("PlayFabPartyManager::SetLocalUser(%lu)", xuid);

    PartyError err;

    if (m_localChatUser != nullptr)
    {
        uint64_t localXuid = 0;

        err = m_localChatUser->GetXboxUserId(&localXuid);
        if (PARTY_SUCCEEDED(err))
        {
            if (localXuid == xuid)
            {
                // This is already our local user
                callback(err);
                return;
            }
            else
            {
                ClearLocalUser();
            }
        }
        else
        {
            LogError_XblErrorWithMessage("PartyXblManager::GetXboxUserId", err);
        }
    }

    err = PartyXblManager::GetSingleton().CreateLocalChatUser(xuid, nullptr, &m_localChatUser);
    if (PARTY_FAILED(err))
    {
        LogError_XblErrorWithMessage("PartyXblManager::CreateLocalChatUser", err);
        callback(err);
        return;
    }

    err = PartyXblManager::GetSingleton().LoginToPlayFab(m_localChatUser, nullptr);
    if (PARTY_FAILED(err))
    {
        LogError_XblErrorWithMessage("PartyXblManager::LoginToPlayFab", err);
        callback(err);
        return;
    }

    m_userCreatedCallback = callback;
}

void PlayFabPartyManager::AddRemoteUser(uint64_t xuid, const std::string& entityId)
{
    DEBUGLOG("PlayFabPartyManager::AddRemoteUser(%llu, %s)", xuid, entityId.c_str());

    PartyXblChatUser* remoteChatUser;
    PartyError err = PartyXblManager::GetSingleton().CreateRemoteChatUser(xuid, &remoteChatUser);
    if (PARTY_SUCCEEDED(err))
    {
        // Look and see if we have an endpoint from this entityid in which case we can notify
        // the engine the user is ready

        uint32_t size = 0;
        PartyNetworkArray list = nullptr;

        // Get local networks
        err = PartyManager::GetSingleton().GetNetworks(&size, &list);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("GetNetworks", err);
            return;
        }

        for (uint32_t i = 0; i < size; i++)
        {
            PartyNetwork* net = list[i];

            uint32_t endpointCount = 0;
            PartyEndpointArray endpointList = nullptr;

            // Get the endpoints
            err = net->GetEndpoints(&endpointCount, &endpointList);
            if (PARTY_FAILED(err))
            {
                LogError_PartyErrorWithMessage("GetEndpoints", err);
                continue;
            }

            for (uint32_t j = 0; j < endpointCount; j++)
            {
                PartyEndpoint* end = endpointList[j];

                PartyString id = nullptr;

                // Get the entityid for the network user
                err = end->GetEntityId(&id);

                if (PARTY_SUCCEEDED(err) && id != nullptr)
                {
                    std::string endpointEntityId(id);

                    // See if they are the same
                    if (entityId == endpointEntityId)
                    {
                        // Tell the engine a user is ready to receive data
                        m_onEndpointChanged(xuid, true);
                        return;
                    }
                }
            }
        }
    }
    else
    {
        LogError_PartyErrorWithMessage("CreateRemoteChatUser", err);
    }
}

void PlayFabPartyManager::RemoveRemoteUser(uint64_t xuid)
{
    DEBUGLOG("PlayFabPartyManager::RemoveLocalUser(%llu)", xuid);

    uint32_t userCount = 0;
    PartyXblChatUserArray userList = nullptr;

    PartyError err = PartyXblManager::GetSingleton().GetChatUsers(&userCount, &userList);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("GetChatUsers", err);
    }

    for (uint32_t x = 0; x < userCount; x++)
    {
        PartyXblChatUser* user = userList[x];

        uint64_t userXuid = 0;
        if (PARTY_SUCCEEDED(user->GetXboxUserId(&userXuid)) && userXuid == xuid)
        {
            err = PartyXblManager::GetSingleton().DestroyChatUser(user);
            if (PARTY_FAILED(err))
            {
                LogError_PartyErrorWithMessage("DestroyChatUser", err);
            }
            else
            {
                break;
            }
        }
    }
}

void PlayFabPartyManager::Shutdown()
{
    DEBUGLOG("PlayFabPartyManager::Shutdown()");

    m_state = NetworkManagerState::Initialize;

    // This cleans up everything allocated in Initialize() and
    // should only be used when done with networking
    PartyManager::GetSingleton().Cleanup();
    PartyXblManager::GetSingleton().Cleanup();

    m_localChatControl = nullptr;
    m_localEndpoint = nullptr;
    m_network = nullptr;
    m_localUser = nullptr;
    m_partyInitialized = false;
    m_partyXblInitialized = false;
}

PartyInvitationConfiguration PlayFabPartyManager::GetPartyInvitationConfiguration(const char *networkId)
{
    // Setup the network invitation configuration to use the network id as an invitation id and allow anyone to join.

    return PartyInvitationConfiguration {networkId, PartyInvitationRevocability::Anyone, 0, nullptr};
}

PartyNetworkConfiguration PlayFabPartyManager::GetPartyNetworkConfiguration()
{
    PartyNetworkConfiguration cfg{};

    cfg.maxDeviceCount = SampleConfig::c_MaxPlayers;
    cfg.maxDevicesPerUserCount = SampleConfig::c_MaxPlayersPerDevice;
    cfg.maxEndpointsPerDeviceCount = SampleConfig::c_MaxPlayersPerDevice;
    cfg.maxUserCount = SampleConfig::c_MaxPlayers;
    cfg.maxUsersPerDeviceCount = SampleConfig::c_MaxPlayersPerDevice;

    // Specify P2P or relayed network type
    switch (m_connectionType)
    {
    case NetworkManagerConnectionType::peerToPeer:
        // Allow peer-to-peer on all platforms with the same login provider (Xbox Live)
        cfg.directPeerConnectivityOptions = PartyDirectPeerConnectivityOptions::AnyPlatformType | PartyDirectPeerConnectivityOptions::SameEntityLoginProvider;
        break;
    case NetworkManagerConnectionType::cloudRelay:
        // Disallow peer-to-peer and use cloud relay only
        cfg.directPeerConnectivityOptions = PartyDirectPeerConnectivityOptions::None;
        break;
    }

    return cfg;
}

void PlayFabPartyManager::CreateAndConnectToNetwork(const char *networkId, std::function<void(std::string)> callback)
{
    DEBUGLOG("PlayFabPartyManager::CreateAndConnectToNetwork()");

    auto networkConfiguration = GetPartyNetworkConfiguration();
    auto invitationConfiguration = GetPartyInvitationConfiguration(networkId);
    auto networkDescriptor = PartyNetworkDescriptor{};

    // Create a new network descriptor
    PartyError err = PartyManager::GetSingleton().CreateNewNetwork(
        m_localUser,                                // Local User
        &networkConfiguration,                      // Network Config
        0,                                          // Region List Count
        nullptr,                                    // Region List
        &invitationConfiguration,                   // Invitation configuration
        nullptr,                                    // Async Identifier
        &networkDescriptor,                         // OUT network descriptor
        nullptr                                     // Applied initial invitation identifier
        );

    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("CreateNewNetwork", err);
        return;
    }

    m_networkId = networkId;

    // Connect to the new network
    if (InternalConnectToNetwork(networkId, networkDescriptor, &m_network, &m_localEndpoint))
    {
        m_state = NetworkManagerState::WaitingForNetwork;
        m_onNetworkCreated = callback;
    }
}

void PlayFabPartyManager::ConnectToNetwork(const char* networkId, const char* descriptor, std::function<void(void)> callback)
{
    DEBUGLOG("PlayFabPartyManager::ConnectToNetwork()");

    PartyNetworkDescriptor networkDescriptor = {};

    // Deserialize the remote network's descriptor
    PartyError err = PartyManager::DeserializeNetworkDescriptor(descriptor, &networkDescriptor);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("DeserializeNetworkDescriptor", err);
        return;
    }

    m_networkId = networkId;

    // Connect to the remote network
    if (InternalConnectToNetwork(networkId, networkDescriptor, &m_network, &m_localEndpoint))
    {
        m_state = NetworkManagerState::WaitingForNetwork;
        m_onNetworkConnected = callback;
    }
}

bool PlayFabPartyManager::InternalConnectToNetwork(const char* networkId, PartyNetworkDescriptor& descriptor, PartyNetwork** network, PartyLocalEndpoint** endpoint)
{
    // This portion of connecting to the network is the same for
    // both creating a new and joining an existing network.

    PartyError err = PartyManager::GetSingleton().ConnectToNetwork(&descriptor, nullptr, network);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("ConnectToNetwork", err);
        return false;
    }

    // Authenticate the local user on the network so we can participate in it
    err = (*network)->AuthenticateLocalUser(m_localUser, networkId, nullptr);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("AuthenticateLocalUser", err);
        return false;
    }

    CreateLocalChatControl();

    // Connect the local user chat control to the network so we can use VOIP
    err = (*network)->ConnectChatControl(m_localChatControl, nullptr);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("ConnectChatControl", err);
        return false;
    }

    // Establish a network endpoint for game message traffic
    err = (*network)->CreateEndpoint(m_localUser, 0, nullptr, nullptr, nullptr, endpoint);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("CreateEndpoint", err);
        return false;
    }

    return true;
}

void PlayFabPartyManager::SendGameMessage(const GameMessage& message)
{
    if (m_localEndpoint)
    {
        std::vector<uint8_t> packet = message.Serialize();

        // Form the data packet into a data buffer structure
        PartyDataBuffer data[] = {
            {
                static_cast<const void*>(packet.data()),
                static_cast<uint32_t>(packet.size())
            },
        };

        PartySendMessageOptions deliveryOptions;

        // ShipInput and ShipData messages don't need to be sent reliably
        // or sequentially, but the rest are needed for gameplay
        switch (message.GetMessageType())
        {
            case GameMessageType::ShipInput:
            case GameMessageType::ShipData:
                deliveryOptions = PartySendMessageOptions::Default;
                break;

            default:
                deliveryOptions = PartySendMessageOptions::GuaranteedDelivery | PartySendMessageOptions::SequentialDelivery;
        }

        // Send out the message to all other peers
        PartyError err = m_localEndpoint->SendMessage(0, nullptr, deliveryOptions, nullptr, 1, data, nullptr);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("SendMessage", err);
        }
    }
}

void PlayFabPartyManager::SetGameMessageHandler(std::function<void(uint64_t, std::shared_ptr<GameMessage>)> callback)
{
    m_onMessageReceived = callback;
}

void PlayFabPartyManager::SetEndpointChangeHandler(std::function<void(uint64_t, bool)> callback)
{
    m_onEndpointChanged = callback;
}

void PlayFabPartyManager::SendTextMessage(std::string text)
{
    if (m_localChatControl != nullptr)
    {
        DEBUGLOG("Send text message: %s", text.c_str());

        std::vector<PartyChatControl*> targets;

        for (const auto& item : m_chatControls)
        {
            PartyLocalChatControl* local = nullptr;

            item.second->GetLocal(&local);

            if (local == nullptr)
            {
                targets.push_back(item.second);
            }
        }

        PartyError err = m_localChatControl->SendText(static_cast<uint32_t>(targets.size()), targets.data(), text.c_str(), 0, nullptr);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("SendText", err);
        }

        // Toast the text on the screen
        std::string displayName = DisplayNameFromChatControl(m_localChatControl);
        Managers::Get<ScreenManager>()->GetSTTWindow()->AddSTTString(displayName, text, false);
    }
}

void PlayFabPartyManager::SendTextAsVoice(std::string text)
{
    if (m_localChatControl != nullptr)
    {
        if (m_enableCognitiveServices)
        {
            DEBUGLOG("Requesting synthesis of: %s", text.c_str());

            PartyError err = m_localChatControl->SynthesizeTextToSpeech(PartySynthesizeTextToSpeechType::VoiceChat, text.c_str(), nullptr);
            if (PARTY_FAILED(err))
            {
                LogError_PartyErrorWithMessage("SynthesizeTextToSpeech", err);
            }
        }
    }
}

void PlayFabPartyManager::LeaveNetwork(std::function<void(void)> callback)
{
    DEBUGLOG("PlayFabPartyManager::LeaveNetwork()");

    if (m_state != NetworkManagerState::Leaving && m_network != nullptr)
    {
        m_state = NetworkManagerState::Leaving;
        m_onNetworkDestroyed = callback;

        // First destroy the chat control
        PartyLocalDevice* localDevice = nullptr;

        // Retrieve the local device
        PartyError err = PartyManager::GetSingleton().GetLocalDevice(&localDevice);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage ("GetLocalDevice", err);
            m_network->LeaveNetwork(nullptr);
            return;
        }

        err = localDevice->DestroyChatControl(m_localChatControl, nullptr);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("DestroyChatControl", err);
            m_network->LeaveNetwork(nullptr);
        }
    }
    else
    {
        if (callback != nullptr)
        {
            callback();
        }
    }
}

void PlayFabPartyManager::MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool, const char*)> onMigrationComplete)
{
    DEBUGLOG("PlayFabPartyManager::MigrateToNewNetwork()");

    if (regionList.empty())
    {
        DEBUGLOG("No regions to migrate to.");
        if (onMigrationComplete)
        {
            onMigrationComplete(false, nullptr);
        }
        return;
    }

    auto networkConfiguration = GetPartyNetworkConfiguration();
    auto invitationConfiguration = GetPartyInvitationConfiguration(m_networkId.c_str());
    auto networkDescriptor = PartyNetworkDescriptor{};
    auto partyRegion = PartyRegion{};

    // We want to create the new network in the 'best' region for all players
    regionList.at(0).copy(partyRegion.regionName, regionList.at(0).size());

    // Create a new network descriptor
    PartyError err = PartyManager::GetSingleton().CreateNewNetwork(
        m_localUser,                                // Local User
        &networkConfiguration,                      // Network Config
        1,                                          // Region List Count
        &partyRegion,                               // Region List
        &invitationConfiguration,                   // Invitation configuration
        nullptr,                                    // Async Identifier
        &networkDescriptor,                         // OUT network descriptor
        nullptr                                     // Applied initial invitation identifier
        );

    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("CreateNewNetwork", err);
        if (onMigrationComplete)
        {
            onMigrationComplete(false, nullptr);
        }
        return;
    }

    // Connect to the new network
    if (InternalConnectToNetwork(m_networkId.c_str(), networkDescriptor, &m_newNetwork, &m_newLocalEndpoint))
    {
        m_state = NetworkManagerState::Migrating;
        m_onRegionMigrated = onMigrationComplete;
    }
}

void PlayFabPartyManager::MigrateToNetwork(const char* descriptor, std::function<void(bool)> callback)
{
    DEBUGLOG("PlayFabPartyManager::MigrateToNetwork()");

    PartyNetworkDescriptor networkDescriptor = {};

    // Deserialize the remote network's descriptor
    PartyError err = PartyManager::DeserializeNetworkDescriptor(descriptor, &networkDescriptor);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("DeserializeNetworkDescriptor", err);
        return;
    }

    // Connect to the remote network
    if (InternalConnectToNetwork(m_networkId.c_str(), networkDescriptor, &m_newNetwork, &m_newLocalEndpoint))
    {
        m_state = NetworkManagerState::Migrating;
        m_onNetworkMigrated = callback;
    }
}

void PlayFabPartyManager::DoWork()
{
    if (m_partyInitialized == false)
    {
        return;
    }

    // Check for entity token refresh
    TryEntityTokenRefresh();

    uint32_t xblStateChangeCount = 0;
    PartyXblStateChangeArray xblChanges;

    // Process Xbl messages
    PartyError err = PartyXblManager::GetSingleton().StartProcessingStateChanges(&xblStateChangeCount, &xblChanges);
    if (PARTY_FAILED(err))
    {
        LogError_XblErrorWithMessage("PartyXblManager::StartProcessingStateChanges", err);
        return;
    }

    for (uint32_t i = 0; i < xblStateChangeCount; i++)
    {
        if (const PartyXblStateChange* change = xblChanges[i])
        {
            switch (change->stateChangeType)
            {
            case PartyXblStateChangeType::CreateLocalChatUserCompleted:      OnXblCreateLocalChatUserCompleted(change);      break;
            case PartyXblStateChangeType::LocalChatUserDestroyed:            OnXblLocalChatUserDestroyed(change);            break;
            case PartyXblStateChangeType::LoginToPlayFabCompleted:           OnXblLoginToPlayFabCompleted(change);           break;
            case PartyXblStateChangeType::RequiredChatPermissionInfoChanged: OnXblRequiredChatPermissionInfoChanged(change); break;
            case PartyXblStateChangeType::TokenAndSignatureRequested:        OnXblTokenAndSignatureRequested(change);        break;
            }
        }
    }

    err = PartyXblManager::GetSingleton().FinishProcessingStateChanges(xblStateChangeCount, xblChanges);
    if (PARTY_FAILED(err))
    {
        LogError_XblErrorWithMessage("PartyXblManager::FinishProcessingStateChanges", err);
        return;
    }

    PartyStateChangeArray changes;

    // Start processing messages from PlayFab Party
    uint32_t partyStateChangeCount = 0;
    err = PartyManager::GetSingleton().StartProcessingStateChanges(&partyStateChangeCount, &changes);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("StartProcessingStateChanges", err);
        return;
    }

    for (uint32_t i = 0; i < partyStateChangeCount; i++)
    {
        if (const PartyStateChange* change = changes[i])
        {
            if (change->stateChangeType != PartyStateChangeType::EndpointMessageReceived)
            {
                DEBUGLOG("PartyStateChange: %s", GetPartyStateChangeTypeString(change));
            }

            switch (change->stateChangeType)
            {
            //State changes events used by the sample
            case PartyStateChangeType::ConnectToNetworkCompleted:                        OnConnectToNetworkCompleted(change);                      break;
            case PartyStateChangeType::LocalUserRemoved:                                 OnLocalUserRemoved(change);                               break;
            case PartyStateChangeType::CreateEndpointCompleted:                          OnCreateEndpointCompleted(change);                        break;
            case PartyStateChangeType::EndpointCreated:                                  OnEndpointCreated(change);                                break;
            case PartyStateChangeType::EndpointDestroyed:                                OnEndpointDestroyed(change);                              break;
            case PartyStateChangeType::LeaveNetworkCompleted:                            OnLeaveNetworkCompleted(change);                          break;
            case PartyStateChangeType::NetworkDestroyed:                                 OnNetworkDestroyed(change);                               break;
            case PartyStateChangeType::EndpointMessageReceived:                          OnEndpointMessageReceived(change);                        break;
            case PartyStateChangeType::CreateChatControlCompleted:                       OnCreateChatControlCompleted(change);                     break;
            case PartyStateChangeType::ChatControlCreated:                               OnChatControlCreated(change);                             break;
            case PartyStateChangeType::ChatControlDestroyed:                             OnChatControlDestroyed(change);                           break;
            case PartyStateChangeType::ChatTextReceived:                                 OnChatTextReceived(change);                               break;
            case PartyStateChangeType::VoiceChatTranscriptionReceived:                   OnVoiceChatTranscriptionReceived(change);                 break;
            case PartyStateChangeType::SetChatAudioInputCompleted:                       OnSetChatAudioInputCompleted(change);                     break;
            case PartyStateChangeType::SetChatAudioOutputCompleted:                      OnSetChatAudioOutputCompleted(change);                    break;
            case PartyStateChangeType::LocalChatAudioInputChanged:                       OnLocalChatAudioInputChanged(change);                     break;
            case PartyStateChangeType::LocalChatAudioOutputChanged:                      OnLocalChatAudioOutputChanged(change);                    break;
            case PartyStateChangeType::PopulateAvailableTextToSpeechProfilesCompleted:   OnPopulateAvailableTextToSpeechProfilesCompleted(change); break;

            //The sample does not react to these events but we log them anyway
            case PartyStateChangeType::RemoteDeviceLeftNetwork:                          OnRemoteDeviceLeftNetwork(change);                        break;
            case PartyStateChangeType::InvitationDestroyed:                              OnInvitationDestroyed(change);                            break;
            case PartyStateChangeType::ChatControlLeftNetwork:                           OnChatControlLeftNetwork(change);                         break;
            case PartyStateChangeType::ConnectChatControlCompleted:                      LogPartyResult(static_cast<const PartyConnectChatControlCompletedStateChange*>(change));                      break;
            case PartyStateChangeType::DestroyChatControlCompleted:                      LogPartyResult(static_cast<const PartyDestroyChatControlCompletedStateChange*>(change));                      break;
            case PartyStateChangeType::AuthenticateLocalUserCompleted:                   LogPartyResult(static_cast<const PartyAuthenticateLocalUserCompletedStateChange*>(change));                   break;
            case PartyStateChangeType::DestroyLocalUserCompleted:                        LogPartyResult(static_cast<const PartyDestroyLocalUserCompletedStateChange*>(change));                        break;
            case PartyStateChangeType::CreateNewNetworkCompleted:                        LogPartyResult(static_cast<const PartyCreateNewNetworkCompletedStateChange*>(change));                        break;
            case PartyStateChangeType::RemoveLocalUserCompleted:                         LogPartyResult(static_cast<const PartyRemoveLocalUserCompletedStateChange*>(change));                         break;
            case PartyStateChangeType::DestroyEndpointCompleted:                         LogPartyResult(static_cast<const PartyDestroyEndpointCompletedStateChange*>(change));                         break;
            case PartyStateChangeType::RevokeInvitationCompleted:                        LogPartyResult(static_cast<const PartyRevokeInvitationCompletedStateChange*>(change));                        break;
            case PartyStateChangeType::KickDeviceCompleted:                              LogPartyResult(static_cast<const PartyKickDeviceCompletedStateChange*>(change));                              break;
            case PartyStateChangeType::SynthesizeTextToSpeechCompleted:                  LogPartyResult(static_cast<const PartySynthesizeTextToSpeechCompletedStateChange*>(change));                  break;
            case PartyStateChangeType::SetLanguageCompleted:                             LogPartyResult(static_cast<const PartySetLanguageCompletedStateChange*>(change));                             break;
            case PartyStateChangeType::SetTranscriptionOptionsCompleted:                 LogPartyResult(static_cast<const PartySetTranscriptionOptionsCompletedStateChange*>(change));                 break;
            case PartyStateChangeType::SetTextChatOptionsCompleted:                      LogPartyResult(static_cast<const PartySetTextChatOptionsCompletedStateChange*>(change));                      break;
            case PartyStateChangeType::DisconnectChatControlCompleted:                   LogPartyResult(static_cast<const PartyDisconnectChatControlCompletedStateChange*>(change));                   break;
            case PartyStateChangeType::ConfigureAudioManipulationVoiceStreamCompleted:   LogPartyResult(static_cast<const PartyConfigureAudioManipulationVoiceStreamCompletedStateChange*>(change));   break;
            case PartyStateChangeType::ConfigureAudioManipulationCaptureStreamCompleted: LogPartyResult(static_cast<const PartyConfigureAudioManipulationCaptureStreamCompletedStateChange*>(change)); break;
            }
        }
    }

    // Return the processed changes back to the PartyManager
    err = PartyManager::GetSingleton().FinishProcessingStateChanges(partyStateChangeCount, changes);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("FinishProcessingStateChanges", err);
    }
}

void PlayFabPartyManager::OnConnectToNetworkCompleted(const PartyStateChange* change)
{
    if (const PartyConnectToNetworkCompletedStateChange* result = static_cast<const PartyConnectToNetworkCompletedStateChange*>(change))
    {
        LogPartyResult(result);

        if (result->result == PartyStateChangeResult::Succeeded)
        {
            if (m_state != NetworkManagerState::Migrating)
            {
                m_state = NetworkManagerState::NetworkConnected;

                // Callback if ConnectToNetwork() was called
                if (m_onNetworkConnected)
                {
                    m_onNetworkConnected();
                }

                // Callback if CreateAndConnectToNetwork() was called
                if (m_onNetworkCreated)
                {
                    char descriptor[c_maxSerializedNetworkDescriptorStringLength + 1] = {};

                    // Serialize our local network descriptor for other peers to use
                    PartyError err = PartyManager::SerializeNetworkDescriptor(&result->networkDescriptor, descriptor);
                    if (PARTY_FAILED(err))
                    {
                        LogError_PartyErrorWithMessage("SerializeNetworkDescriptor", err);
                        m_onNetworkCreated(std::string());
                    }

                    DEBUGLOG("Serialized value: %s", descriptor);
                    // Callback with the descriptor to be shared with connecting clients
                    m_onNetworkCreated(std::string(descriptor));
                }
            }
        }
    }
}

void PlayFabPartyManager::OnLocalUserRemoved(const PartyStateChange* change)
{
    if (const PartyLocalUserRemovedStateChange* result = static_cast<const PartyLocalUserRemovedStateChange*>(change))
    {
        DEBUGLOG("PlayFabPartyManager::OnLocalUserRemoved: Reason : %s", GetPartyLocalUserRemovedReasonString(result->removedReason));

        if (m_state != NetworkManagerState::Leaving && m_state != NetworkManagerState::Migrating)
        {
            DEBUGLOG("PlayFabPartyManager::OnLocalUserRemoved: Unexpected local user removal!");
        }
    }
}

void PlayFabPartyManager::OnCreateEndpointCompleted(const PartyStateChange* change)
{
    if (const PartyCreateEndpointCompletedStateChange* result = static_cast<const PartyCreateEndpointCompletedStateChange*>(change))
    {
        LogPartyResult(result);

        if (result->result == PartyStateChangeResult::Succeeded)
        {
            if (m_state == NetworkManagerState::Migrating)
            {
                // We can leave our old network now
                m_network->LeaveNetwork(nullptr);
            }
        }
    }
}

void PlayFabPartyManager::OnEndpointCreated(const PartyStateChange* change)
{
    if (const PartyEndpointCreatedStateChange* result = static_cast<const PartyEndpointCreatedStateChange*>(change))
    {
        if (m_state != NetworkManagerState::Migrating)
        {
            PartyString user = nullptr;
            PartyError err = result->endpoint->GetEntityId(&user);
            if (PARTY_FAILED(err) || user == nullptr)
            {
                LogError_PartyErrorWithMessage("GetEntityId", err);
            }
            else
            {
                DEBUGLOG("PlayFabPartyManager::OnEndpointCreated: Established endpoint with user %s", user);

                uint64_t xuid = Managers::Get<OnlineManager>()->GetXuidFromEntityId(user);
                if (xuid != 0)
                {
                    // Tell the engine a user is ready to receive data
                    m_onEndpointChanged(xuid, true);
                }
                else
                {
                    DEBUGLOG("No Xuid for entity %s", user);
                }
            }
        }
    }
}

void PlayFabPartyManager::OnEndpointDestroyed(const PartyStateChange* change)
{
    if (const PartyEndpointDestroyedStateChange* result = static_cast<const PartyEndpointDestroyedStateChange*>(change))
    {
        DEBUGLOG("PlayFabPartyManager::OnEndpointDestroyed: Destroyed endpoint is %s", result->endpoint == m_localEndpoint ? "LOCAL" : "REMOTE");
        DEBUGLOG("PlayFabPartyManager::OnEndpointDestroyed: Reason: %s", GetPartyDestroyedReasonString(result->reason));
        DEBUGLOG("PlayFabPartyManager::OnEndpointDestroyed: Error Detail: %s", GetPartyErrorMessage(result->errorDetail));

        if (result->endpoint == m_localEndpoint)
        {
            // Our endpoint was disconnected
            m_localEndpoint = nullptr;

            if (Managers::Get<GameStateManager>()->GetState() == GameState::MigratingNetwork)
            {
                DEBUGLOG("PlayFabPartyManager::OnEndpointDestroyed: Old endpoint destroyed, swapping to new");
                m_localEndpoint = m_newLocalEndpoint;
                m_newLocalEndpoint = nullptr;
            }
        }
        else
        {
            // Another user has disconnected
            PartyString user = nullptr;
            PartyError err = result->endpoint->GetEntityId(&user);
            if (PARTY_FAILED(err) || user == nullptr)
            {
                LogError_PartyErrorWithMessage("GetEntityId", err);
                return;
            }

            uint64_t xuid = Managers::Get<OnlineManager>()->GetXuidFromEntityId(user);

            if (xuid != 0)
            {
                // Tell the engine a user has disconnected
                m_onEndpointChanged(xuid, false);
            }
            else
            {
                DEBUGLOG("PlayFabPartyManager::OnEndpointDestroyed: No Xuid for entity %s", user);
            }
        }
    }
}

void PlayFabPartyManager::OnLeaveNetworkCompleted(const PartyStateChange*)
{
    if (m_state == NetworkManagerState::Migrating)
    {
        m_state = NetworkManagerState::NetworkConnected;

        if (Managers::Get<OnlineManager>()->IsHost())
        {
            PartyNetworkDescriptor descriptor{};
            PartyError err = m_network->GetNetworkDescriptor(&descriptor);
            if (PARTY_FAILED(err))
            {
                LogError_PartyErrorWithMessage("GetNetworkDescriptor", err);
                if (m_onRegionMigrated)
                {
                    m_onRegionMigrated(false, nullptr);
                }
                return;
            }

            char descriptorString[c_maxSerializedNetworkDescriptorStringLength + 1] = {};

            // Serialize our local network descriptor for other peers to use
            err = PartyManager::SerializeNetworkDescriptor(&descriptor, descriptorString);
            if (PARTY_FAILED(err))
            {
                LogError_PartyErrorWithMessage("SerializeNetworkDescriptor", err);
                if (m_onRegionMigrated)
                {
                    m_onRegionMigrated(false, nullptr);
                }
                return;
            }

            if (m_onRegionMigrated)
            {
                m_onRegionMigrated(true, descriptorString);
            }
        }
        else
        {
            if (m_onRegionMigrated)
            {
                m_onRegionMigrated(true, nullptr);
            }
        }
    }
    else
    {
        m_state = NetworkManagerState::Initialize;

        if (m_onNetworkDestroyed)
        {
            m_onNetworkDestroyed();
        }
    }
}

void PlayFabPartyManager::OnNetworkDestroyed(const PartyStateChange* change)
{
    if (const PartyNetworkDestroyedStateChange* result = static_cast<const PartyNetworkDestroyedStateChange*>(change))
    {
        DEBUGLOG("PlayFabPartyManager::OnNetworkDestroyed: Reason: %s", GetPartyDestroyedReasonString(result->reason));

        m_network = nullptr;

        if (m_state == NetworkManagerState::Migrating)
        {
            DEBUGLOG("PlayFabPartyManager::OnNetworkDestroyed: Old network destroyed, swapping to new");
            m_network = m_newNetwork;
            m_newNetwork = nullptr;
        }
        else if (m_state != NetworkManagerState::Leaving)
        {
            DEBUGLOG("PlayFabPartyManager::OnNetworkDestroyed: Unexpected network destruction!");
            m_onMessageReceived(0, std::make_shared<GameMessage>(GameMessageType::NetworkLost, 0));
        }
    }
}

void PlayFabPartyManager::OnEndpointMessageReceived(const PartyStateChange* change)
{
    if (const PartyEndpointMessageReceivedStateChange* result = static_cast<const PartyEndpointMessageReceivedStateChange*>(change))
    {
        const uint8_t* buffer = static_cast<const uint8_t*>(result->messageBuffer);
        std::shared_ptr<GameMessage> packet = std::make_shared<GameMessage>(std::vector<uint8_t>(buffer, buffer + result->messageSize));

        PartyString sender = nullptr;
        PartyError err = result->senderEndpoint->GetEntityId(&sender);
        if (PARTY_SUCCEEDED(err) && sender != nullptr)
        {
            // Give the message to the game engine
            if (m_onMessageReceived)
            {
                uint64_t userXuid = Managers::Get<OnlineManager>()->GetXuidFromEntityId(sender);
                if (userXuid != 0)
                {
                    m_onMessageReceived(userXuid, packet);
                }
                else
                {
                    DEBUGLOG("Message '%s' received from entityid %s but we don't know their xuid yet.", packet->GetGameMessageTypeString(), sender);
                }
            }
        }
        else
        {
            LogError_PartyErrorWithMessage("GetEntityId", err);
        }
    }
}

void PlayFabPartyManager::OnCreateChatControlCompleted(const PartyStateChange* change)
{
    if (const PartyCreateChatControlCompletedStateChange* result = static_cast<const PartyCreateChatControlCompletedStateChange*>(change))
    {
        LogPartyResult(result);

        if (result->result == PartyStateChangeResult::Succeeded)
        {
            SetTextChatAccessibilityOptions();
        }
    }
}

void PlayFabPartyManager::OnChatControlCreated(const PartyStateChange* change)
{
    if (const PartyChatControlCreatedStateChange* result = static_cast<const PartyChatControlCreatedStateChange*>(change))
    {
        PartyString sender = nullptr;
        PartyError err = result->chatControl->GetEntityId(&sender);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("GetEntityId", err);
        }
        else
        {
            DEBUGLOG("Created ChatControl for %s", sender);
            m_chatControls[sender] = result->chatControl;

            PartyLocalChatControl* local = nullptr;
            err = result->chatControl->GetLocal(&local);
            if (PARTY_FAILED(err))
            {
                LogError_PartyErrorWithMessage("GetLocal", err);
            }
            else if (local == nullptr)
            {
                DEBUGLOG("ChatControl is remote");

                // Remote ChatControl added, set chat permissions
                err = m_localChatControl->SetPermissions(result->chatControl, PartyChatPermissionOptions::ReceiveAudio | PartyChatPermissionOptions::ReceiveText | PartyChatPermissionOptions::SendAudio);
                if (PARTY_FAILED(err))
                {
                    LogError_PartyErrorWithMessage("SetPermissions", err);
                }
            }
        }
    }
}

void PlayFabPartyManager::OnChatControlDestroyed(const PartyStateChange* change)
{
    if (const PartyChatControlDestroyedStateChange* result = static_cast<const PartyChatControlDestroyedStateChange*>(change))
    {
        DEBUGLOG("PlayFabPartyManager::OnChatControlDestroyed: Reason: %s", GetPartyDestroyedReasonString(result->reason));
        DEBUGLOG("PlayFabPartyManager::OnChatControlDestroyed: Error Detail: %s", GetPartyErrorMessage(result->errorDetail));

        PartyString sender = nullptr;
        PartyError err = result->chatControl->GetEntityId(&sender);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("GetEntityId", err);
        }
        else
        {
            DEBUGLOG("PlayFabPartyManager::OnChatControlDestroyed: Destroyed ChatControl from %s", sender);

            if (result->chatControl == m_localChatControl)
            {
                DEBUGLOG("PlayFabPartyManager::OnChatControlDestroyed: Local ChatControl destroyed");
                m_localChatControl = nullptr;

                if (m_state == NetworkManagerState::Leaving)
                {
                    // Continue the LeaveNetwork process
                    m_network->LeaveNetwork(nullptr);
                }
            }
            else
            {
                m_chatControls.erase(sender);
            }
        }
    }
}

void PlayFabPartyManager::OnChatTextReceived(const PartyStateChange* change)
{
    if (const PartyChatTextReceivedStateChange* result = static_cast<const PartyChatTextReceivedStateChange*>(change))
    {
        // Toast the text on the screen
        std::string message;

        // First look for translations
        if (result->translationCount > 0)
        {
            // Since we only have one local chat control, there will only be one translation
            if (result->translations[0].result == PartyStateChangeResult::Succeeded)
            {
                message = result->translations[0].translation;
            }
            else
            {
                DEBUGLOG("Translation failed: %s", GetPartyStateChangeReasonString(result->translations[0].result));
            }
        }

        if (message.empty())
        {
            message = result->chatText;
        }

        std::string displayName = DisplayNameFromChatControl(result->senderChatControl);
        Managers::Get<ScreenManager>()->GetSTTWindow()->AddSTTString(displayName, message, false);

        DEBUGLOG("Chat Text: %s", message.c_str());
    }
}

void PlayFabPartyManager::OnVoiceChatTranscriptionReceived(const PartyStateChange* change)
{
    if (const PartyVoiceChatTranscriptionReceivedStateChange* result = static_cast<const PartyVoiceChatTranscriptionReceivedStateChange*>(change))
    {
        if (PARTY_FAILED(result->errorDetail))
        {
            DEBUGLOG("Error Detail: %s", GetPartyErrorMessage(result->errorDetail));
        }

        if (result->result != PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Failed: %s", GetPartyStateChangeReasonString(result->result));
        }
        else if (result->transcription == nullptr)
        {
            DEBUGLOG("Transcription is null");
        }
        else
        {
            // Toast the text on the screen
            std::string message;

            // First look for translations
            if (result->translationCount > 0)
            {
                // Since we only have one local chat control, there will only be one translation
                if (result->translations[0].result == PartyStateChangeResult::Succeeded)
                {
                    message = result->translations[0].translation;
                }
                else
                {
                    DEBUGLOG("Translation failed: %s", GetPartyStateChangeReasonString(result->translations[0].result));
                }
            }

            if (message.empty())
            {
                message = result->transcription;
            }

            Managers::Get<ScreenManager>()->GetSTTWindow()->AddSTTString(
                DisplayNameFromChatControl(result->senderChatControl),
                message,
                true,
                result->type == PartyVoiceChatTranscriptionPhraseType::Hypothesis
                );

            DEBUGLOG("Chat Transcription: %s", message.c_str());
        }
    }
}

void PlayFabPartyManager::OnSetChatAudioInputCompleted(const PartyStateChange* change)
{
    if (const PartySetChatAudioInputCompletedStateChange* result = static_cast<const PartySetChatAudioInputCompletedStateChange*>(change))
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Succeeded");
        }
        else
        {
            DEBUGLOG("Failed: %s", GetPartyStateChangeReasonString(result->result));
            Managers::Get<ScreenManager>()->ShowError("Voice chat failed");
        }
    }
}

void PlayFabPartyManager::OnSetChatAudioOutputCompleted(const PartyStateChange* change)
{
    if (const PartySetChatAudioOutputCompletedStateChange* result = static_cast<const PartySetChatAudioOutputCompletedStateChange*>(change))
    {
        LogPartyResult(result);

        if (result->result != PartyStateChangeResult::Succeeded)
        {
            Managers::Get<ScreenManager>()->ShowError("Voice chat failed");
        }
    }
}

void PlayFabPartyManager::OnLocalChatAudioInputChanged(const PartyStateChange* change)
{
    if (const PartyLocalChatAudioInputChangedStateChange* result = static_cast<const PartyLocalChatAudioInputChangedStateChange*>(change))
    {
        if (PARTY_FAILED(result->errorDetail))
        {
            DEBUGLOG("Error Detail: %s", GetPartyErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnLocalChatAudioOutputChanged(const PartyStateChange* change)
{
    if (const PartyLocalChatAudioOutputChangedStateChange* result = static_cast<const PartyLocalChatAudioOutputChangedStateChange*>(change))
    {
        if (PARTY_FAILED(result->errorDetail))
        {
            DEBUGLOG("Error Detail: %s", GetPartyErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnPopulateAvailableTextToSpeechProfilesCompleted(const PartyStateChange* change)
{
    if (const PartyPopulateAvailableTextToSpeechProfilesCompletedStateChange* result = static_cast<const PartyPopulateAvailableTextToSpeechProfilesCompletedStateChange*>(change))
    {
        uint32_t profileCount = 0;
        PartyTextToSpeechProfileArray profileList = nullptr;

        // Get the profile list
        PartyError err = result->localChatControl->GetAvailableTextToSpeechProfiles(&profileCount, &profileList);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("GetAvailableTextToSpeechProfiles", err);
            return;
        }

        // Loop the profiles looking for one that matches our language
        for (uint32_t j = 0; j < profileCount; j++)
        {
            PartyTextToSpeechProfile* profile = profileList[j];

            PartyString languageCode = nullptr;

            err = profile->GetLanguageCode(&languageCode);
            if (PARTY_FAILED(err))
            {
                LogError_PartyErrorWithMessage("GetLanguageCode", err);
                continue;
            }

            if (m_languageCode == languageCode)
            {
                // Get the profile name
                PartyString profileName = nullptr;
                err = profile->GetIdentifier(&profileName);
                if (PARTY_FAILED(err))
                {
                    LogError_PartyErrorWithMessage ("GetIdentifier", err);
                }

                DEBUGLOG("Setting TTS profile to: %s", profileName);

                // Set the profile to the first we find for our language
                // Ideally, the user would be able to select one
                err = result->localChatControl->SetTextToSpeechProfile(PartySynthesizeTextToSpeechType::VoiceChat, profileName, nullptr);
                if (PARTY_FAILED(err))
                {
                    LogError_PartyErrorWithMessage("SetTextToSpeechProfile", err);
                }

                break;
            }
        }
    }
}

void PlayFabPartyManager::OnRemoteDeviceLeftNetwork(const PartyStateChange* change)
{
    if (const PartyRemoteDeviceLeftNetworkStateChange* result = static_cast<const PartyRemoteDeviceLeftNetworkStateChange*>(change))
    {
        DEBUGLOG("PlayFabPartyManager::OnRemoteDeviceLeftNetwork: Reason: %s", GetPartyDestroyedReasonString(result->reason));
        DEBUGLOG("PlayFabPartyManager::OnRemoteDeviceLeftNetwork: Error Detail: %s", GetPartyErrorMessage(result->errorDetail));
    }
}

void PlayFabPartyManager::OnInvitationDestroyed(const PartyStateChange* change)
{
    if (const PartyInvitationDestroyedStateChange* result = static_cast<const PartyInvitationDestroyedStateChange*>(change))
    {
        DEBUGLOG("PlayFabPartyManager::OnInvitationDestroyed: Reason: %s", GetPartyDestroyedReasonString(result->reason));
        DEBUGLOG("PlayFabPartyManager::OnInvitationDestroyed: Error Detail: %s", GetPartyErrorMessage(result->errorDetail));
    }
}

void PlayFabPartyManager::OnChatControlLeftNetwork(const PartyStateChange* change)
{
    if (const PartyChatControlLeftNetworkStateChange* result = static_cast<const PartyChatControlLeftNetworkStateChange*>(change))
    {
        DEBUGLOG("PlayFabPartyManager::OnChatControlLeftNetwork: Reason: %s", GetPartyDestroyedReasonString(result->reason));
        DEBUGLOG("PlayFabPartyManager::OnChatControlLeftNetwork: Error Detail: %s", GetPartyErrorMessage(result->errorDetail));
    }
}

std::string PlayFabPartyManager::DisplayNameFromChatControl(PartyChatControl* control)
{
    std::string sttuser;
    PartyString sender = nullptr;

    PartyError err = control->GetEntityId(&sender);
    if (PARTY_FAILED(err) || sender == nullptr)
    {
        LogError_PartyErrorWithMessage("GetEntityId", err);
        sttuser = "[ERROR]";
    }
    else
    {
        uint64_t xuid = Managers::Get<OnlineManager>()->GetXuidFromEntityId(sender);

        // Get the display name of the sender
        std::shared_ptr<PlayerState> playerInfo = g_game->GetPlayerState(xuid);

        if (xuid != 0 && playerInfo != nullptr)
        {
            sttuser = playerInfo->DisplayName;
        }
        else
        {
            sttuser = std::to_string(xuid);
        }
    }

    return sttuser;
}

PartyChatControl* PlayFabPartyManager::GetChatControl(std::string& peer)
{
    auto itr = m_chatControls.find(peer);
    if (itr != m_chatControls.end())
    {
        return itr->second;
    }
    return nullptr;
}

void PlayFabPartyManager::OnXblCreateLocalChatUserCompleted(const PartyXblStateChange* change)
{
    if (change)
    {
        DEBUGLOG("PartyXblStateChangeType: CreateLocalChatUserCompleted");
        m_localUserReady = true;
    }
}

void PlayFabPartyManager::OnXblLocalChatUserDestroyed(const PartyXblStateChange* change)
{
    if (change)
    {
        DEBUGLOG("PartyXblStateChangeType: LocalChatUserDestroyed");
    }
}

void PlayFabPartyManager::OnXblLoginToPlayFabCompleted(const PartyXblStateChange* change)
{
    DEBUGLOG("PartyXblStateChangeType: LoginToPlayFabCompleted");
    if (const PartyXblLoginToPlayFabCompletedStateChange* result = static_cast<const PartyXblLoginToPlayFabCompletedStateChange*>(change))
    {
        if (result->result == PartyXblStateChangeResult::Succeeded)
        {
            m_localEntityId = result->entityId;
            m_localEntityToken = result->titlePlayerEntityToken;
            m_localEntityTokenExpirationTime = result->expirationTime;

            // Refresh the token an hour before expiration
            m_localEntityTokenExpirationTime -= 3600;

            if (m_localUser == nullptr)
            {
                // Finish creating our local Party user
                CreateLocalUser();
            }
            else
            {
                // If we already have a user, update the entity token
                PartyError err = m_localUser->UpdateEntityToken(m_localEntityToken.c_str());
                if (PARTY_FAILED(err))
                {
                    DEBUGLOG("UpdateEntityToken failed: %s\n", GetPartyErrorMessage(err));
                }

                Managers::Get<OnlineManager>()->SetEntityToken();
            }

            uint64_t xuid = 0;
            m_localChatUser->GetXboxUserId(&xuid);

            DEBUGLOG("LocalUser EntityId: %s", m_localEntityId.c_str());
            DEBUGLOG("LocalUser EntityToken: %s", m_localEntityToken.c_str());
            DEBUGLOG("LocalUser EntityTokenExpirationTime: %d", m_localEntityTokenExpirationTime);

            m_playfabLoginComplete = true;
        }

        // Don't fire the callback if we are refreshing the entity token
        if (m_userCreatedCallback && m_refreshingEntityToken == false)
        {
            m_userCreatedCallback(result->errorDetail);
        }

        m_refreshingEntityToken = false;
    }
}

void PlayFabPartyManager::OnXblRequiredChatPermissionInfoChanged(const PartyXblStateChange* change)
{
    DEBUGLOG("PartyXblStateChangeType: RequiredChatPermissionInfoChanged");
    if (const PartyXblRequiredChatPermissionInfoChangedStateChange* result = static_cast<const PartyXblRequiredChatPermissionInfoChangedStateChange*>(change))
    {
        PartyXblLocalChatUser* localChatUser = result->localChatUser;
        PartyXblChatUser* targetChatUser = result->targetChatUser;

        PartyXblChatPermissionInfo chatPermissionInfo;
        PartyError err = localChatUser->GetRequiredChatPermissionInfo(targetChatUser, &chatPermissionInfo);
        if (PARTY_FAILED(err))
        {
            LogError_XblErrorWithMessage("PartyXblManager::GetRequiredChatPermissionInfo", err);
            return;
        }

        uint64_t userXuid = 0;
        err = localChatUser->GetXboxUserId(&userXuid);
        if (PARTY_FAILED(err))
        {
            LogError_XblErrorWithMessage("PartyXblManager::GetXboxUserId", err);
            return;
        }

        const char* userEntityId = Managers::Get<OnlineManager>()->GetEntityIdFromXuid(userXuid);
        if (userEntityId == nullptr)
        {
            DEBUGLOG("No entity id for xuid %llu", userXuid);
            return;
        }

        auto it = m_chatControls.find(userEntityId);
        if (it != m_chatControls.end())
        {
            PartyChatControl* targetChatControl = it->second;
            if (m_localChatControl)
            {
                DEBUGLOG("Setting chat permissions with user %llu to 0x%x", userXuid, chatPermissionInfo.chatPermissionMask);
                m_localChatControl->SetPermissions(targetChatControl, chatPermissionInfo.chatPermissionMask);
            }
        }
        else
        {
            DEBUGLOG("No chat control for entityid %s", userEntityId);
        }
    }
}

void PlayFabPartyManager::OnXblTokenAndSignatureRequested(const PartyXblStateChange* change)
{
    DEBUGLOG("PartyXblStateChangeType: TokenAndSignatureRequested");
    if (const PartyXblTokenAndSignatureRequestedStateChange* result = static_cast<const PartyXblTokenAndSignatureRequestedStateChange*>(change))
    {
        // Setup the options flags
        XUserGetTokenAndSignatureOptions tnsOptions = result->forceRefresh ? XUserGetTokenAndSignatureOptions::ForceRefresh : XUserGetTokenAndSignatureOptions::None;

        if (result->allUsers)
        {
            tnsOptions |= XUserGetTokenAndSignatureOptions::AllUsers;
        }

        // Format the headers
        ATG::ArrayView<PartyXblHttpHeader> partyHeaders{ result->headers, result->headerCount };
        std::vector<XUserGetTokenAndSignatureHttpHeader> headers;
        for (auto header : partyHeaders)
        {
            headers.push_back({ header.name, header.value });
        }

        auto asyncManager = Managers::Get<AsyncTaskManager>();
        auto asyncQueue = asyncManager->GetDefaultQueue();
        auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [correlationId = result->correlationId](XAsyncBlock* async)
        {
            size_t bufferSize;
            HRESULT hr = XUserGetTokenAndSignatureResultSize(async, &bufferSize);
            if (FAILED(hr))
            {
                DEBUGLOG("XUserGetTokenAndSignatureResultSize failed: 0x%08X", static_cast<unsigned int>(hr));
            }

            std::vector<uint8_t> buffer(bufferSize);
            XUserGetTokenAndSignatureData* data = nullptr;

            if (SUCCEEDED(hr))
            {
                hr = XUserGetTokenAndSignatureResult(async, buffer.size(), buffer.data(), &data, nullptr /*bufferUsed*/);
                if (FAILED(hr))
                {
                    DEBUGLOG("XUserGetTokenAndSignatureResult failed: 0x%08X", static_cast<unsigned int>(hr));
                }
            }

            // Tell the party manager we're done
            PartyError err = PartyXblManager::GetSingleton().CompleteGetTokenAndSignatureRequest(
                correlationId,
                SUCCEEDED(hr),
                data ? data->token : nullptr,
                data ? data->signature : nullptr
            );

            if (PARTY_SUCCEEDED(err))
            {
                DEBUGLOG("CompleteGetTokenAndSignatureRequest succeeded. \n\tToken: %s \n\tSignature: %s", data ? data->token : "nullptr", data ? data->signature : "nullptr");
            }
            else
            {
                LogError_XblErrorWithMessage("PartyXblManager::CompleteGetTokenAndSignatureRequest", err);
            }
        });

        // Start the auth call
        HRESULT hr = XUserGetTokenAndSignatureAsync(
            *Managers::Get<XboxUserManager>()->GetCurrentUser(),    // XUser to query
            tnsOptions,                                             // Option flags
            result->method,                                         // HTTP method ("GET", "POST")
            result->url,                                            // Auth url
            static_cast<uint32_t>(headers.size()),                  // Additional header count
            headers.data(),                                         // Additional headers
            result->bodySize,                                       // Query body size
            result->body,                                           // Query body
            &asyncHelper->asyncBlock);

        if (FAILED(hr))
        {
            LogError_HRESULT("XUserGetTokenAndSignatureAsync", hr);
            delete asyncHelper;
        }
    }
}

void PlayFabPartyManager::TryEntityTokenRefresh()
{
    // Do not refresh if we are in the middle of refreshing
    if (m_refreshingEntityToken)
    {
        return;
    }

    // Do not refresh if we haven't done the initial login
    if (m_playfabLoginComplete == false || m_localEntityTokenExpirationTime == 0)
    {
        return;
    }

    // Do not refresh if we don't have a local chat user yet
    if (m_localChatUser == nullptr)
    {
        return;
    }

    // Refresh if the token is about to expire
    time_t currentTime = std::time(nullptr);
    if (currentTime > m_localEntityTokenExpirationTime)
    {
        PartyError err = PartyXblManager::GetSingleton().LoginToPlayFab(m_localChatUser, nullptr);
        if (PARTY_FAILED(err))
        {
            LogError_XblErrorWithMessage("PartyXblManager::LoginToPlayFab", err);
        }
        else
        {
            m_refreshingEntityToken = true;
        }
    }
}

bool PlayFabPartyManager::GetPartyNetworkDescriptor(PartyNetworkDescriptor* descriptor)
{
    if (m_network == nullptr)
    {
        return false;
    }

    PartyError err = m_network->GetNetworkDescriptor(descriptor);

    return SUCCEEDED(err);
}

void PlayFabPartyManager::SetCognitiveServicesEnabled(bool bEnabled)
{
    m_enableCognitiveServices = bEnabled;

    if (State() == NetworkManagerState::NetworkConnected)
    {
        SetTextChatAccessibilityOptions();
    }
}

void PlayFabPartyManager::SetNetworkType(bool bIsPeerToPeerConnectionType)
{
    if (bIsPeerToPeerConnectionType)
    {
        m_connectionType = NetworkManagerConnectionType::peerToPeer;
    }
    else
    {
        m_connectionType = NetworkManagerConnectionType::cloudRelay;
    }
}

void PlayFabPartyManager::SetTextChatFilteringEnabled(bool bEnabled)
{
    m_textChatFiltering = bEnabled;

    if (State() == NetworkManagerState::NetworkConnected)
    {
        SetTextChatAccessibilityOptions();
    }
}

void PlayFabPartyManager::SetTextChatTranslationOptions(bool bTranslateToLocalLanguage, bool bEnableTextFiltering)
{
    DEBUGLOG("SetTextChatTranslationOptions: bTranslateToLocalLanguage = %s, bEnableTextFiltering = %s", bTranslateToLocalLanguage ? "Enabled" : "Disabled", bEnableTextFiltering ? "Enabled" : "Disabled");

    PartyTextChatOptions options = PartyTextChatOptions::None;

    if (bTranslateToLocalLanguage)
    {
        options |= PartyTextChatOptions::TranslateToLocalLanguage;
    }

    if (bEnableTextFiltering)
    {
        options |= PartyTextChatOptions::FilterOffensiveText;
    }

    // Enable translation to local language in chat controls.
    PartyError err = m_localChatControl->SetTextChatOptions(options, nullptr);
    if (PARTY_FAILED(err))
    {
        LogError_PartyErrorWithMessage("SetTextChatOptions", err);
    }
}

void PlayFabPartyManager::SetVoiceChatTranscriptionOptions(bool bTranscribeSelf, bool bTranscribeOtherChatControlsWithMatchingLanguages, bool bTranscribeOtherChatControlsWithNonMatchingLanguages, bool bTranslateToLocalLanguage)
{
    if (m_localChatControl)
    {
        DEBUGLOG("Setting Speech-To-Text options.");

        PartyVoiceChatTranscriptionOptions Options = PartyVoiceChatTranscriptionOptions::None;

        if (bTranscribeSelf)
        {
            Options |= PartyVoiceChatTranscriptionOptions::TranscribeSelf;
        }

        if (bTranscribeOtherChatControlsWithMatchingLanguages)
        {
            Options |= PartyVoiceChatTranscriptionOptions::TranscribeOtherChatControlsWithMatchingLanguages;
        }

        if (bTranscribeOtherChatControlsWithNonMatchingLanguages)
        {
            Options |= PartyVoiceChatTranscriptionOptions::TranscribeOtherChatControlsWithNonMatchingLanguages;
        }

        if (bTranslateToLocalLanguage)
        {
            Options |= PartyVoiceChatTranscriptionOptions::TranslateToLocalLanguage;
        }

        // Set the transcription options on our chat control.
        PartyError err = m_localChatControl->SetTranscriptionOptions(Options, nullptr);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("SetTranscriptionOptions", err);
        }
    }
}

std::vector<std::string> PlayFabPartyManager::GetBestRegionList(const std::vector<std::vector<std::pair<std::string, uint64_t>>>& userRegionLatencyList)
{
    std::vector<std::pair<std::string, uint64_t>> candidateRegions;
    std::vector<std::string> regionList;

    for (const auto& userLatencies : userRegionLatencyList)
    {
        if (candidateRegions.empty())
        {
            for (const auto& region : userLatencies)
            {
                candidateRegions.emplace_back(std::pair<std::string, uint64_t>{ region.first, region.second * region.second});
            }
        }
        else
        {
            auto oldCandidates = std::move(candidateRegions);

            candidateRegions.clear();

            for (const auto& region : oldCandidates)
            {
                auto it = std::find_if(
                    std::begin(userLatencies),
                    std::end(userLatencies),
                    [&](const std::pair<std::string, uint64_t>& item)
                    {
                        return item.first == region.first;
                    });

                if (it != std::end(userLatencies))
                {
                    candidateRegions.emplace_back(std::pair<std::string, uint64_t>{ region.first, (*it).second + (region.second * region.second)});
                }
            }
        }
    }

    if (!candidateRegions.empty())
    {
        std::sort(
            std::begin(candidateRegions),
            std::end(candidateRegions),
            [](const std::pair<std::string, uint64_t>& first, const std::pair<std::string, uint64_t>& second)
            {
                return first.second < second.second;
            });

        for (const auto& item : candidateRegions)
        {
            regionList.push_back(item.first);
        }
    }

    return regionList;
}

void PlayFabPartyManager::PopulatePartyRegionLatencies(bool send)
{
    uint32_t regionCount = 0;
    const PartyRegion* regionList = nullptr;

    PartyError err = PartyManager::GetSingleton().GetRegions(&regionCount, &regionList);
    if (PARTY_SUCCEEDED(err))
    {
        DEBUGLOG("Populating Party Regions (%lu)", regionCount);

        std::string uiString;

        uiString.reserve(1024);
        uiString = "Party Regions:";

        for (uint32_t x = 0; x < regionCount; x++)
        {
            uiString += regionList[x].regionName;
            uiString += ":  ";
            uiString += std::to_string(regionList[x].roundTripLatencyInMilliseconds);
            uiString += " ms";

            DEBUGLOG("%20hs:  %lu ms", regionList[x].regionName, regionList[x].roundTripLatencyInMilliseconds);

            if (send)
            {
                // Tell the host about it
                std::string messageStr;

                messageStr += regionList[x].regionName;
                messageStr += ":";
                messageStr += std::to_string(regionList[x].roundTripLatencyInMilliseconds);

                Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::RegionLatency, messageStr));
            }

            // Track our latencies
            g_game->GetLocalPlayerState()->SetRegionLatency(regionList[x].regionName, regionList[x].roundTripLatencyInMilliseconds);
        }
    }
    else
    {
        LogError_PartyErrorWithMessage("GetRegions", err);
    }
}

std::map<uint64_t, uint64_t> PlayFabPartyManager::GetRemotePlayerLatencies()
{
    auto userMap = std::map<uint64_t, uint64_t>();

    if (m_localEndpoint != nullptr && m_state == NetworkManagerState::NetworkConnected)
    {
        // Get all the endpoints on our network
        uint32_t endpointCount;
        PartyEndpointArray endpointArray;

        PartyError err = m_network->GetEndpoints(&endpointCount, &endpointArray);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("GetEndpoints", err);
            return userMap;
        }

        // Get a list of the remote endpoints
        std::vector<PartyEndpoint*> remoteEndpoints;

        for (uint32_t count = 0; count < endpointCount; count++)
        {
            PartyLocalEndpoint* local;

            err = endpointArray[count]->GetLocal(&local);
            if (PARTY_FAILED(err))
            {
                LogError_PartyErrorWithMessage("GetLocal", err);
                return userMap;
            }

            if (local == nullptr)
            {
                remoteEndpoints.push_back(endpointArray[count]);
            }
        }

        if (remoteEndpoints.empty())
        {
            return userMap;
        }

        // Get the round trip latency value for all remote endpoints
        auto statValues = std::vector<uint64_t>(remoteEndpoints.size());
        auto statType = PartyEndpointStatistic::AverageDeviceRoundTripLatencyInMilliseconds;

        err = m_localEndpoint->GetEndpointStatistics(static_cast<uint32_t>(remoteEndpoints.size()),remoteEndpoints.data(), 1, &statType, statValues.data());
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("GetEndpointStatistics", err);
            return userMap;
        }

        // Create a map of xuid to latency value
        for (size_t count = 0; count < remoteEndpoints.size(); count++)
        {
            auto endpoint = remoteEndpoints[count];

            PartyString remote = nullptr;
            err = endpoint->GetEntityId(&remote);
            if (PARTY_FAILED(err) || remote == nullptr)
            {
                LogError_PartyErrorWithMessage("GetEntityId", err);
                return userMap;
            }

            uint64_t xuid = Managers::Get<OnlineManager>()->GetXuidFromEntityId(remote);
            userMap[xuid] = statValues[count];
        }
    }

    return userMap;
}

uint64_t PlayFabPartyManager::GetNetworkQueuedSendMessagesCount()
{
    uint64_t returnValue = 0;

    if (m_network)
    {
        auto stat = PartyNetworkStatistic::CurrentlyQueuedSendMessages;
        auto err = m_network->GetNetworkStatistics(1, &stat, &returnValue);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("GetNetworkStatistics", err);
        }
    }

    return returnValue;
}

uint64_t PlayFabPartyManager::GetNetworkAverageRelayRoundTripTime()
{
    uint64_t returnValue = 0;

    if (m_network)
    {
        auto stat = PartyNetworkStatistic::AverageRelayServerRoundTripLatencyInMilliseconds;
        auto err = m_network->GetNetworkStatistics(1, &stat, &returnValue);
        if (PARTY_FAILED(err))
        {
            LogError_PartyErrorWithMessage("GetNetworkStatistics", err);
        }
    }

    return returnValue;
}
