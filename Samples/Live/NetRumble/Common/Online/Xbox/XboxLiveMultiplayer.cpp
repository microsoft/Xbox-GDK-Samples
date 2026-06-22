#include "pch.h"
#include "Game.h"
#include "XboxConfig.h"
#include "XboxLiveOnlineManager.h"
#include "Managers.h"


// This file contains the code to manage Xbox Live MPSD sessions using MultiplayerManager from XSAPI.
// It also interfaces with the PlayFab Party manager to coordinate creation and usage of networking and VOIP.
//
// There are three ways to enter a multiplayer game:
//
//    - Hosting a game          - HostMultiplayerGame()
//    - Joining a hosted game   - JoinMultiplayerGame()
//    - Matchmaking             - StartMatchmaking()
//
// Each of these will result in the user joining or creating a 'lobby' session, a 'game' session and the Party.

using namespace NetRumble;

#define DEBUGLOG_MPEVENT(eventType) DEBUGLOG(#eventType " - Result: 0x%x\n", mpEvent.Result)

namespace
{
    static GameMessage OnlineDisconnect(GameMessageType::OnlineDisconnect, 0);
    static GameMessage MatchmakingFailed(GameMessageType::MatchmakingFailed, 0);
    static GameMessage MatchmakingCanceled(GameMessageType::MatchmakingCanceled, 0);
    static GameMessage JoinGameFailed(GameMessageType::JoinGameFailed, 0);
    static GameMessage PlayerLeft(GameMessageType::PlayerLeft, 0);
    static GameMessage JoinGameCompleted(GameMessageType::JoinedGameComplete, 0);
    static GameMessage LeaveGameComplete(GameMessageType::LeaveGameComplete, 0);
    static GameMessage JoiningGame(GameMessageType::JoiningGame, 0);
}

// StartMatchmaking is the entry point for SmartMatch based matchmaking.  The flow is:
//
// - Add the local user to the MPM Lobby session, causing it to be created.
//
// - When XblMultiplayerEventType::UserAdded arrives
//   - Submit for matchmaking with XblMultiplayerManagerFindMatch()
//
// - When XblMultiplayerEventType::FindMatchCompleted arrives
//   - If it was cancelled, fire MatchmakingCanceled and reset state
//   - If it failed to find a match, resubmit
//   - If a match is found
//     - Everyone writes their EntityId to the game session
//     - The first member in the game session sets themselves as host
//
// - When XblMultiplayerEventType::HostChanged arrives for the Game session
//   - The host user will create the PlayFab Party and write the connection information to the game session
//   - The host user fires JoinGameCompleted
//
// - When XblMultiplayerEventType::SessionPropertyChanged arrives
//   - Look for the host network configuration and join, fire JoinGameCompleted on completion
//   - Look for other members' EntityIds and add them to PFP if we can match a XUID to an EntityId

void XboxLiveOnlineManager::StartMatchmaking()
{
    DEBUGLOG("Start Matchmaking\n");

    if (m_mpState != OnlineState::Ready)
    {
        DEBUGLOG("Multiplayer State is %d, expected %d.\n", m_mpState, OnlineState::Ready);
        m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
        return;
    }

    Managers::Get<PlayFabPartyManager>()->SetHost(false);

    // The Matchmaking process starts by adding our local player to MPM and waiting for the UserAdded event
    HRESULT hr = XblMultiplayerManagerLobbySessionAddLocalUser(
        GetCurrentUserHandle().get()
        );

    if (FAILED(hr))
    {
        DEBUGLOG("Failed to add local user to MPM: 0x%08X\n", static_cast<unsigned int>(hr));
        m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
        return;
    }

    m_mpState = OnlineState::Matchmaking;
}

