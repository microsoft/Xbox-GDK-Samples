//--------------------------------------------------------------------------------------
// File: MPAManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MPAManager.h"
#include "AsyncHelper.h"

void MPAManager::SetActivity(XblContextHandle xblContext, uint64_t localUserXuid, const char* connectionString, const XblMultiplayerActivityJoinRestriction& joinRestriction, uint32_t maxPlayerCount, uint32_t currentPlayerCount, const char* groupId, bool allowCrossPlatformJoin, std::function<void(bool bSuccess)> callback)
{
    DEBUGLOG("MPAManager::SetActivity()");

    if (localUserXuid == 0)
    {
        DEBUGLOG("MPAManager::SetActivity: xuid was invalid");
        if (callback)
        {
            callback(false);
        }
        return;
    }

    if (connectionString == nullptr)
    {
        DEBUGLOG("MPAManager::SetActivity: connectionString was null");
        if (callback)
        {
            callback(false);
        }
        return;
    }

    if (maxPlayerCount <= 0)
    {
        DEBUGLOG("MPAManager::SetActivity: maxPlayerCount was not greater than 0");
        if (callback)
        {
            callback(false);
        }
        return;
    }

    if (currentPlayerCount > maxPlayerCount)
    {
        DEBUGLOG("MPAManager::SetActivity: currentPlayerCount was greater than maxPlayerCount");
        if (callback)
        {
            callback(false);
        }
        return;
    }

    if (groupId == nullptr)
    {
        DEBUGLOG("MPAManager::SetActivity: groupId was null");
        if (callback)
        {
            callback(false);
        }
        return;
    }

    XblMultiplayerActivityInfo info{};
    info.xuid = localUserXuid;
    info.connectionString = connectionString;
    info.joinRestriction = joinRestriction;
    info.maxPlayers = maxPlayerCount;
    info.currentPlayers = currentPlayerCount;
    info.groupId = groupId;

    auto asyncHelper = new ATG::AsyncHelper(nullptr, [callback](XAsyncBlock* async)
    {
        bool bSuccess = false;
        HRESULT hr = XAsyncGetStatus(async, false);
        if (SUCCEEDED(hr))
        {
            DEBUGLOG("MPAManager::SetActivity: completed successfully");
            bSuccess = true;
        }
        else
        {
            LogError_HRESULT("XblMultiplayerActivitySetActivityAsync", hr);
        }

        if (callback)
        {
            callback(bSuccess);
        }
    });

    HRESULT hr = XblMultiplayerActivitySetActivityAsync(xblContext, &info, allowCrossPlatformJoin, &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblMultiplayerActivitySetActivityAsync", hr);
        delete asyncHelper;
    
        if (callback)
        {
            callback(false);
        }
    }
}

void MPAManager::GetActivities(XblContextHandle xblContext, const std::vector<uint64_t>& xuids, std::function<void(std::vector<std::shared_ptr<UserActivity>> &)> callback)
{
    DEBUGLOG("MPAManager::GetActivities()");

    if (xuids.empty())
    {
        DEBUGLOG("MPAManager::GetActivities: xuids list was empty");
        if (callback)
        {
            std::vector<std::shared_ptr<UserActivity>> users;
            callback(users);
        }
        return;
    }

    auto asyncHelper = new ATG::AsyncHelper(nullptr, [callback](XAsyncBlock* async)
    {
        std::vector<std::shared_ptr<UserActivity>> userActivities;

        //how many results did we get?
        size_t resultSize{};
        HRESULT hr = XblMultiplayerActivityGetActivityResultSize(async, &resultSize);
        if (SUCCEEDED(hr))
        {
            //get the results
            std::vector<uint8_t> buffer(resultSize);
            XblMultiplayerActivityInfo* activityInfo{};
            size_t resultCount{};
            hr = XblMultiplayerActivityGetActivityResult(async, buffer.size(), buffer.data(), &activityInfo, &resultCount, nullptr);
            if (SUCCEEDED(hr))
            {
                userActivities.reserve(resultCount);

                for (size_t i = 0; i < resultCount; ++i)
                {
                    auto userActivity = std::make_shared<UserActivity>();
                    userActivity->xuid = activityInfo[i].xuid;
                    userActivity->connectionString = activityInfo[i].connectionString;
                    userActivity->maxPlayers = activityInfo[i].maxPlayers;
                    userActivity->currentPlayers = activityInfo[i].currentPlayers;

                    userActivities.push_back(userActivity);
                }
            }
            else
            {
                LogError_HRESULT("XblMultiplayerActivityGetActivityResult", hr);
            }
        }

        if (callback)
        {
            callback(userActivities);
        }
    });

    HRESULT hr = XblMultiplayerActivityGetActivityAsync(xblContext, xuids.data(), xuids.size(), &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblMultiplayerActivityGetActivityAsync", hr);
        delete asyncHelper;

        if (callback)
        {
            std::vector<std::shared_ptr<UserActivity>> userActivities;
            callback(userActivities);
        }
    }
}

