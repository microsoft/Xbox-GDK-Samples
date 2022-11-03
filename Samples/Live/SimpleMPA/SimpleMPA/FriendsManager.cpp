//--------------------------------------------------------------------------------------
// File: FriendsManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "FriendsManager.h"

namespace
{
    std::string GetXblSocialManagerEventTypeString(XblSocialManagerEventType eventType)
    {
        switch (eventType)
        {
        case XblSocialManagerEventType::UsersAddedToSocialGraph:     return STRINGIFY(XblSocialManagerEventType::UsersAddedToSocialGraph);
        case XblSocialManagerEventType::UsersRemovedFromSocialGraph: return STRINGIFY(XblSocialManagerEventType::UsersRemovedFromSocialGraph);
        case XblSocialManagerEventType::PresenceChanged:             return STRINGIFY(XblSocialManagerEventType::PresenceChanged);
        case XblSocialManagerEventType::ProfilesChanged:             return STRINGIFY(XblSocialManagerEventType::ProfilesChanged);
        case XblSocialManagerEventType::SocialRelationshipsChanged:  return STRINGIFY(XblSocialManagerEventType::SocialRelationshipsChanged);
        case XblSocialManagerEventType::LocalUserAdded:              return STRINGIFY(XblSocialManagerEventType::LocalUserAdded);
        case XblSocialManagerEventType::SocialUserGroupLoaded:       return STRINGIFY(XblSocialManagerEventType::SocialUserGroupLoaded);
        case XblSocialManagerEventType::SocialUserGroupUpdated:      return STRINGIFY(XblSocialManagerEventType::SocialUserGroupUpdated);
        case XblSocialManagerEventType::UnknownEvent:                return STRINGIFY(XblSocialManagerEventType::UnknownEvent);
        }

        //we should never get here
        assert(false);
        return "Unknown enumeration value";
    }
}

void FriendsManager::AddUserToSocialManager(XblContextHandle xblContext, XUserHandle user)
{
    DEBUGLOG("FriendsManager::AddUserToSocialManager()");

    XblContextDuplicateHandle(xblContext, &m_xblContext); //XUserCloseHandle;
    XUserDuplicateHandle(user, &m_user);
    

    m_xblContext = xblContext;
    m_user = user;

    // Add the local user, no extra details
    HRESULT hr = XblSocialManagerAddLocalUser(m_user, XblSocialManagerExtraDetailLevel::NoExtraDetail, nullptr);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblSocialManagerAddLocalUser", hr);
        return;
    }

    // create 4 different groups with various filters.
    // users in these groups will be filled in by the XblSocialManagerDoWork call below.
    CreateSocialGroupFromFilters(allFriends, XblPresenceFilter::All, XblRelationshipFilter::Friends);
    CreateSocialGroupFromFilters(allOnlineFriends, XblPresenceFilter::AllOnline, XblRelationshipFilter::Friends);
    CreateSocialGroupFromFilters(allFriendsInGame, XblPresenceFilter::TitleOnline, XblRelationshipFilter::Friends);
    CreateSocialGroupFromFilters(allFavorites, XblPresenceFilter::All, XblRelationshipFilter::Favorite);
}

void FriendsManager::RemoveUserFromSocialManager(XUserHandle user)
{
    DEBUGLOG("FriendsManager::RemoveUserFromSocialManager()");

    if (XUserCompare(m_user, user))
    {
        DestroySocialGroup(allFriends);
        DestroySocialGroup(allOnlineFriends);
        DestroySocialGroup(allFriendsInGame);
        DestroySocialGroup(allFavorites);

        // Remove local user
        HRESULT hr = XblSocialManagerRemoveLocalUser(m_user);
        if (FAILED(hr))
        {
            LogError_HRESULT("XblSocialManagerRemoveLocalUser", hr);
        }

        XblContextCloseHandle(m_xblContext);
        m_xblContext = nullptr;

        XUserCloseHandle(m_user);
        m_user = nullptr;
    }
}

void FriendsManager::CreateSocialGroupFromFilters(XblSocialManagerUserGroupHandle& group, XblPresenceFilter presenceFilter, XblRelationshipFilter relationshipFilter)
{
    DEBUGLOG("FriendsManager::CreateSocialGroupFromFilters()");

    HRESULT hr = XblSocialManagerCreateSocialUserGroupFromFilters(m_user, presenceFilter, relationshipFilter, &group);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblSocialManagerCreateSocialUserGroupFromFilters", hr);
    }
}

