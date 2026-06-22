//--------------------------------------------------------------------------------------
// FriendsManager.cpp
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

namespace PlayFabMultiplayerRumble
{
    FriendsManager::~FriendsManager()
    {
        if (m_user)
        {
            RemoveUserFromSocialManager(m_user);
        }
    }

    void FriendsManager::AddUserToSocialManager(ATG::XboxHandle<XblContextHandle> xblContext, ATG::XboxHandle<XUserHandle> user)
    {
        DEBUGLOG("FriendsManager::AddUserToSocialManager()");

        m_xblContext = xblContext;
        m_user = user;

        // Add the local user, no extra details
        HRESULT hr = XblSocialManagerAddLocalUser(m_user.get(), XblSocialManagerExtraDetailLevel::NoExtraDetail, nullptr);
        if (FAILED(hr))
        {
            LogError_HRESULT("XblSocialManagerAddLocalUser", hr);
            return;
        }

        // create 4 different groups with various filters.
        // users in these groups will be filled in by the XblSocialManagerDoWork call below.
        CreateSocialGroupFromFilters(allFriends,       XblPresenceFilter::All,         XblRelationshipFilter::Friends);
        CreateSocialGroupFromFilters(allOnlineFriends, XblPresenceFilter::AllOnline,   XblRelationshipFilter::Friends);
        CreateSocialGroupFromFilters(allFriendsInGame, XblPresenceFilter::TitleOnline, XblRelationshipFilter::Friends);
        CreateSocialGroupFromFilters(allFavorites,     XblPresenceFilter::All,         XblRelationshipFilter::Favorite);
    }

    void FriendsManager::RemoveUserFromSocialManager(ATG::XboxHandle<XUserHandle> user)
    {
        DEBUGLOG("FriendsManager::RemoveUserFromSocialManager()");

        if (XUserCompare(m_user.get(), user.get()))
        {
            XblSocialManagerDestroySocialUserGroup(allFriends);
            XblSocialManagerDestroySocialUserGroup(allOnlineFriends);
            XblSocialManagerDestroySocialUserGroup(allFriendsInGame);
            XblSocialManagerDestroySocialUserGroup(allFavorites);

            // Remove local user
            XblSocialManagerRemoveLocalUser(m_user.get());

            m_user.clear();
            m_xblContext.clear();
        }
    }

    void FriendsManager::CreateSocialGroupFromFilters(XblSocialManagerUserGroupHandle& group, XblPresenceFilter presenceFilter, XblRelationshipFilter relationshipFilter)
    {
        DEBUGLOG("FriendsManager::CreateSocialGroupFromFilters()");

        HRESULT hr = XblSocialManagerCreateSocialUserGroupFromFilters(m_user.get(), presenceFilter, relationshipFilter, &group);
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

        if (events != nullptr)
        {
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
            else
            {
                break;
            }
        }
    }

    void FriendsManager::ReadUserDisplayNameAsync(uint64_t xuid, std::function<void(bool, uint64_t)> callback)
    {
        DEBUGLOG("FriendsManager::ReadUserDisplayNameAsync()");

        if (xuid == 0)
        {
            DEBUGLOG("FriendsManager::ReadUserDisplayNameAsync: xuid was invalid");
            if (callback)
            {
                callback(false, xuid);
            }
            return;
        }

        auto name = m_xuidToDisplayNameMap.find(xuid);
        if (name != m_xuidToDisplayNameMap.end())
        {
            if (callback)
            {
                callback(true, xuid);
            }
            
            return;
        }

        auto asyncManager = Managers::Get<AsyncTaskManager>();
        auto asyncQueue = asyncManager->GetDefaultQueue();
        auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [this, callback](XAsyncBlock* async)
        {
            bool bSuccess = false;
            uint64_t xuid = 0;

            XblUserProfile profile{};
            HRESULT hr = XblProfileGetUserProfileResult(async, &profile);
            if (SUCCEEDED(hr))
            {
                DEBUGLOG("Display name updated: xuid: %llu name: %s", profile.xboxUserId, profile.gameDisplayName);

                m_xuidToDisplayNameMap[profile.xboxUserId] = profile.gameDisplayName;

                xuid = profile.xboxUserId;
                bSuccess = true;
            }
            else
            {
                LogError_HRESULT("XblProfileGetUserProfileResult", hr);
            }

            if (callback)
            {
                callback(bSuccess, xuid);
            }
        });

