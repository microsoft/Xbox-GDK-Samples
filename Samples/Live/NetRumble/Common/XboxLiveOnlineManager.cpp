#include "pch.h"
#include "Game.h"
#include "XboxLiveOnlineManager.h"
#include "XboxUser.h"
#include "Managers.h"
#include "XboxConfig.h"
#include "GameScreen.h"

using namespace NetRumble;

namespace
{
    GameMessage PlayerJoined(GameMessageType::PlayerJoined, 0);
    GameMessage PlayerLeft(GameMessageType::PlayerLeft, 0);
    GameMessage MultiplayerPrivilegeError(GameMessageType::MPPrivilegeError, 0);
    GameMessage CrossplayPrivilegeError(GameMessageType::CPPrivilegeError, 0);
}

XboxLiveOnlineManager::XboxLiveOnlineManager() noexcept(false) :
    m_hasMultiplayerPrivileges(false),
    m_hasCrossplayPrivileges(false),
    m_isMatchFound(false),
    m_mpState(OnlineState::Ready),
    m_networkAvailableToken{},
    m_inviteRegistration{},
    m_signInCallbackToken(0),
    m_signOutCallbackToken(0),
    m_socialGroup(nullptr)
{
}

XboxLiveOnlineManager::~XboxLiveOnlineManager()
{
}

void XboxLiveOnlineManager::Initialize()
{
    XblInitArgs xblInit = { nullptr, XboxConfig::c_ServiceConfigId };
    auto hr = XblInitialize(&xblInit);
    DX::ThrowIfFailed(hr);

    auto userMgr = Managers::Get<XboxUserManager>();
    m_signInCallbackToken = userMgr->AddUserSignedInCallback(
        [this](const User &user)
        {
            auto xboxUser = static_cast<const XboxUser&>(user);

            InitializeXboxLive(xboxUser);
        });

    m_signOutCallbackToken = userMgr->AddUserSignedOutCallback(
        [this](const User &user)
        {
            if (m_context)
            {
                ATG::XboxHandle<XUserHandle> currentUser = {};
                XblContextGetUser(m_context.get(), currentUser.get_address_of());

                if (XUserCompare(currentUser.get(), static_cast<const XboxUser&>(user)))
                {
                    m_context.clear();
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

                auto memberCount = XblMultiplayerManagerGameSessionMembersCount();

                // If we've migrated everyone (+1 for the local user) we can begin the game
                if (m_migratedXuids.size() + 1 == memberCount)
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
                DEBUGLOG("Removing Peer %llu\n", xuid);
            }
            else
            {
                m_messageHandler(xuid, &PlayerJoined);
                DEBUGLOG("Adding Peer %llu\n", xuid);
            }
        }
    });

    auto asyncManager = Managers::Get<AsyncTaskManager>();

    XNetworkingRegisterConnectivityHintChanged(
        asyncManager->GetDefaultQueue().get(),
        this,
        [](void* context, _In_ const XNetworkingConnectivityHint* connectivityHint)
        {
            auto mgr = reinterpret_cast<XboxLiveOnlineManager*>(context);
            if (connectivityHint->networkInitialized)
            {
                if (nullptr != mgr->m_networkAvailableCallback)
                {
                    mgr->m_networkAvailableCallback();
                }
            }
        },
        &m_networkAvailableToken);
}

void XboxLiveOnlineManager::ShutdownAsync(std::function<void()> completionCallback)
{
    DEBUGLOG("XboxLiveOnlineManager::ShutdownAsync\n");

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

    // snippet from the GDK docs under the section named "Cleaning up XSAPI" on the
    // page entitled "Getting started with Xbox Live APIs"

    XAsyncBlock* asyncCleanupBlock = new XAsyncBlock
    {
        asyncManager->GetAsyncCompletionQueue().get(),
        new std::function<void()>(completionCallback),
        [](XAsyncBlock* asyncBlock)
        {
            DEBUGLOG("XblCleanupAsync::Lambda\n");
            auto callbackMethod = reinterpret_cast<std::function<void()>*>(asyncBlock->context);
            (*callbackMethod)();
            delete callbackMethod;
            delete asyncBlock;
        }
    };

    XblCleanupAsync(asyncCleanupBlock);
}

bool XboxLiveOnlineManager::IsNetworkAvailable()
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

void XboxLiveOnlineManager::SetNetworkAvailableCallback(std::function<void()> callback)
{
    m_networkAvailableCallback = callback;
}

bool XboxLiveOnlineManager::HasMultiplayerPrivileges() const
{
    return m_hasMultiplayerPrivileges;
}

bool XboxLiveOnlineManager::HasCrossplayPrivileges() const
{
    return m_hasCrossplayPrivileges;
}

void XboxLiveOnlineManager::Tick(float)
{
    SocialTick();
    MultiplayerTick();
}

void XboxLiveOnlineManager::OnNetworkLost()
{
    m_context.clear();
}

