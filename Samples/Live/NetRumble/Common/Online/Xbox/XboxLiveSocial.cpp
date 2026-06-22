// TODO: Clean up file structure

#include "pch.h"
#include "ArrayView.h"
#include "XboxLiveOnlineManager.h"

using namespace NetRumble;

void XboxLiveOnlineManager::SocialInit()
{
    XblSocialManagerAddLocalUser(GetCurrentUserHandle().get(), XblSocialManagerExtraDetailLevel::NoExtraDetail, Managers::Get<AsyncTaskManager>()->GetDefaultQueue().get());
}

void XboxLiveOnlineManager::SocialTick()
{
    const XblSocialManagerEvent *events = nullptr;
    size_t count = 0;
    XblSocialManagerDoWork(&events, &count);

    auto view = ATG::ArrayView<const XblSocialManagerEvent>(events, count);

    for (auto &&socialEvent : view)
    {
        switch(socialEvent.eventType)
        {
        case XblSocialManagerEventType::LocalUserAdded:
        {
            XblSocialManagerCreateSocialUserGroupFromFilters(GetCurrentUserHandle().get(), XblPresenceFilter::AllOnline, XblRelationshipFilter::Friends, &m_socialGroup);
            break;
        }
        case XblSocialManagerEventType::UsersAddedToSocialGraph:
        {
            UpdateFriendDisplayNames();
            break;
        }
        case XblSocialManagerEventType::UsersRemovedFromSocialGraph:
            break;
        case XblSocialManagerEventType::SocialRelationshipsChanged:
            break;
        case XblSocialManagerEventType::SocialUserGroupLoaded:
            UpdateFriendDisplayNames();
            break;
        case XblSocialManagerEventType::SocialUserGroupUpdated:
            UpdateFriendDisplayNames();
            break;
        default:
            break;
        }
    }
}

void XboxLiveOnlineManager::SocialCleanup()
{
    if (m_socialGroup)
    {
        XblSocialManagerDestroySocialUserGroup(m_socialGroup);
        m_socialGroup = nullptr;
    }

    XblSocialManagerRemoveLocalUser(GetCurrentUserHandle().get());
}

bool XboxLiveOnlineManager::HasForDisplayNameCached(uint64_t id)
{
    return m_xuidToDisplayNameMap.find(id) != m_xuidToDisplayNameMap.end();
}

void XboxLiveOnlineManager::UpdateFriendDisplayNames()
{
    size_t userCount = 0;
    const XblSocialManagerUser* const* rawUsers = nullptr;

    XblSocialManagerUserGroupGetUsers(m_socialGroup,  &rawUsers, &userCount);

    if (userCount == 0)
    {
        return;
    }

    ATG::ArrayView<const XblSocialManagerUser* const> users(rawUsers, userCount);

    for (auto &&user : users)
    {
        m_xuidToDisplayNameMap[user->xboxUserId] = user->displayName;
    }
}

void XboxLiveOnlineManager::UpdateDisplayName(uint64_t id, std::string_view name)
{
    m_xuidToDisplayNameMap[id] = name;
}

HRESULT XboxLiveOnlineManager::QueryUserDisplayNameAsync(uint64_t id, XTaskQueueHandle taskQueue, std::function<void(const OnlineUser &)> then)
{
    auto name = m_xuidToDisplayNameMap.find(id);
    if (name != m_xuidToDisplayNameMap.end())
    {
        then(OnlineUser{ name->second, name->first });
        return S_OK;
    }

    return ATG::AsyncHelper(&XblProfileGetUserProfileAsync).Invoke(m_context.get(), id, [this, then = std::move(then)](XAsyncBlock *async)
        {
            XblUserProfile profile = {};
            auto result = XblProfileGetUserProfileResult(async, &profile);

            if (SUCCEEDED(result))
            {
                UpdateDisplayName(profile.xboxUserId, profile.gameDisplayName);
                then(OnlineUser{ profile.gameDisplayName, profile.xboxUserId });
            }
        }, taskQueue);
}

HRESULT XboxLiveOnlineManager::QueryUserDisplayNamesAsync(uint64_t *ids, size_t count, XTaskQueueHandle taskQueue, std::function<void(const std::vector<OnlineUser> &)> then)
{
    return ATG::AsyncHelper(&XblProfileGetUserProfilesAsync).Invoke(m_context.get(), ids, count, [this, then = std::move(then)](XAsyncBlock *async)
    {
        size_t count = 0;
        auto result = XblProfileGetUserProfilesResultCount(async, &count);

        if (SUCCEEDED(result))
        {
            std::vector<XblUserProfile> profiles;
            profiles.resize(count);

            XblProfileGetUserProfilesResult(async, count, profiles.data());

            std::vector<OnlineUser> users;
            users.reserve(count);

            for (auto &&profile : profiles)
            {
                UpdateDisplayName(profile.xboxUserId, profile.gameDisplayName);
                users.emplace_back(OnlineUser{ profile.gameDisplayName, profile.xboxUserId });
            }

            then(users);
        }
    }, taskQueue);
}