        HRESULT hr = XblProfileGetUserProfileAsync(m_xblContext.get(), xuid, &asyncHelper->asyncBlock);
        if (FAILED(hr))
        {
            delete asyncHelper;

            LogError_HRESULT("XblProfileGetUserProfileAsync", hr);
            if (callback)
            {
                callback(false, xuid);
            }
        }
    }

    void FriendsManager::ReadUserDisplayNamesAsync(uint64_t* xboxUserIds, size_t xboxUserIdsCount, std::function<void(bool, const std::vector<uint64_t> &)> callback)
    {
        DEBUGLOG("FriendsManager::ReadUserDisplayNamesAsync()");

        if (xboxUserIds == nullptr)
        {
            DEBUGLOG("FriendsManager::ReadUserDisplayNamesAsync: xboxUserIds was invalid");
            if (callback)
            {
                std::vector<uint64_t> users;
                callback(false, users);
            }
            return;
        }

        if (xboxUserIdsCount == 0)
        {
            DEBUGLOG("FriendsManager::ReadUserDisplayNamesAsync: xboxUserIdsCount was zero");
            if (callback)
            {
                std::vector<uint64_t> users;
                callback(false, users);
            }
            return;
        }

        auto asyncManager = Managers::Get<AsyncTaskManager>();
        auto asyncQueue = asyncManager->GetDefaultQueue();
        auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [this, callback](XAsyncBlock* async)
        {
            bool bSuccess = false;
            std::vector<uint64_t> users;

            size_t count = 0;
            HRESULT hr = XblProfileGetUserProfilesResultCount(async, &count);
            if (SUCCEEDED(hr))
            {
                std::vector<XblUserProfile> profiles;
                profiles.resize(count);

                hr = XblProfileGetUserProfilesResult(async, count, profiles.data());
                if (SUCCEEDED(hr))
                {
                    users.reserve(count);

                    for (auto &&profile : profiles)
                    {
                        DEBUGLOG("Display name updated: xuid: %llu name: %s", profile.xboxUserId, profile.gameDisplayName);

                        m_xuidToDisplayNameMap[profile.xboxUserId] = profile.gameDisplayName;
                        users.emplace_back(profile.xboxUserId);
                    }

                    bSuccess = true;
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

            if(callback)
            {
                callback(bSuccess, users);
            }
        });

        HRESULT hr = XblProfileGetUserProfilesAsync(m_xblContext.get(), xboxUserIds, xboxUserIdsCount, &asyncHelper->asyncBlock);
        if (FAILED(hr))
        {
            delete asyncHelper;

            LogError_HRESULT("XblProfileGetUserProfilesAsync", hr);
            if (callback)
            {
                std::vector<uint64_t> users;
                callback(false, users);
            }
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

    std::vector<uint64_t> FriendsManager::GetFriends()
    {
        DEBUGLOG("FriendsManager::GetFriends()");

        return GetUsers(allFriends);
    }

    std::vector<uint64_t> FriendsManager::GetOnlineFriends()
    {
        DEBUGLOG("FriendsManager::GetOnlineFriends()");

        return GetUsers(allOnlineFriends);
    }

    std::vector<uint64_t> FriendsManager::GetFriendsInGame()
    {
        DEBUGLOG("FriendsManager::GetFriendsInGame()");

        return GetUsers(allFriendsInGame);
    }

    std::vector<uint64_t> FriendsManager::GetFavorites()
    {
        DEBUGLOG("FriendsManager::GetFavorites()");

        return GetUsers(allFavorites);
    }

    std::vector<uint64_t> FriendsManager::GetUsers(XblSocialManagerUserGroupHandle group)
    {
        DEBUGLOG("FriendsManager::GetUsers()");

        std::vector<uint64_t> outFriends;

        size_t userCount = 0;
        const XblSocialManagerUser* const* rawUsers = nullptr;
        HRESULT hr = XblSocialManagerUserGroupGetUsers(group, &rawUsers, &userCount);
        if (SUCCEEDED(hr))
        {
            outFriends.reserve(userCount);

            ATG::ArrayView<const XblSocialManagerUser* const> users(rawUsers, userCount);
            for (auto &&user : users)
            {
                outFriends.push_back(user->xboxUserId);
            }
        }
        else
        {
            LogError_HRESULT("XblSocialManagerUserGroupGetUsers", hr);
        }

        return outFriends;
    }
}
