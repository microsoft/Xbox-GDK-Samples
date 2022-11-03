//--------------------------------------------------------------------------------------
// File: MPAManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

struct UserActivity
{
    uint64_t xuid;
    std::string connectionString;
    size_t maxPlayers;
    size_t currentPlayers;
};

class MPAManager
{
public:
    MPAManager() = default;
    ~MPAManager() = default;

    void SetActivity(XblContextHandle xblContext, uint64_t xuid, const char* connectionString, const XblMultiplayerActivityJoinRestriction& joinRestriction, uint32_t maxPlayerCount, uint32_t currentPlayerCount, const char* groupId, bool allowCrossPlatformJoin, std::function<void(bool bSuccess)> callback);
    void GetActivities(XblContextHandle xblContext, const std::vector<uint64_t>& xuids, std::function<void(std::vector<std::shared_ptr<UserActivity>> &)> callback);
    void DeleteActivity(XblContextHandle xblContext, uint64_t xuid, std::function<void(bool bSuccess)> callback);

    void ShowInviteUI(XUserHandle user, std::function<void(bool)> callback);
    void SendInvites(XblContextHandle xblContext, const std::vector<uint64_t>& xuids, const char* connectionString, bool allowCrossPlatformJoin, std::function<void(bool bSuccess)> callback);

    void UpdateRecentPlayers(XblContextHandle xblContext, uint64_t metPlayerXuid, std::function<void(bool bSuccess)> callback);

private:
    std::function<void(bool bSuccess)> m_onSetActivityCompleted;
    std::function<void(bool bSuccess)> m_onDeleteActivityCompleted;
    std::function<void(bool bSuccess)> m_onSendInvitesCompleted;
    std::function<void(bool bSuccess)> m_onUpdateRecentPlayersCompleted;
};
