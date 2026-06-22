//--------------------------------------------------------------------------------------
// OnlineManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "Game.h"
#include "OnlineManager.h"
#include "XboxUser.h"
#include "Managers.h"
#include "SampleConfig.h"

using namespace PlayFabMultiplayerRumble;

namespace
{
    static GameMessage PlayerJoined(GameMessageType::PlayerJoined, 0);
    static GameMessage MultiplayerPrivilegeError(GameMessageType::MPPrivilegeError, 0); 
    static GameMessage MatchmakingFailed(GameMessageType::MatchmakingFailed, 0);
    static GameMessage MatchmakingCanceled(GameMessageType::MatchmakingCanceled, 0);
    static GameMessage JoinGameFailed(GameMessageType::JoinGameFailed, 0);
    static GameMessage PlayerLeft(GameMessageType::PlayerLeft, 0);
    static GameMessage JoinGameCompleted(GameMessageType::JoinedGameComplete, 0);
    static GameMessage LeaveGameComplete(GameMessageType::LeaveGameComplete, 0);
    static GameMessage JoiningGame(GameMessageType::JoiningGame, 0);
    static GameMessage CrossplayPrivilegeError(GameMessageType::CPPrivilegeError, 0);
    static GameMessage FindLobbiesCompleted(GameMessageType::FindLobbiesCompleted, 0);
    static GameMessage FindLobbiesFailed(GameMessageType::FindLobbiesFailed, 0);

    void LogError_PFMultiplayer(const char* functionName, HRESULT hr)
    {
        if (const char* errorMessage = PFMultiplayerGetErrorMessage(hr))
        {
            LogError_HRESULTWithMessage(functionName, hr, errorMessage);
        }
    }

    std::string GetPFLobbyStateChangeTypeString(PFLobbyStateChangeType stateChangeType)
    {
        switch (stateChangeType)
        {
        case PFLobbyStateChangeType::CreateAndJoinLobbyCompleted: return STRINGIFY(PFLobbyStateChangeType::CreateAndJoinLobbyCompleted);
        case PFLobbyStateChangeType::JoinLobbyCompleted:          return STRINGIFY(PFLobbyStateChangeType::JoinLobbyCompleted);
        case PFLobbyStateChangeType::MemberAdded:                 return STRINGIFY(PFLobbyStateChangeType::MemberAdded);
        case PFLobbyStateChangeType::AddMemberCompleted:          return STRINGIFY(PFLobbyStateChangeType::AddMemberCompleted);
        case PFLobbyStateChangeType::MemberRemoved:               return STRINGIFY(PFLobbyStateChangeType::MemberRemoved);
        case PFLobbyStateChangeType::ForceRemoveMemberCompleted:  return STRINGIFY(PFLobbyStateChangeType::ForceRemoveMemberCompleted);
        case PFLobbyStateChangeType::LeaveLobbyCompleted:         return STRINGIFY(PFLobbyStateChangeType::LeaveLobbyCompleted);
        case PFLobbyStateChangeType::Updated:                     return STRINGIFY(PFLobbyStateChangeType::Updated);
        case PFLobbyStateChangeType::PostUpdateCompleted:         return STRINGIFY(PFLobbyStateChangeType::PostUpdateCompleted);
        case PFLobbyStateChangeType::Disconnecting:               return STRINGIFY(PFLobbyStateChangeType::Disconnecting);
        case PFLobbyStateChangeType::Disconnected:                return STRINGIFY(PFLobbyStateChangeType::Disconnected);
        case PFLobbyStateChangeType::JoinArrangedLobbyCompleted:  return STRINGIFY(PFLobbyStateChangeType::JoinArrangedLobbyCompleted);
        case PFLobbyStateChangeType::FindLobbiesCompleted:        return STRINGIFY(PFLobbyStateChangeType::FindLobbiesCompleted);
        case PFLobbyStateChangeType::InviteReceived:              return STRINGIFY(PFLobbyStateChangeType::InviteReceived);
        case PFLobbyStateChangeType::InviteListenerStatusChanged: return STRINGIFY(PFLobbyStateChangeType::InviteListenerStatusChanged);
        case PFLobbyStateChangeType::SendInviteCompleted:         return STRINGIFY(PFLobbyStateChangeType::SendInviteCompleted);
        }

        //we should never get here
        assert(false);
        return "Unknown enumeration value";
    }

    std::string GetPFMatchmakingStateChangeTypeString(PFMatchmakingStateChangeType stateChangeType)
    {
        switch (stateChangeType)
        {
        case PFMatchmakingStateChangeType::TicketStatusChanged: return STRINGIFY(PFMatchmakingStateChangeType::TicketStatusChanged);
        case PFMatchmakingStateChangeType::TicketCompleted:     return STRINGIFY(PFMatchmakingStateChangeType::TicketCompleted);
        }

        //we should never get here
        assert(false);
        return "Unknown enumeration value";
    }

    std::string GetPFMatchmakingTicketStatusString(PFMatchmakingTicketStatus ticketStatus)
    {
        switch (ticketStatus)
        {
        case PFMatchmakingTicketStatus::Creating:          return STRINGIFY(PFMatchmakingTicketStatus::Creating);
        case PFMatchmakingTicketStatus::Joining:           return STRINGIFY(PFMatchmakingTicketStatus::Joining);
        case PFMatchmakingTicketStatus::WaitingForPlayers: return STRINGIFY(PFMatchmakingTicketStatus::WaitingForPlayers);
        case PFMatchmakingTicketStatus::WaitingForMatch:   return STRINGIFY(PFMatchmakingTicketStatus::WaitingForMatch);
        case PFMatchmakingTicketStatus::Matched:           return STRINGIFY(PFMatchmakingTicketStatus::Matched);
        case PFMatchmakingTicketStatus::Canceled:          return STRINGIFY(PFMatchmakingTicketStatus::Canceled);
        case PFMatchmakingTicketStatus::Failed:            return STRINGIFY(PFMatchmakingTicketStatus::Failed);
        }

        //we should never get here
        assert(false);
        return "Unknown enumeration value";
    }

    std::string EntityKeyToString(PFEntityKey member)
    {
        std::string out;

        out += "EntityKey: Id: " + std::string(member.id) + " Type: " + std::string(member.type);

        return out;
    }

    bool EntityKeysEqual(const PFEntityKey& a, const PFEntityKey& b)
    {
        if (strcmp(a.id, b.id) != 0)
        {
            return false;
        }

        if (strcmp(a.type, b.type) != 0)
        {
            return false;
        }

        return true;
    }
}

