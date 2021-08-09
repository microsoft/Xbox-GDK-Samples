//--------------------------------------------------------------------------------------
// XboxLiveManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "PlayFabPartyManager.h"
#include "UserManager.h"
#include "XboxLiveManager.h"
#include "InGameChat.h"

XboxLiveManager::XboxLiveManager() :
    m_scid{},
    m_processMessages(false),
    m_initialized(false),
    m_usersAdded(0)
{
    // Generate our SCID from the TitleID
    uint32_t titleId = 0;
    auto hr = XGameGetXboxTitleId(&titleId);

    if (FAILED(hr))
    {
        DebugTrace("Unable to retrieve Title ID!");
        DX::ThrowIfFailed(hr);
    }

    sprintf_s(m_scid, "00000000-0000-0000-0000-0000%08X", titleId);

    XblInitArgs xblInit = { };

    xblInit.queue = nullptr;
    xblInit.scid = m_scid;

    // Initialize Xbox Live
    hr = XblInitialize(&xblInit);

    if (FAILED(hr))
    {
        DebugTrace("Failed to initialize XBL!");
        DX::ThrowIfFailed(hr);
    }
}

XboxLiveManager::~XboxLiveManager()
{
}

HRESULT XboxLiveManager::Initialize()
{
    if (m_initialized)
    {
        return S_OK;
    }

    m_networkManager = std::make_unique<PlayFabPartyManager>();

    // Register handler for peer connection changes
    m_networkManager->SetEndpointChangeHandler(
        [this](uint64_t xuid, bool connected)
        {
            OnEndpointChanged(xuid, connected);
        });

    // Register handler for data arrival events
    m_networkManager->SetNetworkMessageHandler(
        [this](uint64_t endpointId, std::vector<uint8_t>& data)
        {
            std::lock_guard<std::recursive_mutex> lock(m_dataLock);

            for(auto item : m_mapChatIdToEndpointId)
            {
                if (item.second == endpointId)
                {
                    Sample::Instance()->GetChatManager()->ProcessChatPacket(item.first, data);
                    return;
                }
            }

            DebugTrace("No chat id found for endpoint %lu", endpointId);
        });

    // Initialize the mulitplayer manager
    auto hr = XblMultiplayerManagerInitialize(
        "LobbySession",
        nullptr
        );

    if (FAILED(hr))
    {
        return hr;
    }

    auto users = Sample::Instance()->GetUserManager()->GetUsers();

    // Register the local users
    for (const auto& user : users)
    {
        hr = AddLocalUser(user->UserHandle);

        if (FAILED(hr))
        {
            DebugTrace("Unable to add local user %lu", user->UserHandle);
        }
    }

    // Create the peer network
    m_networkManager->Initialize();

    m_initialized = true;

    return S_OK;
}

HRESULT XboxLiveManager::AddLocalUser(XUserHandle handle)
{
    auto async = new XAsyncBlock{};

    async->context = this;
    async->callback = [](XAsyncBlock* async)
    {
        size_t profileCount = 0;
        auto profiles = std::vector<XblUserProfile>();

        auto hr = XblProfileGetUserProfilesForSocialGroupResultCount(
            async,
            &profileCount
            );

        if (SUCCEEDED(hr))
        {
            profiles.resize(profileCount);

            hr = XblProfileGetUserProfilesForSocialGroupResult(
                async,
                profileCount,
                profiles.data()
                );
        }

        if (SUCCEEDED(hr))
        {
            auto This = reinterpret_cast<XboxLiveManager*>(async->context);

            for (const auto& profile : profiles)
            {
                This->SetGamertagForXuid(profile.xboxUserId, profile.gameDisplayName);
            }
        }
    };

    auto user = Sample::Instance()->GetUserManager()->GetUserByHandle(handle);

    // Query the user's friends so we can correlate XUIDs to GamerTags
    auto hr = XblProfileGetUserProfilesForSocialGroupAsync(
        user->ContextHandle,
        "People",
        async
        );

    if (FAILED(hr))
    {
        DebugTrace("Unable to get social profiles!");
    }

    // Get the user's GamerTag
    char gamertag[XUserGamertagComponentClassicMaxBytes] = {};

    hr = XUserGetGamertag(
        user->UserHandle,
        XUserGamertagComponent::Classic,
        XUserGamertagComponentClassicMaxBytes,
        gamertag,
        nullptr
        );

    if (FAILED(hr))
    {
        DebugTrace("Unable to get gamertag!");
    }

    uint64_t xuid = 0;

    // Get the user's XUID
    hr = XUserGetId(
        user->UserHandle,
        &xuid
        );

    if (FAILED(hr))
    {
        DebugTrace("Unable to get XUID!");
    }

    // Map the XUID to the GamerTag for UI display
    {
        std::lock_guard<std::recursive_mutex> lock(m_dataLock);
        m_mapXuidToGamertag[xuid] = gamertag;
    }

    if (m_processMessages)
    {
        // Session has already started so join the lobby immediatly
        AddUserToSession(user->UserHandle);
    }

    return S_OK;
}

