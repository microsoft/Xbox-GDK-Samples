//--------------------------------------------------------------------------------------
// MPAManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "IManager.h"

namespace PlayFabMultiplayerRumble
{
    class MPAManager : public IManager
    {
    public:
        MPAManager() = default;
        ~MPAManager() = default;

        void SetActivity(const ATG::XboxHandle<XblContextHandle>& xblContext, const char* connectionString, const XblMultiplayerActivityJoinRestriction& joinRestriction, uint32_t maxPlayerCount, uint32_t currentPlayerCount, const char* groupId, bool allowCrossPlatformJoin, std::function<void(bool)> callback);
        void GetActivities(const ATG::XboxHandle<XblContextHandle>& xblContext, const std::vector<uint64_t>& xuids, std::function<void(bool, std::vector<std::shared_ptr<OnlineUser>> &)> callback);
        void DeleteActivity(const ATG::XboxHandle<XblContextHandle>& xblContext, std::function<void(bool)> callback);

        void ShowInviteUI(const ATG::XboxHandle<XUserHandle>& user, std::function<void(bool)> callback) noexcept;
        void SendInvites(const ATG::XboxHandle<XblContextHandle>& xblContext, const std::vector<uint64_t>& xuids, const char* connectionString, bool allowCrossPlatformJoin, std::function<void(bool)> callback);

        void UpdateRecentPlayers(const ATG::XboxHandle<XblContextHandle>& xblContext, uint64_t metPlayerXuid, std::function<void(bool)> callback);
    };
}