void OnlineManager::Initialize() noexcept
{
    XblInitArgs xblInit = { nullptr, SampleConfig::c_ServiceConfigId };
    auto hr = XblInitialize(&xblInit);
    DX::ThrowIfFailed(hr);

    auto userMgr = Managers::Get<XboxUserManager>();
    m_signInCallbackToken = userMgr->AddUserSignedInCallback(
        [this](const User &user)
        {
            auto xboxUser = static_cast<const XboxUser&>(user);

            InitializeXboxLive(xboxUser);
            Managers::Get<FriendsManager>()->AddUserToSocialManager(m_xblContext, m_user);
        });

    m_signOutCallbackToken = userMgr->AddUserSignedOutCallback(
        [this](const User &user)
        {
            if (m_xblContext)
            {
                ATG::XboxHandle<XUserHandle> currentUser{};
                XblContextGetUser(m_xblContext.get(), currentUser.get_address_of());

                if (XUserCompare(currentUser.get(), static_cast<const XboxUser&>(user)))
                {
                    Managers::Get<FriendsManager>()->RemoveUserFromSocialManager(currentUser);

                    m_xblContext.clear();
                }
            }
        });

    Managers::Get<PlayFabPartyManager>()->SetEndpointChangeHandler([this](uint64_t xuid, bool connected)
        {
            if (Managers::Get<GameStateManager>()->GetState() == GameState::WaitingForPeerMigration)
            {
                if (connected)
                {
                    m_migratedXuids.push_back(xuid);

                    uint32_t currentMemberCount = GetLobbyMemberCount();
                    
                    // If we've migrated everyone (+1 for the local user) we can begin the game
                    if (m_migratedXuids.size() + 1 == currentMemberCount)
                    {
                        m_migratedXuids.clear();
                        Managers::Get<GameStateManager>()->SwitchToState(GameState::InGame);
                    }
                }
            }
            else if (Managers::Get<GameStateManager>()->GetState() != GameState::MigratingNetwork)
            {
                if (!connected)
                {
                    m_messageHandler(xuid, &PlayerLeft);
                    DEBUGLOG("Removing Peer %llu", xuid);
                }
                else
                {
                    m_messageHandler(xuid, &PlayerJoined);
                    DEBUGLOG("Adding Peer %llu", xuid);
                }
            }
        });

    XNetworkingRegisterConnectivityHintChanged(
        nullptr,
        this,
        [](void* context, _In_ const XNetworkingConnectivityHint* connectivityHint)
        {
            auto mgr = reinterpret_cast<OnlineManager*>(context);
            if (connectivityHint->networkInitialized)
            {
                if (mgr->m_pfmHandle == nullptr)
                {
                    mgr->InitializePlayFabMultiplayer();
                }
                if (nullptr != mgr->m_networkAvailableCallback)
                {
                    mgr->m_networkAvailableCallback();
                }
            }
        },
        &m_networkAvailableToken);

}

void OnlineManager::InitializePlayFabMultiplayer()
{
    DEBUGLOG("OnlineManager::InitializePlayFabMultiplayer()");

    MultiplayerInitializationConfiguration initConfig = {};
    initConfig.titleId = SampleConfig::c_pfTitleId;
    
    HRESULT hr = PFMultiplayerInitialize(&initConfig, &m_pfmHandle);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerInitialize", hr);
    }
}

void OnlineManager::UninitializePlayFabMultiplayer()
{
    DEBUGLOG("OnlineManager::UninitializePlayFabMultiplayer()");

    HRESULT hr = PFMultiplayerUninitialize(m_pfmHandle);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerUninitialize", hr);
    }
}

void OnlineManager::ShutdownAsync(std::function<void()> completionCallback) noexcept
{
    DeleteMPAActivity();

    UninitializePlayFabMultiplayer();

    if (m_networkAvailableToken.token != 0)
    {
        XNetworkingUnregisterConnectivityHintChanged(m_networkAvailableToken, true);
        m_networkAvailableToken = {};
    }

    m_hasMultiplayerPrivileges = false;

    Managers::Get<PlayFabPartyManager>()->SetEndpointChangeHandler(nullptr);

    auto userMgr = Managers::Get<XboxUserManager>();

    userMgr->RemoveUserSignedInCallback(m_signInCallbackToken);
    m_signInCallbackToken = 0;

    userMgr->RemoveUserSignedOutCallback(m_signOutCallbackToken);
    m_signInCallbackToken = 0;

    auto asyncManager = Managers::Get<AsyncTaskManager>();
    auto asyncQueue = asyncManager->GetDefaultQueue();
    auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [callback = std::move(completionCallback)](XAsyncBlock*)
    {
        if (callback)
        {
            callback();
        }
    });

    HRESULT hr = XblCleanupAsync(&asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblCleanupAsync", hr);
    }
}

bool OnlineManager::IsNetworkAvailable() noexcept
{
    if (XGameRuntimeIsFeatureAvailable(XGameRuntimeFeature::XNetworking))
    {
        XNetworkingConnectivityHint connectivityHint;

        auto hr = XNetworkingGetConnectivityHint(&connectivityHint);

        if (SUCCEEDED(hr))
        {
            return connectivityHint.networkInitialized;
        }

        return false;
    }

    return true;
}

void OnlineManager::SetNetworkAvailableCallback(std::function<void()> callback) noexcept
{
    m_networkAvailableCallback = callback;
}

void OnlineManager::SetLobbySearchCallback(std::function<void(uint32_t, const PFLobbySearchResult*)> callback) noexcept
{
    m_lobbySearchCallback = callback;
}

void OnlineManager::Tick(float) noexcept
{
    if (m_pfmHandle != nullptr)
    {
        ProcessLobbyStateChanges();
        ProcessMatchmakingStateChanges();
    }
}

void OnlineManager::MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool)> callback) noexcept
{
    Managers::Get<PlayFabPartyManager>()->MigrateToRegion(
        regionList,
        [this, callback](bool success, const char* descriptor)
        {
            if (success)
            {
                m_networkDescriptor = descriptor;

                // Set the values in the session so all the other clients can find and join the new Party session
                PropertyHelper lobbyProperties;
                lobbyProperties.AddProperty("descriptor", m_networkDescriptor);

                SetLobbyProperties(lobbyProperties);
            }

            if (callback != nullptr)
            {
                callback(success);
            }
        });
}

void OnlineManager::InitializeXboxLive(ATG::XboxHandle<XUserHandle> user)
{
    HRESULT hr = XblContextCreateHandle(user.get(), m_xblContext.release_and_get_address_of());
    DX::ThrowIfFailed(hr);
    m_user = user;

    m_hasMultiplayerPrivileges = CheckPrivilege(m_user, XUserPrivilegeOptions::None, XUserPrivilege::Multiplayer);
    if (m_hasMultiplayerPrivileges == false)
    {
        ResolvePrivilege(m_user, XUserPrivilegeOptions::None, XUserPrivilege::Multiplayer, [this](bool bPrivilegeResolved)
        {
            m_hasMultiplayerPrivileges = bPrivilegeResolved;

            if (m_hasMultiplayerPrivileges == false)
            {
                //Message back to game that user cannot do MP.
                if (m_messageHandler)
                {
                    m_messageHandler(GetCurrentUserXuid(), &MultiplayerPrivilegeError);
                }
            }
            else
            {
                //we resolved the privilege, now check the next one
                CheckCrossPlayPrivilege();
            }
        });
    }
    else
    {
        //we have the privilege, now check the next one
        CheckCrossPlayPrivilege();
    }

    // register for invite events and handle them when they come in
    XGameActivationRegisterForEvent(
        nullptr,
        this,
        [](void* context, const XGameActivationInfo* activationInfo)
        {
            if (activationInfo->type != XGameActivationType::AcceptedGameInvite)
            {
                return;
            }

            if (auto pThis = reinterpret_cast<OnlineManager*>(context))
            {
                // We need to parse the connectionString value out of the uri
                std::string uri = activationInfo->inviteUri;

                std::string str = "connectionString=";
                auto startPos = uri.find(str) + str.length();
                auto endPos = uri.find('&', startPos);

                // If the connectionString is at the end of the string then end will return not found.
                if (endPos == std::string::npos)
                {
                    endPos = uri.length() + 1;
                }

                std::string handle = uri.substr(startPos, endPos - startPos);

                pThis->JoinGameFromInvite(handle);
            }
        },
        &m_inviteRegistration);
}