void XboxLiveManager::SetGamertagForXuid(uint64_t xuid, const char* gamertag)
{
    std::lock_guard<std::recursive_mutex> lock(m_dataLock);
    m_mapXuidToGamertag[xuid] = gamertag;
}

std::wstring XboxLiveManager::GetGamertagForXuid(uint64_t xuid)
{
    std::lock_guard<std::recursive_mutex> lock(m_dataLock);

    auto it = m_mapXuidToGamertag.find(xuid);
    if (it != m_mapXuidToGamertag.end())
    {
        return DX::Utf8ToWide((*it).second);
    }

    return std::to_wstring(xuid);
}

void XboxLiveManager::SendNetworkMessage(uint64_t chatId, std::vector<uint8_t>& message, bool reliable)
{
    if (m_networkManager)
    {
        std::lock_guard<std::recursive_mutex> lock(m_dataLock);

        auto it = m_mapChatIdToEndpointId.find(chatId);
        if (it != m_mapChatIdToEndpointId.end())
        {
            m_networkManager->SendNetworkMessage(
                (*it).second,
                message,
                reliable
                );
        }
        else
        {
            DebugTrace("No endpoint found for chat id %lu", chatId);
        }
    }
}

void XboxLiveManager::SetJoinableSessions(std::vector<XblMultiplayerActivityDetails> sessions)
{
    m_joinableSessions = sessions;
}

HRESULT XboxLiveManager::AddUserToSession(XUserHandle user)
{
    uint64_t xuid = 0;
    XUserGetId(user, &xuid);

    // Adds a local user to the MPM Lobby session
    auto hr = XblMultiplayerManagerLobbySessionAddLocalUser(user);

    if (SUCCEEDED(hr))
    {
        hr = XblMultiplayerManagerLobbySessionSetLocalMemberConnectionAddress(
            user,
            m_networkDescriptor.c_str(),
            nullptr
            );

        if (SUCCEEDED(hr))
        {
            DebugTrace("Added local user %lu to lobby", xuid);
        }
        else
        {
            DebugTrace("Unable to set user's connection address");
        }
    }
    else
    {
        DebugTrace("Unable to add local user %lu to lobby!", xuid);
    }

    return hr;
}

HRESULT XboxLiveManager::CreateSession()
{
    // Add the local users to the MPM Lobby and start processing MPM events
    auto users = Sample::Instance()->GetUserManager()->GetUsers();

    // Set the first user as the primary for the network
    uint64_t xuid = 0;
    auto hr = XUserGetId(users.at(0)->UserHandle, &xuid);

    if (FAILED(hr))
    {
        DebugTrace("Unable to get user's XUID: 0x%x", hr);
        return hr;
    }

    // Set the user that will be used for the PFP network
    m_networkManager->SetLocalUser(
        xuid,
        [this, xuid, users](PartyError err)
        {
            if (PARTY_FAILED(err))
            {
                DebugTrace("SetLocalUser failed: %d", err);
                return;
            }

            // Get a list of the xuids of all local users
            std::vector<uint64_t> xuids;

            for (const auto& user : users)
            {
                uint64_t userXuid = 0;
                auto hr = XUserGetId(user->UserHandle, &userXuid);

                if (SUCCEEDED(hr) && userXuid != xuid)
                {
                    xuids.push_back(userXuid);
                }
            }

            auto continuation = [this, users]()
            {
                // Once we have the EntityID <-> xuid mapping we can create the PFP network
                m_networkManager->CreateAndConnectToNetwork(
                    c_networkId,
                    [this, users](std::string descriptor)
                    {
                        // Store the descriptor, it's used by AddUserToSession() next
                        m_networkDescriptor = descriptor;

                        // Add local users to MPM
                        for (const auto& user : users)
                        {
                            AddUserToSession(user->UserHandle);
                        }

                        m_usersAdded = 0;
                        m_processMessages = true;
                    });
            };

            if (xuids.size() > 0)
            {
                // Perform the EntityId lookup for each local user xuid
                m_networkManager->LookupEntityIdsForXuids(
                    xuids.size(),
                    xuids.data(),
                    [users, continuation](bool success)
                    {
                        if (success)
                        {
                            continuation();
                        }
                    });
            }
            else
            {
                continuation();
            }
        });

    return S_OK;
}

