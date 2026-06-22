//--------------------------------------------------------------------------------------
// FriendsManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "IManager.h"
namespace PlayFabMultiplayerRumble
{
    struct XboxLiveOnlineUser : public OnlineUser
    {
        std::string connectionString;
    };

    class FriendsManager : public IManager
    {
    public:
        FriendsManager() = default;
        ~FriendsManager();

        void AddUserToSocialManager(ATG::XboxHandle<XblContextHandle> xblContext, ATG::XboxHandle<XUserHandle> user);
        void RemoveUserFromSocialManager(ATG::XboxHandle<XUserHandle> user);
        void CreateSocialGroupFromFilters(XblSocialManagerUserGroupHandle& group, XblPresenceFilter presenceFilter, XblRelationshipFilter relationshipFilter);
        void DestroySocialGroup(XblSocialManagerUserGroup* group);
        void DoWork();

        void ReadUserDisplayNameAsync(uint64_t id, std::function<void(bool, uint64_t)> callback);
        void ReadUserDisplayNamesAsync(uint64_t* xboxUserIds, size_t xboxUserIdsCount, std::function<void(bool, const std::vector<uint64_t> &)> callback);

        std::string GetUserDisplayName(uint64_t xuid);

        std::vector<uint64_t> GetFriends();
        std::vector<uint64_t> GetOnlineFriends();
        std::vector<uint64_t> GetFriendsInGame();
        std::vector<uint64_t> GetFavorites();

    private:
        XblSocialManagerUserGroupHandle allFriends = nullptr;
        XblSocialManagerUserGroupHandle allOnlineFriends = nullptr;
        XblSocialManagerUserGroupHandle allFriendsInGame = nullptr;
        XblSocialManagerUserGroupHandle allFavorites = nullptr;

        std::map<uint64_t, std::string> m_xuidToDisplayNameMap;
        ATG::XboxHandle<XblContextHandle> m_xblContext;
        ATG::XboxHandle<XUserHandle> m_user;

        uint64_t GetXuidForXUserHandle(XUserHandle user);
        void UpdateFriendDisplayNames(const XblSocialManagerEvent& socialEvent);

        std::vector<uint64_t> GetUsers(XblSocialManagerUserGroupHandle group);
    };
}