void OnlineManager::OnNetworkLost()
{
    m_xblContext.clear();
}

void OnlineManager::CheckCrossPlayPrivilege()
{
    m_hasCrossplayPrivileges = CheckPrivilege(m_user, XUserPrivilegeOptions::None, XUserPrivilege::CrossPlay);
    if (m_hasCrossplayPrivileges == false)
    {
        ResolvePrivilege(m_user, XUserPrivilegeOptions::None, XUserPrivilege::CrossPlay, [this](bool bPrivilegeResolved)
        {
            m_hasCrossplayPrivileges = bPrivilegeResolved;

            if (m_hasCrossplayPrivileges == false)
            {
                //Message back to game that user cannot do crossplay.
                if (m_messageHandler)
                {
                    m_messageHandler(GetCurrentUserXuid(), &CrossplayPrivilegeError);
                }
            }
        });
    }
}

bool OnlineManager::CheckPrivilege(ATG::XboxHandle<XUserHandle> user, XUserPrivilegeOptions option, XUserPrivilege privilege)
{
    bool bHasPrivilege = false;

    // Perform the privilege check
    XUserPrivilegeDenyReason reason = {};
    HRESULT hr = XUserCheckPrivilege(user.get(), option, privilege, &bHasPrivilege, &reason);
    if (SUCCEEDED(hr))
    {
        if (bHasPrivilege == false)
        {
            // If the bool is false then the user doesn't have permission for some reason.
            // We log the reason, but do not use it in game logic.
            // Instead call the Resolve function to let the system figure out what the user must do.
            DEBUGLOG("OnlineManager::CheckPrivilege: User does not have privilege %d. Reason: %d", privilege, reason);
        }
        else
        {
            DEBUGLOG("OnlineManager::CheckPrivilege: User has privilege %d", privilege);
        }
    }
    else
    {
        LogError_HRESULT("XUserCheckPrivilege", hr);
    }

    return bHasPrivilege;
}

void OnlineManager::ResolvePrivilege(ATG::XboxHandle<XUserHandle> user, XUserPrivilegeOptions option, XUserPrivilege privilege, std::function<void(bool)> callback)
{
    auto asyncManager = Managers::Get<AsyncTaskManager>();
    auto asyncQueue = asyncManager->GetDefaultQueue();
    auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [privilege, callback](XAsyncBlock* async)
    {
        bool bPrivilegeResolved = false;

        HRESULT hr = XUserResolvePrivilegeWithUiResult(async);
        if (SUCCEEDED(hr))
        {
            DEBUGLOG("OnlineManager::ResolvePrivilege: Privilege %d resolved.", privilege);
            bPrivilegeResolved = true;
        }
        else
        {
            DEBUGLOG("OnlineManager::ResolvePrivilege: Privilege %d not resolved.", privilege);
        }

        if (callback)
        {
            callback(bPrivilegeResolved);
        }
    });

    HRESULT hr = XUserResolvePrivilegeWithUiAsync(user.get(), option, privilege, &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        delete asyncHelper;

        LogError_HRESULT("XUserResolvePrivilegeWithUiAsync", hr);
        if (callback)
        {
            callback(false);
        }
    }
}

void OnlineManager::SetEntityToken()
{
    DEBUGLOG("OnlineManager::SetEntityToken()");

    auto partyMgr = Managers::Get<PlayFabPartyManager>();

    PFEntityKey entityKey = GetCurrentUserEntityKey();
    const char* entityToken = partyMgr->GetCurrentUserEntityToken();

    HRESULT hr = PFMultiplayerSetEntityToken(m_pfmHandle, &entityKey, entityToken);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerSetEntityToken", hr);
    }
}

uint64_t OnlineManager::GetCurrentUserXuid() const
{
    uint64_t xuid = {};

    if (m_xblContext)
    {
        XblContextGetXboxUserId(m_xblContext.get(), &xuid);
    }

    return xuid;
}

void OnlineManager::ShowInviteUI() noexcept
{
    Managers::Get<MPAManager>()->ShowInviteUI(m_user, [](bool bSuccess)
    {
        if (bSuccess)
        {
            DEBUGLOG("OnlineManager::ShowInviteUI: Successfully opened the invite UI");
        }
        else
        {
            DEBUGLOG("OnlineManager::ShowInviteUI: Failed to show the invite UI");
        }
    });
}

uint64_t OnlineManager::GetNetworkId() const noexcept
{
    return GetCurrentUserXuid();
}

void OnlineManager::HostMultiplayerGame(bool privateSession) noexcept
{
    DEBUGLOG("OnlineManager::HostMultiplayerGame: %s", privateSession ? "Private" : "Open");

    if (m_mpState != OnlineState::Ready)
    {
        DEBUGLOG("Multiplayer state is %d, expected %d.", m_mpState, OnlineState::Ready);
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
        return;
    }

    PFLobbyAccessPolicy accessPolicy = PFLobbyAccessPolicy::Public;

    if (privateSession)
    {
        accessPolicy = PFLobbyAccessPolicy::Private;
    }

    PFLobbyCreateConfiguration createConfig{};
    createConfig.maxMemberCount = SampleConfig::c_MaxPlayers;
    createConfig.ownerMigrationPolicy = PFLobbyOwnerMigrationPolicy::Automatic;
    createConfig.accessPolicy = accessPolicy;

    PropertyHelper memberProperties;
    memberProperties.AddProperty("xuid", std::to_string(GetCurrentUserXuid()));

    PFLobbyJoinConfiguration joinConfig{};
    joinConfig.memberPropertyCount = memberProperties.GetCount();
    joinConfig.memberPropertyKeys = memberProperties.GetKeyData();
    joinConfig.memberPropertyValues = memberProperties.GetValueData();

    PFEntityKey entityKey = GetCurrentUserEntityKey();

    HRESULT hr = PFMultiplayerCreateAndJoinLobby(m_pfmHandle, &entityKey, &createConfig, &joinConfig, nullptr, &m_myLobby);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerCreateAndJoinLobby", hr);
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
    }
}