HRESULT XboxLiveManager::LeaveSession()
{
    auto users = Sample::Instance()->GetUserManager()->GetUsers();

    m_usersAdded = 0;

    // Remove all local users from the MPM Lobby
    for (const auto& user : users)
    {
        auto hr = XblMultiplayerManagerLobbySessionRemoveLocalUser(user->UserHandle);

        if (SUCCEEDED(hr))
        {
            m_usersAdded++;
        }
        else
        {
            DebugTrace("Unable to leave lobby!");
        }
    }

    // Leave the network too
    m_networkManager->LeaveNetwork([]()
    {
        DebugTrace("LeaveNetwork completed.");
    });

    m_pendingConnections.clear();
    m_mapChatIdToEndpointId.clear();
    m_mapDeviceToChatId.clear();
    m_mapXuidToDevice.clear();

    return S_OK;
}

HRESULT XboxLiveManager::JoinSession(const char* sessionHandle)
{
    // Add the local users to the MPM Lobby and start processing MPM events
    auto users = Sample::Instance()->GetUserManager()->GetUsers();

    // Set the first user as the primary for the network
    auto userHandle = users.at(0)->UserHandle;

    uint64_t xuid = 0;
    auto hr = XUserGetId(userHandle, &xuid);

    if (FAILED(hr))
    {
        DebugTrace("Unable to get user's XUID: 0x%x", hr);
        return hr;
    }

    std::string joiningSession = sessionHandle;

    m_networkManager->SetLocalUser(
        xuid,
        [this, users, joiningSession](PartyError err)
        {
            if (PARTY_FAILED(err))
            {
                DebugTrace("SetLocalUser failed: %d", err);
                return;
            }

            for (const auto& user : users)
            {
                // Join all the local users to the remote Lobby
                auto hr = XblMultiplayerManagerJoinLobby(
                    joiningSession.c_str(),
                    user->UserHandle
                    );

                uint64_t xuid = 0;
                XUserGetId(user->UserHandle, &xuid);

                if (SUCCEEDED(hr))
                {
                    DebugTrace("Added user %lu to lobby", xuid);
                }
                else
                {
                    DebugTrace("Unable to add user hane %lu to lobby!", xuid);
                }
            }

            m_usersAdded = 0;
            m_processMessages = true;
        });

    return S_OK;
}

HRESULT XboxLiveManager::FindJoinableSessions(std::function<void()> callback)
{
    // Query for friends' activity sessions
    uint64_t xuid = 0;
    auto userctx = Sample::Instance()->GetUserManager()->GetUsers().at(0);

    auto hr = XUserGetId(
        userctx->UserHandle,
        &xuid
        );

    if (FAILED(hr))
    {
        return hr;
    }

    auto async = new XAsyncBlock{};

    async->context = new FindSessionsContext(this, callback);
    async->callback = [](XAsyncBlock *async)
    {
        std::vector<XblMultiplayerActivityDetails> results;
        size_t resultCount = 0;

        auto hr = XblMultiplayerGetActivitiesForSocialGroupResultCount(
            async,
            &resultCount
            );

        if (SUCCEEDED(hr))
        {
            results.resize(resultCount);

            hr = XblMultiplayerGetActivitiesForSocialGroupResult(
                async,
                resultCount,
                results.data()
                );
        }

        auto context = static_cast<FindSessionsContext*>(async->context);

        if (SUCCEEDED(hr))
        {
            context->Manager->SetJoinableSessions(results);
            context->Callback();
        }

        delete context;
        delete async;
    };

    hr = XblMultiplayerGetActivitiesForSocialGroupAsync(
        userctx->ContextHandle,
        m_scid,
        xuid,
        "People",
        async
        );

    return hr;
}

HRESULT XboxLiveManager::InviteFriends(XUserHandle user)
{
    auto hr = XblMultiplayerManagerLobbySessionInviteFriends(
        user,
        nullptr,
        nullptr
        );

    if (FAILED(hr))
    {
        DebugTrace("Unable to send invites!");
    }

    return hr;
}