void XboxLiveOnlineManager::CancelMatchmaking(bool immediate)
{
    DEBUGLOG("Cancel Matchmaking\n");

    if (m_mpState != OnlineState::Matchmaking)
    {
        DEBUGLOG("Multiplayer state is %d, expected %d.\n", m_mpState, OnlineState::Ready);
        return;
    }

    // on we want to terminate immediate, we pretend that the cancellation completed and
    // issue the callback early (when the event comes from MPM, we will just ignore it
    // if we are not in the "cancelling" state.
    if (immediate)
    {
        m_messageHandler(GetCurrentUserXuid(), &MatchmakingCanceled);
        m_mpState = OnlineState::Ready;
    }
    else
    {
        m_mpState = OnlineState::Canceling;
    }

    // Send a cancelation requst to MPM
    XblMultiplayerManagerCancelMatch();
}

// HostMultiplayerGame is the entry point for hosting a joinable game session.  The flow is:
//
// - Set the joinability to 'invite only' on request
//
// - Add the local user to the MPM Lobby session, causing it to be created.
//
// - When XblMultiplayerEventType::UserAdded arrives
//   - Create the MPM Game session by calling XblMultiplayerManagerJoinGameFromLobby()
//
// - When XblMultiplayerEventType::JoinGameCompleted arrives
//   - Write EntityId to the game session
//   - Sets ourselves as host
//   - Create the PlayFab Party and write the connection information to the game session
//   - Fires JoinGameCompleted
//
// - When XblMultiplayerEventType::SessionPropertyChanged arrives
//   - Look for other members' EntityIds and add them to PFP if we can match a XUID to an EntityId

void XboxLiveOnlineManager::HostMultiplayerGame(
    bool privateSession
    )
{
    DEBUGLOG("Host Multiplayer Game - %s\n", privateSession ? "Private" : "Open");

    if (m_mpState != OnlineState::Ready)
    {
        DEBUGLOG("Multiplayer state is %d, expected %d.\n", m_mpState, OnlineState::Ready);
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
        return;
    }

    if (privateSession)
    {
        HRESULT hr = XblMultiplayerManagerSetJoinability(
            XblMultiplayerJoinability::None,
            nullptr
            );

        if (FAILED(hr))
        {
            DEBUGLOG("Unable to set joinability: 0x%08X\n", static_cast<unsigned int>(hr));
        }
    }

    Managers::Get<PlayFabPartyManager>()->SetHost(true);

    // When hosting a game we first join the local user to MPM and wait for the UserAdded event
    HRESULT hr = XblMultiplayerManagerLobbySessionAddLocalUser(
        GetCurrentUserHandle().get()
        );

    if (FAILED(hr))
    {
        DEBUGLOG("Failed to add local user to MPM: 0x%08X\n", static_cast<unsigned int>(hr));
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
        return;
    }

    m_mpState = OnlineState::Hosting;
}

void XboxLiveOnlineManager::JoinPendingInviteSession()
{
    DEBUGLOG("Join Pending Invite Session\n");

    if (!m_joiningSession.empty())
    {
        JoinMultiplayerGame(m_joiningSession.c_str());
        m_joiningSession.clear();
    }
}

void XboxLiveOnlineManager::JoinGameFromInvite(const char* session)
{
    DEBUGLOG("Join Game from Invite '%hs'\n", session);

    if (m_mpState == OnlineState::Ready)
    {
        JoinMultiplayerGame(session);
    }
    else
    {
        m_joiningSession = session;
        LeaveMultiplayerGame(false);
    }
}

// JoinMultiplayerGame is the entry point for joining an existing game session.  The flow is:
//
// - A session moniker via Social activity or activation is provided by the caller
//
// - Join the Lobby session using the session moniker by calling XblMultiplayerManagerJoinLobby()
//
// - When XblMultiplayerEventType::JoinLobbyCompleted arrives
//   - Join the MPM Game session by calling XblMultiplayerManagerJoinGameFromLobby()
//
// - When XblMultiplayerEventType::JoinGameCompleted arrives
//   - Write EntityId to the game session
//   - Check for existing members' EntityIds and add them to PFP
//   - Get the PFP network information from the session properties and join
//   - Fires JoinGameCompleted
//
// - When XblMultiplayerEventType::SessionPropertyChanged arrives
//   - Look for other members' EntityIds and add them to PFP if we can match a XUID to an EntityId