void OnlineManager::JoinPendingInviteSession() noexcept
{
    DEBUGLOG("Join Pending Invite Session");

    if (!m_joiningSession.empty())
    {
        JoinMultiplayerGame(m_joiningSession);
        m_joiningSession.clear();
    }
}

void OnlineManager::JoinGameFromInvite(const std::string& connectionString)
{
    DEBUGLOG("OnlineManager::JoinGameFromInvite()");

    if (m_mpState == OnlineState::Ready)
    {
        JoinMultiplayerGame(connectionString);
    }
    else
    {
        m_joiningSession = connectionString;
        LeaveMultiplayerGame();
    }
}

void OnlineManager::JoinMultiplayerGame(const std::string& connectionString)
{
    DEBUGLOG("OnlineManager::JoinMultiplayerGame()");

    if (m_mpState != OnlineState::Ready)
    {
        LeaveMultiplayerGame();
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
        return;
    }

    if (connectionString.empty() == false)
    {
        PropertyHelper memberProperties;
        memberProperties.AddProperty("xuid", std::to_string(GetCurrentUserXuid()));

        PFLobbyJoinConfiguration joinConfig{};
        joinConfig.memberPropertyCount = memberProperties.GetCount();
        joinConfig.memberPropertyKeys = memberProperties.GetKeyData();
        joinConfig.memberPropertyValues = memberProperties.GetValueData();

        PFEntityKey currentUserEntityKey = GetCurrentUserEntityKey();
        HRESULT hr = PFMultiplayerJoinLobby(m_pfmHandle, &currentUserEntityKey, connectionString.c_str(), &joinConfig, nullptr, &m_myLobby);
        if (FAILED(hr))
        {
            LogError_PFMultiplayer("PFMultiplayerJoinLobby", hr);
        }
        else
        {
            m_mpState = OnlineState::Joining;
            m_messageHandler(GetCurrentUserXuid(), &JoiningGame);
        }
    }
    else
    {
        DEBUGLOG("OnlineManager::JoinMultiplayerGame: connectionString was empty");
    }
}

void OnlineManager::JoinMultiplayerGame(OnlineUser* host) noexcept
{
    DEBUGLOG("OnlineManager::JoinMultiplayerGame()");

    if (XboxLiveOnlineUser* xblUser = static_cast<XboxLiveOnlineUser*>(host))
    {
        JoinMultiplayerGame(xblUser->connectionString);
    }
}

void OnlineManager::LeaveMultiplayerGame() noexcept
{
    DEBUGLOG("OnlineManager::LeaveMultiplayerGame()");

    if (m_mpState != OnlineState::Ready)
    {
        if (m_myLobby)
        {
            PFEntityKey currentUserEntityKey = GetCurrentUserEntityKey();
            HRESULT hr = PFLobbyLeave(m_myLobby, &currentUserEntityKey, nullptr);
            if (FAILED(hr))
            {
                LogError_PFMultiplayer("PFLobbyLeave", hr);
            }
        }

        Managers::Get<PlayFabPartyManager>()->LeaveNetwork(
        [this]()
        {
            m_messageHandler(GetCurrentUserXuid(), &LeaveGameComplete);
        });

        m_networkId.clear();
        m_networkDescriptor.clear();

        m_mpState = OnlineState::Ready;
    }
    else
    {
        DEBUGLOG("OnlineManager::LeaveMultiplayerGame: Trying to leave while OnlineState is: %d", m_mpState);
    }
}

void OnlineManager::CreatePlayFabParty()
{
    DEBUGLOG("OnlineManager::CreatePlayFabParty()");

    m_networkId = DX::GuidUtil::NewGuid();

    Managers::Get<PlayFabPartyManager>()->CreateAndConnectToNetwork(
        m_networkId.c_str(),
        [this](std::string descriptor)
        {
            DEBUGLOG("Create PlayFab Party complete");
            m_networkDescriptor = descriptor;

            // Set the values in the session so all the other clients can find and join the Party session
            PropertyHelper lobbyProperties;
            lobbyProperties.AddProperty("invite", m_networkId);
            lobbyProperties.AddProperty("descriptor", m_networkDescriptor);

            SetLobbyProperties(lobbyProperties);

            SetMPAActivity();

            // We're now ready to be in the game lobby
            m_mpState = OnlineState::InGame;
            m_messageHandler(GetCurrentUserXuid(), &JoinGameCompleted);
        });
}

void OnlineManager::MigrateToNewNetwork()
{
    DEBUGLOG("OnlineManager::MigrateToNewNetwork()");

    // Refresh the descriptor value from the session
    m_networkDescriptor = GetLobbyProperty("descriptor");

    // Ask the PFP manager to swap networks
    Managers::Get<PlayFabPartyManager>()->MigrateToNetwork(
        m_networkDescriptor.c_str(),
        [](bool succeeded)
        {
            DEBUGLOG("MigrateToNetwork completed %s", succeeded ? "successfully" : "unsuccessfully");

            Managers::Get<OnlineManager>()->SendGameMessage(GameMessage(GameMessageType::PlayerState, g_game->GetLocalPlayerState()->SerializePlayerStateData()));
        });
}

void OnlineManager::FindAndConnectToNetwork()
{
    DEBUGLOG("OnlineManager::FindAndConnectToNetwork()");

    if (!m_networkId.empty() && !m_networkDescriptor.empty())
    {
        // We have values already, so no need to try again
        DEBUGLOG("Already attempting to connect or connected");
        return;
    }

    m_networkId = GetLobbyProperty("invite");
    m_networkDescriptor = GetLobbyProperty("descriptor");

    // If the info is there, connect now otherwise wait for the properties to be set
    if (!m_networkId.empty() && !m_networkDescriptor.empty())
    {
        Managers::Get<PlayFabPartyManager>()->ConnectToNetwork(
            m_networkId.c_str(),
            m_networkDescriptor.c_str(),
            [this]()
            {
                DEBUGLOG("Connect To Network complete");

                SetMPAActivity();

                m_mpState = OnlineState::InGame;
                m_messageHandler(GetCurrentUserXuid(), &JoinGameCompleted);
            });
    }
}

void OnlineManager::FindLobbies() noexcept
{
    DEBUGLOG("OnlineManager::FindLobbies()");

    const uint32_t c_resultCount = 4;
    const char* c_filterString = "";
    const char* c_sortString = "";

    PFLobbySearchConfiguration config{};
    config.filterString = c_filterString;
    config.sortString = c_sortString;
    config.clientSearchResultCount = &c_resultCount;

    PFEntityKey entityKey = GetCurrentUserEntityKey();
    HRESULT hr = PFMultiplayerFindLobbies(m_pfmHandle, &entityKey, &config, nullptr);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerFindLobbies", hr);
        return;
    }
}