#ifdef __clang__
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif

#pragma warning(disable : 4061)

HRESULT XboxLiveManager::DoWork(float)
{
    if (!m_initialized)
    {
        return E_FAIL;
    }

    if (m_networkManager)
    {
        m_networkManager->DoWork();
    }

    if (!m_processMessages)
    {
        return S_FALSE;
    }

    const XblMultiplayerEvent *events = nullptr;
    size_t eventCount = 0;

    auto hr = XblMultiplayerManagerDoWork(&events, &eventCount);

    if (FAILED(hr))
    {
        return hr;
    }

    for (size_t x = 0; x < eventCount; x++)
    {
        auto& event = events[x];

        switch (event.EventType)
        {
        case XblMultiplayerEventType::LocalMemberPropertyWriteCompleted:
            DebugTrace("XblMultiplayerEventType::LocalMemberPropertyWriteCompleted");
            break;

        case XblMultiplayerEventType::LocalMemberConnectionAddressWriteCompleted:
            DebugTrace("XblMultiplayerEventType::LocalMemberConnectionAddressWriteCompleted");
            break;

        case XblMultiplayerEventType::HostChanged:
            DebugTrace("XblMultiplayerEventType::HostChanged");
            break;

        case XblMultiplayerEventType::InviteSent:
            DebugTrace("XblMultiplayerEventType::InviteSent");
            break;

        case XblMultiplayerEventType::ClientDisconnectedFromMultiplayerService:
            DebugTrace("XblMultiplayerEventType::ClientDisconnectedFromMultiplayerService");
            if (Sample::Instance()->GetState() == Sample::SampleState::ChatLobby)
            {
                Sample::Instance()->LeaveChatSession();
            }
            break;

        case XblMultiplayerEventType::MemberPropertyChanged:
            DebugTrace("XblMultiplayerEventType::MemberPropertyChanged");
            OnMemberPropertychanged(event);
            break;

        case XblMultiplayerEventType::JoinLobbyCompleted:
            DebugTrace("XblMultiplayerEventType::JoinLobbyCompleted");
            OnJoinLobbyCompleted(event);
            break;

        case XblMultiplayerEventType::UserAdded:
            DebugTrace("XblMultiplayerEventType::UserAdded");
            OnUserAdded(event);
            break;

        case XblMultiplayerEventType::UserRemoved:
            DebugTrace("XblMultiplayerEventType::UserRemoved");
            OnUserRemoved(event);
            break;

        case XblMultiplayerEventType::MemberJoined:
            DebugTrace("XblMultiplayerEventType::MemberJoined");
            OnMemberJoined(event);
            break;

        case XblMultiplayerEventType::MemberLeft:
            DebugTrace("XblMultiplayerEventType::MemberLeft");
            OnMemberLeft(event);
            break;

        default:
            DebugTrace("Received MPM event %u", event.EventType);
            break;
        }
    }

    ProcessPendingConnections();

    return S_OK;
}

void XboxLiveManager::OnMemberPropertychanged(const XblMultiplayerEvent& event)
{
    XblMultiplayerManagerMember member{};

    auto hr = XblMultiplayerEventArgsMember(
        event.EventArgsHandle,
        &member
        );

    if (SUCCEEDED(hr))
    {
        auto json = nlohmann::json::parse(member.PropertiesJson);
        auto it = json["channel"];

        auto chan = Sample::Instance()->GetChatManager()->GetChannelForUser(member.Xuid);

        if (it.is_number())
        {
            chan = it.get<uint8_t>();
        }
        else if (it.is_string())
        {
            chan = static_cast<uint8_t>(std::stoi(it.get<std::string>()));
        }

        DebugTrace("Setting channel for %ws to %d", GetGamertagForXuid(member.Xuid).c_str(), chan);

        Sample::Instance()->GetChatManager()->ChangeChannelForUser(
            member.Xuid,
            chan
            );
    }
}

