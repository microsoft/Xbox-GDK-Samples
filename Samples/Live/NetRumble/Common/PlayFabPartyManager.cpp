//--------------------------------------------------------------------------------------
// PlayFabPartyManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Managers.h"
#include "Game.h"

#include "PartyImpl.h"
#include "PartyXboxLiveImpl.h"
#include "XboxLiveOnlineManager.h"
#include "Managers.h"
#include "NetworkMessages.h"
#include "STTOverlayScreen.h"
#include "PlayerState.h"

#include <ctime>

using namespace NetRumble;
using namespace Party;

namespace
{
    PartyString GetErrorMessage(PartyError error)
    {
        PartyString errString = nullptr;

        PartyError err = PartyManager::GetErrorMessage(error, &errString);
        if (PARTY_FAILED(err))
        {
            DEBUGLOG("Failed to get error message %lu.\n", error);
            return "[ERROR]";
        }

        return errString;
    }

    PartyString GetXblErrorMessage(PartyError error)
    {
        PartyString errString = nullptr;

        PartyError err = PartyXblManager::GetErrorMessage(error, &errString);
        if (PARTY_FAILED(err))
        {
            DEBUGLOG("Failed to get error message %lu.\n", error);
            return "[ERROR]";
        }

        return errString;
    }

    std::string PartyStateChangeResultToReasonString(PartyStateChangeResult result)
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
        return "Unknown enumeration value";
    }

    template <typename result_type>
    void LogResult(const result_type &err)
    {
        if (PARTY_FAILED(err->errorDetail))
        {
            DEBUGLOG("Error Detail: %hs\n", GetErrorMessage(err->errorDetail));
        }
        if (err->result != PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(err->result).c_str());
        }
    }

    std::string GetPartyStateChangeTypeString(PartyStateChangeType Type)
    {
        switch (Type)
        {
        case PartyStateChangeType::RegionsChanged:return "RegionsChanged"; break;
        case PartyStateChangeType::DestroyLocalUserCompleted:return "DestroyLocalUserCompleted"; break;
        case PartyStateChangeType::CreateNewNetworkCompleted:return "CreateNewNetworkCompleted"; break;
        case PartyStateChangeType::ConnectToNetworkCompleted:return "ConnectToNetworkCompleted"; break;
        case PartyStateChangeType::AuthenticateLocalUserCompleted:return "AuthenticateLocalUserCompleted"; break;
        case PartyStateChangeType::NetworkConfigurationMadeAvailable:return "NetworkConfigurationMadeAvailable"; break;
        case PartyStateChangeType::NetworkDescriptorChanged:return "NetworkDescriptorChanged"; break;
        case PartyStateChangeType::LocalUserRemoved:return "LocalUserRemoved"; break;
        case PartyStateChangeType::RemoveLocalUserCompleted:return "RemoveLocalUserCompleted"; break;
        case PartyStateChangeType::LocalUserKicked:return "LocalUserKicked"; break;
        case PartyStateChangeType::CreateEndpointCompleted:return "CreateEndpointCompleted"; break;
        case PartyStateChangeType::DestroyEndpointCompleted:return "DestroyEndpointCompleted"; break;
        case PartyStateChangeType::EndpointCreated:return "EndpointCreated"; break;
        case PartyStateChangeType::EndpointDestroyed:return "EndpointDestroyed"; break;
        case PartyStateChangeType::RemoteDeviceCreated:return "RemoteDeviceCreated"; break;
        case PartyStateChangeType::RemoteDeviceDestroyed:return "RemoteDeviceDestroyed"; break;
        case PartyStateChangeType::RemoteDeviceJoinedNetwork:return "RemoteDeviceJoinedNetwork"; break;
        case PartyStateChangeType::RemoteDeviceLeftNetwork:return "RemoteDeviceLeftNetwork"; break;
        case PartyStateChangeType::DevicePropertiesChanged:return "DevicePropertiesChanged"; break;
        case PartyStateChangeType::LeaveNetworkCompleted:return "LeaveNetworkCompleted"; break;
        case PartyStateChangeType::NetworkDestroyed:return "NetworkDestroyed"; break;
        case PartyStateChangeType::EndpointMessageReceived:return "EndpointMessageReceived"; break;
        case PartyStateChangeType::DataBuffersReturned:return "DataBuffersReturned"; break;
        case PartyStateChangeType::EndpointPropertiesChanged:return "EndpointPropertiesChanged"; break;
        case PartyStateChangeType::SynchronizeMessagesBetweenEndpointsCompleted:return "SynchronizeMessagesBetweenEndpointsCompleted"; break;
        case PartyStateChangeType::CreateInvitationCompleted:return "CreateInvitationCompleted"; break;
        case PartyStateChangeType::RevokeInvitationCompleted:return "RevokeInvitationCompleted"; break;
        case PartyStateChangeType::InvitationCreated:return "InvitationCreated"; break;
        case PartyStateChangeType::InvitationDestroyed:return "InvitationDestroyed"; break;
        case PartyStateChangeType::NetworkPropertiesChanged:return "NetworkPropertiesChanged"; break;
        case PartyStateChangeType::KickDeviceCompleted:return "KickDeviceCompleted"; break;
        case PartyStateChangeType::KickUserCompleted:return "KickUserCompleted"; break;
        case PartyStateChangeType::CreateChatControlCompleted:return "CreateChatControlCompleted"; break;
        case PartyStateChangeType::DestroyChatControlCompleted:return "DestroyChatControlCompleted"; break;
        case PartyStateChangeType::ChatControlCreated:return "ChatControlCreated"; break;
        case PartyStateChangeType::ChatControlDestroyed:return "ChatControlDestroyed"; break;
        case PartyStateChangeType::SetChatAudioEncoderBitrateCompleted:return "SetChatAudioEncoderBitrateCompleted"; break;
        case PartyStateChangeType::ChatTextReceived:return "ChatTextReceived"; break;
        case PartyStateChangeType::VoiceChatTranscriptionReceived:return "VoiceChatTranscriptionReceived"; break;
        case PartyStateChangeType::SetChatAudioInputCompleted:return "SetChatAudioInputCompleted"; break;
        case PartyStateChangeType::SetChatAudioOutputCompleted:return "SetChatAudioOutputCompleted"; break;
        case PartyStateChangeType::LocalChatAudioInputChanged:return "LocalChatAudioInputChanged"; break;
        case PartyStateChangeType::LocalChatAudioOutputChanged:return "LocalChatAudioOutputChanged"; break;
        case PartyStateChangeType::SetTextToSpeechProfileCompleted:return "SetTextToSpeechProfileCompleted"; break;
        case PartyStateChangeType::SynthesizeTextToSpeechCompleted:return "SynthesizeTextToSpeechCompleted"; break;
        case PartyStateChangeType::SetLanguageCompleted:return "SetLanguageCompleted"; break;
        case PartyStateChangeType::SetTranscriptionOptionsCompleted:return "SetTranscriptionOptionsCompleted"; break;
        case PartyStateChangeType::SetTextChatOptionsCompleted:return "SetTextChatOptionsCompleted"; break;
        case PartyStateChangeType::ChatControlPropertiesChanged:return "ChatControlPropertiesChanged"; break;
        case PartyStateChangeType::ChatControlJoinedNetwork:return "ChatControlJoinedNetwork"; break;
        case PartyStateChangeType::ChatControlLeftNetwork:return "ChatControlLeftNetwork"; break;
        case PartyStateChangeType::ConnectChatControlCompleted:return "ConnectChatControlCompleted"; break;
        case PartyStateChangeType::DisconnectChatControlCompleted:return "DisconnectChatControlCompleted"; break;
        case PartyStateChangeType::PopulateAvailableTextToSpeechProfilesCompleted:return "PopulateAvailableTextToSpeechProfilesCompleted"; break;
        case PartyStateChangeType::ConfigureAudioManipulationVoiceStreamCompleted:return "ConfigureAudioManipulationVoiceStreamCompleted"; break;
        case PartyStateChangeType::ConfigureAudioManipulationCaptureStreamCompleted:return "ConfigureAudioManipulationCaptureStreamCompleted"; break;
        case PartyStateChangeType::ConfigureAudioManipulationRenderStreamCompleted:return "ConfigureAudioManipulationRenderStreamCompleted"; break;
        }

        return "Unknown PartyStateChangeType";
    }

    void LogPartyStateChangeType(const PartyStateChange* change)
    {
        if (change)
        {
            DEBUGLOG("PartyStateChange: PartyStateChangeType::%s \n", GetPartyStateChangeTypeString(change->stateChangeType).c_str());
        }
        else
        {
            DEBUGLOG("PlayFabPartyManager::LogPartyStateChangeType: change was null \n");
        }
    }
}