void OnlineManager::SetLobbyProperties(PropertyHelper& properties)
{
    DEBUGLOG("OnlineManager::SetLobbyProperties()");

    if (properties.GetCount() == 0)
    {
        DEBUGLOG("OnlineManager::SetLobbyProperties: property was empty");
        return;
    }

    PFEntityKey localUserEntityKey = GetCurrentUserEntityKey();
    PFLobbyDataUpdate lobbyUpdateData{};

    lobbyUpdateData.lobbyPropertyCount = properties.GetCount();
    lobbyUpdateData.lobbyPropertyKeys = properties.GetKeyData();
    lobbyUpdateData.lobbyPropertyValues = properties.GetValueData();

    HRESULT hr = PFLobbyPostUpdate(m_myLobby, &localUserEntityKey, &lobbyUpdateData, nullptr, nullptr);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFLobbyPostUpdate", hr);
    }
}
 
std::string OnlineManager::GetLobbyProperty(const char* propertyName)
{
    std::string value;

    if (propertyName)
    {
        const char* propertyValue = nullptr;
        HRESULT hr = PFLobbyGetLobbyProperty(m_myLobby, propertyName, &propertyValue);
        if (SUCCEEDED(hr))
        {
            if (propertyValue != nullptr)
            {
                value = propertyValue;
            }
        }
        else
        {
            LogError_PFMultiplayer("PFLobbyGetLobbyProperty", hr);
        }
    }
    else
    {
        DEBUGLOG("OnlineManager::GetLobbyProperty: propertyName was null");
    }

    return value;
}

#pragma region Lobby
void OnlineManager::ProcessLobbyStateChanges()
{
    uint32_t stateChangeCount = 0;
    const PFLobbyStateChange * const * stateChanges = nullptr;

    HRESULT hr = PFMultiplayerStartProcessingLobbyStateChanges(m_pfmHandle, &stateChangeCount, &stateChanges);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerStartProcessingLobbyStateChanges", hr);
        return;
    }

    for (uint32_t i = 0; i < stateChangeCount; ++i)
    {
        const PFLobbyStateChange& change = *stateChanges[i];

        DEBUGLOG("LobbyStateChange: %s", GetPFLobbyStateChangeTypeString(change.stateChangeType).c_str());
        switch (change.stateChangeType)
        {
        case PFLobbyStateChangeType::CreateAndJoinLobbyCompleted: OnCreateAndJoinLobbyCompleted(change); break;
        case PFLobbyStateChangeType::JoinLobbyCompleted:          OnJoinLobbyCompleted(change);          break;
        case PFLobbyStateChangeType::MemberAdded:                 OnMemberAdded(change);                 break;
        case PFLobbyStateChangeType::MemberRemoved:               OnMemberRemoved(change);               break;
        case PFLobbyStateChangeType::LeaveLobbyCompleted:         OnLeaveLobbyCompleted(change);         break;
        case PFLobbyStateChangeType::Updated:                     OnUpdated(change);                     break;
        case PFLobbyStateChangeType::PostUpdateCompleted:         OnPostUpdateCompleted(change);         break;
        case PFLobbyStateChangeType::Disconnecting:               OnDisconnecting(change);               break;
        case PFLobbyStateChangeType::JoinArrangedLobbyCompleted:  OnJoinArrangedLobbyCompleted(change);  break;
        case PFLobbyStateChangeType::FindLobbiesCompleted:        OnFindLobbiesCompleted(change);        break;

        //The sample does not currently react to these events
        case PFLobbyStateChangeType::AddMemberCompleted:          break;
        case PFLobbyStateChangeType::ForceRemoveMemberCompleted:  break;
        case PFLobbyStateChangeType::Disconnected:                break;
        case PFLobbyStateChangeType::InviteReceived:              break;
        case PFLobbyStateChangeType::InviteListenerStatusChanged: break;
        case PFLobbyStateChangeType::SendInviteCompleted:         break;
        }
    }

    hr = PFMultiplayerFinishProcessingLobbyStateChanges(m_pfmHandle, stateChangeCount, stateChanges);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerFinishProcessingLobbyStateChanges", hr);
    }
}

void OnlineManager::OnCreateAndJoinLobbyCompleted(const PFLobbyStateChange& change)
{
    DEBUGLOG("OnlineManager::OnCreateAndJoinLobbyCompleted()");

    const auto& stateChange = static_cast<const PFLobbyCreateAndJoinLobbyCompletedStateChange&>(change);
    if (SUCCEEDED(stateChange.result))
    {
        m_myLobby = stateChange.lobby;
        m_mpState = OnlineState::Hosting;

        CreatePlayFabParty();
    }
    else
    {
        LogError_PFMultiplayer("OnCreateAndJoinLobbyCompleted", stateChange.result);
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
    }
}

void OnlineManager::OnJoinLobbyCompleted(const PFLobbyStateChange& change)
{
    const auto& stateChange = static_cast<const PFLobbyJoinLobbyCompletedStateChange&>(change);
    if (SUCCEEDED(stateChange.result))
    {
        m_myLobby = stateChange.lobby;

        FindAndConnectToNetwork();
    }
    else
    {
        LogError_PFMultiplayer("PFLobbyJoinLobbyCompletedStateChange", stateChange.result);

        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
    }
}

void OnlineManager::OnMemberAdded(const PFLobbyStateChange& change)
{
    const auto& stateChange = static_cast<const PFLobbyMemberAddedStateChange&>(change);

    DEBUGLOG("OnlineManager::OnMemberAdded: Member added to lobby: %s", EntityKeyToString(stateChange.member).c_str());

    const char* value = nullptr;
    HRESULT hr = PFLobbyGetMemberProperty(stateChange.lobby, &stateChange.member, "xuid", &value);
    if (SUCCEEDED(hr) && value != nullptr)
    {
        uint64_t xuid = std::stoull(value);
        uint64_t currentUserXuid = GetCurrentUserXuid();

        std::string newMemberEntityId = stateChange.member.id;
        if (newMemberEntityId.empty() == false)
        {
            // If the member is remote and we've not seen them before
            if (xuid != currentUserXuid && m_xuidToEntityIdMap.find(xuid) == m_xuidToEntityIdMap.end())
            {
                DEBUGLOG("Adding member %llu (%s) to party manager", xuid, newMemberEntityId.c_str());

                Managers::Get<MPAManager>()->UpdateRecentPlayers(m_xblContext, xuid, [](bool bSuccess)
                {
                    if (bSuccess)
                    {
                        DEBUGLOG("OnlineManager::OnMemberAdded: Successfully updated recently met players list");
                    }
                    else
                    {
                        DEBUGLOG("OnlineManager::OnMemberAdded: Failed to update recetnly met players list");
                    }
                });

                Managers::Get<PlayFabPartyManager>()->AddRemoteUser(xuid, newMemberEntityId);

                m_xuidToEntityIdMap[xuid] = newMemberEntityId;
                m_entityIdToXuidMap[newMemberEntityId] = xuid;
            }
        }
        else
        {
            DEBUGLOG("OnlineManager::OnMemberAdded: stateChange.member was invalid");
        }
    }
    else
    {
        LogError_PFMultiplayer("PFLobbyGetMemberProperty", hr);
    }
}

