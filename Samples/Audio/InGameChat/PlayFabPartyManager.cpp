//--------------------------------------------------------------------------------------
// PlayFabPartyManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "InGameChat.h"
#include "PlayFabPartyManager.h"
#include "PartyImpl.h"
#include "PartyXboxLiveImpl.h"

#include <ctime>

using namespace Party;

namespace
{
    PartyString GetErrorMessage(PartyError error)
    {
        PartyString errString = nullptr;

        PartyError err = PartyManager::GetErrorMessage(error, &errString);
        if (PARTY_FAILED(err))
        {
            DebugTrace("Failed to get error message %lu.", error);
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
            DebugTrace("Failed to get error message %lu.", error);
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
        case PartyStateChangeResult::FailedToBindToLocalUdpSocket: return "Failed to bind to the local UDP socket";
        }
        return "Unknown enumeration value";
    }

    template <typename result_type>
    void LogResult(const result_type &err)
    {
        if (PARTY_FAILED(err->errorDetail))
        {
            DebugTrace("Error Detail: %hs", GetErrorMessage(err->errorDetail));
        }
        if (err->result != PartyStateChangeResult::Succeeded)
        {
            DebugTrace("Failed: %hs", PartyStateChangeResultToReasonString(err->result).c_str());
        }
    }

    std::string GetPartyStateChangeTypeString(Party::PartyStateChangeType Type)
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
            DebugTrace("PartyStateChange: PartyStateChangeType::%s ", GetPartyStateChangeTypeString(change->stateChangeType).c_str());
        }
        else
        {
            DebugTrace("PlayFabPartyManager::LogPartyStateChangeType: change was null ");
        }
    }
}

PlayFabPartyManager::~PlayFabPartyManager()
{
    DebugTrace("PlayFabPartyManager::~PlayFabPartyManager()");
}

void PlayFabPartyManager::Initialize()
{
    DebugTrace("PlayFabPartyManager::Initialize()");

    PartyManager& partyManager = PartyManager::GetSingleton();
    PartyError err;

    if (m_partyInitialized == false)
    {
        // Initialize PlayFab Party
        err = partyManager.Initialize(c_pfTitleId);
        if (PARTY_FAILED(err))
        {
            DebugTrace("Initialize failed: %hs", GetErrorMessage(err));
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
            DebugTrace("Initialize failed: %hs", GetXblErrorMessage(err));
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
        DebugTrace("CreateLocalUser with entityId %s", m_localEntityId.c_str());

        // Create a local user object
        err = partyManager.CreateLocalUser(
            m_localEntityId.c_str(),                    // User id
            m_localEntityToken.c_str(),                 // User entity token
            &m_localUser                                // OUT local user object
            );

        if (PARTY_FAILED(err))
        {
            DebugTrace("CreateLocalUser failed: %hs", GetErrorMessage(err));
            return;
        }
    }
}

void PlayFabPartyManager::SetLocalUser(uint64_t xuid, std::function<void(PartyError)> callback)
{
    DebugTrace("PlayFabPartyManager::SetLocalUser(%lu)", xuid);

    if (m_localChatUser != nullptr)
    {
        uint64_t localXuid = 0;

        PartyError err = m_localChatUser->GetXboxUserId(&localXuid);

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
                err = PartyXblManager::GetSingleton().DestroyChatUser(m_localChatUser);

                if (PARTY_FAILED(err))
                {
                    DebugTrace("DestroyChatUser failed: %s", GetXblErrorMessage(err));
                    // Fallthrough and try to create a new user
                }

                m_localChatUser = nullptr;
            }
        }
    }

    PartyError err = PartyXblManager::GetSingleton().CreateLocalChatUser(
        xuid,
        nullptr,
        &m_localChatUser
        );

    if (PARTY_FAILED(err))
    {
        DebugTrace("CreateLocalChatUser failed: %s", GetXblErrorMessage(err));
        callback(err);
        return;
    }

    err = PartyXblManager::GetSingleton().LoginToPlayFab(m_localChatUser, nullptr);
    if (PARTY_FAILED(err))
    {
        DebugTrace("LoginToPlayFab failed: %s", GetXblErrorMessage(err));
        callback(err);
        return;
    }

    m_userCreatedCallback = callback;
}

