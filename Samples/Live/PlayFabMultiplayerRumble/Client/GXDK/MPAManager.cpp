//--------------------------------------------------------------------------------------
// MPAManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"
#include "MPAManager.h"

namespace PlayFabMultiplayerRumble
{
    void MPAManager::SetActivity(const ATG::XboxHandle<XblContextHandle>& xblContext, const char* connectionString, const XblMultiplayerActivityJoinRestriction& joinRestriction, uint32_t maxPlayerCount, uint32_t currentPlayerCount, const char* groupId, bool allowCrossPlatformJoin, std::function<void(bool)> callback)
    {
        DEBUGLOG("MPAManager::SetActivity()");

        if (!xblContext)
        {
            DEBUGLOG("MPAManager::SetActivity: xblContext was null");
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

        if (maxPlayerCount > 0 == false)
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

        uint64_t localUserXuid = 0;
        XblContextGetXboxUserId(xblContext.get(), &localUserXuid);
        if (localUserXuid == 0)
        {
            DEBUGLOG("MPAManager::SetActivity: xuid was invalid");
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

        auto asyncManager = Managers::Get<AsyncTaskManager>();
        auto asyncQueue = asyncManager->GetDefaultQueue();
        auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [callback](XAsyncBlock* async)
        {
            bool bSuccess = false;

            HRESULT hr = XAsyncGetStatus(async, false);
            if (SUCCEEDED(hr))
            {
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
            
        HRESULT hr = XblMultiplayerActivitySetActivityAsync(xblContext.get(), &info, allowCrossPlatformJoin, &asyncHelper->asyncBlock);
        if (FAILED(hr))
        {
            delete asyncHelper;

            LogError_HRESULT("XblMultiplayerActivitySetActivityAsync", hr);
            if (callback)
            {
                callback(false);
            }
        }
    }

    void MPAManager::GetActivities(const ATG::XboxHandle<XblContextHandle>& xblContext, const std::vector<uint64_t>& xuids, std::function<void(bool, std::vector<std::shared_ptr<OnlineUser>> &)> callback)
    {
        DEBUGLOG("MPAManager::GetActivities()");

        if (!xblContext)
        {
            DEBUGLOG("MPAManager::GetActivities: xblContext was null");
            if (callback)
            {
                std::vector<std::shared_ptr<OnlineUser>> users;
                callback(false, users);
            }
            return;
        }

        if (xuids.empty())
        {
            DEBUGLOG("MPAManager::GetActivities: xuids list was empty");
            if (callback)
            {
                std::vector<std::shared_ptr<OnlineUser>> users;
                callback(false, users);
            }
        }

        auto asyncManager = Managers::Get<AsyncTaskManager>();
        auto asyncQueue = asyncManager->GetDefaultQueue();
        auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [callback](XAsyncBlock* async)
        {
            bool bSuccess = false;
            std::vector<std::shared_ptr<OnlineUser>> users;

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
                    users.reserve(resultCount);

                    for (size_t i = 0; i < resultCount; ++i)
                    {
                        auto xblOnlineUser = std::make_shared<XboxLiveOnlineUser>();
                        xblOnlineUser->Id = activityInfo[i].xuid;
                        xblOnlineUser->connectionString = activityInfo[i].connectionString;

                        users.push_back(xblOnlineUser);
                    }

                    bSuccess = true;
                }
                else
                {
                    LogError_HRESULT("XblMultiplayerActivityGetActivityResult", hr);
                }
            }
            else
            {
                LogError_HRESULT("XblMultiplayerActivityGetActivityResultSize", hr);
            }

            if (callback)
            {
                callback(bSuccess, users);
            }
        });

        HRESULT hr = XblMultiplayerActivityGetActivityAsync(xblContext.get(), xuids.data(), xuids.size(), &asyncHelper->asyncBlock);
        if (FAILED(hr))
        {
            delete asyncHelper;

            LogError_HRESULT("XblMultiplayerActivityGetActivityAsync", hr);   
            if (callback)
            {
                std::vector<std::shared_ptr<OnlineUser>> users;
                callback(false, users);
            }
        }
    }

    void MPAManager::DeleteActivity(const ATG::XboxHandle<XblContextHandle>& xblContext, std::function<void(bool)> callback)
    {
        DEBUGLOG("MPAManager::DeleteActivity()");

        if (!xblContext)
        {
            DEBUGLOG("MPAManager::DeleteActivity: xblContext was null");
            if (callback)
            {
                callback(false);
            }
            return;
        }

        auto asyncManager = Managers::Get<AsyncTaskManager>();
        auto asyncQueue = asyncManager->GetDefaultQueue();
        auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [callback](XAsyncBlock* async)
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

        HRESULT hr = XblMultiplayerActivityDeleteActivityAsync(xblContext.get(), &asyncHelper->asyncBlock);
        if (FAILED(hr))
        {
            delete asyncHelper;

            LogError_HRESULT("XblMultiplayerActivityDeleteActivityAsync", hr);
            if (callback)
            {
                callback(false);
            }
        }
    }

    void MPAManager::ShowInviteUI(const ATG::XboxHandle<XUserHandle>& user, std::function<void(bool)> callback) noexcept
    {
        auto asyncManager = Managers::Get<AsyncTaskManager>();
        auto asyncQueue = asyncManager->GetDefaultQueue();
        auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [callback](XAsyncBlock* async)
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

        HRESULT hr = XGameUiShowMultiplayerActivityGameInviteAsync(&asyncHelper->asyncBlock, user.get());
        if (FAILED(hr))
        {
            delete asyncHelper;

            LogError_HRESULT("XGameUiShowMultiplayerActivityGameInviteAsync", hr);
            if (callback)
            {
                callback(false);
            }
        }
    }

    void MPAManager::SendInvites(const ATG::XboxHandle<XblContextHandle>& xblContext, const std::vector<uint64_t>& xuids, const char* connectionString, bool allowCrossPlatformJoin, std::function<void(bool)> callback)
    {
        DEBUGLOG("MPAManager::SendInvites()");

        if (!xblContext)
        {
            DEBUGLOG("MPAManager::SendInvites: xblContext was null");
            if (callback)
            {
                callback(false);
            }
            return;
        }

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

        auto asyncManager = Managers::Get<AsyncTaskManager>();
        auto asyncQueue = asyncManager->GetDefaultQueue();
        auto asyncHelper = new ATG::AsyncHelper(asyncQueue.get(), [callback](XAsyncBlock* async)
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

        HRESULT hr = XblMultiplayerActivitySendInvitesAsync(xblContext.get(), xuids.data(), xuids.size(), allowCrossPlatformJoin, connectionString, &asyncHelper->asyncBlock);
        if (FAILED(hr))
        {
            delete asyncHelper;

            LogError_HRESULT("XblMultiplayerActivitySendInvitesAsync", hr);
            if (callback)
            {
                callback(false);
            }
        }
    }

    void MPAManager::UpdateRecentPlayers(const ATG::XboxHandle<XblContextHandle>& xblContext, uint64_t metPlayerXuid, std::function<void(bool)> callback)
    {
        DEBUGLOG("MPAManager::UpdateRecentPlayers()");

        if (!xblContext)
        {
            DEBUGLOG("MPAManager::UpdateRecentPlayers: xblContext was null");
            if (callback)
            {
                callback(false);
            }
            return;
        }

        if (metPlayerXuid == 0)
        {
            DEBUGLOG("MPAManager::UpdateRecentPlayers: metPlayerXuid was invalid");
            if (callback)
            {
                callback(false);
            }
            return;
        }

        XblMultiplayerActivityRecentPlayerUpdate update{};
        update.xuid = metPlayerXuid;
        update.encounterType = XblMultiplayerActivityEncounterType::Default;

        HRESULT hr = XblMultiplayerActivityUpdateRecentPlayers(xblContext.get(), &update, 1);
        if (SUCCEEDED(hr))
        {
            DEBUGLOG("MPAManager::UpdateRecentPlayers: completed successfully");
            if (callback)
            {
                callback(true);
            }
        }
        else
        {
            LogError_HRESULT("XblMultiplayerActivityUpdateRecentPlayers", hr);
            if (callback)
            {
                callback(false);
            }
        }
    }
}