void FriendsManager::DestroySocialGroup(XblSocialManagerUserGroup* group)
{
    DEBUGLOG("FriendsManager::DestroySocialGroup()");

    HRESULT hr = XblSocialManagerDestroySocialUserGroup(group);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblSocialManagerDestroySocialUserGroup", hr);
    }
}

void FriendsManager::DoWork()
{
    const XblSocialManagerEvent* events = nullptr;
    size_t count = 0;
    XblSocialManagerDoWork(&events, &count);

    for (size_t i = 0; i < count; ++i)
    {
        const XblSocialManagerEvent& socialEvent = events[i];

        DEBUGLOG("Social Manager Event: %s", GetXblSocialManagerEventTypeString(socialEvent.eventType).c_str());

        switch (socialEvent.eventType)
        {
        case XblSocialManagerEventType::UsersAddedToSocialGraph: UpdateFriendDisplayNames(socialEvent); break;
        case XblSocialManagerEventType::SocialUserGroupLoaded:   UpdateFriendDisplayNames(socialEvent); break;
        case XblSocialManagerEventType::SocialUserGroupUpdated:  UpdateFriendDisplayNames(socialEvent); break;

            //The sample does not currently react to these events
        case XblSocialManagerEventType::LocalUserAdded:              break;
        case XblSocialManagerEventType::UsersRemovedFromSocialGraph: break;
        case XblSocialManagerEventType::SocialRelationshipsChanged:  break;
        case XblSocialManagerEventType::PresenceChanged:             break;
        case XblSocialManagerEventType::ProfilesChanged:             break;
        case XblSocialManagerEventType::UnknownEvent:                break;
        }
    }
}

uint64_t FriendsManager::GetXuidForXUserHandle(XUserHandle user)
{
    DEBUGLOG("FriendsManager::GetXuidForXUserHandle()");

    uint64_t xuid = 0;
    HRESULT hr = XUserGetId(user, &xuid);
    if (FAILED(hr))
    {
        LogError_HRESULT("XUserGetId", hr);
    }
    return xuid;
}

void FriendsManager::UpdateFriendDisplayNames(const XblSocialManagerEvent& socialEvent)
{
    DEBUGLOG("FriendsManager::UpdateFriendDisplayNames()");

    for (uint32_t i = 0; i < XBL_SOCIAL_MANAGER_MAX_AFFECTED_USERS_PER_EVENT; i++)
    {
        if (XblSocialManagerUser* user = socialEvent.usersAffected[i])
        {
            DEBUGLOG("Display name updated: xuid: %llu name: %s", user->xboxUserId, user->displayName);
            m_xuidToDisplayNameMap[user->xboxUserId] = user->displayName;
        }
    }
}

void FriendsManager::ReadUserDisplayNameAsync(uint64_t xuid, std::function<void(uint64_t)> callback)
{
    DEBUGLOG("FriendsManager::ReadUserDisplayNameAsync()");

    if (xuid == 0)
    {
        DEBUGLOG("FriendsManager::ReadUserDisplayNameAsync: xuid was invalid");
        return;
    }

    if (callback == nullptr)
    {
        DEBUGLOG("FriendsManager::ReadUserDisplayNameAsync: callback was null");
        return;
    }

    auto name = m_xuidToDisplayNameMap.find(xuid);
    if (name != m_xuidToDisplayNameMap.end())
    {
        callback(xuid);
        return;
    }

    auto asyncHelper = new ATG::AsyncHelper(nullptr, [this, callback](XAsyncBlock* async)
    {
        XblUserProfile profile{};
        HRESULT hr = XblProfileGetUserProfileResult(async, &profile);
        if (SUCCEEDED(hr))
        {
            DEBUGLOG("Display name updated: xuid: %llu name: %s", profile.xboxUserId, profile.gameDisplayName);

            m_xuidToDisplayNameMap[profile.xboxUserId] = profile.gameDisplayName;

            if (callback)
            {
                callback(profile.xboxUserId);
            }
        }
        else
        {
            LogError_HRESULT("XblProfileGetUserProfileResult", hr);
        }
    });

    HRESULT hr = XblProfileGetUserProfileAsync(m_xblContext, xuid, &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblProfileGetUserProfileAsync", hr);
        delete asyncHelper;
    }
}