void PlayFabPartyManager::Shutdown()
{
    DebugTrace("PlayFabPartyManager::Shutdown()");

    m_state = NetworkManagerState::Initialize;

    // This cleans up everything allocated in Initialize() and
    // should only be used when done with networking
    PartyManager::GetSingleton().Cleanup();
    PartyXblManager::GetSingleton().Cleanup();

    m_localEndpoint = nullptr;
    m_network = nullptr;
    m_localUser = nullptr;
    m_partyInitialized = false;
    m_partyXblInitialized = false;
}

void PlayFabPartyManager::CreateAndConnectToNetwork(const char *networkId, std::function<void(std::string)> callback)
{
    DebugTrace("Creating PFP network '%s'", networkId);

    PartyNetworkConfiguration cfg = {};

    // Setup the network to allow 8 single-device players of any device type
    cfg.maxDeviceCount = 8;
    cfg.maxDevicesPerUserCount = 1;
    cfg.maxEndpointsPerDeviceCount = 1;
    cfg.maxUserCount = 8;
    cfg.maxUsersPerDeviceCount = 1;

    // Setup the network invitation configuration to use the network id as an invitation id and allow anyone to join.
    PartyInvitationConfiguration invitationConfiguration{
        networkId,                                  // invitation identifier
        PartyInvitationRevocability::Anyone,        // revokability
        0,                                          // authorized user count
        nullptr                                     // authorized user list
        };

    PartyNetworkDescriptor networkDescriptor = {};

    // Create a new network descriptor
    PartyError err = PartyManager::GetSingleton().CreateNewNetwork(
        m_localUser,                                // Local User
        &cfg,                                       // Network Config
        0,                                          // Region List Count
        nullptr,                                    // Region List
        &invitationConfiguration,                   // Invitation configuration
        nullptr,                                    // Async Identifier
        &networkDescriptor,                         // OUT network descriptor
        nullptr                                     // Applied initial invitation identifier
        );

    if (PARTY_FAILED(err))
    {
        DebugTrace("Failed to create PFP network: %hs", GetErrorMessage(err));
        return;
    }

    // Connect to the new network
    if (InternalConnectToNetwork(networkId, networkDescriptor))
    {
        m_state = NetworkManagerState::WaitingForNetwork;
        m_onNetworkCreated = callback;
    }
}

void PlayFabPartyManager::ConnectToNetwork(const char* networkId, const char* descriptor, std::function<void(void)> callback)
{
    DebugTrace("Connecting to PFP network '%s'", networkId);

    PartyNetworkDescriptor networkDescriptor = {};

    // Deserialize the remote network's descriptor
    PartyError err = PartyManager::DeserializeNetworkDescriptor(descriptor, &networkDescriptor);

    if (PARTY_FAILED(err))
    {
        DebugTrace("ConnectToNetwork failed to deserialize descriptor: %hs", GetErrorMessage(err));
        return;
    }

    // Connect to the remote network
    if (InternalConnectToNetwork(networkId, networkDescriptor))
    {
        m_state = NetworkManagerState::WaitingForNetwork;
        m_onNetworkConnected = callback;
    }
}

bool PlayFabPartyManager::InternalConnectToNetwork(const char* networkId, Party::PartyNetworkDescriptor& descriptor)
{
    // This portion of connecting to the network is the same for
    // both creating a new and joining an existing network.

    PartyError err = PartyManager::GetSingleton().ConnectToNetwork(
        &descriptor,                                // Network descriptor
        nullptr,                                    // Async identifier
        &m_network                                  // OUT network
        );

    if (PARTY_FAILED(err))
    {
        DebugTrace("Failed to connect to PFP network: %hs", GetErrorMessage(err));
        return false;
    }

    // Authenticate the local user on the network so we can participate in it
    err = m_network->AuthenticateLocalUser(
        m_localUser,                                // Local user
        networkId,                                  // Invite value
        nullptr                                     // Async identifier
        );

    if (PARTY_FAILED(err))
    {
        DebugTrace("Failed to authenticate with PFP network:: %hs", GetErrorMessage(err));
        return false;
    }

    // Establish a network endpoint for game message traffic
    err = m_network->CreateEndpoint(
        m_localUser,                                // Local user
        0,                                          // Property Count
        nullptr,                                    // Property name keys
        nullptr,                                    // Property Values
        nullptr,                                    // Async identifier
        &m_localEndpoint                            // OUT local endpoint
        );

    if (PARTY_FAILED(err))
    {
        DebugTrace("Failed to create PFP endpoint: %hs", GetErrorMessage(err));
        return false;
    }

    return true;
}

