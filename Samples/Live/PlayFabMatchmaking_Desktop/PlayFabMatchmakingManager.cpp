//--------------------------------------------------------------------------------------
// PlayfabMatchmakingManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "pch.h"

#include "PlayFabMatchmakingManager.h"
#include "StepTimer.h"

#include "playfab\PlayFabApiSettings.h"
#include "playfab/PlayFabAuthenticationContext.h"
#include "playfab\PlayFabClientApi.h"
#include "playfab\PlayFabCloudScriptApi.h"
#include "playfab\PlayFabProfilesApi.h"
#include "playfab\PlayFabSettings.h"
#include "playfab\PlayFabAuthenticationApi.h"
#include "playfab\PlayFabEventsApi.h"
#include "playfab\PlayFabMultiplayerApi.h"
#include "playfab\PlayFabDataApi.h"

using namespace PlayFab;
using namespace ClientModels;

PlayFabMatchmakingManager::PlayFabMatchmakingManager(const ILoggingInterface& logger) :
    m_logger(logger),
    m_polling(false),
    m_matchStatus(MatchmakingStatus::NotMatching)
{
}

void PlayFabMatchmakingManager::Initialize(
    const char* playFabId,
    const char* entityId,
    const char* entityToken)
{
    m_playFabId = playFabId;
    m_entityKey = entityId;
    m_entityToken = entityToken;

    PlayFabSettings::staticPlayer->entityId = entityId;
    PlayFabSettings::staticPlayer->entityType = "title_player_account";
    PlayFabSettings::staticPlayer->entityToken = entityToken;

    m_matchTimer.SetTargetElapsedSeconds(13.0f);
    m_matchTimer.SetFixedTimeStep(true);
}

void PlayFabMatchmakingManager::BeginMatchmaking(
    const std::string& queueName,
    const MatchmakingParameters& parameters,
    int timeoutInSeconds,
    std::function<void()> completionCallback)
{
    Log("PlayFabManager::BeginMatchmaking('%hs', %d)", queueName.c_str(), timeoutInSeconds);

    if (m_matchStatus == MatchmakingStatus::Matching)
    {
        Log("BeginMatchmaking() called while already matching");
        return;
    }

    m_matchStatus = MatchmakingStatus::Matching;
    m_matchError = "";
    m_pendingCompletionCallback = completionCallback;

    // for the matchmaking ticket set up, refer to this page:
    // https://docs.microsoft.com/en-us/gaming/playfab/features/multiplayer/matchmaking/ticket-attributes
    // we only deployed a test server to the West US region, so if the numbers 
    // below were switched around (latency) then the system should fail to match
    // if the the West US latency number was above 200ms

    MultiplayerModels::EntityKey entityKey;
    entityKey.Id = m_entityKey;
    entityKey.Type = "title_player_account";

    // for sample purposes, pretend we have an WestUS latency of 100ms
    // to the West US Azure data center
    Json::Value measurement0;
    measurement0["region"] = "WestUs";
    measurement0["latency"] = parameters.WestUsLatencyInMs;

    // for sample purposes, pretend we have an EastUS latency of 150ms
    // to the East US Azure data center
    Json::Value measurement1;
    measurement1["region"] = "EastUs";
    measurement1["latency"] = parameters.EastUsLatencyInMs;

    Json::Value measurements;
    measurements[0] = measurement0;
    measurements[1] = measurement1;

    Json::Value dataObject;
    dataObject["Level"] = parameters.Level;
    dataObject["PlayFabId"] = m_playFabId;
    dataObject["Latency"] = measurements;

    Json::Value attributeJson;
    attributeJson["DataObject"] = dataObject;

    auto testJsonString = attributeJson.toStyledString();

    MultiplayerModels::MatchmakingPlayerAttributes attributes;
    attributes.FromJson(attributeJson);

    MultiplayerModels::MatchmakingPlayer player;
    player.Entity = entityKey;
    player.Attributes = attributes;

    MultiplayerModels::CreateMatchmakingTicketRequest ticketRequest;
    ticketRequest.Creator = player;
    ticketRequest.GiveUpAfterSeconds = timeoutInSeconds;
    ticketRequest.QueueName = queueName;

    m_matchQueue = ticketRequest.QueueName;

    PlayFabMultiplayerAPI::CreateMatchmakingTicket(
        ticketRequest,
        [this](const MultiplayerModels::CreateMatchmakingTicketResult& result, void*)
        {
            Log("CreateMatchmakingTicket succeeded: %hs", result.TicketId.c_str());
            m_ticketId = result.TicketId;
            m_matchTimer.ResetElapsedTime();
        },
        [this](const PlayFabError& error, void*)
        {
            Log("CreateMatchmakingTicket failed: %hs", error.ErrorMessage.c_str());
            m_matchError = error.ErrorMessage;
            m_matchStatus = MatchmakingStatus::MatchingFailed;
        });
}