void XboxLiveOnlineManager::JoinMultiplayerGame(const char* session)
{
    DEBUGLOG("Join Multiplayer Game '%hs'\n", session);

    if (m_mpState != OnlineState::Ready)
    {
        LeaveMultiplayerGame(false);
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
        return;
    }

    Managers::Get<PlayFabPartyManager>()->SetHost(false);

    HRESULT hr = XblMultiplayerManagerJoinLobby(
        session,
        GetCurrentUserHandle().get()
        );

    if (FAILED(hr))
    {
        DEBUGLOG("Unable to join lobby: 0x%08X\n", static_cast<unsigned int>(hr));
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
        return;
    }

    m_mpState = OnlineState::Joining;
    m_messageHandler(GetCurrentUserXuid(), &JoiningGame);
}

void XboxLiveOnlineManager::JoinMultiplayerGame(OnlineUser* host)
{
    JoinMultiplayerGame(static_cast<XboxLiveOnlineUser*>(host)->mpActivity.HandleId);
}

void XboxLiveOnlineManager::LeaveMultiplayerGame(bool immediate)
{
    DEBUGLOG("Leave Multipalyer Game\n");

    if (m_mpState != OnlineState::Ready)
    {
        HRESULT hr = XblMultiplayerManagerLobbySessionRemoveLocalUser(
            GetCurrentUserHandle().get()
            );

        if (FAILED(hr))
        {
            DEBUGLOG("Failed to remove local user: 0x%08X\n", static_cast<unsigned int>(hr));
        }

        if (immediate)
        {
            Managers::Get<PlayFabPartyManager>()->LeaveNetwork();
            m_mpState = OnlineState::Ready;
            m_messageHandler(GetCurrentUserXuid(), &LeaveGameComplete);
        }
        else
        {
            Managers::Get<PlayFabPartyManager>()->LeaveNetwork(
                [this]()
                {
                    m_mpState = OnlineState::Ready;
                    m_messageHandler(GetCurrentUserXuid(), &LeaveGameComplete);
                });
        }

        m_networkId.clear();
        m_networkDescriptor.clear();
    }
    else
    {
        DEBUGLOG("Trying to leave while OnlineState is: %d\n", m_mpState);
    }
}