void OnlineManager::OnMemberRemoved(const PFLobbyStateChange& change)
{
    const auto& stateChange = static_cast<const PFLobbyMemberRemovedStateChange&>(change);

    const char* value = nullptr;
    HRESULT hr = PFLobbyGetMemberProperty(stateChange.lobby, &stateChange.member, "xuid", &value);
    if (SUCCEEDED(hr) && value != nullptr)
    {
        uint64_t xuid = std::stoull(value);

        DEBUGLOG("User %lu has left the session, removing from party", xuid);
    
        Managers::Get<PlayFabPartyManager>()->RemoveRemoteUser(xuid);
        m_xuidToEntityIdMap.erase(xuid);
        m_entityIdToXuidMap.erase(stateChange.member.id);
    }
    else
    {
        LogError_PFMultiplayer("PFLobbyGetMemberProperty", hr);
    }
}

void OnlineManager::OnLeaveLobbyCompleted(const PFLobbyStateChange&)
{
    m_myLobby = nullptr;
    m_xuidToEntityIdMap.clear();
    m_entityIdToXuidMap.clear();

    DeleteMPAActivity();
}

void OnlineManager::OnUpdated(const PFLobbyStateChange& change)
{
    const auto& StateChange = static_cast<const PFLobbyUpdatedStateChange&>(change);

    bool bUpdateActivity = false;
    HRESULT hr = S_OK;

    for (uint32_t i = 0; i < StateChange.updatedLobbyPropertyCount; ++i)
    {
        const char* updatedLobbyKey = StateChange.updatedLobbyPropertyKeys[i];
        const char* updatedLobbyPropertyValue;
        hr = PFLobbyGetLobbyProperty(StateChange.lobby, updatedLobbyKey, &updatedLobbyPropertyValue);
        if (SUCCEEDED(hr))
        {
            if (updatedLobbyPropertyValue == nullptr)
            {
                DEBUGLOG("FOnlineSessionPlayFab::OnUpdated Remove Key: %s", updatedLobbyKey);
            }
            else
            {
                DEBUGLOG("OnlineManager::OnUpdated: updatedLobbyKey: %s, updatedLobbyPropertyValue: %s", updatedLobbyKey, updatedLobbyPropertyValue);
            }
        }
        else
        {
            LogError_PFMultiplayer("PFLobbyGetLobbyProperty", hr);
        }
    }

    for (uint32_t i = 0; i < StateChange.updatedSearchPropertyCount; ++i)
    {
        const char* updatedSearchKey = StateChange.updatedSearchPropertyKeys[i];
        const char* updatedSearchPropertyValue;
        hr = PFLobbyGetSearchProperty(StateChange.lobby, updatedSearchKey, &updatedSearchPropertyValue);
        if (SUCCEEDED(hr))
        {
            if (updatedSearchPropertyValue == nullptr)
            {
                DEBUGLOG(("OnlineManager::OnUpdated: Remove Key: %s"), (updatedSearchKey));
            }
            else
            {
                DEBUGLOG(("OnlineManager::OnUpdated: Search Key: %s, value: %s"), (updatedSearchKey), (updatedSearchPropertyValue));
            }
        }
        else
        {
            LogError_PFMultiplayer("PFLobbyGetSearchProperty", hr);
        }
    }

    if (StateChange.ownerUpdated)
    {
        const PFEntityKey* OwnerPtr = nullptr;
        hr = PFLobbyGetOwner(StateChange.lobby, &OwnerPtr);
        if (SUCCEEDED(hr))
        {
            if (OwnerPtr != nullptr)
            {
                DEBUGLOG("OnlineManager::OnUpdated: new session owner: %s", EntityKeyToString(*OwnerPtr).c_str());
            }
            else
            {
                DEBUGLOG(("OnlineManager::OnUpdated: owner is removed from the session!"));
            }
        }
        else
        {
            LogError_PFMultiplayer("PFLobbyGetOwner", hr);
        }
    }

    if (StateChange.maxMembersUpdated)
    {
        uint32_t newMaxMemberCount;
        hr = PFLobbyGetMaxMemberCount(StateChange.lobby, &newMaxMemberCount);
        if (SUCCEEDED(hr))
        {
            bUpdateActivity = true;
            DEBUGLOG(("OnlineManager::OnUpdated: max member count updated to: %u"), newMaxMemberCount);
        }
        else
        {
            LogError_PFMultiplayer("PFLobbyGetMaxMemberCount", hr);
        }
    }

    if (StateChange.accessPolicyUpdated)
    {
        PFLobbyAccessPolicy NewAccessPolicy;
        hr = PFLobbyGetAccessPolicy(StateChange.lobby, &NewAccessPolicy);
        if (SUCCEEDED(hr))
        {
            bUpdateActivity = true;

            switch (NewAccessPolicy)
            {
            case PFLobbyAccessPolicy::Public: DEBUGLOG(("OnlineManager::OnUpdated: access policy updated to: Public")); break;
            case PFLobbyAccessPolicy::Friends: DEBUGLOG(("OnlineManager::OnUpdated: access policy updated to: Friends")); break;
            case PFLobbyAccessPolicy::Private: DEBUGLOG(("OnlineManager::OnUpdated: access policy updated to: Private")); break;
            }
        }
        else
        {
            LogError_PFMultiplayer("PFLobbyGetAccessPolicy", hr);
        }
    }

    DEBUGLOG(("OnlineManager::OnUpdated: member update count: %u"), StateChange.memberUpdateCount);
    for (uint32_t i = 0; i < StateChange.memberUpdateCount; ++i)
    {
        const PFLobbyMemberUpdateSummary& MemberUpdate = StateChange.memberUpdates[i];
        const PFEntityKey* MemberEntity = &MemberUpdate.member;
        
        for (uint32_t j = 0; j < MemberUpdate.updatedMemberPropertyCount; ++j)
        {
            const char* Key = MemberUpdate.updatedMemberPropertyKeys[j];
            const char* Value;
            hr = PFLobbyGetMemberProperty(StateChange.lobby, MemberEntity, Key, &Value);
            if (SUCCEEDED(hr))
            {
                DEBUGLOG("OnlineManager::OnUpdated: Update member property for: %s, Key: %s Value: %s", EntityKeyToString(*MemberEntity).c_str(), Key, Value);
            }
            else
            {
                LogError_PFMultiplayer("PFLobbyGetMemberProperty", hr);
            }
        }
    }

    if (bUpdateActivity)
    {
        SetMPAActivity();
    }

    if (Managers::Get<GameStateManager>()->GetState() == GameState::MigratingNetwork)
    {
        // We've received a new network to migrate to
        if (IsHost() == false)
        {
            MigrateToNewNetwork();
        }
    }
    else
    {
        // If we're not the host, try to connect to the network
        if (IsHost() == false)
        {
            FindAndConnectToNetwork();
        }
    }
}

void OnlineManager::OnPostUpdateCompleted(const PFLobbyStateChange& change)
{
    DEBUGLOG("OnlineManager::OnPostUpdateCompleted()");

    const auto& StateChange = static_cast<const PFLobbyPostUpdateCompletedStateChange&>(change);

    if (FAILED(StateChange.result))
    {
        LogError_PFMultiplayer("OnlineManager::OnPostUpdateCompleted", StateChange.result);
    }
}

