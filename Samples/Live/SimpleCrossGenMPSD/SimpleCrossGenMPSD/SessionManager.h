//-----------------------------------------------------------------------------
// SessionManager.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once
#include "LiveResources.h"
#include <map>
#include <queue>
#include <sstream>

#define MATCH_MAKING_DURATION ((long long) 60 * 1000 * 5) //five minutes in milliseconds

enum class MatchmakingState
{
    Unknown,
    None,
    Searching,
    Expired,
    Found,
    Canceled,
    Failed
};

struct SessionWriteContext
{
    class SessionManager* m_sessionManager = nullptr;
    XblMultiplayerSessionWriteMode m_writeMode = XblMultiplayerSessionWriteMode::UpdateOrCreateNew;
    std::function<void(bool)> m_onWriteSessionComplete = nullptr;

    SessionWriteContext(SessionWriteContext&&) = delete;
    SessionWriteContext& operator= (SessionWriteContext&&) = delete;

    SessionWriteContext(SessionWriteContext const&) = delete;
    SessionWriteContext& operator= (SessionWriteContext const&) = delete;
};

class SessionManager
{
public:
    SessionManager() = default;
    ~SessionManager() = default;

    void Init(XblContextHandle inLiveContext, XTaskQueueHandle inTaskQueue);
    void CleanUp();

    void RegisterForEvents();
    void UnregisterForEvents();

    void RegisterForMPSDEvents();
    void RegisterForConnectionIdChangedEvent();
    void RegisterSessionChangedEvent();
    void RegisterSubscriptionLostEvent();
    void RegisterForInvites();

    void OnConnectionIdChanged();
    void OnSessionChanged(const XblMultiplayerSessionChangeEventArgs& eventArgs);

    void CreateSession(const std::string& sessionTemplateName);
    void StartMatchmaking(bool allowCrossGen);
    void JoinSession(const std::string& handle);
    void InternalJoinSession();
    void LeaveSession();

    void SetSessionActivity();
    void ClearSessionActivity();

    void WriteSession(XblMultiplayerSessionWriteMode writeMode, std::function<void(bool)> onCompleted);
    void WriteSessionByHandle(XblMultiplayerSessionWriteMode newWriteMode, std::function<void(bool)> onCompleted);
    void CancelMatchmaking();

    void ShowPlatformInviteUI(XUserHandle inUser);

    void ProcessSessionChange();

    void TryProcessNextSessionChange();

    void ProcessSessionChangeComplete(const std::string& sessionName, XblMultiplayerSessionChangeTypes diff);
    void HandleInitilizationStateChanged();
    void HandleMatchmakingStatusChange();
    void HandleMemberListChange();

    void SetHostDeviceToken();

    std::function<void(bool)> OnCreateSessionCompleted;
    std::function<void(bool)> OnJoinSessionCompleted;
    std::function<void(bool)> OnLeaveSessionCompleted;
    std::function<void(uint64_t)> OnPlayerJoinedSession;
    std::function<void(uint64_t)> OnPlayerLeftSession;
    std::function<void(MatchmakingState)> OnMatchmakingChanged;
    std::function<void(bool)> OnCancelMatchmakingCompleted;

private:
    XblContextHandle m_liveContext = nullptr;
    XTaskQueueHandle m_taskQueue = nullptr;

    bool m_eventsRegistered = false;
    XblFunctionContext m_connectionIdChangedHandlerContext = 0;
    XblFunctionContext m_subscriptionLostHandlerContext = 0;
    XblFunctionContext m_sessionChangedHandlerContext = 0;
    XTaskQueueRegistrationToken m_gameInviteEventToken;

    std::vector<uint64_t> m_sessionXuids;

    std::string m_joinSessionHandle;

    XblMultiplayerSessionHandle m_currentSessionHandle = nullptr;
    std::string m_currentSessionTemplateName;
    std::string m_currentSessionName;

    uint64_t m_lastChangeNumber = 0;

    bool m_matchmakingInProgress = false;
    MatchmakingState m_matchmakingState = MatchmakingState::None;
    std::string m_hopperName;
    std::string m_matchTicketId;

    std::mutex m_sessionChangeLock;
    std::queue<XblMultiplayerSessionChangeEventArgs> m_sessionChanges;
    bool m_processingSessionChanges = false;
};
