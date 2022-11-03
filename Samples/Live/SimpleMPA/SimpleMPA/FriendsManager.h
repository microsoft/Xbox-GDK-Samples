//--------------------------------------------------------------------------------------
// File: FriendsManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

class FriendsManager
{
public:
    FriendsManager() = default;
    ~FriendsManager() = default;

    void AddUserToSocialManager(XblContextHandle xblContext, XUserHandle user);
    void RemoveUserFromSocialManager(XUserHandle user);
    void CreateSocialGroupFromFilters(XblSocialManagerUserGroupHandle& group, XblPresenceFilter presenceFilter, XblRelationshipFilter relationshipFilter);
    void DestroySocialGroup(XblSocialManagerUserGroup* group);
    void DoWork();

    void ReadUserDisplayNameAsync(uint64_t id, std::function<void(uint64_t)> callback);
    void ReadUserDisplayNamesAsync(uint64_t* xboxUserIds, size_t xboxUserIdsCount, std::function<void(const std::vector<uint64_t> &)> callback);

    std::string GetUserDisplayName(uint64_t xuid);

    void GetFriends(std::vector<uint64_t>& outFriends);
    void GetOnlineFriends(std::vector<uint64_t>& outFriends);
    void GetFriendsInGame(std::vector<uint64_t>& outFriends);
    void GetFavorites(std::vector<uint64_t>& outFriends);

private:
    XblSocialManagerUserGroupHandle allFriends = nullptr;
    XblSocialManagerUserGroupHandle allOnlineFriends = nullptr;
    XblSocialManagerUserGroupHandle allFriendsInGame = nullptr;
    XblSocialManagerUserGroupHandle allFavorites = nullptr;

    std::map<uint64_t, std::string> m_xuidToDisplayNameMap;
    XblContextHandle m_xblContext;
    XUserHandle m_user;

    uint64_t GetXuidForXUserHandle(XUserHandle user);
    void UpdateFriendDisplayNames(const XblSocialManagerEvent& socialEvent);

    void GetUsers(XblSocialManagerUserGroupHandle group, std::vector<uint64_t>& outFriends);
};