void PlayFabPartyManager::SendNetworkMessage(uint64_t xuid, std::vector<uint8_t>& message, bool reliable)
{
    if (m_localEndpoint && m_state == NetworkManagerState::NetworkConnected)
    {
        // Form the data packet into a data buffer structure
        PartyDataBuffer data[] = {
            {
                static_cast<const void*>(message.data()),
                static_cast<uint32_t>(message.size())
            },
        };

        PartySendMessageOptions deliveryOptions =
            reliable ? PartySendMessageOptions::GuaranteedDelivery : PartySendMessageOptions::BestEffortDelivery;

        PartyEndpointArray sendToEndpoint = { &m_partyEndpoints[xuid] };

        // Send out the message to all other peers
        PartyError err = m_localEndpoint->SendMessage(
            1,                                      // endpoint count; 0 = broadcast
            sendToEndpoint,                         // endpoint list
            deliveryOptions,                        // send message options
            nullptr,                                // configuration
            1,                                      // buffer count
            data,                                   // buffer
            nullptr                                 // async identifier
            );

        if (PARTY_FAILED(err))
        {
            DebugTrace("Failed to SendMessage: %hs", GetErrorMessage(err));
        }
    }
}

void PlayFabPartyManager::SetNetworkMessageHandler(std::function<void(uint64_t, std::vector<uint8_t>&)> callback)
{
    m_onMessageReceived = callback;
}

void PlayFabPartyManager::SetEndpointChangeHandler(std::function<void(uint64_t, bool)> callback)
{
    m_onEndpointChanged = callback;
}

void PlayFabPartyManager::SetXuidToEntityIdHandler(std::function<void(uint64_t, std::string)> callback)
{
    m_onXuidMapped = callback;
}

void PlayFabPartyManager::LeaveNetwork(std::function<void(void)> callback)
{
    DebugTrace("Leaving PFP network");

    m_partyEndpoints.clear();
    m_pendingEndpoints.clear();
    m_xuidToEntityId.clear();
    m_entityIdToXuid.clear();

    if (m_state != NetworkManagerState::Leaving && m_network != nullptr)
    {
        m_state = NetworkManagerState::Leaving;
        m_onNetworkDestroyed = callback;

        m_network->LeaveNetwork(nullptr);
    }
    else
    {
        if (callback != nullptr)
        {
            callback();
        }
    }
}