PlayFabPartyManager::~PlayFabPartyManager()
{
    DEBUGLOG("PlayFabPartyManager::~PlayFabPartyManager()\n");
}

void PlayFabPartyManager::SetLanguageCode(const char* lang, const char* name)
{
    m_languageCode = lang;
    m_languageName = name;
}

void NetRumble::PlayFabPartyManager::SetTextFilteringLevel(PartyTextChatFilterLevel level)
{
    PartyError err = PartyManager::SetOption(nullptr, PartyOption::TextChatFilterLevel, &level);

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("Failed to set TextChatFilterLevel Option: %hs\n", GetErrorMessage(err));
    }
}

PartyTextChatFilterLevel PlayFabPartyManager::GetTextFilteringLevel()
{
    PartyTextChatFilterLevel value = PartyTextChatFilterLevel::FamilyFriendly;

    PartyError err = PartyManager::GetOption(nullptr, PartyOption::TextChatFilterLevel, &value);

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("Failed to get TextChatFilterLevel Option: %hs\n", GetErrorMessage(err));
    }

    return value;
}

void PlayFabPartyManager::Initialize()
{
    DEBUGLOG("PlayFabPartyManager::Initialize()\n");

    PartyManager& partyManager = PartyManager::GetSingleton();
    PartyError err;

    if (m_partyInitialized == false)
    {
        // Initialize PlayFab Party
        PartyInitializationConfiguration config = {};
        config.titleId = c_pfTitleId;

        err = partyManager.Initialize(&config);
        if (PARTY_FAILED(err))
        {
            DEBUGLOG("Initialize failed: %hs\n", GetErrorMessage(err));
            return;
        }

        m_partyInitialized = true;
    }

    if (m_partyXblInitialized == false)
    {
        // Initialize XboxLive Plugin
        err = PartyXblManager::GetSingleton().Initialize(c_pfTitleId);
        if (PARTY_FAILED(err))
        {
            DEBUGLOG("Initialize failed: %hs\n", GetXblErrorMessage(err));
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
        DEBUGLOG("CreateLocalUser with entityId %s\n", m_localEntityId.c_str());

        // Create a local user object
        err = partyManager.CreateLocalUser(
            reinterpret_cast<PFEntityHandle>(m_localChatUser),  // Entity handle
            &m_localUser                                // OUT local user object
        );

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("CreateLocalUser failed: %hs\n", GetErrorMessage(err));
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
            DEBUGLOG("GetLocalDevice failed: %hs\n", GetErrorMessage(err));
            return;
        }

        // Create a chat control for the local user on the local device
        err = localDevice->CreateChatControl(
            m_localUser,                                // Local user object
            m_languageCode.c_str(),                     // Language id
            nullptr,                                    // Async identifier
            &m_localChatControl                         // OUT local chat control
        );

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("CreateChatControl failed: %hs\n", GetErrorMessage(err));
            return;
        }

        uint64_t xuid = 0;
        m_localChatUser->GetXboxUserId(&xuid);
        auto xuidString = std::to_string(xuid);

        // Use automatic settings for the audio input device
        err = m_localChatControl->SetAudioInput(
            PartyAudioDeviceSelectionType::PlatformUserDefault, // Selection type
            xuidString.c_str(),                                 // Device id
            nullptr                                             // Async identifier
        );

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("SetAudioInput failed: %hs\n", GetErrorMessage(err));
            return;
        }

        // Use automatic settings for the audio output device
        err = m_localChatControl->SetAudioOutput(
            PartyAudioDeviceSelectionType::PlatformUserDefault, // Selection type
            xuidString.c_str(),                                 // Device id
            nullptr                                             // Async identifier
        );

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("SetAudioOutput failed: %hs\n", GetErrorMessage(err));
        }

        // Get the available list of text to speech profiles
        err = m_localChatControl->PopulateAvailableTextToSpeechProfiles(nullptr);

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("Populating available TextToSpeechProfiles failed: %s \n", GetErrorMessage(err));
        }
    }
}

void PlayFabPartyManager::SetTextChatAccessibilityOptions()
{
    DEBUGLOG("SetTextChatAccessibilityOptions()\n");

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
        DEBUGLOG("GetAccessibilitySettings failed: %s\n", GetErrorMessage(err));
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
            DEBUGLOG("DestroyLocalUser failed: %s\n", GetXblErrorMessage(err));
        }
    }

    if (m_localChatUser != nullptr)
    {
        err = PartyXblManager::GetSingleton().DestroyChatUser(m_localChatUser);

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("DestroyChatUser failed: %s\n", GetXblErrorMessage(err));
        }
    }

    m_localUser = nullptr;
    m_localChatUser = nullptr;
    m_userCreatedCallback = nullptr;
}

void PlayFabPartyManager::SetLocalUser(uint64_t xuid, std::function<void(PartyError)> callback)
{
    DEBUGLOG("PlayFabPartyManager::SetLocalUser(%lu)\n", xuid);

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
            DEBUGLOG("GetXboxUserId failed: %s\n", GetXblErrorMessage(err));
        }
    }

    err = PartyXblManager::GetSingleton().CreateLocalChatUser(
        xuid,
        nullptr,
        &m_localChatUser
        );

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("CreateLocalChatUser failed: %s\n", GetXblErrorMessage(err));
        callback(err);
        return;
    }

    err = PartyXblManager::GetSingleton().LoginToPlayFab(m_localChatUser, nullptr);
    if (PARTY_FAILED(err))
    {
        DEBUGLOG("LoginToPlayFab failed: %s\n", GetXblErrorMessage(err));
        callback(err);
        return;
    }

    m_userCreatedCallback = callback;
}

void PlayFabPartyManager::AddRemoteUser(uint64_t xuid, const char* entityId)
{
    DEBUGLOG("PlayFabPartyManager::AddRemoteUser(%llu, %s)\n", xuid, entityId);

    PartyXblChatUser* remoteChatUser;
    PartyError err = PartyXblManager::GetSingleton().CreateRemoteChatUser(xuid, &remoteChatUser);
    if (PARTY_SUCCEEDED(err))
    {
        m_xuidToEntityId[xuid] = entityId;
        m_entityIdToXuid[entityId] = xuid;

        // Look and see if we have an endpoint from this entityid in which case we can notify
        // the engine the user is ready

        uint32_t size = 0;
        PartyNetworkArray list = nullptr;

        // Get local networks
        err = PartyManager::GetSingleton().GetNetworks(&size, &list);

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("GetNetworks failed: %s\n", GetErrorMessage(err));
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
                DEBUGLOG("GetEndpoints failed: %s\n", GetErrorMessage(err));
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
                    std::string first(entityId), second(id);

                    // See if they are the same
                    if (first == second)
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
        DEBUGLOG("CreateRemoteChatUser failed: %s\n", GetErrorMessage(err));
    }
}