void XboxLiveManager::OnJoinLobbyCompleted(const XblMultiplayerEvent&)
{
    // Wait for all local users to be added before continuing
    if (XblMultiplayerManagerLobbySessionLocalMembersCount() == Sample::Instance()->GetUserManager()->GetUsers().size())
    {
        auto count = XblMultiplayerManagerLobbySessionMembersCount();
        auto members = std::vector<XblMultiplayerManagerMember>(count);

        auto hr = XblMultiplayerManagerLobbySessionMembers(
            count,
            members.data()
            );

        if (SUCCEEDED(hr))
        {
            std::vector<uint64_t> xuids;

            for (const auto& member : members)
            {
                xuids.push_back(member.Xuid);
            }

            // Look up the EntityIds for new peers so we can map them when they connect
            // to the PFP network
            m_networkManager->LookupEntityIdsForXuids(
                xuids.size(),
                xuids.data(),
                [this](bool success)
                {
                    if (success)
                    {
                        // We're joining a remote lobby so once all our users are in the session
                        // we can get the network descriptor from the session host and join the PFP network

                        XblMultiplayerManagerMember hostMember;
                        auto hr = XblMultiplayerManagerLobbySessionHost(&hostMember);

                        if (SUCCEEDED(hr))
                        {
                            m_networkDescriptor = hostMember.ConnectionAddress;

                            // Initiate connecting to the network
                            m_networkManager->ConnectToNetwork(
                                c_networkId,
                                m_networkDescriptor.c_str(),
                                []()
                                {
                                    // After joining the lobby session and the PFP network it's
                                    // safe to join chat
                                    Sample::Instance()->GetChatManager()->Initialize();
                                });
                        }
                        else
                        {
                            DebugTrace("Unable to get session host, can't join network");
                        }
                    }
                    else
                    {
                        DebugTrace("Failed to map EntityIds to Xuids!");
                    }
                });
        }
        else
        {
            DebugTrace("Unable to get Lobby Session members: 0x%x", hr);
        }
    }
}

void XboxLiveManager::OnUserAdded(const XblMultiplayerEvent&)
{
    m_usersAdded++;

    // Wait for all local users to be added before continuing
    if (m_usersAdded == int(Sample::Instance()->GetUserManager()->GetUsers().size()))
    {
        // We're hosting the network and all users are in the session so
        // it's safe to add them to chat now
        Sample::Instance()->GetChatManager()->Initialize();
    }
}

void XboxLiveManager::OnUserRemoved(const XblMultiplayerEvent&)
{
    m_usersAdded--;

    if (m_usersAdded == 0)
    {
        Sample::Instance()->ReturnToStart();
    }
}

void XboxLiveManager::OnMemberJoined(const XblMultiplayerEvent& event)
{
    size_t count = 0;

    auto hr = XblMultiplayerEventArgsMembersCount(
        event.EventArgsHandle,
        &count
        );

    std::vector<XblMultiplayerManagerMember> members;

    if (SUCCEEDED(hr))
    {
        members.resize(count);

        hr = XblMultiplayerEventArgsMembers(
            event.EventArgsHandle,
            count,
            members.data()
            );
    }

    if (SUCCEEDED(hr))
    {
        // Look up the EntityIds for new peers
        std::vector<uint64_t> xuids;

        for (const auto& member : members)
        {
            xuids.push_back(member.Xuid);
        }

        m_networkManager->LookupEntityIdsForXuids(
            xuids.size(),
            xuids.data(),
            [this, xuids](bool success)
            {
                if (success)
                {
                    std::lock_guard<std::recursive_mutex> lock(m_dataLock);
                    for (auto xuid : xuids)
                    {
                        // See if there are other players on the same device
                        auto users = GetMembersOnSameDevice(xuid);

                        if (users.size() > 1)
                        {
                            uint64_t chatId = 0;

                            // Now see if one is the endpoint user
                            for (auto user : users)
                            {
                                if (m_networkManager->IsEndpointXuid(user.Xuid))
                                {
                                    chatId = user.Xuid;
                                }
                            }

                            // If there is already an endpoint for this device, add the user
                            if (chatId != 0)
                            {
                                // Add them to chat
                                Sample::Instance()->GetChatManager()->AddRemoteUser(
                                    xuid,
                                    chatId
                                    );

                                XblMultiplayerManagerMember user;
                                GetMemberByXuid(xuid, &user);
                                // Keep track of the GamerTag for the UI
                                m_mapXuidToGamertag[xuid] = user.DebugGamertag;
                            }
                        }
                    }
                }
            });
    }
}