void OnlineManager::OnDisconnecting(const PFLobbyStateChange& change)
{
    DEBUGLOG("OnlineManager::OnDisconnecting()");

    const auto& stateChange = static_cast<const PFLobbyDisconnectingStateChange&>(change);

    switch (stateChange.reason)
    {
    case PFLobbyDisconnectingReason::NoLocalMembers: DEBUGLOG("OnlineManager::OnDisconnecting: reason: NoLocalMembers"); break;
    case PFLobbyDisconnectingReason::LobbyDeleted:   DEBUGLOG("OnlineManager::OnDisconnecting: reason: LobbyDeleted");   break;
    default: DEBUGLOG("OnlineManager::OnDisconnecting: Unhandled disconnect reason"); break;
    }
}

void OnlineManager::OnJoinArrangedLobbyCompleted(const PFLobbyStateChange& change)
{
    const auto& stateChange = static_cast<const PFLobbyJoinArrangedLobbyCompletedStateChange&>(change);

    if (SUCCEEDED(stateChange.result))
    {
        m_myLobby = stateChange.lobby;

        if (IsHost())
        {
            CreatePlayFabParty();
        }
        else
        {
            FindAndConnectToNetwork();
        }
    }
    else
    {
        LogError_PFMultiplayer("PFLobbyJoinArrangedLobbyCompletedStateChange", stateChange.result);
        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
    }
}

void OnlineManager::OnFindLobbiesCompleted(const PFLobbyStateChange& change)
{
    DEBUGLOG("OnlineManager::OnFindLobbiesCompleted()");

    const auto& stateChange = static_cast<const PFLobbyFindLobbiesCompletedStateChange&>(change);

    if (SUCCEEDED(stateChange.result))
    {
        if (m_lobbySearchCallback != nullptr)
        {
            m_lobbySearchCallback(stateChange.searchResultCount, stateChange.searchResults);
        }
    }
    else
    {
        LogError_PFMultiplayer("PFLobbyFindLobbiesCompletedStateChange", stateChange.result);
        m_messageHandler(GetCurrentUserXuid(), &FindLobbiesFailed);
    }
}
#pragma endregion

#pragma region Matchmaking
void OnlineManager::StartMatchmaking() noexcept
{
    DEBUGLOG("OnlineManager::StartMatchmaking()");

    if (m_mpState != OnlineState::Ready)
    {
        DEBUGLOG("OnlineManager::StartMatchmaking: Multiplayer State is %d, expected %d.", m_mpState, OnlineState::Ready);
        m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
        return;
    }

    PFMatchmakingTicketConfiguration ticketConfig{};
    ticketConfig.timeoutInSeconds = SampleConfig::c_MatchTimeoutSeconds;
    ticketConfig.queueName = SampleConfig::c_MatchQueueName;

    // The array of local user attribute strings. There should be one attribute string for each local user.
    // Each attribute string should either be an empty string or a serialized JSON object.
    // For example, {"player_color":"blue","player_role":"tank"}.
    const char* localUserAttributes[]
    {
        "",
    };

    PFMatchmakingTicketHandle matchTicket{};
    PFEntityKey currentUserEntityKey = GetCurrentUserEntityKey();
    HRESULT hr = PFMultiplayerCreateMatchmakingTicket(m_pfmHandle, 1, &currentUserEntityKey, localUserAttributes, &ticketConfig, nullptr, &matchTicket);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerCreateMatchmakingTicket", hr);
        m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
    }
    else
    {
        m_mpState = OnlineState::Matchmaking;
        m_activeMatchmakingTicket = matchTicket;
    }
}

void OnlineManager::CancelMatchmaking() noexcept
{
    DEBUGLOG("OnlineManager::CancelMatchmaking()");

    if (m_mpState != OnlineState::Matchmaking)
    {
        DEBUGLOG("OnlineManager::CancelMatchmaking: Multiplayer state is %d, expected %d.", m_mpState, OnlineState::Matchmaking);
        return;
    }

    HRESULT hr = PFMultiplayerDestroyMatchmakingTicket(m_pfmHandle, m_activeMatchmakingTicket);
    if (SUCCEEDED(hr))
    {
        m_mpState = OnlineState::Canceling;
        m_messageHandler(GetCurrentUserXuid(), &MatchmakingCanceled);
    }
    else
    {
        LogError_PFMultiplayer("PFMultiplayerDestroyMatchmakingTicket", hr);
    }
}

void OnlineManager::ProcessMatchmakingStateChanges()
{
    uint32_t stateChangeCount;
    const PFMatchmakingStateChange * const * stateChanges;

    HRESULT hr = PFMultiplayerStartProcessingMatchmakingStateChanges(m_pfmHandle, &stateChangeCount, &stateChanges);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerStartProcessingMatchmakingStateChanges", hr);
        return;
    }

    for (uint32_t i = 0; i < stateChangeCount; ++i)
    {
        const PFMatchmakingStateChange& stateChange = *stateChanges[i];

        DEBUGLOG("MatchmakingStateChange: %s", GetPFMatchmakingStateChangeTypeString(stateChange.stateChangeType).c_str());
        switch (stateChange.stateChangeType)
        {
        case PFMatchmakingStateChangeType::TicketStatusChanged: OnTicketStatusChanged(stateChange); break;
        case PFMatchmakingStateChangeType::TicketCompleted:     OnTicketCompleted(stateChange);     break;
        }
    }

    hr = PFMultiplayerFinishProcessingMatchmakingStateChanges(m_pfmHandle, stateChangeCount, stateChanges);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFMultiplayerFinishProcessingMatchmakingStateChanges", hr);
    }
}

void OnlineManager::OnTicketStatusChanged(const PFMatchmakingStateChange& change)
{
    const auto& stateChange = static_cast<const PFMatchmakingTicketStatusChangedStateChange&>(change);

    PFMatchmakingTicketStatus status{};
    HRESULT hr = PFMatchmakingTicketGetStatus(stateChange.ticket, &status);
    if (SUCCEEDED(hr))
    {
        DEBUGLOG("Match Ticket Status is now: %s", GetPFMatchmakingTicketStatusString(status).c_str());
    }
    else
    {
        LogError_PFMultiplayer("PFMatchmakingTicketGetStatus", hr);
    }
}