void MPAManager::DeleteActivity(XblContextHandle xblContext, uint64_t localUserXuid, std::function<void(bool bSuccess)> callback)
{
    DEBUGLOG("MPAManager::DeleteActivity()");

    if (localUserXuid == 0)
    {
        DEBUGLOG("MPAManager::DeleteActivity: localUserXuid was invalid");
        if (callback)
        {
            callback(false);
        }
        return;
    }

    auto asyncHelper = new ATG::AsyncHelper(nullptr, [callback](XAsyncBlock* async)
    {
        bool bSuccess = false;

        HRESULT hr = XAsyncGetStatus(async, false);
        if (SUCCEEDED(hr))
        {
            bSuccess = true;
        }
        else
        {
            LogError_HRESULT("XblMultiplayerActivityDeleteActivityAsync", hr);
        }

        if (callback)
        {
            callback(bSuccess);
        }
    });

    HRESULT hr = XblMultiplayerActivityDeleteActivityAsync(xblContext, &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblMultiplayerActivityDeleteActivityAsync", hr);
        delete asyncHelper;

        if (callback)
        {
            callback(false);
        }
    }
}

void MPAManager::ShowInviteUI(XUserHandle user, std::function<void(bool)> callback)
{
    auto asyncHelper = new ATG::AsyncHelper(nullptr, [callback](XAsyncBlock* async)
    {
        bool bSuccess = false;
    
        HRESULT hr = XGameUiShowMultiplayerActivityGameInviteResult(async);
        if (SUCCEEDED(hr))
        {
            bSuccess = true;
        }
        else
        {
            LogError_HRESULT("XGameUiShowMultiplayerActivityGameInviteResult", hr);
        }
    
        if (callback)
        {
            callback(bSuccess);
        }
    });
    
    HRESULT hr = XGameUiShowMultiplayerActivityGameInviteAsync(&asyncHelper->asyncBlock, user);
    if (FAILED(hr))
    {
        LogError_HRESULT("XGameUiShowMultiplayerActivityGameInviteAsync", hr);
        delete asyncHelper;
    
        if (callback)
        {
            callback(false);
        }
    }
}

void MPAManager::SendInvites(XblContextHandle xblContext, const std::vector<uint64_t>& xuids, const char* connectionString, bool allowCrossPlatformJoin, std::function<void(bool bSuccess)> callback)
{
    DEBUGLOG("MPAManager::SendInvites()");

    if (xuids.empty())
    {
        DEBUGLOG("MPAManager::SendInvites: xuids was empty");
        if (callback)
        {
            callback(false);
        }
        return;
    }

    if (connectionString == nullptr)
    {
        DEBUGLOG("MPAManager::SendInvites: connectionString was null");
        if (callback)
        {
            callback(false);
        }
        return;
    }

    auto asyncHelper = new ATG::AsyncHelper(nullptr, [callback](XAsyncBlock* async)
    {
        bool bSuccess = false;

        HRESULT hr = XAsyncGetStatus(async, false);
        if (SUCCEEDED(hr))
        {
            bSuccess = true;
        }
        else
        {
            LogError_HRESULT("XblMultiplayerActivitySendInvitesAsync", hr);
        }

        if (callback)
        {
            callback(bSuccess);
        }
    });

    HRESULT hr = XblMultiplayerActivitySendInvitesAsync(xblContext, xuids.data(), xuids.size(), allowCrossPlatformJoin, connectionString, &asyncHelper->asyncBlock);
    if (FAILED(hr))
    {
        LogError_HRESULT("XblMultiplayerActivitySendInvitesAsync", hr);
        delete asyncHelper;

        if (callback)
        {
            callback(false);
        }
    }
}

void MPAManager::UpdateRecentPlayers(XblContextHandle xblContext, uint64_t metPlayerXuid, std::function<void(bool bSuccess)> callback)
{
    DEBUGLOG("MPAManager::UpdateRecentPlayers()");

    bool bSuccess = false;

    if (metPlayerXuid != 0)
    {
        XblMultiplayerActivityRecentPlayerUpdate update{};
        update.xuid = metPlayerXuid;
        update.encounterType = XblMultiplayerActivityEncounterType::Default;

        HRESULT hr = XblMultiplayerActivityUpdateRecentPlayers(xblContext, &update, 1);
        if (SUCCEEDED(hr))
        {
            bSuccess = true;
        }
        else
        {
            LogError_HRESULT("XblMultiplayerActivityUpdateRecentPlayers", hr);
        }
    }
    else
    {
        DEBUGLOG("MPAManager::UpdateRecentPlayers: metPlayerXuid was invalid");
    }

    if (callback)
    {
        callback(bSuccess);
    }
}