void XboxLiveManager::OnMemberLeft(const XblMultiplayerEvent& event)
{
    size_t count = 0;

    auto hr = XblMultiplayerEventArgsMembersCount(
        event.EventArgsHandle,
        &count
        );

    std::vector<XblMultiplayerManagerMember> members;

    if (SUCCEEDED(hr))
    {
        members.resize(count);

        hr = XblMultiplayerEventArgsMembers(
            event.EventArgsHandle,
            count,
            members.data()
            );
    }

    if (SUCCEEDED(hr))
    {
        // Remove users from chat
        for (const auto& member : members)
        {
            DebugTrace("Removing user %lu", member.Xuid);

            Sample::Instance()->GetChatManager()->RemoveRemoteUser(
                member.Xuid
                );
        }
    }
}

void XboxLiveManager::OnEndpointChanged(uint64_t xuid, bool connected)
{
    DebugTrace("Endpoint change for user %lu %sconnected", xuid, connected ? "" : "dis");

    if (connected)
    {
        // Place the user in a queue so we can match them up with their
        // EntityId and DeviceToken
        m_pendingConnections.push_back(xuid);
    }
    else
    {
        DebugTrace("User %lu disconnected", xuid);
    }
}

void XboxLiveManager::ProcessPendingConnections()
{
    if (m_pendingConnections.size() == 0)
    {
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(m_dataLock);
    std::vector<uint64_t> leftovers;

    for (auto xuid : m_pendingConnections)
    {
        // A peer has connected to the network;  match them up with the corresponding lobby
        // member entry so we can store the GamerTag and add them to the Chat session
        XblMultiplayerManagerMember member = {};

        auto hr = GetMemberByXuid(xuid, &member);

        if (SUCCEEDED(hr))
        {
            std::string entityId = m_networkManager->GetEntityIdFromXuid(xuid);

            if (!entityId.empty())
            {
                uint64_t chatId = 0;

                // Find or create a mapping between the entityid and a chat id
                auto it = m_mapDeviceToChatId.find(entityId);
                if (it == m_mapDeviceToChatId.end())
                {
                    chatId = member.Xuid;
                    m_mapDeviceToChatId[entityId] = chatId;
                }
                else
                {
                    chatId = (*it).second;
                }

                // If this xuid is the id for an endpoint create an association
                // with the chat id
                if (m_networkManager->IsEndpointXuid(xuid))
                {
                    m_mapChatIdToEndpointId[chatId] = xuid;
                }

                // Add all users on the same device to chat
                auto userList = GetMembersOnSameDevice(xuid);

                for (auto user : userList)
                {
                    // Add them to chat
                    Sample::Instance()->GetChatManager()->AddRemoteUser(
                        user.Xuid,
                        chatId
                        );

                    // Keep track of the GamerTag for the UI
                    m_mapXuidToGamertag[user.Xuid] = user.DebugGamertag;
                }
            }
            else
            {
                leftovers.push_back(xuid);
            }
        }
        else
        {
            leftovers.push_back(xuid);
        }
    }

    m_pendingConnections = leftovers;
}

std::vector<XblMultiplayerManagerMember> XboxLiveManager::GetMembersOnSameDevice(uint64_t xuid)
{
    std::vector<XblMultiplayerManagerMember> memberList;
    XblMultiplayerManagerMember primary = {};

    auto hr = GetMemberByXuid(xuid, &primary);

    if (SUCCEEDED(hr))
    {
        auto count = XblMultiplayerManagerLobbySessionMembersCount();
        auto members = std::vector<XblMultiplayerManagerMember>(count);

        hr = XblMultiplayerManagerLobbySessionMembers(
            count,
            members.data()
            );

        if (SUCCEEDED(hr))
        {
            for (auto member : members)
            {
                if (XblMultiplayerManagerMemberAreMembersOnSameDevice(&primary, &member))
                {
                    memberList.push_back(member);
                }
            }
        }
    }

    return memberList;
}

HRESULT XboxLiveManager::GetMemberByXuid(uint64_t xuid, XblMultiplayerManagerMember* sessionMember)
{
    if (sessionMember == nullptr)
    {
        return E_POINTER;
    }

    *sessionMember = {};

    auto count = XblMultiplayerManagerLobbySessionMembersCount();
    auto members = std::vector<XblMultiplayerManagerMember>(count);

    auto hr = XblMultiplayerManagerLobbySessionMembers(
        count,
        members.data()
        );

    if (SUCCEEDED(hr))
    {
        for (auto member : members)
        {
            if (member.Xuid == xuid)
            {
                *sessionMember = member;
            }
        }
    }

    return hr;
}