void XboxLiveOnlineManager::MultiplayerTick()
{
    const XblMultiplayerEvent *events = nullptr;
    size_t eventCount = 0;

    // Process MPM events
    HRESULT hr = XblMultiplayerManagerDoWork(
        &events,
        &eventCount
        );

    if (FAILED(hr))
    {
        return;
    }

    // Loop through events and take action
    auto view = ATG::ArrayView<const XblMultiplayerEvent>(events, eventCount);
    for (auto &&mpEvent : view)
    {
        switch (mpEvent.EventType)
        {
        case XblMultiplayerEventType::UserAdded:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::UserAdded);

            // User was added to the local Lobby so continue the process of finding a game session

            if (m_mpState == OnlineState::Matchmaking)
            {
                // Submit the lobby to matchmaking
                SubmitLobbyToMatchmaking();
            }
            else if (m_mpState == OnlineState::Hosting)
            {
                // Create the Game session
                CreateGameSession();
            }
            else
            {
                DEBUGLOG("UserAdded in unexpected state: %d\n", m_mpState);
            }
            break;

        case XblMultiplayerEventType::MemberJoined:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::MemberJoined);

            // Add new user to PFP
            FindAndAddRemoteUsers();
            break;

        case XblMultiplayerEventType::MemberLeft:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::MemberLeft);

            // Remove user and cleanup
            FindAndRemoveRemoteUsers();

            // Reset the host in case that was the user that left
            CheckAndTrySetHost();
            break;

        case XblMultiplayerEventType::HostChanged:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::HostChanged);

            if (mpEvent.SessionType == XblMultiplayerSessionType::GameSession)
            {
                auto isHost = XblMultiplayerManagerGameSessionIsHost(GetCurrentUserXuid());

                DEBUGLOG("Local user is host: %s\n", isHost ? "true" : "false");

                Managers::Get<PlayFabPartyManager>()->SetHost(isHost);

                // If we're still in the matchmaking flow have the host create the PFP session
                if (isHost && m_mpState == OnlineState::Matchmaking)
                {
                    // The host creates the PFP
                    CreatePlayFabParty();
                }
            }
            break;

        case XblMultiplayerEventType::SynchronizedHostWriteCompleted:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::SynchronizedHostWriteCompleted);

            // Just check for an error and retry host write
            if (FAILED(mpEvent.Result))
            {
                DEBUGLOG("SynchronizedHostWrite failed: 0x%x\n", mpEvent.Result);

                if (mpEvent.Result == HTTP_E_STATUS_PRECOND_FAILED)
                {
                    DEBUGLOG(" - Retrying\n");
                    CheckAndTrySetHost(mpEvent.Context != nullptr);
                }
            }
            break;

        case XblMultiplayerEventType::FindMatchCompleted:
        {
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::FindMatchCompleted);

            XblMultiplayerMatchStatus matchStatus = {};
            XblMultiplayerMeasurementFailure failureCause = {};

            hr = XblMultiplayerEventArgsFindMatchCompleted(
                mpEvent.EventArgsHandle,
                &matchStatus,
                &failureCause
                );

            if (FAILED(hr))
            {
                DEBUGLOG("Failed to get FindMatchCompleted args: 0x%08X\n", static_cast<unsigned int>(hr));
                m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
                break;
            }

            switch (matchStatus)
            {
                case XblMultiplayerMatchStatus::Canceled:
                case XblMultiplayerMatchStatus::Canceling:
                    // The ticket was canceled
                    DEBUGLOG("Matchmaking was canceled.\n");
                    if (m_mpState == OnlineState::Canceling)
                    {
                        m_messageHandler(GetCurrentUserXuid(), &MatchmakingCanceled);
                        m_mpState = OnlineState::Ready;
                    }
                    break;
                case XblMultiplayerMatchStatus::Expired:
                case XblMultiplayerMatchStatus::Failed:
                    DEBUGLOG("Match ticket expired or failed to find a match.\n");
                    // Resubmit ticket until the user cancels or a match succeeds
                    SubmitLobbyToMatchmaking();
                    break;
                case XblMultiplayerMatchStatus::Completed:
                    DEBUGLOG("Match found successfully.\n");
                    // We have a successful match which means we have a game session with all the members in it
                    WriteEntityIdToSession();

                    // The first user in the list will set themselves as host and the HostChanged event triggers
                    // continuation of the process
                    CheckAndTrySetHost();
                    break;
                default:
                    break;
            }
            break;
        }
        case XblMultiplayerEventType::JoinGameCompleted:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::JoinGameCompleted);

            if (FAILED(mpEvent.Result))
            {
                m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
                break;
            }

            // Write our entity Id to the session
            WriteEntityIdToSession();

            if (m_mpState == OnlineState::Hosting)
            {
                // Make ourself host
                CheckAndTrySetHost(true);

                // Create the PFP
                CreatePlayFabParty();
            }
            else if (m_mpState == OnlineState::Joining)
            {
                // Add existing session members to PFP
                FindAndAddRemoteUsers();

                // Join the PFP
                FindAndConnectToNetwork();
            }
            break;

        case XblMultiplayerEventType::SessionPropertyChanged:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::SessionPropertyChanged);

            if (mpEvent.SessionType == XblMultiplayerSessionType::GameSession)
            {
                if (Managers::Get<GameStateManager>()->GetState() == GameState::MigratingNetwork)
                {
                    // We've received a new network to migrate to
                    if (!Managers::Get<PlayFabPartyManager>()->IsHost())
                    {
                        MigrateToNewNetwork();
                    }
                }
                else
                {
                    // Check if a user has set their EntityId
                    FindAndAddRemoteUsers();

                    // If we're not the host, try to connect to the network
                    if (!Managers::Get<PlayFabPartyManager>()->IsHost())
                    {
                        FindAndConnectToNetwork();
                    }
                }
            }
            break;

        case XblMultiplayerEventType::LeaveGameCompleted:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::LeaveGameCompleted);

            m_mpState = OnlineState::Ready;
            m_messageHandler(GetCurrentUserXuid(), &LeaveGameComplete);
            break;

        case XblMultiplayerEventType::ClientDisconnectedFromMultiplayerService:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::ClientDisconnectedFromMultiplayerService);
            break;

        case XblMultiplayerEventType::JoinLobbyCompleted:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::JoinLobbyCompleted);

            // If we're joining a remote session, go ahead and joing the game session too
            if (m_mpState == OnlineState::Joining)
            {
                // Now join the game session
                CreateGameSession();
            }
            break;

        case XblMultiplayerEventType::UserRemoved:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::UserRemoved);
            break;
        case XblMultiplayerEventType::JoinabilityStateChanged:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::JoinabilityStateChanged);
            break;
        case XblMultiplayerEventType::InviteSent:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::InviteSent);
            break;
        case XblMultiplayerEventType::PerformQosMeasurements:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::PerformQosMeasurements);
            break;
        case XblMultiplayerEventType::MemberPropertyChanged:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::MemberPropertyChanged);
            break;
        case XblMultiplayerEventType::LocalMemberPropertyWriteCompleted:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::LocalMemberPropertyWriteCompleted);
            break;
        case XblMultiplayerEventType::LocalMemberConnectionAddressWriteCompleted:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::LocalMemberConnectionAddressWriteCompleted);
            break;
        case XblMultiplayerEventType::SessionPropertyWriteCompleted:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::SessionPropertyWriteCompleted);
            break;
        case XblMultiplayerEventType::SessionSynchronizedPropertyWriteCompleted:
            DEBUGLOG_MPEVENT(XblMultiplayerEventType::SessionSynchronizedPropertyWriteCompleted);
            break;
        default:
            break;
        }
    }
}

