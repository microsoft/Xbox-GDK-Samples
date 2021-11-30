//--------------------------------------------------------------------------------------
// File: PlayFabResources.cpp
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright(c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "PlayFabResources.h"

#include "playfab/PlayFabSettings.h"
#include "playfab/PlayFabApiSettings.h"


using namespace PlayFab;

namespace ATG
{

/// This class when constructed takes in a PlayFab title ID and a valid Xbox Live user (XUser)
/// and does the token request and LoginWithXbox required for authentication
/// Result is an authenticated state and IDs required for calling Client and Entity API

PlayFabResources::PlayFabResources(
    const char* titleId,
    XUserHandle user,
    const char* gamerTag,
    XTaskQueueHandle queue,
    std::function<bool(HRESULT hr, const char* msg)> cb) :
    m_playFabTitleID(titleId),
    m_xUser(user),
    m_gamertag(gamerTag),
    m_taskQueue(queue),
    m_cb(cb),
    m_playFabId(""),
    m_entityId(""),
    m_entityType(""),
    m_entityToken("")
{
    RequestLiveUserToken();
}

PlayFabResources::~PlayFabResources()
{
}

void PlayFabResources::RequestLiveUserToken()
{
    auto tokenAsyncBlock = new XAsyncBlock
    {
        m_taskQueue,
        this
    };

    tokenAsyncBlock->callback = [](XAsyncBlock* ab)
    {
        size_t bufferSize = 0;
        auto hr = XUserGetTokenAndSignatureResultSize(ab, &bufferSize);

        auto pThis = static_cast<PlayFabResources*>(ab->context);

        if (FAILED(hr))
        {
            pThis->m_cb(hr, "XUserGetTokenAndSignatureResultSize failed");
        }
        else
        {
            std::vector<uint8_t> buffer(bufferSize);

            XUserGetTokenAndSignatureData* userTokenData = nullptr;

            hr = XUserGetTokenAndSignatureResult(
                ab,
                bufferSize,
                buffer.data(),
                &userTokenData,
                nullptr/*bufferUsed*/);

            if (FAILED(hr))
            {
                pThis->m_cb(hr, "XUserGetTokenAndSignatureResult failed");
            }
            else
            {
                pThis->LoginToPlayFab(userTokenData->token);
            }
        }

        delete ab;
    };

    XUserGetTokenAndSignatureOptions options = XUserGetTokenAndSignatureOptions::None;
    options |= XUserGetTokenAndSignatureOptions::ForceRefresh;

    auto hr = XUserGetTokenAndSignatureAsync(
        m_xUser,
        options,
        "GET",
        "https://playfabapi.com/",
        0,
        nullptr,
        0,
        nullptr,
        tokenAsyncBlock);

    if (FAILED(hr))
    {
        delete tokenAsyncBlock;
    }
}

void PlayFabResources::LoginToPlayFab(const char* token)
{
    PlayFabSettings::staticSettings->titleId = m_playFabTitleID;

    ClientModels::LoginWithXboxRequest loginRequest;
    loginRequest.CreateAccount = true;
    loginRequest.TitleId = m_playFabTitleID;
    loginRequest.XboxToken = token;

    PlayFabClientAPI::LoginWithXbox(
        loginRequest,
        [this](ClientModels::LoginResult loginResult, void*)
        {
            m_playFabId = loginResult.PlayFabId;
            m_entityId = loginResult.EntityToken->Entity->Id;
            m_entityType = loginResult.EntityToken->Entity->Type;
            m_entityToken = loginResult.EntityToken->EntityToken;

            m_cb(S_OK, "LoginWithXbox success");

            ClientModels::UpdateUserTitleDisplayNameRequest request;
            request.DisplayName = m_gamertag;

            PlayFabClientAPI::UpdateUserTitleDisplayName(
                request,
                [](const ClientModels::UpdateUserTitleDisplayNameResult&, void*)
                {});
        },
        [this](const PlayFabError& error, void*)
        {
            char msg[64] = "";
            sprintf_s(msg, "LoginWithXbox failed with error %d", error.ErrorCode);
            m_cb(E_FAIL, msg);

        }, nullptr);
}
}
