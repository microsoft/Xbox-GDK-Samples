//--------------------------------------------------------------------------------------
// File: PlayFabResources.h
//
// Handles Users signing in and out and the related Xbox Live Contexts
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright(c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#pragma once

#include "playfab\PlayFabClientApi.h"

namespace ATG
{
    class PlayFabResources
    {
    public:
        PlayFabResources(
            const char* titleId,
            XUserHandle user,
            const char* gamertag,
            XTaskQueueHandle queue,
            std::function<bool(HRESULT, const char*)> cb);
        ~PlayFabResources();

        PlayFabResources(PlayFabResources&&) = delete;
        PlayFabResources& operator= (PlayFabResources&&) = delete;

        PlayFabResources(PlayFabResources const&) = delete;
        PlayFabResources& operator= (PlayFabResources const&) = delete;

        void RequestLiveUserToken();
        void LoginToPlayFab(const char* token);

        std::string GetPlayFabId() const { return m_playFabId; }
        std::string GetEntityId() const { return m_entityId; }
        std::string GetEntityType() const { return m_entityType; }
        std::string GetEntityToken() const { return m_entityToken; }

    private:
        std::string m_playFabTitleID;
        XUserHandle m_xUser;
        std::string m_gamertag;
        XTaskQueueHandle m_taskQueue;
        std::function<bool(HRESULT hr, const char* msg)> m_cb;

        std::string m_playFabId;
        std::string m_entityId;
        std::string m_entityType;
        std::string m_entityToken;
    };
}
