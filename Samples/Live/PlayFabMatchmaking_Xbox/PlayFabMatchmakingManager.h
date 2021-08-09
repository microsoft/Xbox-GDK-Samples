//--------------------------------------------------------------------------------------
// PlayFabMatchmakingManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#pragma once

#include "StepTimer.h"

#include "ILoggingInterface.h"

#include "playfab\PlayFabClientDataModels.h"
#include "playfab\PlayFabAuthenticationDataModels.h"
#include "playfab\PlayFabEventsDataModels.h"
#include "playfab\PlayFabMultiplayerDataModels.h"
#include "playfab\PlayFabProfilesDataModels.h"

enum class MatchmakingStatus
{
    NotMatching,
    Matching,
    MatchingComplete,
    CancellingMatch,
    MatchingFailed,
    MatchingCancelled
};

struct MatchmakingParameters
{
    int Level;
    int EastUsLatencyInMs;
    int WestUsLatencyInMs;
};

class PlayFabMatchmakingManager
{
public:
    // situational error when matchmaking may have been successful, but the
    // write to the "session document" by the host perhaps did not complete
    // yet and the caller should try again after a little while.
    static constexpr const char* NoDataError = u8"No Data";

public:
    PlayFabMatchmakingManager(const ILoggingInterface& logger);
    ~PlayFabMatchmakingManager() = default;

    void Initialize(const char* playFabId, const char* entityKey, const char* entityToken);

    void BeginMatchmaking(
        const std::string& queueName,
        const MatchmakingParameters& parameters,
        int timeoutInSeconds,
        std::function<void()> completionCallback);
    void CancelMatchmaking(std::function<void()> completionCallback);

    void Update();

    inline bool IsMatchmaking() const { return MatchStatus() == MatchmakingStatus::Matching; }

    inline MatchmakingStatus MatchStatus() const { return m_matchStatus; }
    inline const std::string& MatchError() const { return m_matchError; }

    std::vector<std::string> MatchPlayerIds();
    inline const PlayFab::MultiplayerModels::ServerDetails& MatchServer()
    {
        return m_matchedServer;
    }

    void GetMatchedPlayerProfile(
        size_t matchedPlayerIndex,
        std::function<void(std::string displayName, int level)> completionCallback);

    bool IsGameHost();

private:
    void RetrieveMatchResults();
    void PollMatchmaking();
    void IssuePendingCallback()
    {
        if (m_pendingCompletionCallback)
        {
            auto callback = m_pendingCompletionCallback;
            m_pendingCompletionCallback = nullptr;
            callback();
        }
    }

    #ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-security"
    #endif

    template<typename ... Args>
    void Log(const std::string& format, Args ... args) const
    {
        auto expectedSize = snprintf(nullptr, size_t(0), format.c_str(), args ...) + 1; // Extra space for '\0'
        if (expectedSize <= 0) { throw std::runtime_error("Error during formatting."); }

        auto size = static_cast<size_t>(expectedSize);
        std::unique_ptr<char[]> buf(new char[size]);

        snprintf(buf.get(), static_cast<size_t>(size), format.c_str(), args ...);
        auto logLine = std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside

        m_logger.AppendLineOfText(logLine);
    }

    #ifdef __clang__
    #pragma clang diagnostic pop
    #endif

    const ILoggingInterface& m_logger;

    std::string m_playFabId;
    std::string m_entityKey;
    std::string m_entityToken;
    std::function<void()> m_pendingCompletionCallback;

    struct MatchedPlayer
    {
        std::string PlayFabId;
        int Level;
        PlayFab::MultiplayerModels::EntityKey EntityKey;
    };

    std::vector<MatchedPlayer> m_matchedPlayers;
    PlayFab::MultiplayerModels::ServerDetails m_matchedServer;

    bool m_polling;
    DX::StepTimer m_matchTimer;
    std::string m_ticketId;
    std::string m_matchId;
    std::string m_matchQueue;
    std::string m_matchError;
    MatchmakingStatus m_matchStatus;
};