void PlayFabPartyManager::LookupEntityIdsForXuids(size_t count, uint64_t* xuids, std::function<void(bool)> callback)
{
    std::vector<uint64_t> xuidsToQuery;

    for (size_t x = 0; x < count; x++)
    {
        uint64_t xuid = *(xuids + uint64_t(x));

        if (m_xuidToEntityId.find(xuid) == m_xuidToEntityId.end())
        {
            // Only query it if we don't already have it
            xuidsToQuery.push_back(xuid);

            DebugTrace("Looking up EntityId for %lu", xuid);
        }
    }

    if (xuidsToQuery.size() > 0)
    {
        XuidLookupContext* context = new XuidLookupContext{};

        context->callback = callback;

        PartyError err = PartyXblManager::GetSingleton().GetEntityIdsFromXboxLiveUserIds(
            static_cast<uint32_t>(xuidsToQuery.size()), // Count of xuids
            xuidsToQuery.data(),                        // Xuids list
            m_localChatUser,                            // Local user used to make the query
            context                                     // async identifier
            );

        if (PARTY_FAILED(err))
        {
            DebugTrace("GetEntityIdsFromXboxLiveUserIds failed: %hs", GetXblErrorMessage(err));
            delete context;
        }
    }
    else
    {
        DebugTrace("No new Xuids to look up");

        if (callback)
        {
            callback(false);
        }
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

#pragma warning(disable : 4061 4062)

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
        DebugTrace("StartProcessingStateChanges failed: %hs", GetXblErrorMessage(err));
        return;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        const PartyXblStateChange* change = xblChanges[i];

        switch (change->stateChangeType)
        {
        case PartyXblStateChangeType::CreateLocalChatUserCompleted: OnCreateLocalChatUserCompleted(change); break;
        case PartyXblStateChangeType::LoginToPlayFabCompleted: OnLoginToPlayFabCompleted(change); break;
        case PartyXblStateChangeType::GetEntityIdsFromXboxLiveUserIdsCompleted: OnGetEntityIdsFromXboxLiveUserIdsCompleted(change); break;
        default: break;
        }
    }

    err = PartyXblManager::GetSingleton().FinishProcessingStateChanges(count, xblChanges);

    if (PARTY_FAILED(err))
    {
        DebugTrace("FinishProcessingStateChanges failed: %hs", GetXblErrorMessage(err));
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
        DebugTrace("StartProcessingStateChanges failed: %hs", GetErrorMessage(err));
        return;
    }

    for (uint32_t i = 0; i < count; i++)
    {
        const PartyStateChange* change = changes[i];

        switch (change->stateChangeType)
        {
        case PartyStateChangeType::CreateNewNetworkCompleted: OnCreateNewNetworkCompleted(change); break;
        case PartyStateChangeType::ConnectToNetworkCompleted: OnConnectToNetworkCompleted(change); break;
        case PartyStateChangeType::LocalUserRemoved: OnLocalUserRemoved(change); break;
        case PartyStateChangeType::CreateEndpointCompleted: OnCreateEndpointCompleted(change); break;
        case PartyStateChangeType::EndpointCreated: OnEndpointCreated(change); break;
        case PartyStateChangeType::EndpointDestroyed: OnEndpointDestroyed(change); break;
        case PartyStateChangeType::LeaveNetworkCompleted: OnLeaveNetworkCompleted(change); break;
        case PartyStateChangeType::NetworkDestroyed: OnNetworkDestroyed(change); break;
        case PartyStateChangeType::EndpointMessageReceived: OnEndpointMessageReceived(change); break;
        default: break;
        }
    }

    // Return the processed changes back to the PartyManager
    err = PartyManager::GetSingleton().FinishProcessingStateChanges(count, changes);

    if (PARTY_FAILED(err))
    {
        DebugTrace("FinishProcessingStateChanges failed: %hs", GetErrorMessage(err));
    }

    if (m_pendingEndpoints.size() > 0)
    {
        for (auto it = m_pendingEndpoints.begin(); it != m_pendingEndpoints.end(); it++)
        {
            PartyString entityId;
            (*it)->GetEntityId(&entityId);

            uint64_t xuid = GetXuidFromEntityId(entityId);
            if(xuid != 0)
            {
                // Tell the engine a user is ready to receive data
                if (m_onEndpointChanged)
                {
                    m_onEndpointChanged(xuid, true);
                }

                m_pendingEndpoints.erase(it);

                break;
            }
        }
    }
}