void XboxLiveOnlineManager::UpdateStatistic(std::string_view name, int value)
{
    UNREFERENCED_PARAMETER(name);
    UNREFERENCED_PARAMETER(value);
    // TODO: Event service is not yet available

    //XblEventsWriteInGameEvent(m_context, name.data(), "", "");
}

void XboxLiveOnlineManager::SendTelemetry(std::string_view eventName)
{
    UNREFERENCED_PARAMETER(eventName);
    // TODO: Event service is not yet available
}

void XboxLiveOnlineManager::MigrateToRegion(const std::vector<std::string>& regionList, std::function<void(bool)> callback)
{
    Managers::Get<PlayFabPartyManager>()->MigrateToRegion(
        regionList,
        [this, callback](bool success, const char* descriptor)
        {
            if (success)
            {
                m_networkDescriptor = descriptor;

                // Set the values in the session so all the other clients can find and join the new Party session
                SetSessionProperty("descriptor", m_networkDescriptor.c_str());
            }

            if (callback != nullptr)
            {
                callback(success);
            }
        });
}

void XboxLiveOnlineManager::InitializeXboxLive(ATG::XboxHandle<XUserHandle> user)
{
    auto asyncManager = Managers::Get<AsyncTaskManager>();
    HRESULT hr = XblContextCreateHandle(user.get(), m_context.release_and_get_address_of());
    DX::ThrowIfFailed(hr);
    m_user = user;

    SocialInit();

    // Perform the multiplayer privilege check
    XUserPrivilegeDenyReason reason = {};
    XUserCheckPrivilege(user.get(), XUserPrivilegeOptions::None, XUserPrivilege::Multiplayer, &m_hasMultiplayerPrivileges, &reason);

    if (!m_hasMultiplayerPrivileges)
    {
        // If the bool is false then the user doesn't have permission for some reason.  We log the reason, but do not use it in
        // game logic.  Instead call the Resolve function to let the system figure out what the user must do.
        DEBUGLOG("XUserCheckPrivilege failed: %d, attempting resolution\n", reason);

        hr = ATG::AsyncHelper(&XUserResolvePrivilegeWithUiAsync).Invoke(user.get(), XUserPrivilegeOptions::None, XUserPrivilege::Multiplayer,
            [this, user](XAsyncBlock* async)
            {
                HRESULT hr = XUserResolvePrivilegeWithUiResult(async);

                if (SUCCEEDED(hr))
                {
                    DEBUGLOG("Privilege resolved.\n");
                    m_hasMultiplayerPrivileges = true;
                    Managers::Get<ScreenManager>()->GetCurrentGameScreen()->Reset();
                    CheckCrossplayPrivilege(user);
                }
                else
                {
                    DEBUGLOG("Privilege not resolved.\n");
                    // Message back to game that user cannot do MP.
                    if (m_messageHandler)
                    {
                        m_messageHandler(GetCurrentUserXuid(), &MultiplayerPrivilegeError);
                    }
                }
            },
            Managers::Get<AsyncTaskManager>()->GetDefaultQueue().get());

        if (FAILED(hr))
        {
            DEBUGLOG("XUserResolvePrivilegeWithUiAsync failed: 0x%x\n", hr);

            if (m_messageHandler)
            {
                m_messageHandler(GetCurrentUserXuid(), &MultiplayerPrivilegeError);
            }
        }
    }
    else
    {
        CheckCrossplayPrivilege(user);
    }

    // register for invite events and handle them when they come in
    XGameActivationRegisterForEvent(
        asyncManager->GetDefaultQueue().get(),
        this,
        [](void* context, const XGameActivationInfo* activationInfo)
        {
            if (activationInfo->type != XGameActivationType::AcceptedGameInvite)
            {
                return;
            }

            auto pThis = reinterpret_cast<XboxLiveOnlineManager*>(context);
            
            // We need to parse the handle value out of the uri
            // ms-xbl-76b1590e://inviteHandleAccept/?invitedXuid=2814645439730606&handle=27a705f6-f080-4aa7-8bd7-ec8c51befd3d&senderXuid=2814626418925179
            std::string uri = activationInfo->inviteUri;

            auto pos = uri.find("handle=") + 7;
            auto end = uri.find('&', pos);

            // If the session is at the end of the string then end will return not found.
            if (end == std::string::npos)
            {
                end = uri.length() + 1;
            }

            std::string handle = uri.substr(pos, end - pos);

            pThis->JoinGameFromInvite(handle.c_str());

        },
        &m_inviteRegistration);
}