void XboxLiveOnlineManager::SubmitLobbyToMatchmaking()
{
    DEBUGLOG("Submit Lobby to Matchmaking\n");

    HRESULT hr = XblMultiplayerManagerFindMatch(
        XboxConfig::c_MatchHopperName,      // Partner Center configured match hopper name
        XboxConfig::c_MatchAttributes,      // Match session attributes
        XboxConfig::c_MatchTimeoutSeconds   // Timeout period in seconds
        );

    if (FAILED(hr))
    {
        DEBUGLOG("Failed to start matchmaking: 0x%08X\n", static_cast<unsigned int>(hr));
        m_mpState = OnlineState::Ready;
        m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
    }
}

void XboxLiveOnlineManager::CreateGameSession()
{
    DEBUGLOG("Create Game Session\n");

    // Create the Game session
    HRESULT hr = XblMultiplayerManagerJoinGameFromLobby(
        XboxConfig::c_MPSDGameTemplate
        );

    if (FAILED(hr))
    {
        DEBUGLOG("Failed to join game from lobby: 0x%08X\n", static_cast<unsigned int>(hr));
        m_mpState = OnlineState::Ready;
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
    }
}

void XboxLiveOnlineManager::CreatePlayFabParty()
{
    DEBUGLOG("Create PlayFab Party\n");

    m_networkId = DX::GuidUtil::NewGuid();

    Managers::Get<PlayFabPartyManager>()->CreateAndConnectToNetwork(
        m_networkId.c_str(),
        [this](std::string descriptor)
        {
            DEBUGLOG("Create PlayFab Party complete\n");
            m_networkDescriptor = descriptor;

            // Set the values in the session so all the other clients can find and join the Party session
            SetSessionProperty("invite", m_networkId.c_str());
            SetSessionProperty("descriptor", m_networkDescriptor.c_str());

            // We're now ready to be in the game lobby
            m_mpState = OnlineState::InGame;
            m_messageHandler(GetCurrentUserXuid(), &JoinGameCompleted);
        });
}