void PlayFabPartyManager::RemoveRemoteUser(uint64_t xuid)
{
    DEBUGLOG("PlayFabPartyManager::RemoveLocalUser(%llu)\n", xuid);

    uint32_t userCount = 0;
    PartyXblChatUserArray userList = nullptr;

    PartyError err = PartyXblManager::GetSingleton().GetChatUsers(&userCount, &userList);
    if (PARTY_FAILED(err))
    {
        DEBUGLOG("GetChatUsers failed: %s\n", GetErrorMessage(err));
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
                DEBUGLOG("DestroyChatUser failed: %s\n", GetErrorMessage(err));
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
    DEBUGLOG("PlayFabPartyManager::Shutdown()\n");

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

    return PartyInvitationConfiguration {
        networkId,                                  // invitation identifier
        PartyInvitationRevocability::Anyone,        // revokability
        0,                                          // authorized user count
        nullptr                                     // authorized user list
        };
}

PartyNetworkConfiguration PlayFabPartyManager::GetPartyNetworkConfiguration()
{
    PartyNetworkConfiguration cfg{};

    // Setup the network to allow 8 single-device players of any device type
    cfg.maxDeviceCount = 8;
    cfg.maxDevicesPerUserCount = 1;
    cfg.maxEndpointsPerDeviceCount = 1;
    cfg.maxUserCount = 8;
    cfg.maxUsersPerDeviceCount = 1;

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
    DEBUGLOG("PlayFabPartyManager::CreateAndConnectToNetwork()\n");

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
        DEBUGLOG("CreateNewNetwork failed: %hs\n", GetErrorMessage(err));
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
    DEBUGLOG("PlayFabPartyManager::ConnectToNetwork()\n");

    PartyNetworkDescriptor networkDescriptor = {};

    // Deserialize the remote network's descriptor
    PartyError err = PartyManager::DeserializeNetworkDescriptor(descriptor, &networkDescriptor);

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("ConnectToNetwork failed to deserialize descriptor: %hs\n", GetErrorMessage(err));
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

    PartyError err = PartyManager::GetSingleton().ConnectToNetwork(
        &descriptor,                                // Network descriptor
        nullptr,                                    // Async identifier
        network                                     // OUT network
        );

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("ConnectToNetwork failed: %hs\n", GetErrorMessage(err));
        return false;
    }

    // Authenticate the local user on the network so we can participate in it
    err = (*network)->AuthenticateLocalUser(
        m_localUser,                                // Local user
        networkId,                                  // Invite value
        nullptr                                     // Async identifier
        );

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("AuthenticateLocalUser failed: %hs\n", GetErrorMessage(err));
        return false;
    }

    CreateLocalChatControl();

    // Connect the local user chat control to the network so we can use VOIP
    err = (*network)->ConnectChatControl(
        m_localChatControl,                         // Local chat control
        nullptr                                     // Async identifier
        );

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("ConnectChatControl failed: %hs\n", GetErrorMessage(err));
        return false;
    }

    // Establish a network endpoint for game message traffic
    err = (*network)->CreateEndpoint(
        m_localUser,                                // Local user
        0,                                          // Property Count
        nullptr,                                    // Property name keys
        nullptr,                                    // Property Values
        nullptr,                                    // Async identifier
        endpoint                                    // OUT local endpoint
        );

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("Failed to CreateEndpoint: %hs\n", GetErrorMessage(err));
        return false;
    }

    return true;
}

void PlayFabPartyManager::SendGameMessage(const GameMessage & message)
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
        switch (message.MessageType())
        {
            case GameMessageType::ShipInput:
            case GameMessageType::ShipData:
                deliveryOptions = PartySendMessageOptions::Default;
                break;

            default:
                deliveryOptions = PartySendMessageOptions::GuaranteedDelivery |
                                  PartySendMessageOptions::SequentialDelivery;
        }

        // Send out the message to all other peers
        PartyError err = m_localEndpoint->SendMessage(
            0,                                      // endpoint count; 0 = broadcast
            nullptr,                                // endpoint list
            deliveryOptions,                        // send message options
            nullptr,                                // configuration
            1,                                      // buffer count
            data,                                   // buffer
            nullptr                                 // async identifier
            );

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("Failed to SendMessage: %hs\n", GetErrorMessage(err));
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
        DEBUGLOG("Send text message: %hs\n", text.c_str());

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

        PartyError err = m_localChatControl->SendText(
            static_cast<uint32_t>(targets.size()),  // Count of target controls
            targets.data(),                         // Target controls
            text.c_str(),                           // Text to synthesize
            0,                                      // Data buffer size
            nullptr                                 // Data buffer
            );

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("Failed to SendText: %hs\n", GetErrorMessage(err));
        }

        // Toast the text on the screen
        Managers::Get<ScreenManager>()->GetSTTWindow()->AddSTTString(
            DisplayNameFromChatControl(m_localChatControl),
            text.c_str(),
            false
            );
    }
}

void PlayFabPartyManager::SendTextAsVoice(std::string text)
{
    if (m_localChatControl != nullptr)
    {
        if (m_enableCognitiveServices)
        {
            DEBUGLOG("Requesting synthesis of: %hs\n", text.c_str());

            PartyError err = m_localChatControl->SynthesizeTextToSpeech(
                PartySynthesizeTextToSpeechType::VoiceChat,
                text.c_str(),                           // Text to synthesize
                nullptr                                 // Async identifier
            );

            if (PARTY_FAILED(err))
            {
                DEBUGLOG("Failed to SynthesizeTextToSpeech: %hs\n", GetErrorMessage(err));
            }
        }
    }
}

void PlayFabPartyManager::LeaveNetwork(std::function<void(void)> callback)
{
    DEBUGLOG("PlayFabPartyManager::LeaveNetwork()\n");

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
            DEBUGLOG("GetLocalDevice failed: %hs\n", GetErrorMessage(err));
            m_network->LeaveNetwork(nullptr);
            return;
        }

        err = localDevice->DestroyChatControl(m_localChatControl, nullptr);

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("DestroyChatControl failed: %hs\n", GetErrorMessage(err));
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
    DEBUGLOG("PlayFabPartyManager::MigrateToNewNetwork()\n");

    if (regionList.size() == 0)
    {
        DEBUGLOG("No regions to migrate to.\n");
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
        DEBUGLOG("CreateNewNetwork failed: %hs\n", GetErrorMessage(err));
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
    DEBUGLOG("PlayFabPartyManager::MigrateToNetwork()\n");

    PartyNetworkDescriptor networkDescriptor = {};

    // Deserialize the remote network's descriptor
    PartyError err = PartyManager::DeserializeNetworkDescriptor(descriptor, &networkDescriptor);

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("MigrateToNetwork failed to deserialize descriptor: %hs\n", GetErrorMessage(err));
        return;
    }

    // Connect to the remote network
    if (InternalConnectToNetwork(m_networkId.c_str(), networkDescriptor, &m_newNetwork, &m_newLocalEndpoint))
    {
        m_state = NetworkManagerState::Migrating;
        m_onNetworkMigrated = callback;
    }
}