void OnlineManager::OnTicketCompleted(const PFMatchmakingStateChange& change)
{
    const auto& stateChange = static_cast<const PFMatchmakingTicketCompletedStateChange&>(change);

    DEBUGLOG("OnlineManager::OnTicketCompleted: Ticket: 0x%p Completed.", stateChange.ticket);

    if (SUCCEEDED(stateChange.result))
    {
        PFMatchmakingTicketStatus status{};

        HRESULT hr = PFMatchmakingTicketGetStatus(stateChange.ticket, &status);
        if (SUCCEEDED(hr))
        {
            if (status == PFMatchmakingTicketStatus::Matched)
            {
                const PFMatchmakingMatchDetails* matchDetails = nullptr;
                hr = PFMatchmakingTicketGetMatch(stateChange.ticket, &matchDetails);
                if (SUCCEEDED(hr))
                {
                    DEBUGLOG("PFMatchmakingMatchDetails: MatchId: %s LobbyArrangementString: %s", matchDetails->matchId, matchDetails->lobbyArrangementString);

                    PropertyHelper memberProperties;
                    memberProperties.AddProperty("xuid", std::to_string(GetCurrentUserXuid()));

                    PFLobbyArrangedJoinConfiguration joinConfig{};
                    joinConfig.accessPolicy = PFLobbyAccessPolicy::Private;
                    joinConfig.maxMemberCount = SampleConfig::c_MaxPlayers;
                    joinConfig.ownerMigrationPolicy = PFLobbyOwnerMigrationPolicy::Automatic;

                    joinConfig.memberPropertyCount = memberProperties.GetCount();
                    joinConfig.memberPropertyKeys = memberProperties.GetKeyData();
                    joinConfig.memberPropertyValues = memberProperties.GetValueData();

                    PFEntityKey entityKey = GetCurrentUserEntityKey();
                    hr = PFMultiplayerJoinArrangedLobby(m_pfmHandle, &entityKey, matchDetails->lobbyArrangementString, &joinConfig, nullptr, &m_myLobby);
                    if (FAILED(hr))
                    {
                        LogError_PFMultiplayer("PFMultiplayerJoinArrangedLobby", hr);
                        m_messageHandler(GetCurrentUserXuid(), &JoinGameFailed);
                    }
                }
                else
                {
                    LogError_PFMultiplayer("PFMatchmakingTicketGetMatch", hr);
                    m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
                }
            }
            else if (status == PFMatchmakingTicketStatus::Canceled)
            {
                m_messageHandler(GetCurrentUserXuid(), &MatchmakingCanceled);
            }
            else if (status == PFMatchmakingTicketStatus::Failed)
            {
                m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
            }
        }
        else
        {
            LogError_PFMultiplayer("PFMatchmakingTicketGetStatus", hr);
            m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
        }
    }
    else
    {
        LogError_PFMultiplayer("PFMatchmakingTicketCompletedStateChange", stateChange.result);
        m_messageHandler(GetCurrentUserXuid(), &MatchmakingFailed);
    }
}
#pragma endregion

PFEntityKey OnlineManager::GetCurrentUserEntityKey() const
{
    auto partyMgr = Managers::Get<PlayFabPartyManager>();

    PFEntityKey entityKey;
    entityKey.id = partyMgr->GetCurrentUserEntityId();
    entityKey.type = "title_player_account";

    return entityKey;
}

uint32_t OnlineManager::GetLobbyMemberCount()
{
    DEBUGLOG("OnlineManager::GetLobbyMemberCount()");

    uint32_t currentMemberCount = 0;
    const PFEntityKey* lobbyMembers = nullptr;
    HRESULT hr = PFLobbyGetMembers(m_myLobby, &currentMemberCount, &lobbyMembers);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("GetLobbyMembers", hr);
    }

    return currentMemberCount;
}

const char* OnlineManager::GetLobbyConnectionString()
{
    DEBUGLOG("OnlineManager::GetLobbyConnectionString()");

    const char* connectionString = nullptr;
    HRESULT hr = PFLobbyGetConnectionString(m_myLobby, &connectionString);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFLobbyGetConnectionString", hr);
    }

    return connectionString;
}

bool OnlineManager::IsHost() noexcept
{
    bool isHost = false;

    if (m_myLobby)
    {
        const PFEntityKey* OwnerPtr = nullptr;
        HRESULT hr = PFLobbyGetOwner(m_myLobby, &OwnerPtr);
        if (SUCCEEDED(hr))
        {
            if (OwnerPtr)
            {
                PFEntityKey currentUserEntityKey = GetCurrentUserEntityKey();
                if (EntityKeysEqual(currentUserEntityKey, *OwnerPtr))
                {
                    isHost = true;
                }
                else
                {
                    isHost = false;
                }
            }
            else
            {
                DEBUGLOG("OnlineManager::IsHost: lobby does not have an owner");
            }
        }
        else
        {
            LogError_PFMultiplayer("PFLobbyGetOwner", hr);
        }
    }
    else
    {
        DEBUGLOG("OnlineManager::IsHost: lobby is null");
    }

    return isHost;
}

void OnlineManager::SetMPAActivity()
{
    DEBUGLOG("OnlineManager::MPASetActivity()");

    uint32_t currentMemberCount = GetLobbyMemberCount();

    const char* connectionString = GetLobbyConnectionString();

    uint32_t maxLobbyMemberCount = 0;
    HRESULT hr = PFLobbyGetMaxMemberCount(m_myLobby, &maxLobbyMemberCount);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFLobbyGetMaxMemberCount", hr);
        return;
    }

    const char* lobbyId = nullptr;
    hr = PFLobbyGetLobbyId(m_myLobby, &lobbyId);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFLobbyGetLobbyId", hr);
        return;
    }

    PFLobbyAccessPolicy NewAccessPolicy = PFLobbyAccessPolicy::Friends;
    hr = PFLobbyGetAccessPolicy(m_myLobby, &NewAccessPolicy);
    if (FAILED(hr))
    {
        LogError_PFMultiplayer("PFLobbyGetAccessPolicy", hr);
    }

    XblMultiplayerActivityJoinRestriction joinRestriction = XblMultiplayerActivityJoinRestriction::Followed;
    switch (NewAccessPolicy)
    {
    case PFLobbyAccessPolicy::Public:  joinRestriction = XblMultiplayerActivityJoinRestriction::Public;     break;
    case PFLobbyAccessPolicy::Friends: joinRestriction = XblMultiplayerActivityJoinRestriction::Followed;   break;
    case PFLobbyAccessPolicy::Private: joinRestriction = XblMultiplayerActivityJoinRestriction::InviteOnly; break;
    }

    Managers::Get<MPAManager>()->SetActivity(
        m_xblContext,
        connectionString,
        joinRestriction,
        maxLobbyMemberCount,
        currentMemberCount,
        lobbyId,
        true,
        [](bool bSuccess)
        {
            if (bSuccess)
            {
                DEBUGLOG("OnlineManager::SetMPAActivity: Successfully set MPA activity");
            }
            else
            {
                DEBUGLOG("OnlineManager::SetMPAActivity: Failed to set MPA activity");
            }
        });
}

void OnlineManager::DeleteMPAActivity()
{
    Managers::Get<MPAManager>()->DeleteActivity(m_xblContext, [](bool bSuccess)
    {
        if (bSuccess)
        {
            DEBUGLOG("OnlineManager::DeleteMPAActivity: Successfully deleted MPA activity");
        }
        else
        {
            DEBUGLOG("OnlineManager::DeleteMPAActivity: Failed to delete MPA activity");
        }
    });
}