void XboxLiveOnlineManager::CheckAndTrySetHost(bool forceLocal)
{
    DEBUGLOG("Check And Try Set Host\n");

    size_t count = XblMultiplayerManagerGameSessionMembersCount();

    DEBUGLOG("There are %lu game session members\n", count);

    if (count > 0)
    {
        std::vector<XblMultiplayerManagerMember> members(count);

        HRESULT hr = XblMultiplayerManagerGameSessionMembers(
            count,
            members.data()
            );

        if (FAILED(hr))
        {
            DEBUGLOG("Failed to get game session members: 0x%08X\n", static_cast<unsigned int>(hr));
        }

        uint64_t xuid = GetCurrentUserXuid();

        if (forceLocal)
        {
            // Find the device token for the local user and make us host
            for (size_t x = 0; x < count; x++)
            {
                if (members[x].Xuid == xuid)
                {
                    DEBUGLOG("Setting ourself as host of game session\n");
                    hr = XblMultiplayerManagerGameSessionSetSynchronizedHost(
                        members[x].DeviceToken,
                        (void*)forceLocal
                        );

                    if (FAILED(hr))
                    {
                        DEBUGLOG("Failed to set synchronized host: 0x%08X\n", static_cast<unsigned int>(hr));
                    }
                }
            }
        }
        else
        {
            // If the current user is the first user in the session member list, claim host
            if (members[0].Xuid == xuid && !XblMultiplayerManagerGameSessionIsHost(xuid))
            {
                DEBUGLOG("Setting ourself as host of game session\n");
                hr = XblMultiplayerManagerGameSessionSetSynchronizedHost(
                    members[0].DeviceToken,
                    nullptr
                    );

                if (FAILED(hr))
                {
                    DEBUGLOG("Failed to set synchronized host: 0x%08X\n", static_cast<unsigned int>(hr));
                }
            }
            else
            {
                DEBUGLOG("We are not the host\n");
            }
        }
    }
}

void XboxLiveOnlineManager::WriteEntityIdToSession()
{
    auto entityId = Managers::Get<PlayFabPartyManager>()->GetLocalUserEntityId();

    DEBUGLOG("Writing entity id to session: %s\n", entityId);

    SetSessionProperty(
        std::to_string(GetCurrentUserXuid()).c_str(),
        entityId
        );
}

void XboxLiveOnlineManager::MigrateToNewNetwork()
{
    DEBUGLOG("Migrate To New Network\n");

    // Refresh the descriptor value from the session
    m_networkDescriptor = GetSessionProperty("descriptor");

    // Ask the PFP manager to swap networks
    Managers::Get<PlayFabPartyManager>()->MigrateToNetwork(
        m_networkDescriptor.c_str(),
        [](bool succeeded)
        {
            DEBUGLOG("MigrateToNetwork completed %s\n", succeeded ? "successfully" : "unsuccessfully");

            Managers::Get<OnlineManager>()->SendGameMessage(
                GameMessage(
                    GameMessageType::PlayerState,
                    g_game->GetLocalPlayerState()->SerializePlayerStateData()
                )
            );
        });
}

void XboxLiveOnlineManager::FindAndConnectToNetwork()
{
    DEBUGLOG("Find And Connect To Network\n");

    if (!m_networkId.empty() && !m_networkDescriptor.empty())
    {
        // We have values already, so no need to try again
        DEBUGLOG("Already attempting to connect or connected\n");
        return;
    }

    m_networkId = GetSessionProperty("invite");
    m_networkDescriptor = GetSessionProperty("descriptor");

    // If the info is there, connect now otherwise wait for the properties to be set
    if (!m_networkId.empty() && !m_networkDescriptor.empty())
    {
        Managers::Get<PlayFabPartyManager>()->ConnectToNetwork(
            m_networkId.c_str(),
            m_networkDescriptor.c_str(),
            [this]()
            {
                DEBUGLOG("Connect To Network complete\n");
                m_mpState = OnlineState::InGame;
                m_messageHandler(GetCurrentUserXuid(), &JoinGameCompleted);
            });
    }
}