void PlayFabMatchmakingManager::GetMatchedPlayerProfile(
    size_t matchedPlayerIndex,
    std::function<void(std::string displayName, int level)> completionCallback)
{
    Log("GetMatchedPlayerProfile(%d)", matchedPlayerIndex);

    static std::string emptyString;
    if (matchedPlayerIndex >= m_matchedPlayers.size())
    {
        completionCallback(emptyString, -1);
    }

    auto matchedPlayer = m_matchedPlayers[matchedPlayerIndex];

    ClientModels::GetPlayerProfileRequest request;

    request.PlayFabId = matchedPlayer.PlayFabId;

    PlayFabClientAPI::GetPlayerProfile(
        request,
        [completionCallback, matchedPlayer](const ClientModels::GetPlayerProfileResult& response, void* context) {
            PlayFabMatchmakingManager* manager = reinterpret_cast<PlayFabMatchmakingManager*>(context);
            auto displayName = response.PlayerProfile->DisplayName;
            auto level = matchedPlayer.Level;
            manager->Log(">>> success.");
            completionCallback(displayName, level);
        },
        [completionCallback](const PlayFabError& error, void* context) {
            PlayFabMatchmakingManager* manager = reinterpret_cast<PlayFabMatchmakingManager*>(context);
            manager->Log(">>> PlayFab Error: %s", error.ErrorMessage.c_str());
            completionCallback(emptyString, -1);
        }, this);

}

void PlayFabMatchmakingManager::CancelMatchmaking(std::function<void()> completionCallback)
{
    Log("PlayFabManager::CancelMatchmaking()");

    if (m_matchStatus == MatchmakingStatus::CancellingMatch)
    {
        Log("CancelMatchmaking called while already cancelling");
        return;
    }

    if (m_matchStatus != MatchmakingStatus::Matching)
    {
        Log("CancelMatchmaking called when not matchmaking");
        return;
    }

    m_matchStatus = MatchmakingStatus::CancellingMatch;
    m_pendingCompletionCallback = completionCallback;

    MultiplayerModels::CancelMatchmakingTicketRequest cancelRequest;
    cancelRequest.TicketId = m_ticketId;
    cancelRequest.QueueName = m_matchQueue;

    PlayFabMultiplayerAPI::CancelMatchmakingTicket(
        cancelRequest,
        [this](const MultiplayerModels::CancelMatchmakingTicketResult&, void*)
        {
            Log("CancelMatchmakingTicket complete");
        },
        [this](const PlayFabError& error, void*)
        {
            Log("CancelMatchmakingTicket failed: %hs", error.ErrorMessage.c_str());
            m_matchError = error.ErrorMessage;
            m_matchStatus = MatchmakingStatus::MatchingFailed;
        });
}

void PlayFabMatchmakingManager::Update()
{
    if (m_matchStatus == MatchmakingStatus::Matching ||
        m_matchStatus == MatchmakingStatus::CancellingMatch)
    {
        m_matchTimer.Tick([&]()
            {
                PollMatchmaking();
            });
    }
}