void XboxLiveOnlineManager::CheckCrossplayPrivilege(ATG::XboxHandle<XUserHandle> user)
{
    auto asyncManager = Managers::Get<AsyncTaskManager>();

    // Perform the crossplay privilege check
    XUserPrivilegeDenyReason reason = {};
    XUserCheckPrivilege(user.get(), XUserPrivilegeOptions::None, XUserPrivilege::CrossPlay, &m_hasCrossplayPrivileges, &reason);

    if (!m_hasCrossplayPrivileges)
    {
        HRESULT hr = ATG::AsyncHelper(&XUserResolvePrivilegeWithUiAsync).Invoke(user.get(), XUserPrivilegeOptions::None, XUserPrivilege::CrossPlay,
            [this](XAsyncBlock* async)
            {
                HRESULT hr = XUserResolvePrivilegeWithUiResult(async);

                if (SUCCEEDED(hr))
                {
                    DEBUGLOG("Privilege resolved.\n");
                    m_hasCrossplayPrivileges = true;
                    XblMultiplayerManagerInitialize(XboxConfig::c_MPSDLobbyTemplate, Managers::Get<AsyncTaskManager>()->GetDefaultQueue().get());
                    Managers::Get<ScreenManager>()->GetCurrentGameScreen()->Reset();
                }
                else
                {
                    DEBUGLOG("Privilege not resolved.\n");
                    // Message back to game that user cannot do MP.
                    if (m_messageHandler)
                    {
                        m_messageHandler(GetCurrentUserXuid(), &MultiplayerPrivilegeError);
                    }
                }
            },
            Managers::Get<AsyncTaskManager>()->GetDefaultQueue().get());

        if (FAILED(hr))
        {
            DEBUGLOG("XUserResolvePrivilegeWithUiAsync failed: 0x%x\n", hr);

            if (m_messageHandler)
            {
                m_messageHandler(GetCurrentUserXuid(), &MultiplayerPrivilegeError);
            }
        }
    }
    else
    {
        XblMultiplayerManagerInitialize(XboxConfig::c_MPSDLobbyTemplate, asyncManager->GetDefaultQueue().get());
    }
}

ATG::XboxHandle<XblUserHandle> XboxLiveOnlineManager::GetCurrentUserHandle() const
{
    return m_user;
}

uint64_t XboxLiveOnlineManager::GetCurrentUserXuid() const
{
    uint64_t xuid = {};

    if (m_context)
    {
        XblContextGetXboxUserId(m_context.get(), &xuid);
    }

    return xuid;
}

void XboxLiveOnlineManager::ShowInviteUI()
{
    XblMultiplayerManagerLobbySessionInviteFriends(GetCurrentUserHandle().get(), nullptr, nullptr);
}


HRESULT XboxLiveOnlineManager::GetInGameFriendsAsync(XTaskQueueHandle taskQueue, std::function<void(std::vector<std::shared_ptr<OnlineUser>> &)> then)
{
    return ATG::AsyncHelper(&XblMultiplayerGetActivitiesForSocialGroupAsync).Invoke(m_context.get(), XboxConfig::c_ServiceConfigId, GetCurrentUserXuid(), "People",
        [this, then = std::move(then)](XAsyncBlock *async)
        {
            size_t count = 0;
            std::vector<XblMultiplayerActivityDetails> activites;

            XblMultiplayerGetActivitiesForSocialGroupResultCount(async, &count);
            activites.resize(count);

            XblMultiplayerGetActivitiesForSocialGroupResult(async, count, activites.data());

            std::vector<uint64_t> uncachedUsers;
            std::vector<std::shared_ptr<OnlineUser>> onlineUserActivity;
            onlineUserActivity.resize(count);
            std::transform(activites.begin(), activites.end(), onlineUserActivity.begin(), [&](const XblMultiplayerActivityDetails &details)
                {
                    auto xblOnlineUser = std::make_shared<XboxLiveOnlineUser>();
                    xblOnlineUser->Id = details.OwnerXuid;
                    xblOnlineUser->mpActivity = details;

                    if (HasForDisplayNameCached(details.OwnerXuid))
                    {
                        QueryUserDisplayNameAsync(details.OwnerXuid, async->queue, [xblOnlineUser](const OnlineUser &user)
                            {
                                xblOnlineUser->Name = user.Name;
                            });
                    }
                    else
                    {
                        xblOnlineUser->Name = "Friend";
                        uncachedUsers.push_back(details.OwnerXuid);
                    }

                    return std::static_pointer_cast<OnlineUser>(xblOnlineUser);
                });

            then(onlineUserActivity);

            if (!uncachedUsers.empty())
            {
                std::sort(onlineUserActivity.begin(), onlineUserActivity.end(), [](auto &&left, auto &&right) {
                    return left->Id < right->Id;
                    });
                QueryUserDisplayNamesAsync(uncachedUsers.data(), uncachedUsers.size(), async->queue, [onlineUserActivity, then](const std::vector<OnlineUser> &users)
                    {
                        for (auto &&user : users)
                        {
                            auto result = std::find_if(onlineUserActivity.begin(), onlineUserActivity.end(), [user](const std::shared_ptr<OnlineUser> &onlineUser) {
                                return user.Id == onlineUser->Id;
                                });

                            if (result != onlineUserActivity.end())
                            {
                                (*result)->Name = user.Name;
                            }
                        }
                    });
            }
        }, taskQueue);
}