void XboxLiveOnlineManager::FindAndAddRemoteUsers()
{
    DEBUGLOG("Find And Add Remote Users\n");

    size_t count = XblMultiplayerManagerGameSessionMembersCount();

    if (count == 0)
    {
        DEBUGLOG("No GameSession members found.\n");
        return;
    }

    std::vector<XblMultiplayerManagerMember> members(count);

    HRESULT hr = XblMultiplayerManagerGameSessionMembers(
        count,
        members.data()
        );

    if (FAILED(hr))
    {
        DEBUGLOG("Failed to get session members: 0x%08X\n", static_cast<unsigned int>(hr));
        return;
    }

    // Loop over members and add any remote users to Party Manager
    for (const auto& member : members)
    {
        // If the member is remote and we've not seen them before
        if (!member.IsLocal && m_xuidToEntityIdMap.find(member.Xuid) == m_xuidToEntityIdMap.end())
        {
            // If the user has set their EntityId it will be under their Xuid as the key
            auto memberId = GetSessionProperty(std::to_string(member.Xuid).c_str());

            if (!memberId.empty())
            {
                DEBUGLOG("Adding member %llu (%s) to party manager\n", member.Xuid, memberId.c_str());

                Managers::Get<PlayFabPartyManager>()->AddRemoteUser(
                    member.Xuid,
                    memberId.c_str()
                    );

                m_xuidToEntityIdMap[member.Xuid] = memberId;
            }
            else
            {
                DEBUGLOG("No EntityId found for member %llu: %s\n", member.Xuid, member.PropertiesJson);
            }
        }
    }
}

void XboxLiveOnlineManager::FindAndRemoveRemoteUsers()
{
    DEBUGLOG("Find And Remove Remote Users\n");

    size_t count = XblMultiplayerManagerGameSessionMembersCount();

    if (count == 0)
    {
        DEBUGLOG("No GameSession members found\n");
        return;
    }

    std::vector<XblMultiplayerManagerMember> members(count);

    HRESULT hr = XblMultiplayerManagerGameSessionMembers(
        count,
        members.data()
        );

    if (FAILED(hr))
    {
        DEBUGLOG("Failed to get session members: 0x%08X\n", static_cast<unsigned int>(hr));
        return;
    }

    std::list<uint64_t> sessionXuids;
    std::list<uint64_t> removedXuids;

    // Loop over the member list and generate a list of xuids
    for (const auto& member : members)
    {
        sessionXuids.emplace_back(member.Xuid);
    }

    // Loop over our map and look for entries that aren't in the session anymore
    for (const auto& entry : m_xuidToEntityIdMap)
    {
        if (std::find(sessionXuids.begin(), sessionXuids.end(), entry.first) == sessionXuids.end())
        {
            removedXuids.emplace_back(entry.first);
        }
    }

    // Remove the users from PFP and our map
    for (const auto& xuid : removedXuids)
    {
        DEBUGLOG("User %lu has left the session, removing from party\n", xuid);

        Managers::Get<PlayFabPartyManager>()->RemoveRemoteUser(xuid);
        m_xuidToEntityIdMap.erase(xuid);
    }
}

void XboxLiveOnlineManager::SetSessionProperty(const char* name, const char* value)
{
    // Wrap the value in quotes so it's a valid JSON string value
    std::string quotedValue = "\"";
    quotedValue += value;
    quotedValue += "\"";

    DEBUGLOG("SetSessionProperty: %s = %s\n", name, value);

    HRESULT hr = XblMultiplayerManagerGameSessionSetProperties(
        name,
        quotedValue.c_str(),
        nullptr
        );

    if (FAILED(hr))
    {
        DEBUGLOG("Failed to set session property '%s' to '%s': 0x%08X\n", name, value, static_cast<unsigned int>(hr));
    }
}

std::string XboxLiveOnlineManager::GetSessionProperty(const char* name)
{
    std::string value;

    const char* props = XblMultiplayerManagerGameSessionPropertiesJson();

    if (props != nullptr)
    {
        auto propsJson = json::parse(props);

        if (propsJson.contains(name))
        {
            value = propsJson[name];
        }

        DEBUGLOG("GetSessionProperty \"%s\": %s\n", name, value.c_str());
    }
    else
    {
        DEBUGLOG("GetSessionProperty \"%s\": Game session properties JSON is null\n", name);
    }

    return value;
}