uint64_t PlayFabPartyManager::GetXuidFromEntityId(const char* entityId)
{
    auto it = m_entityIdToXuid.find(entityId);
    if (it != m_entityIdToXuid.end())
    {
        return it->second;
    }

    return 0;
}

const char* PlayFabPartyManager::GetEntityIdFromXuid(uint64_t xuid)
{
    auto it = m_xuidToEntityId.find(xuid);
    if (it != m_xuidToEntityId.end())
    {
        return it->second.c_str();
    }

    return nullptr;
}

void PlayFabPartyManager::DoWork()
{
    // Check for entity token refresh
    TryEntityTokenRefresh();

    uint32_t count;
    PartyXblStateChangeArray xblChanges;

    // Process Xbl messages
    PartyError err = PartyXblManager::GetSingleton().StartProcessingStateChanges(
        &count,
        &xblChanges
        );

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("StartProcessingStateChanges failed: %hs\n", GetXblErrorMessage(err));
        return;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        const PartyXblStateChange* change = xblChanges[i];
        if (change)
        {
            switch (change->stateChangeType)
            {
            case PartyXblStateChangeType::CreateLocalChatUserCompleted: OnCreateLocalChatUserCompleted(change); break;
            case PartyXblStateChangeType::LocalChatUserDestroyed: OnLocalChatUserDestroyed(change); break;
            case PartyXblStateChangeType::LoginToPlayFabCompleted: OnLoginToPlayFabCompleted(change); break;
            case PartyXblStateChangeType::RequiredChatPermissionInfoChanged: OnRequiredChatPermissionInfoChanged(change); break;
            case PartyXblStateChangeType::TokenAndSignatureRequested: OnTokenAndSignatureRequested(change); break;
            }
        }
    }

    err = PartyXblManager::GetSingleton().FinishProcessingStateChanges(count, xblChanges);

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("FinishProcessingStateChanges failed: %hs\n", GetXblErrorMessage(err));
        return;
    }

    PartyStateChangeArray changes;

    // Start processing messages from PlayFab Party
    err = PartyManager::GetSingleton().StartProcessingStateChanges(
        &count,
        &changes
        );

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("StartProcessingStateChanges failed: %hs\n", GetErrorMessage(err));
        return;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        const PartyStateChange* change = changes[i];
        if (change)
        {
            switch (change->stateChangeType)
            {
            case PartyStateChangeType::RegionsChanged: OnRegionsChanged(change); break;
            case PartyStateChangeType::DestroyLocalUserCompleted: OnDestroyLocalUserCompleted(change); break;
            case PartyStateChangeType::CreateNewNetworkCompleted: OnCreateNewNetworkCompleted(change); break;
            case PartyStateChangeType::ConnectToNetworkCompleted: OnConnectToNetworkCompleted(change); break;
            case PartyStateChangeType::AuthenticateLocalUserCompleted: OnAuthenticateLocalUserCompleted(change); break;
            case PartyStateChangeType::NetworkConfigurationMadeAvailable: OnNetworkConfigurationMadeAvailable(change); break;
            case PartyStateChangeType::NetworkDescriptorChanged: OnNetworkDescriptorChanged(change); break;
            case PartyStateChangeType::LocalUserRemoved: OnLocalUserRemoved(change); break;
            case PartyStateChangeType::RemoveLocalUserCompleted: OnRemoveLocalUserCompleted(change); break;
            case PartyStateChangeType::LocalUserKicked: OnLocalUserKicked(change); break;
            case PartyStateChangeType::CreateEndpointCompleted: OnCreateEndpointCompleted(change); break;
            case PartyStateChangeType::DestroyEndpointCompleted: OnDestroyEndpointCompleted(change); break;
            case PartyStateChangeType::EndpointCreated: OnEndpointCreated(change); break;
            case PartyStateChangeType::EndpointDestroyed: OnEndpointDestroyed(change); break;
            case PartyStateChangeType::RemoteDeviceCreated: OnRemoteDeviceCreated(change); break;
            case PartyStateChangeType::RemoteDeviceDestroyed: OnRemoteDeviceDestroyed(change); break;
            case PartyStateChangeType::RemoteDeviceJoinedNetwork: OnRemoteDeviceJoinedNetwork(change); break;
            case PartyStateChangeType::RemoteDeviceLeftNetwork: OnRemoteDeviceLeftNetwork(change); break;
            case PartyStateChangeType::DevicePropertiesChanged: OnDevicePropertiesChanged(change); break;
            case PartyStateChangeType::LeaveNetworkCompleted: OnLeaveNetworkCompleted(change); break;
            case PartyStateChangeType::NetworkDestroyed: OnNetworkDestroyed(change); break;
            case PartyStateChangeType::EndpointMessageReceived: OnEndpointMessageReceived(change); break;
            case PartyStateChangeType::DataBuffersReturned: OnDataBuffersReturned(change); break;
            case PartyStateChangeType::EndpointPropertiesChanged: OnEndpointPropertiesChanged(change); break;
            case PartyStateChangeType::SynchronizeMessagesBetweenEndpointsCompleted: OnSynchronizeMessagesBetweenEndpointsCompleted(change); break;
            case PartyStateChangeType::CreateInvitationCompleted: OnCreateInvitationCompleted(change); break;
            case PartyStateChangeType::RevokeInvitationCompleted: OnRevokeInvitationCompleted(change); break;
            case PartyStateChangeType::InvitationCreated: OnInvitationCreated(change); break;
            case PartyStateChangeType::InvitationDestroyed: OnInvitationDestroyed(change); break;
            case PartyStateChangeType::NetworkPropertiesChanged: OnNetworkPropertiesChanged(change); break;
            case PartyStateChangeType::KickDeviceCompleted: OnKickDeviceCompleted(change); break;
            case PartyStateChangeType::KickUserCompleted: OnKickUserCompleted(change); break;
            case PartyStateChangeType::CreateChatControlCompleted: OnCreateChatControlCompleted(change); break;
            case PartyStateChangeType::DestroyChatControlCompleted: OnDestroyChatControlCompleted(change); break;
            case PartyStateChangeType::ChatControlCreated: OnChatControlCreated(change); break;
            case PartyStateChangeType::ChatControlDestroyed: OnChatControlDestroyed(change); break;
            case PartyStateChangeType::SetChatAudioEncoderBitrateCompleted: OnSetChatAudioEncoderBitrateCompleted(change); break;
            case PartyStateChangeType::ChatTextReceived: OnChatTextReceived(change); break;
            case PartyStateChangeType::VoiceChatTranscriptionReceived: OnVoiceChatTranscriptionReceived(change); break;
            case PartyStateChangeType::SetChatAudioInputCompleted: OnSetChatAudioInputCompleted(change); break;
            case PartyStateChangeType::SetChatAudioOutputCompleted: OnSetChatAudioOutputCompleted(change); break;
            case PartyStateChangeType::LocalChatAudioInputChanged: OnLocalChatAudioInputChanged(change); break;
            case PartyStateChangeType::LocalChatAudioOutputChanged: OnLocalChatAudioOutputChanged(change); break;
            case PartyStateChangeType::SetTextToSpeechProfileCompleted: OnSetTextToSpeechProfileCompleted(change); break;
            case PartyStateChangeType::SynthesizeTextToSpeechCompleted: OnSynthesizeTextToSpeechCompleted(change); break;
            case PartyStateChangeType::SetLanguageCompleted: OnSetLanguageCompleted(change); break;
            case PartyStateChangeType::SetTranscriptionOptionsCompleted: OnSetTranscriptionOptionsCompleted(change); break;
            case PartyStateChangeType::SetTextChatOptionsCompleted: OnSetTextChatOptionsCompleted(change); break;
            case PartyStateChangeType::ChatControlPropertiesChanged: OnChatControlPropertiesChanged(change); break;
            case PartyStateChangeType::ChatControlJoinedNetwork: OnChatControlJoinedNetwork(change); break;
            case PartyStateChangeType::ChatControlLeftNetwork: OnChatControlLeftNetwork(change); break;
            case PartyStateChangeType::ConnectChatControlCompleted: OnConnectChatControlCompleted(change); break;
            case PartyStateChangeType::DisconnectChatControlCompleted: OnDisconnectChatControlCompleted(change); break;
            case PartyStateChangeType::PopulateAvailableTextToSpeechProfilesCompleted: OnPopulateAvailableTextToSpeechProfilesCompleted(change); break;
            case PartyStateChangeType::ConfigureAudioManipulationVoiceStreamCompleted: OnConfigureAudioManipulationVoiceStreamCompleted(change); break;
            case PartyStateChangeType::ConfigureAudioManipulationCaptureStreamCompleted: OnConfigureAudioManipulationCaptureStreamCompleted(change); break;
            case PartyStateChangeType::ConfigureAudioManipulationRenderStreamCompleted: OnConfigureAudioManipulationRenderStreamCompleted(change); break;
            }
        }
    }

    // Return the processed changes back to the PartyManager
    err = PartyManager::GetSingleton().FinishProcessingStateChanges(count, changes);

    if (PARTY_FAILED(err))
    {
        DEBUGLOG("FinishProcessingStateChanges failed: %hs\n", GetErrorMessage(err));
    }
}