void FriendsManager::ReadUserDisplayNamesAsync(uint64_t* xboxUserIds, size_t xboxUserIdsCount, std::function<void(const std::vector<uint64_t> &)> callback)
{
    DEBUGLOG("FriendsManager::ReadUserDisplayNamesAsync()");

    if (xboxUserIds == nullptr)
    {
        DEBUGLOG("FriendsManager::ReadUserDisplayNamesAsync: xboxUserIds was invalid");
        return;
    }

    if (xboxUserIdsCount == 0)
    {
        DEBUGLOG("FriendsManager::ReadUserDisplayNamesAsync: xboxUserIdsCount was zero");
        return;
    }

    if (callback == nullptr)
    {
        DEBUGLOG("FriendsManager::ReadUserDisplayNamesAsync: callback was null");
        return;
    }

    auto asyncHelper = new ATG::AsyncHelper(nullptr, [this, callback](XAsyncBlock* async)
    {
        size_t count = 0;
        HRESULT hr = XblProfileGetUserProfilesResultCount(async, &count);
        if (SUCCEEDED(hr))
        {
            std::vector<XblUserProfile> profiles;
            profiles.resize(count);

            hr = XblProfileGetUserProfilesResult(async, count, profiles.data());
            if (SUCCEEDED(hr))
            {
                std::vector<uint64_t> users;
                users.reserve(count);

                for (auto&& profile : profiles)
                {
                    DEBUGLOG("Display name updated: xuid: %llu name: %s", profile.xboxUserId, profile.gameDisplayName);

                    m_xuidToDisplayNameMap[profile.xboxUserId] = profile.gameDisplayName;
                    users.emplace_back(profile.xboxUserId);
                }

                callback(users);
            }
            else
            {
                LogError_HRESULT("XblProfileGetUserProfilesResult", hr);
            }
        }
        else
        {
            LogError_HRESULT("XblProfileGetUserProfilesResultCount", hr);
        }
    });

    HRESULT hr = XblProfileGetUserProfilesAsync(m_xblContext, xboxUserIds, xboxUserIdsCount, &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblProfileGetUserProfilesAsync", hr);
        delete asyncHelper;
    }
}

std::string FriendsManager::GetUserDisplayName(uint64_t xuid)
{
    std::string userDisplayName;

    auto name = m_xuidToDisplayNameMap.find(xuid);
    if (name != m_xuidToDisplayNameMap.end())
    {
        userDisplayName = name->second;
    }

    return userDisplayName;
}

void FriendsManager::GetFriends(std::vector<uint64_t>& outFriends)
{
    DEBUGLOG("FriendsManager::GetFriends()");

    GetUsers(allFriends, outFriends);
}

void FriendsManager::GetOnlineFriends(std::vector<uint64_t>& outFriends)
{
    DEBUGLOG("FriendsManager::GetOnlineFriends()");

    GetUsers(allOnlineFriends, outFriends);
}

void FriendsManager::GetFriendsInGame(std::vector<uint64_t>& outFriends)
{
    DEBUGLOG("FriendsManager::GetFriendsInGame()");

    GetUsers(allFriendsInGame, outFriends);
}

void FriendsManager::GetFavorites(std::vector<uint64_t>& outFriends)
{
    DEBUGLOG("FriendsManager::GetFavorites()");

    GetUsers(allFavorites, outFriends);
}

void FriendsManager::GetUsers(XblSocialManagerUserGroupHandle group, std::vector<uint64_t>& outFriends)
{
    DEBUGLOG("FriendsManager::GetUsers()");

    size_t userCount = 0;
    const XblSocialManagerUser* const* rawUsers = nullptr;
    HRESULT hr = XblSocialManagerUserGroupGetUsers(group, &rawUsers, &userCount);
    if (SUCCEEDED(hr))
    {
        outFriends.reserve(userCount);

        for (size_t i = 0; i < userCount; ++i)
        {
            outFriends.push_back(rawUsers[i]->xboxUserId);
        }
    }
    else
    {
        LogError_HRESULT("XblSocialManagerUserGroupGetUsers", hr);
    }
}