void PlayFabMatchmakingManager::PollMatchmaking()
{
    Log("PlayFabManager::PollMatchmaking()");

    if (m_matchStatus != MatchmakingStatus::Matching &&
        m_matchStatus != MatchmakingStatus::CancellingMatch)
    {
        Log("PollMatchmaking called when not matchmaking");
        return;
    }

    if (m_polling)
    {
        Log("Already polling...");
        return;
    }

    if (m_ticketId.empty() || m_matchQueue.empty())
    {
        Log("No ticket id or match queue to poll");
        return;
    }

    m_polling = true;

    MultiplayerModels::GetMatchmakingTicketRequest matchRequest;
    matchRequest.TicketId = m_ticketId;
    matchRequest.QueueName = m_matchQueue;

    PlayFabMultiplayerAPI::GetMatchmakingTicket(
        matchRequest,
        [this](const MultiplayerModels::GetMatchmakingTicketResult& result, void*)
        {
            m_polling = false;

            // Check if the match was canceled

            if (result.Status == "Canceled")
            {
                m_matchError = result.CancellationReasonString;
                m_matchStatus = MatchmakingStatus::MatchingCancelled;
                IssuePendingCallback();
            }

            Log("GetMatchmakingTicket Status: %hs", result.Status.c_str());
            Log(">>> Match error? %s", !m_matchError.empty() ? m_matchError.c_str() : "<none>");

            // See if we have a match
            if (!result.MatchId.empty())
            {
                m_matchId = result.MatchId;
                RetrieveMatchResults();
            }
        },
        [this](const PlayFabError& error, void*)
        {
            Log("GetMatchmakingTicket failed: %hs", error.ErrorMessage.c_str());
            m_matchError = error.ErrorMessage;
            m_matchStatus = MatchmakingStatus::MatchingFailed;
            m_polling = false;
            IssuePendingCallback();
        });
}

void PlayFabMatchmakingManager::RetrieveMatchResults()
{
    Log("PlayFabManager::RetrieveMatchResults()");

    if (m_matchStatus != MatchmakingStatus::Matching || m_matchId.empty())
    {
        Log("RetrieveMatchResults called when no results available");
        return;
    }

    MultiplayerModels::GetMatchRequest matchRequest;
    matchRequest.MatchId = m_matchId;
    matchRequest.QueueName = m_matchQueue;
    matchRequest.ReturnMemberAttributes = true;

    PlayFabMultiplayerAPI::GetMatch(
        matchRequest,
        [this](const MultiplayerModels::GetMatchResult& result, void*)
        {
            Log("GetMatch succeeded");
            m_matchedPlayers.clear();

            for (const auto& member : result.Members)
            {
                Log(">>> Matched player id: %s", member.Entity.Id.c_str());
                MatchedPlayer mp;
                mp.EntityKey = member.Entity;
                mp.PlayFabId = member.Attributes->DataObject["PlayFabId"].asString();
                mp.Level = member.Attributes->DataObject["Level"].asInt();
                m_matchedPlayers.push_back(mp);
            }

            if (result.pfServerDetails.notNull())
            {
                m_matchedServer.IPV4Address = result.pfServerDetails->IPV4Address;
                m_matchedServer.Ports = result.pfServerDetails->Ports;
                m_matchedServer.Region = result.pfServerDetails->Region;
                Log(">>> Matched server in region: %s", m_matchedServer.Region.c_str());
                Log(">>> %s:%d", m_matchedServer.IPV4Address.c_str(), m_matchedServer.Ports.front().Num);
                Log(">>> %s:%d", m_matchedServer.Ports.front().Name.c_str(), m_matchedServer.Ports.front().Protocol);
            }

            Log(">>> Is Host? %s", IsGameHost() ? "true" : "false");
            m_matchStatus = MatchmakingStatus::MatchingComplete;
            IssuePendingCallback();
        },
        [this](const PlayFabError& error, void*)
        {
            Log("GetMatch failed: %hs", error.ErrorMessage.c_str());
            m_matchError = error.ErrorMessage;
            m_matchStatus = MatchmakingStatus::MatchingFailed;
            IssuePendingCallback();
        });
}

bool PlayFabMatchmakingManager::IsGameHost()
{
    if (!m_matchedPlayers.empty())
    {
        return m_matchedPlayers[0].EntityKey.Id == m_entityKey;
    }

    return false;
}

std::vector<std::string> PlayFabMatchmakingManager::MatchPlayerIds()
{
    std::vector<std::string> playerIds;

    for (const auto& player : m_matchedPlayers)
    {
        playerIds.push_back(player.EntityKey.Id);
    }

    return playerIds;
}