void PlayFabPartyManager::PopulatePartyRegionLatencies(bool send)
{
    uint32_t regionCount;
    const PartyRegion* regionList;

    PartyError err = PartyManager::GetSingleton().GetRegions(&regionCount, &regionList);

    if (PARTY_SUCCEEDED(err))
    {
        DEBUGLOG("Populating Party Regions (%lu)\n", regionCount);

        std::string uiString;

        uiString.reserve(1024);
        uiString = "Party Regions:\n";

        for (uint32_t x = 0; x < regionCount; x++)
        {
            uiString += regionList[x].regionName;
            uiString += ":  ";
            uiString += std::to_string(regionList[x].roundTripLatencyInMilliseconds);
            uiString += " ms\n";

            DEBUGLOG("%20hs:  %lu ms\n", regionList[x].regionName, regionList[x].roundTripLatencyInMilliseconds);

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
            g_game->GetLocalPlayerState()->SetRegionLatency(
                regionList[x].regionName,
                regionList[x].roundTripLatencyInMilliseconds);
        }
    }
    else
    {
        DEBUGLOG("GetRegions() failed with %hs\n", GetErrorMessage(err));
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
            DEBUGLOG("GetEndpoints() failed with %hs\n", GetErrorMessage(err));
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
                DEBUGLOG("GetLocal() failed with %hs\n", GetErrorMessage(err));
                return userMap;
            }

            if (local == nullptr)
            {
                remoteEndpoints.push_back(endpointArray[count]);
            }
        }

        if (remoteEndpoints.size() == 0)
        {
            return userMap;
        }

        // Get the rount trip latency value for all remote endpoints
        auto statValues = std::vector<uint64_t>(remoteEndpoints.size());
        auto statType = PartyEndpointStatistic::AverageDeviceRoundTripLatencyInMilliseconds;

        err = m_localEndpoint->GetEndpointStatistics(
            static_cast<uint32_t>(remoteEndpoints.size()),
            remoteEndpoints.data(),
            1,
            &statType,
            statValues.data()
            );

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("GetEndpointStatistics() failed with %hs\n", GetErrorMessage(err));
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
                DEBUGLOG("GetEntityId failed: %hs\n", GetErrorMessage(err));
                return userMap;
            }

            userMap[GetXuidFromEntityId(remote)] = statValues[count];
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
            DEBUGLOG("GetNetworkStatistics failed: %hs\n", GetErrorMessage(err));
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
            DEBUGLOG("GetNetworkStatistics failed: %hs\n", GetErrorMessage(err));
        }
    }

    return returnValue;
}

void PlayFabPartyManager::OnRegionsChanged(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyRegionsChangedStateChange* result = static_cast<const PartyRegionsChangedStateChange*>(change);
    if (result)
    {
        LogResult(result);
    }
}