void PlayFabPartyManager::OnCreateNewNetworkCompleted(const PartyStateChange* change)
{
    const PartyCreateNewNetworkCompletedStateChange* result = static_cast<const PartyCreateNewNetworkCompletedStateChange*>(change);
    if(result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
        {
            PartyString entityId;
            result->localUser->GetEntityId(&entityId);
            DebugTrace("CreateNewNetworkCompleted:  EntityId: %s", entityId);
        }
        else
        {
            DebugTrace("CreateNewNetworkCompleted:  FAIL:  %hs", PartyStateChangeResultToReasonString(result->result).c_str());
            DebugTrace("ErrorDetail: %hs", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnConnectToNetworkCompleted(const PartyStateChange* change)
{
    const PartyConnectToNetworkCompletedStateChange* result = static_cast<const PartyConnectToNetworkCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyStateChangeResult::Succeeded)
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
                    DebugTrace("Failed to serialize network descriptor: %hs", GetErrorMessage(err));
                    m_onNetworkCreated(std::string());
                }

                // Callback with the descriptor to be shared with connecting clients
                m_onNetworkCreated(std::string(descriptor));
            }
        }
        else
        {
            DebugTrace("ConnectToNetworkCompleted:  FAIL:  %hs", PartyStateChangeResultToReasonString(result->result).c_str());
            DebugTrace("ErrorDetail: %hs", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnLocalUserRemoved(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    if (m_state != NetworkManagerState::Leaving)
    {
        DebugTrace("Unexpected local user removal!");
    }
}

void PlayFabPartyManager::OnCreateEndpointCompleted(const PartyStateChange* change)
{
    const PartyCreateEndpointCompletedStateChange* result = static_cast<const PartyCreateEndpointCompletedStateChange*>(change);
    if (result)
    {
        if (result->result != PartyStateChangeResult::Succeeded)
        {
            DebugTrace("CreateEndpointCompleted:  FAIL:  %hs", PartyStateChangeResultToReasonString(result->result).c_str());
            DebugTrace("ErrorDetail: %hs", GetErrorMessage(result->errorDetail));
        }
    }
}

void PlayFabPartyManager::OnEndpointCreated(const PartyStateChange* change)
{
    const PartyEndpointCreatedStateChange* result = static_cast<const PartyEndpointCreatedStateChange*>(change);
    if (result)
    {
        if (result->endpoint == m_localEndpoint)
        {
            // Skip local endpoints
            return;
        }

        PartyString user = nullptr;
        PartyError err = result->endpoint->GetEntityId(&user);

        if (PARTY_FAILED(err) || user == nullptr)
        {
            DebugTrace("Unable to retrieve user id from endpoint: %hs", GetErrorMessage(err));
        }
        else
        {
            DebugTrace("Established endpoint with EntityId %s", user);

            uint64_t xuid = GetXuidFromEntityId(user);

            if (xuid == 0)
            {
                // Store it until the xuid is looked up
                m_pendingEndpoints.push_back(result->endpoint);
            }
            else if (m_onEndpointChanged != nullptr)
            {
                // Store the endpoint for sends
                m_partyEndpoints[xuid] = result->endpoint;

                // Tell the engine a user is ready to receive data
                m_onEndpointChanged(xuid, true);
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
        if (result->endpoint == m_localEndpoint)
        {
            // Our endpoint was disconnected
            m_localEndpoint = nullptr;
        }
        else
        {
            // Another user has disconnected
            PartyString user = nullptr;
            PartyError err = result->endpoint->GetEntityId(&user);

            if (PARTY_FAILED(err))
            {
                DebugTrace("Unable to retrieve user id from endpoint: %hs", GetErrorMessage(err));
                return;
            }

            uint64_t xuid = GetXuidFromEntityId(user);

            if (xuid != 0 && m_onEndpointChanged != nullptr)
            {
                m_partyEndpoints.erase(xuid);

                // Tell the engine a user has disconnected
                m_onEndpointChanged(xuid, false);
            }
        }
    }
}

void PlayFabPartyManager::OnLeaveNetworkCompleted(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    m_state = NetworkManagerState::Initialize;
    if (m_onNetworkDestroyed)
    {
        m_onNetworkDestroyed();
    }
}

void PlayFabPartyManager::OnNetworkDestroyed(const PartyStateChange* change)
{
    LogPartyStateChangeType(change);

    m_network = nullptr;
    if (m_state != NetworkManagerState::Leaving)
    {
        DebugTrace("Unexpected network destruction!");
    }
}

void PlayFabPartyManager::OnEndpointMessageReceived(const PartyStateChange* change)
{
    const PartyEndpointMessageReceivedStateChange* result = static_cast<const PartyEndpointMessageReceivedStateChange*>(change);
    if (result)
    {
        const uint8_t* buffer = static_cast<const uint8_t*>(result->messageBuffer);
        std::vector<uint8_t> message(buffer, buffer + result->messageSize);

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
                    m_onMessageReceived(userXuid, message);
                }
                else
                {
                    DebugTrace("Message received from entityid %s but we don't know their xuid yet.", sender);
                }
            }
        }
        else
        {
            DebugTrace("GetEntityId failed: %hs", GetErrorMessage(err));
        }
    }
}

void PlayFabPartyManager::OnCreateLocalChatUserCompleted(const PartyXblStateChange* change)
{
    const PartyXblCreateLocalChatUserCompletedStateChange* result = static_cast<const PartyXblCreateLocalChatUserCompletedStateChange*>(change);
    if (result)
    {
        DebugTrace("PartyXblStateChangeType: CreateLocalChatUserCompleted");
        m_localUserReady = true;
    }
}

void PlayFabPartyManager::OnLoginToPlayFabCompleted(const PartyXblStateChange* change)
{
    DebugTrace("PartyXblStateChangeType: LoginToPlayFabCompleted");

    const PartyXblLoginToPlayFabCompletedStateChange* result = static_cast<const PartyXblLoginToPlayFabCompletedStateChange*>(change);
    if (result)
    {
        if (result->result == PartyXblStateChangeResult::Succeeded)
        {
            m_localEntityId = result->entityId;
            m_localEntityToken = result->titlePlayerEntityToken;
            m_localEntityTokenExpirationTime = result->expirationTime;

            // Refresh the token at half the remaining time
            time_t currentTime = std::time(nullptr);
            m_localEntityTokenExpirationTime -= static_cast<int>((result->expirationTime - currentTime) / 2);

            // Finish creating our local Party user
            if (m_localUser == nullptr)
            {
                DebugTrace("CreateLocalUser with entityId %s", m_localEntityId.c_str());

                // Create a local user object
                PartyError err = PartyManager::GetSingleton().CreateLocalUser(
                    m_localEntityId.c_str(),                    // User id
                    m_localEntityToken.c_str(),                 // User entity token
                    &m_localUser                                // OUT local user object
                    );

                if (PARTY_FAILED(err))
                {
                    DebugTrace("CreateLocalUser failed: %hs", GetErrorMessage(err));

                    if (m_userCreatedCallback)
                    {
                        m_userCreatedCallback(err);
                    }

                    return;
                }
            }

            uint64_t xuid = 0;
            m_localChatUser->GetXboxUserId(&xuid);

            m_xuidToEntityId[xuid] = m_localEntityId;
            m_entityIdToXuid[m_localEntityId] = xuid;

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

void PlayFabPartyManager::OnGetEntityIdsFromXboxLiveUserIdsCompleted(const PartyXblStateChange* change)
{
    const PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange* result = static_cast<const PartyXblGetEntityIdsFromXboxLiveUserIdsCompletedStateChange*>(change);
    if (result)
    {
        XuidLookupContext* context = nullptr;

        if (result->asyncIdentifier)
        {
            context = reinterpret_cast<XuidLookupContext*>(result->asyncIdentifier);
        }

        if (result->result == PartyXblStateChangeResult::Succeeded)
        {
            for (uint32_t x = 0; x < result->entityIdMappingCount; x++)
            {
                m_xuidToEntityId[result->entityIdMappings[x].xboxLiveUserId] = result->entityIdMappings[x].playfabEntityId;
                m_entityIdToXuid[result->entityIdMappings[x].playfabEntityId] = result->entityIdMappings[x].xboxLiveUserId;

                DebugTrace("Received EntityId for %lu", result->entityIdMappings[x].xboxLiveUserId);
            }

            if (context && context->callback)
            {
                context->callback(true);
            }
        }
        else
        {
            DebugTrace("Failed to map EntityIds: %hs", GetXblErrorMessage(result->errorDetail));

            if (context && context->callback)
            {
                context->callback(false);
            }
        }

        if (context != nullptr)
        {
            delete context;
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
        DebugTrace("Refreshing PlayFab Token");

        PartyError err = PartyXblManager::GetSingleton().LoginToPlayFab(m_localChatUser, nullptr);
        if (PARTY_FAILED(err))
        {
            DebugTrace("PlayFabPartyManager::TryEntityTokenRefresh: LoginToPlayFab failed: %s", GetXblErrorMessage(err));
        }
        else
        {
            m_refreshingEntityToken = true;
        }
    }
}
