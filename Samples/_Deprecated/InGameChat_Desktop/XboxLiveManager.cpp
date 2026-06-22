//--------------------------------------------------------------------------------------
// XboxLiveManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "UnsecuredNetworkManager.h"
#include "UserManager.h"
#include "XboxLiveManager.h"
#include "InGameChat_Desktop.h"

XboxLiveManager::XboxLiveManager() :
    m_scid{},
    m_processMessages(false),
    m_initialized(false),
    m_usersAdded(0),
    m_peerId(0)
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
    m_networkManager = std::make_unique<UnsecuredNetworkManager>();

    // Register handler for peer connection changes
    m_networkManager->RegisterPeerChangeHandler([this](uint32_t peerId, bool connected)
    {
        if (connected)
        {
            // A peer has connected to the network;  match them up with the corresponding lobby
            // member entry so we can store the GamerTag and add them to the Chat session
            auto count = XblMultiplayerManagerLobbySessionMembersCount();
            auto members = std::vector<XblMultiplayerManagerMember>(count);
            auto hr = XblMultiplayerManagerLobbySessionMembers(
                count,
                members.data()
                );

            if (SUCCEEDED(hr))
            {
                std::lock_guard<std::mutex> lock(m_dataLock);

                for (const auto& member : members)
                {
                    if (XUID_TO_PEER(member.Xuid) == peerId)
                    {
                        // Add them to chat
                        Sample::Instance()->GetChatManager()->AddRemoteUser(
                            peerId,
                            member.Xuid
                            );

                        // Keep track of the GamerTag for the UI
                        m_mapXuidToGamertag[member.Xuid] = member.DebugGamertag;
                        break;
                    }
                }
            }
            else
            {
                DebugTrace("Unable to get lobby session members!");
            }
        }
        else
        {
            DebugTrace("Peer %lu disconnected from mesh", peerId);
        }
    });

    // Register handler for data arrival events
    m_networkManager->RegisterNetworkMessageHandler([](uint32_t peerId, std::vector<uint8_t> &data)
    {
        Sample::Instance()->GetChatManager()->ProcessChatPacket(peerId, data);
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
    uint64_t xuid = 0;

    hr = XUserGetId(
        users.at(0)->UserHandle,
        &xuid
        );

    if (FAILED(hr))
    {
        return hr;
    }

    // Generate the peer id off of the first local user's XUID
    m_peerId = XUID_TO_PEER(xuid);

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
    m_networkManager->Initialize(m_peerId);
    m_networkManager->CreateNetwork();

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
        XUserGamertagComponent::Classic, XUserGamertagComponentClassicMaxBytes,
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
        std::lock_guard<std::mutex> lock(m_dataLock);
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
    std::lock_guard<std::mutex> lock(m_dataLock);
    m_mapXuidToGamertag[xuid] = gamertag;
}

std::wstring XboxLiveManager::GetGamertagForXuid(uint64_t xuid)
{
    std::lock_guard<std::mutex> lock(m_dataLock);

    auto it = m_mapXuidToGamertag.find(xuid);
    if (it != m_mapXuidToGamertag.end())
    {
        return DX::Utf8ToWide((*it).second);
    }

    return std::to_wstring(xuid);
}

void XboxLiveManager::SendNetworkMessage(uint32_t peerId, std::vector<uint8_t>& message)
{
    if (m_networkManager)
    {
        m_networkManager->SendNetworkMessage(peerId, message);
    }
}

uint64_t XboxLiveManager::GetXuidForPeer(uint32_t peerId)
{
    auto it = m_mapPeerToXuid.find(peerId);
    if (it != m_mapPeerToXuid.end())
    {
        return (*it).second;
    }

    return 0;
}

void XboxLiveManager::SetJoinableSessions(std::vector<XblMultiplayerActivityDetails> sessions)
{
    m_joinableSessions = sessions;
}

HRESULT XboxLiveManager::AddUserToSession(XUserHandle user)
{
    // Adds a local user to the MPM Lobby session
    auto hr = XblMultiplayerManagerLobbySessionAddLocalUser(user);

    if (SUCCEEDED(hr))
    {
        hr = XblMultiplayerManagerLobbySessionSetLocalMemberConnectionAddress(
            user,
            m_networkManager->GetExernalAddress().c_str(),
            nullptr
            );
    }
    else
    {
        DebugTrace("Unable to add user %lu to lobby!", user);
    }

    if (SUCCEEDED(hr))
    {
        DebugTrace("Added user %lu to lobby with address %s", user, m_networkManager->GetExernalAddress().c_str());
    }
    else
    {
        DebugTrace("Unable to set user's connection address");
    }

    return hr;
}

HRESULT XboxLiveManager::CreateSession()
{
    // Add the local users to the MPM Lobby and start processing MPM events
    auto users = Sample::Instance()->GetUserManager()->GetUsers();

    for (const auto& user : users)
    {
        AddUserToSession(user->UserHandle);
    }

    m_usersAdded = 0;
    m_processMessages = true;

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

    return S_OK;
}

HRESULT XboxLiveManager::JoinSession(const char* handle)
{
    auto users = Sample::Instance()->GetUserManager()->GetUsers();

    // Join all the local users to the remote Lobby
    for (const auto& user : users)
    {
        auto hr = XblMultiplayerManagerJoinLobby(
            handle,
            user->UserHandle
            );

        if (SUCCEEDED(hr))
        {
            hr = XblMultiplayerManagerLobbySessionSetLocalMemberConnectionAddress(
                user->UserHandle,
                m_networkManager->GetExernalAddress().c_str(),
                nullptr
                );
        }
        else
        {
            DebugTrace("Unable to add user %lu to lobby!", user->UserHandle);
        }

        if (SUCCEEDED(hr))
        {
            DebugTrace("Added user %lu to lobby with address %s", user->UserHandle, m_networkManager->GetExernalAddress().c_str());
        }
        else
        {
            DebugTrace("Unable to set user's connection address");
        }
    }

    m_usersAdded = 0;
    m_processMessages = true;

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

HRESULT XboxLiveManager::DoWork(float delta)
{
    if (!m_initialized)
    {
        return E_FAIL;
    }

    if (m_networkManager)
    {
        m_networkManager->DoWork(delta);
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

        case XblMultiplayerEventType::ClientDisconnectedFromMultiplayerService:
            DebugTrace("XblMultiplayerEventType::ClientDisconnectedFromMultiplayerService");
            if (Sample::Instance()->GetState() == Sample::SampleState::ChatLobby)
            {
                Sample::Instance()->LeaveChatSession();
            }
            break;

        case XblMultiplayerEventType::MemberPropertyChanged:
            {
                DebugTrace("XblMultiplayerEventType::MemberPropertyChanged");

                XblMultiplayerManagerMember member = {};

                hr = XblMultiplayerEventArgsMember(
                    event.EventArgsHandle,
                    &member
                    );

                if (SUCCEEDED(hr))
                {
                    auto json = nlohmann::json::parse(member.PropertiesJson);
                    auto it = json["channel"];
                    if (it.is_string())
                    {
                        auto chan = static_cast<uint8_t>(std::stoi(it.get<std::string>()));

                        DebugTrace("Setting channel for %ws to %d", GetGamertagForXuid(member.Xuid).c_str(), chan);

                        Sample::Instance()->GetChatManager()->ChangeChannelForUser(
                            member.Xuid,
                            chan
                            );
                    }
                }
            }
            break;

            case XblMultiplayerEventType::JoinLobbyCompleted:
            {
                DebugTrace("XblMultiplayerEventType::JoinLobbyCompleted");
                Sample::Instance()->GetChatManager()->Initialize();
            }
            break;

            case XblMultiplayerEventType::UserAdded:
            {
                DebugTrace("XblMultiplayerEventType::UserAdded");
                m_usersAdded++;

                if (m_usersAdded == int(Sample::Instance()->GetUserManager()->GetUsers().size()))
                {
                    // Wait for all local users to be added before initializing chat
                    Sample::Instance()->GetChatManager()->Initialize();
                }
            }
            break;

            case XblMultiplayerEventType::UserRemoved:
            {
                DebugTrace("XblMultiplayerEventType::UserRemoved");
                m_usersAdded--;

                if (m_usersAdded == 0)
                {
                    Sample::Instance()->ReturnToStart();
                }
            }
            break;

            case XblMultiplayerEventType::MemberJoined:
            {
                DebugTrace("XblMultiplayerEventType::MemberJoined");
                size_t count = 0;

                hr = XblMultiplayerEventArgsMembersCount(
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
                    // Connect to the new peers
                    for (const auto &member : members)
                    {
                        DebugTrace("Connecting to peer %lu at %s", XUID_TO_PEER(member.Xuid), member.ConnectionAddress);

                        m_networkManager->ConnectToPeer(
                            XUID_TO_PEER(member.Xuid),
                            member.ConnectionAddress
                            );
                    }
                }
            }
            break;

            case XblMultiplayerEventType::MemberLeft:
            {
                DebugTrace("XblMultiplayerEventType::MemberLeft");
                size_t count = 0;

                hr = XblMultiplayerEventArgsMembersCount(
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
                    for (const auto &member : members)
                    {
                        DebugTrace("Removing peer %lu", XUID_TO_PEER(member.Xuid));

                        Sample::Instance()->GetChatManager()->RemoveRemoteUser(
                            XUID_TO_PEER(member.Xuid),
                            member.Xuid
                            );
                    }
                }
            }
            break;

            default:
                DebugTrace("Received MPM event %u", event.EventType);
                break;
        }
    }

    return S_OK;
}