void PlayFabPartyManager::OnDestroyLocalUserCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnCreateNewNetworkCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyCreateNewNetworkCompletedStateChange* result = static_cast<const PartyCreateNewNetworkCompletedStateChange*>(change);
    if(result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("CreateNewNetworkCompleted:  SUCCESS\n");
            PartyString entityId;
            result->localUser->GetEntityId(&entityId);
            DEBUGLOG("CreateNewNetworkCompleted:  EntityId: %s\n", entityId);
        }
        else
        {
            DEBUGLOG("CreateNewNetworkCompleted:  FAIL:  %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
            DEBUGLOG("ErrorDetail: %hs\n", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnConnectToNetworkCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyConnectToNetworkCompletedStateChange* result = static_cast<const PartyConnectToNetworkCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("ConnectToNetworkCompleted:  SUCCESS\n");
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
                    PartyError err = PartyManager::SerializeNetworkDescriptor(
                        &result->networkDescriptor,
                        descriptor
                        );

                    if (PARTY_FAILED(err))
                    {
                        DEBUGLOG("Failed to serialize network descriptor: %hs\n", GetErrorMessage(err));
                        m_onNetworkCreated(std::string());
                    }

                    DEBUGLOG("Serialized value: %hs\n", descriptor);
                    // Callback with the descriptor to be shared with connecting clients
                    m_onNetworkCreated(std::string(descriptor));
                }
            }
        }
        else
        {
            DEBUGLOG("ConnectToNetworkCompleted:  FAIL:  %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
            DEBUGLOG("ErrorDetail: %hs\n", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnAuthenticateLocalUserCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyAuthenticateLocalUserCompletedStateChange* result = static_cast<const PartyAuthenticateLocalUserCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Succeeded\n");
        }
        else
        {
            DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
            DEBUGLOG("ErrorDetail: %hs\n", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnNetworkConfigurationMadeAvailable(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnNetworkDescriptorChanged(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnLocalUserRemoved(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    if (m_state != NetworkManagerState::Leaving && m_state != NetworkManagerState::Migrating)
    {
        DEBUGLOG("Unexpected local user removal!\n");
    }
}

void PlayFabPartyManager::OnRemoveLocalUserCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnLocalUserKicked(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnCreateEndpointCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyCreateEndpointCompletedStateChange* result = static_cast<const PartyCreateEndpointCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("CreateEndpointCompleted:  SUCCESS\n");

            if (m_state == NetworkManagerState::Migrating)
            {
                // We can leave our old network now
                m_network->LeaveNetwork(nullptr);
            }
        }
        else
        {
            DEBUGLOG("CreateEndpointCompleted:  FAIL:  %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
            DEBUGLOG("ErrorDetail: %hs\n", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnDestroyEndpointCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnEndpointCreated(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyEndpointCreatedStateChange* result = static_cast<const PartyEndpointCreatedStateChange*>(change);
    if (result && m_state != NetworkManagerState::Migrating)
    {
        PartyString user = nullptr;
        PartyError err = result->endpoint->GetEntityId(&user);

        if (PARTY_FAILED(err) || user == nullptr)
        {
            DEBUGLOG("Unable to retrieve user id from endpoint: %hs\n", GetErrorMessage(err));
        }
        else
        {
            DEBUGLOG("Established endpoint with user %s\n", user);

            uint64_t xuid = GetXuidFromEntityId(user);

            if (xuid != 0)
            {
                // Tell the engine a user is ready to receive data
                m_onEndpointChanged(xuid, true);
            }
            else
            {
                DEBUGLOG("No Xuid for entity %s\n", user);
            }
        }
    }
}

void PlayFabPartyManager::OnEndpointDestroyed(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyEndpointDestroyedStateChange* result = static_cast<const PartyEndpointDestroyedStateChange*>(change);
    if (result)
    {
        DEBUGLOG("Endpoint is %hs\n", result->endpoint == m_localEndpoint ? "local" : "remote");
        DEBUGLOG("Reason: %d\n", result->reason);
        DEBUGLOG("Error Detail: %hs\n", GetErrorMessage(result->errorDetail));

        if (result->endpoint == m_localEndpoint)
        {
            // Our endpoint was disconnected
            m_localEndpoint = nullptr;

            if (Managers::Get<GameStateManager>()->GetState() == GameState::MigratingNetwork)
            {
                DEBUGLOG("Migrating: Old endpoint destroyed, swapping to new\n");
                m_localEndpoint = m_newLocalEndpoint;
                m_newLocalEndpoint = nullptr;
            }
        }
        else
        {
            // Another user has disconnected
            PartyString user = nullptr;
            PartyError err = result->endpoint->GetEntityId(&user);

            if (PARTY_FAILED(err))
            {
                DEBUGLOG("Unable to retrieve user id from endpoint: %hs\n", GetErrorMessage(err));
                return;
            }

            uint64_t xuid = GetXuidFromEntityId(user);

            if (xuid != 0)
            {
                // Tell the engine a user has disconnected
                m_onEndpointChanged(xuid, false);
            }
            else
            {
                DEBUGLOG("No Xuid for entity %s\n", user);
            }
        }
    }
}

void PlayFabPartyManager::OnRemoteDeviceCreated(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnRemoteDeviceDestroyed(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnRemoteDeviceJoinedNetwork(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnRemoteDeviceLeftNetwork(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnDevicePropertiesChanged(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnLeaveNetworkCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    if (m_state == NetworkManagerState::Migrating)
    {
        m_state = NetworkManagerState::NetworkConnected;

        if (Managers::Get<OnlineManager>()->IsHost())
        {
            PartyNetworkDescriptor descriptor{};

            PartyError err = m_network->GetNetworkDescriptor(&descriptor);

            if (PARTY_FAILED(err))
            {
                DEBUGLOG("Failed to get migrating network descriptor: %hs\n", GetErrorMessage(err));
                if (m_onRegionMigrated)
                {
                    m_onRegionMigrated(false, nullptr);
                }
                return;
            }

            char descriptorString[c_maxSerializedNetworkDescriptorStringLength + 1] = {};

            // Serialize our local network descriptor for other peers to use
            err = PartyManager::SerializeNetworkDescriptor(
                &descriptor,
                descriptorString
                );

            if (PARTY_FAILED(err))
            {
                DEBUGLOG("Failed to serialize network descriptor: %hs\n", GetErrorMessage(err));
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
    LogPartyStateChangeType(change);

    m_network = nullptr;

    if (m_state == NetworkManagerState::Migrating)
    {
        DEBUGLOG("Migrating: Old network destroyed, swapping to new\n");
        m_network = m_newNetwork;
        m_newNetwork = nullptr;
    }
    else if (m_state != NetworkManagerState::Leaving)
    {
        DEBUGLOG("Unexpected network destruction!\n");
        m_onMessageReceived(0, std::make_shared<GameMessage>(GameMessageType::NetworkLost, 0));
    }
}

void PlayFabPartyManager::OnEndpointMessageReceived(const PartyStateChange* change)
{
    // This is spammy, but can be useful when debugging
    //LogPartyStateChangeType(change);

    const PartyEndpointMessageReceivedStateChange* result = static_cast<const PartyEndpointMessageReceivedStateChange*>(change);
    if (result)
    {
        const uint8_t* buffer = static_cast<const uint8_t*>(result->messageBuffer);
        std::shared_ptr<GameMessage> packet = std::make_shared<GameMessage>(
            std::vector<uint8_t>(buffer, buffer + result->messageSize)
            );

        PartyString sender = nullptr;
        PartyError err = result->senderEndpoint->GetEntityId(&sender);

        if (PARTY_SUCCEEDED(err))
        {
            // Give the message to the game engine
            if (m_onMessageReceived)
            {
                uint64_t userXuid = GetXuidFromEntityId(sender);
                if (userXuid != 0)
                {
                    m_onMessageReceived(userXuid, packet);
                }
                else
                {
                    DEBUGLOG("Message '%s' received from entityid %s but we don't know their xuid yet.\n", MessageTypeString(packet->MessageType()), sender);
                }
            }
        }
        else
        {
            DEBUGLOG("GetEntityId failed: %hs\n", GetErrorMessage(err));
        }
    }
}

void PlayFabPartyManager::OnDataBuffersReturned(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnEndpointPropertiesChanged(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnSynchronizeMessagesBetweenEndpointsCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnCreateInvitationCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnRevokeInvitationCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnInvitationCreated(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnInvitationDestroyed(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnNetworkPropertiesChanged(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnKickDeviceCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnKickUserCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnCreateChatControlCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyCreateChatControlCompletedStateChange* result = static_cast<const PartyCreateChatControlCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Succeeded\n");
            SetTextChatAccessibilityOptions();
        }
        else
        {
            DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
            DEBUGLOG("Error detail: %hs\n", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnDestroyChatControlCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyDestroyChatControlCompletedStateChange* result = static_cast<const PartyDestroyChatControlCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Succeeded\n");
        }
        else
        {
            DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
            DEBUGLOG("Error detail: %hs\n", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnChatControlCreated(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyChatControlCreatedStateChange* result = static_cast<const PartyChatControlCreatedStateChange*>(change);
    if (result)
    {
        PartyString sender = nullptr;
        PartyError err = result->chatControl->GetEntityId(&sender);

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("GetEntityId failed: %hs\n", GetErrorMessage(err));
        }
        else
        {
            DEBUGLOG("Created ChatControl for %hs\n", sender);
            m_chatControls[sender] = result->chatControl;

            PartyLocalChatControl* local = nullptr;
            err = result->chatControl->GetLocal(&local);

            if (PARTY_FAILED(err))
            {
                DEBUGLOG("Failed to get LocalChatControl: %hs\n", GetErrorMessage(err));
            }
            else if (local == nullptr)
            {
                DEBUGLOG("ChatControl is remote\n");

                // Remote ChatControl added, set chat permissions
                err = m_localChatControl->SetPermissions(
                    result->chatControl,
                    PartyChatPermissionOptions::ReceiveAudio |
                    PartyChatPermissionOptions::ReceiveText |
                    PartyChatPermissionOptions::SendAudio
                );

                if (PARTY_FAILED(err))
                {
                    DEBUGLOG("Failed to SetPermissions on ChatControl: %hs\n", GetErrorMessage(err));
                }
            }
        }
    }
}

void PlayFabPartyManager::OnChatControlDestroyed(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyChatControlDestroyedStateChange* result = static_cast<const PartyChatControlDestroyedStateChange*>(change);
    if (result)
    {
        PartyString sender = nullptr;
        PartyError err = result->chatControl->GetEntityId(&sender);

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("GetEntityId failed: %hs\n", GetErrorMessage(err));
        }
        else
        {
            DEBUGLOG("Destroyed ChatControl from %hs\n", sender);

            if (result->chatControl == m_localChatControl)
            {
                DEBUGLOG("Local ChatControl destroyed\n");
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

void PlayFabPartyManager::OnSetChatAudioEncoderBitrateCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnChatTextReceived(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyChatTextReceivedStateChange* result = static_cast<const PartyChatTextReceivedStateChange*>(change);
    if (result)
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
                DEBUGLOG("Translation failed: %hs\n", PartyStateChangeResultToReasonString(result->translations[0].result).c_str());
            }
        }

        if (message.empty())
        {
            message = result->chatText;
        }

        Managers::Get<ScreenManager>()->GetSTTWindow()->AddSTTString(
            DisplayNameFromChatControl(result->senderChatControl),
            message,
            false
            );

        DEBUGLOG("Chat Text: %hs\n", message.c_str());
    }
}

void PlayFabPartyManager::OnVoiceChatTranscriptionReceived(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyVoiceChatTranscriptionReceivedStateChange* result = static_cast<const PartyVoiceChatTranscriptionReceivedStateChange*>(change);
    if (result)
    {
        if (PARTY_FAILED(result->errorDetail))
        {
            DEBUGLOG("Error Detail: %hs\n", GetErrorMessage(result->errorDetail));
        }

        if (result->result != PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
        }
        else if (result->transcription == nullptr)
        {
            DEBUGLOG("Transcription is null\n");
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
                    DEBUGLOG("Translation failed: %hs\n", PartyStateChangeResultToReasonString(result->translations[0].result).c_str());
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

            DEBUGLOG("Chat Transcription: %hs\n", message.c_str());
        }
    }
}

void PlayFabPartyManager::OnSetChatAudioInputCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartySetChatAudioInputCompletedStateChange* result = static_cast<const PartySetChatAudioInputCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Succeeded\n");
        }
        else
        {
            DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
            Managers::Get<ScreenManager>()->ShowError("Voice chat failed");
        }
    }
}

void PlayFabPartyManager::OnSetChatAudioOutputCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartySetChatAudioOutputCompletedStateChange* result = static_cast<const PartySetChatAudioOutputCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Succeeded\n");
        }
        else
        {
            DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
            DEBUGLOG("ErrorDetail: %hs\n", GetErrorMessage(result->errorDetail));
            Managers::Get<ScreenManager>()->ShowError("Voice chat failed");
        }
    }
}

void PlayFabPartyManager::OnLocalChatAudioInputChanged(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyLocalChatAudioInputChangedStateChange* result = static_cast<const PartyLocalChatAudioInputChangedStateChange*>(change);
    if (result)
    {
        if (PARTY_FAILED(result->errorDetail))
        {
            DEBUGLOG("Error Detail: %hs\n", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnLocalChatAudioOutputChanged(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyLocalChatAudioOutputChangedStateChange* result = static_cast<const PartyLocalChatAudioOutputChangedStateChange*>(change);
    if (result)
    {
        if (PARTY_FAILED(result->errorDetail))
        {
            DEBUGLOG("Error Detail: %hs\n", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnSetTextToSpeechProfileCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartySetTextToSpeechProfileCompletedStateChange* result = static_cast<const PartySetTextToSpeechProfileCompletedStateChange*>(change);
    if (result)
    {
        LogResult(result);
    }
}

void PlayFabPartyManager::OnSynthesizeTextToSpeechCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartySynthesizeTextToSpeechCompletedStateChange* result = static_cast<const PartySynthesizeTextToSpeechCompletedStateChange*>(change);
    if (result)
    {
        LogResult(result);
    }
}

void PlayFabPartyManager::OnSetLanguageCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnSetTranscriptionOptionsCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartySetTranscriptionOptionsCompletedStateChange* result = static_cast<const PartySetTranscriptionOptionsCompletedStateChange*>(change);
    if (result)
    {
        LogResult(result);
    }
}

void PlayFabPartyManager::OnSetTextChatOptionsCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartySetTextChatOptionsCompletedStateChange* result = static_cast<const PartySetTextChatOptionsCompletedStateChange*>(change);
    if (result)
    {
        LogResult(result);
    }
}

void PlayFabPartyManager::OnChatControlPropertiesChanged(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnChatControlJoinedNetwork(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnChatControlLeftNetwork(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnConnectChatControlCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyConnectChatControlCompletedStateChange* result = static_cast<const PartyConnectChatControlCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            DEBUGLOG("Succeeded\n");
        }
        else
        {
            DEBUGLOG("Failed: %hs\n", PartyStateChangeResultToReasonString(result->result).c_str());
            DEBUGLOG("Error detail: %hs\n", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnDisconnectChatControlCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnPopulateAvailableTextToSpeechProfilesCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    const PartyPopulateAvailableTextToSpeechProfilesCompletedStateChange* result = static_cast<const PartyPopulateAvailableTextToSpeechProfilesCompletedStateChange*>(change);
    if (result)
    {
        uint32_t profileCount = 0;
        PartyTextToSpeechProfileArray profileList = nullptr;

        // Get the profile list
        PartyError err = result->localChatControl->GetAvailableTextToSpeechProfiles(
            &profileCount,
            &profileList
        );

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("GetAvailableTextToSpeechProfiles failed: %s\n", GetErrorMessage(err));
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
                DEBUGLOG("GetLanguageCode failed: %s\n", GetErrorMessage(err));
                continue;
            }

            if (m_languageCode == languageCode)
            {
                // Get the profile name
                PartyString profileName = nullptr;
                err = profile->GetIdentifier(&profileName);

                if (PARTY_FAILED(err))
                {
                    DEBUGLOG("GetIdentifier failed: %s\n", GetErrorMessage(err));
                }

                DEBUGLOG("Setting TTS profile to: %s\n", profileName);

                // Set the profile to the first we find for our language
                // Ideally, the user would be able to select one
                err = result->localChatControl->SetTextToSpeechProfile(
                    PartySynthesizeTextToSpeechType::VoiceChat,
                    profileName,
                    nullptr
                );

                if (PARTY_FAILED(err))
                {
                    DEBUGLOG("SetTextToSpeechProfile to '%s' failed: %s\n", profileName, GetErrorMessage(err));
                }

                break;
            }
        }
    }
}

void PlayFabPartyManager::OnConfigureAudioManipulationVoiceStreamCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnConfigureAudioManipulationCaptureStreamCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

void PlayFabPartyManager::OnConfigureAudioManipulationRenderStreamCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);
}

std::string PlayFabPartyManager::DisplayNameFromChatControl(PartyChatControl* control)
{
    std::string sttuser;
    PartyString sender = nullptr;

    PartyError err = control->GetEntityId(&sender);
    if (PARTY_FAILED(err) || sender == nullptr)
    {
        DEBUGLOG("GetEntityId failed: %hs\n", GetErrorMessage(err));
        sttuser = "[ERROR]";
    }
    else
    {
        uint64_t xuid = GetXuidFromEntityId(sender);

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

void PlayFabPartyManager::OnCreateLocalChatUserCompleted(const PartyXblStateChange* change)
{
    const PartyXblCreateLocalChatUserCompletedStateChange* result = static_cast<const PartyXblCreateLocalChatUserCompletedStateChange*>(change);
    if (result)
    {
        DEBUGLOG("PartyXblStateChangeType: CreateLocalChatUserCompleted\n");
        m_localUserReady = true;
    }
}

void PlayFabPartyManager::OnLocalChatUserDestroyed(const PartyXblStateChange* change)
{
    const PartyXblLocalChatUserDestroyedStateChange* result = static_cast<const PartyXblLocalChatUserDestroyedStateChange*>(change);
    if (result)
    {
        DEBUGLOG("PartyXblStateChangeType: LocalChatUserDestroyed\n");
    }
}

void PlayFabPartyManager::OnLoginToPlayFabCompleted(const PartyXblStateChange* change)
{
    DEBUGLOG("PartyXblStateChangeType: LoginToPlayFabCompleted\n");
    const PartyXblLoginToPlayFabCompletedStateChange* result = static_cast<const PartyXblLoginToPlayFabCompletedStateChange*>(change);
    if (result)
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
                    DEBUGLOG("UpdateEntityToken failed: %s\n", GetErrorMessage(err));
                }
            }

            uint64_t xuid = 0;
            m_localChatUser->GetXboxUserId(&xuid);

            m_xuidToEntityId[xuid] = m_localEntityId;
            m_entityIdToXuid[m_localEntityId] = xuid;

            DEBUGLOG("LocalUser EntityId: %s\n", m_localEntityId.c_str());
            DEBUGLOG("LocalUser EntityToken: %s\n", m_localEntityToken.c_str());
            DEBUGLOG("LocalUser EntityTokenExpirationTime: %d\n", m_localEntityTokenExpirationTime);

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

void PlayFabPartyManager::OnRequiredChatPermissionInfoChanged(const PartyXblStateChange* change)
{
    DEBUGLOG("PartyXblStateChangeType: RequiredChatPermissionInfoChanged\n");
    const PartyXblRequiredChatPermissionInfoChangedStateChange* result = static_cast<const PartyXblRequiredChatPermissionInfoChangedStateChange*>(change);
    if (result)
    {
        PartyXblLocalChatUser* localChatUser = result->localChatUser;
        PartyXblChatUser* targetChatUser = result->targetChatUser;

        PartyXblChatPermissionInfo chatPermissionInfo;
        PartyError err = localChatUser->GetRequiredChatPermissionInfo(targetChatUser, &chatPermissionInfo);
        if (PARTY_FAILED(err))
        {
            DEBUGLOG("GetRequiredChatPermissionInfo failed: %s\n", GetXblErrorMessage(err));
            return;
        }

        uint64_t userXuid = 0;
        err = localChatUser->GetXboxUserId(&userXuid);
        if (PARTY_FAILED(err))
        {
            DEBUGLOG("GetXboxUserId failed: %s\n", GetXblErrorMessage(err));
            return;
        }

        const char* userEntityId = GetEntityIdFromXuid(userXuid);
        if (userEntityId == nullptr)
        {
            DEBUGLOG("No entity id for xuid %llu\n", userXuid);
            return;
        }

        auto it = m_chatControls.find(userEntityId);
        if (it != m_chatControls.end())
        {
            PartyChatControl* targetChatControl = it->second;
            if (m_localChatControl)
            {
                DEBUGLOG("Setting chat permissions with user %llu to 0x%x\n", userXuid, chatPermissionInfo.chatPermissionMask);
                m_localChatControl->SetPermissions(targetChatControl, chatPermissionInfo.chatPermissionMask);
            }
        }
        else
        {
            DEBUGLOG("No chat control for entityid %s\n", userEntityId);
        }
    }
}

void PlayFabPartyManager::OnTokenAndSignatureRequested(const PartyXblStateChange* change)
{
    DEBUGLOG("PartyXblStateChangeType: TokenAndSignatureRequested\n");
    const PartyXblTokenAndSignatureRequestedStateChange* result = static_cast<const PartyXblTokenAndSignatureRequestedStateChange*>(change);
    if (result)
    {
        // Setup the options flags
        XUserGetTokenAndSignatureOptions tnsOptions = result->forceRefresh ?
            XUserGetTokenAndSignatureOptions::ForceRefresh : XUserGetTokenAndSignatureOptions::None;

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

        // Start the auth call
        ATG::AsyncHelper(&XUserGetTokenAndSignatureAsync).Invoke(
            *Managers::Get<XboxUserManager>()->GetCurrentUser(),    // XUser to query
            tnsOptions,                                             // Option flags
            result->method,                                         // HTTP method ("GET", "POST")
            result->url,                                            // Auth url
            static_cast<uint32_t>(headers.size()),                  // Additional header count
            headers.data(),                                         // Additional headers
            result->bodySize,                                       // Query body size
            result->body,                                           // Query body
            [correlationId = result->correlationId](XAsyncBlock* async)
        {
            size_t bufferSize;
            HRESULT hr = XUserGetTokenAndSignatureResultSize(async, &bufferSize);

            if (FAILED(hr))
            {
                DEBUGLOG("XUserGetTokenAndSignatureResultSize failed: 0x%08X\n", static_cast<unsigned int>(hr));
            }

            std::vector<uint8_t> buffer(bufferSize);
            XUserGetTokenAndSignatureData* data = nullptr;

            if (SUCCEEDED(hr))
            {
                hr = XUserGetTokenAndSignatureResult(async, buffer.size(), buffer.data(), &data, nullptr /*bufferUsed*/);

                if (FAILED(hr))
                {
                    DEBUGLOG("XUserGetTokenAndSignatureResult failed: 0x%08X\n", static_cast<unsigned int>(hr));
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
                DEBUGLOG("CompleteGetTokenAndSignatureRequest succeeded.\n\tToken: %s\n\tSignature: %s\n", data ? data->token : "nullptr", data ? data->signature : "nullptr");
            }
            else
            {
                DEBUGLOG("CompleteGetTokenAndSignatureRequest failed: %hs\n", GetXblErrorMessage(err));
            }
        },
            Managers::Get<AsyncTaskManager>()->GetDefaultQueue().get()
            );
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
            DEBUGLOG("PlayFabPartyManager::TryEntityTokenRefresh: LoginToPlayFab failed: %s\n", GetXblErrorMessage(err));
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
    DEBUGLOG("SetTextChatTranslationOptions: bTranslateToLocalLanguage = %s, bEnableTextFiltering = %s\n",
        bTranslateToLocalLanguage ? "Enabled" : "Disabled",
        bEnableTextFiltering ? "Enabled" : "Disabled");

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
        DEBUGLOG("SetTextChatOptions failed: %s\n", GetErrorMessage(err));
    }
}

void PlayFabPartyManager::SetVoiceChatTranscriptionOptions(bool bTranscribeSelf, bool bTranscribeOtherChatControlsWithMatchingLanguages, bool bTranscribeOtherChatControlsWithNonMatchingLanguages, bool bTranslateToLocalLanguage)
{
    if (m_localChatControl)
    {
        DEBUGLOG("Setting Speech-To-Text options.\n");

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
        PartyError err = m_localChatControl->SetTranscriptionOptions(
            Options, // Transcription options
            nullptr  // Async identifier
            );

        if (PARTY_FAILED(err))
        {
            DEBUGLOG("SetTranscriptionOptions failed: %s\n", GetErrorMessage(err));
        }
    }
}

std::vector<std::string> PlayFabPartyManager::GetBestRegionList(const std::vector<std::vector<std::pair<std::string, uint64_t>>>& userRegionLatencyList)
{
    std::vector<std::pair<std::string, uint64_t>> candidateRegions;
    std::vector<std::string> regionList;

    for (const auto& userLatencies : userRegionLatencyList)
    {
        if (candidateRegions.size() == 0)
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

    if (candidateRegions.size() != 0)
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
