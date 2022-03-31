//-----------------------------------------------------------------------------
// SessionManager.cpp
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "pch.h"
#include "SessionManager.h"
#include "Debug.h"
#include "GuidUtil.h"

#include <XGameInvite.h>
#include <XGameRuntimeFeature.h>
#include <XGameUI.h>

std::string SessionReferenceToString(const XblMultiplayerSessionReference& inSessionReference)
{
    std::ostringstream stringStream;
    stringStream << "Scid: " << inSessionReference.Scid;
    stringStream << " SessionTemplateName: " << inSessionReference.SessionTemplateName;
    stringStream << " SessionName: " << inSessionReference.SessionName;
    return stringStream.str();
}

void SessionManager::Init(XblContextHandle inLiveContext, XTaskQueueHandle inTaskQueue)
{
    XblContextDuplicateHandle(inLiveContext, &m_liveContext);
    XTaskQueueDuplicateHandle(inTaskQueue, &m_taskQueue);
}

void SessionManager::CleanUp()
{
    UnregisterForEvents();

    OnCreateSessionCompleted = nullptr;
    OnJoinSessionCompleted = nullptr;
    OnLeaveSessionCompleted = nullptr;
    OnPlayerJoinedSession = nullptr;
    OnPlayerLeftSession = nullptr;
}

void SessionManager::RegisterForEvents()
{
    if (m_eventsRegistered == false)
    {
        DEBUGLOG("SessionManager::RegisterForEvents:");

        RegisterSubscriptionLostEvent();
        RegisterForConnectionIdChangedEvent();
        RegisterForMPSDEvents();
        RegisterForInvites();

        m_eventsRegistered = true;
    }
}

void SessionManager::UnregisterForEvents()
{
    DEBUGLOG("SessionManager::UnregisterForEvents:");

    if (m_eventsRegistered)
    {
        HRESULT hr = XblMultiplayerSetSubscriptionsEnabled(m_liveContext, false);
        if (FAILED(hr))
        {
            DEBUGLOG("SessionManager::UnregisterForEvents: XblMultiplayerSetSubscriptionsEnabled failed with HRESULT = 0x%08x", hr);
        }

        XblMultiplayerRemoveSubscriptionLostHandler(m_liveContext, m_subscriptionLostHandlerContext);
        XblMultiplayerRemoveConnectionIdChangedHandler(m_liveContext, m_connectionIdChangedHandlerContext);
        XGameInviteUnregisterForEvent(m_gameInviteEventToken, false);

        m_eventsRegistered = false;
    }
}

void SessionManager::RegisterSubscriptionLostEvent()
{
    DEBUGLOG("SessionManager::RegisterSubscriptionLostEvent:");

    auto SubscriptionLost = [](void* context)
    {
        if (auto pThis = reinterpret_cast<SessionManager*>(context))
        {
            DEBUGLOG("MPSD Subscription Lost");
        }
    };

    m_subscriptionLostHandlerContext = XblMultiplayerAddSubscriptionLostHandler(m_liveContext, SubscriptionLost, this);
}

void SessionManager::RegisterForConnectionIdChangedEvent()
{
    auto ConnectionIdChanged = [](void* context)
    {
        if (auto pThis = reinterpret_cast<SessionManager*>(context))
        {
            pThis->OnConnectionIdChanged();
        }
    };

    m_connectionIdChangedHandlerContext = XblMultiplayerAddConnectionIdChangedHandler(m_liveContext, ConnectionIdChanged, this);
}

void SessionManager::RegisterSessionChangedEvent()
{
    DEBUGLOG("SessionManager::AddSessionChangedHandler");

    //MPSD session changed event
    auto SessionChanged = [](void* context, XblMultiplayerSessionChangeEventArgs eventArgs)
    {
        if (auto pThis = reinterpret_cast<SessionManager*>(context))
        {
            pThis->OnSessionChanged((const XblMultiplayerSessionChangeEventArgs&)eventArgs);
        }
    };

    m_sessionChangedHandlerContext = XblMultiplayerAddSessionChangedHandler(m_liveContext, SessionChanged, this);

    HRESULT hr = XblMultiplayerSessionSetSessionChangeSubscription(m_currentSessionHandle, XblMultiplayerSessionChangeTypes::Everything);
    if (FAILED(hr))
    {
        DEBUGLOG("SessionManager::RegisterSessionChangedEvent: XblMultiplayerSessionSetSessionChangeSubscription failed with HRESULT = 0x%08x", hr);
    }
}

void SessionManager::RegisterForMPSDEvents()
{
    HRESULT hr = XblMultiplayerSetSubscriptionsEnabled(m_liveContext, true);
    if (FAILED(hr))
    {
        DEBUGLOG("SessionManager::RegisterForEvents: XblMultiplayerSetSubscriptionsEnabled failed with HRESULT = 0x%08x", hr);
    }
}

void SessionManager::RegisterForInvites()
{
    DEBUGLOG("SessionManager::RegisterForInvites:");

    auto InviteHandlerLambda = [](void* context, const char* inviteUri)
    {
        if (inviteUri != nullptr)
        {
            if (auto pThis = reinterpret_cast<SessionManager*>(context))
            {
                std::string uri = inviteUri;

                auto pos = uri.find("handle=") + 7;
                auto end = uri.find('&', pos);

                // If the session is at the end of the string then end will return not found.
                if (end == std::string::npos)
                {
                    end = uri.length() + 1;
                }

                std::string handle = uri.substr(pos, end - pos);

                pThis->JoinSession(handle);
            }
        }
    };

    XGameInviteRegisterForEvent(nullptr, this, InviteHandlerLambda, &m_gameInviteEventToken);
}

void SessionManager::OnConnectionIdChanged()
{
    DEBUGLOG("SessionManager::OnConnectionIdChanged:");

    if (m_currentSessionHandle == nullptr)
    {
        DEBUGLOG("SessionManager::OnConnectionIdChanged: m_currentSessionHandle was null");
        return;
    }

    HRESULT hr = XblMultiplayerSessionCurrentUserSetStatus(m_currentSessionHandle, XblMultiplayerSessionMemberStatus::Active);
    if (FAILED(hr))
    {
        DEBUGLOG("SessionManager::OnConnectionIdChanged: XblMultiplayerSessionCurrentUserSetStatus failed with HRESULT = 0x%08x", hr);
        return;
    }

    WriteSession(XblMultiplayerSessionWriteMode::UpdateExisting, nullptr);
}

void SessionManager::OnSessionChanged(const XblMultiplayerSessionChangeEventArgs& eventArgs)
{
    {
        std::lock_guard<std::mutex> lock(m_sessionChangeLock);
        m_sessionChanges.emplace(eventArgs);
    }

    if (m_processingSessionChange == false)
    {
        ProcessSessionChanged();
    }
}

void SessionManager::ProcessSessionChanged()
{
    XblMultiplayerSessionChangeEventArgs eventArgs;

    {
        std::lock_guard<std::mutex> lock(m_sessionChangeLock);

        if (m_sessionChanges.empty())
        {
            m_processingSessionChange = false;
            return;
        }

        m_processingSessionChange = true;

        eventArgs = m_sessionChanges.front();
        m_sessionChanges.pop();
    }

    DEBUGLOG("SessionManager::OnSessionChanged: %s, Branch: %s ChangeNumber: %u", SessionReferenceToString(eventArgs.SessionReference).c_str(), eventArgs.Branch, eventArgs.ChangeNumber);

    if (eventArgs.ChangeNumber > m_lastChangeNumber)
    {
        auto asyncBlock = std::make_unique<XAsyncBlock>();
        asyncBlock->queue = m_taskQueue;
        asyncBlock->context = this;
        asyncBlock->callback = [](XAsyncBlock* asyncBlock)
        {
            std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*

            if (auto pThis = reinterpret_cast<SessionManager*>(asyncBlock->context))
            {
                XblMultiplayerSessionHandle updatedHandle;
                HRESULT hr = XblMultiplayerGetSessionResult(asyncBlockPtr.get(), &updatedHandle);
                if (hr == S_OK)
                {
                    if (const XblMultiplayerSessionInfo* updatedSessionInfo = XblMultiplayerSessionGetInfo(updatedHandle))
                    {
                        pThis->m_lastChangeNumber = updatedSessionInfo->ChangeNumber;

                        const XblMultiplayerSessionReference* currentSessionRef = XblMultiplayerSessionSessionReference(pThis->m_currentSessionHandle);
                        if (currentSessionRef && XblMultiplayerSessionReferenceIsValid(currentSessionRef))
                        {
                            const XblMultiplayerSessionReference* updatedSessionRef = XblMultiplayerSessionSessionReference(updatedHandle);
                            if (updatedSessionRef && XblMultiplayerSessionReferenceIsValid(updatedSessionRef))
                            {
                                DEBUGLOG("SessionManager::OnSessionChanged: currentSessionRef: %s", SessionReferenceToString(*currentSessionRef).c_str());
                                DEBUGLOG("SessionManager::OnSessionChanged: updatedSessionRef: %s", SessionReferenceToString(*updatedSessionRef).c_str());

                                //if the session references are the same
                                if (_stricmp(currentSessionRef->Scid, updatedSessionRef->Scid) == 0 &&
                                    _stricmp(currentSessionRef->SessionTemplateName, updatedSessionRef->SessionTemplateName) == 0 &&
                                    _stricmp(currentSessionRef->SessionName, updatedSessionRef->SessionName) == 0)
                                {
                                    XblMultiplayerSessionChangeTypes diff = XblMultiplayerSessionCompare(updatedHandle, pThis->m_currentSessionHandle);

                                    XblMultiplayerSessionCloseHandle(pThis->m_currentSessionHandle); //Close Original Handle
                                    pThis->m_currentSessionHandle = updatedHandle;

                                    pThis->ProcessSessionChangeComplete(updatedSessionRef->SessionName, diff);
                                }
                                else
                                {
                                    DEBUGLOG("SessionManager::OnSessionChanged: Received session change event for incorrect session");
                                }
                            }
                        }
                    }
                }
                else
                {
                    // Handle failure
                    DEBUGLOG("SessionManager::OnSessionChanged: XblMultiplayerGetSessionResult failed with HRESULT = 0x%08x", hr);

                    XblMultiplayerSessionCloseHandle(updatedHandle);
                }

                bool changesPending = false;
                {
                    std::lock_guard<std::mutex> lock(pThis->m_sessionChangeLock);

                    pThis->m_processingSessionChange = false;
                    changesPending = pThis->m_sessionChanges.empty() == false;
                }

                if(changesPending)
                {
                    pThis->ProcessSessionChanged();
                }
            }
        };

        HRESULT hr = XblMultiplayerGetSessionAsync(m_liveContext, &eventArgs.SessionReference, asyncBlock.get());
        if (SUCCEEDED(hr))
        {
            // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
            // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
            asyncBlock.release();
        }
        else
        {
            DEBUGLOG("SessionManager::OnSessionChanged: XblMultiplayerGetSessionAsync failed with HRESULT = 0x%08x", hr);

            bool changesPending = false;
            {
                std::lock_guard<std::mutex> lock(m_sessionChangeLock);

                m_processingSessionChange = false;
                changesPending = m_sessionChanges.empty() == false;
            }

            if (changesPending)
            {
                ProcessSessionChanged();
            }
        }
    }
}

void SessionManager::ProcessSessionChangeComplete(const std::string& sessionName, XblMultiplayerSessionChangeTypes diff)
{
    if ((diff & XblMultiplayerSessionChangeTypes::None) == XblMultiplayerSessionChangeTypes::None)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::None", sessionName.c_str());
    }

    if ((diff & XblMultiplayerSessionChangeTypes::Everything) == XblMultiplayerSessionChangeTypes::Everything)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::Everything", sessionName.c_str());
    }

    if ((diff & XblMultiplayerSessionChangeTypes::HostDeviceTokenChange) == XblMultiplayerSessionChangeTypes::HostDeviceTokenChange)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::HostDeviceTokenChange", sessionName.c_str());
    }

    if ((diff & XblMultiplayerSessionChangeTypes::InitializationStateChange) == XblMultiplayerSessionChangeTypes::InitializationStateChange)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::InitializationStateChange", sessionName.c_str());
        HandleInitilizationStateChanged();
    }

    if ((diff & XblMultiplayerSessionChangeTypes::MatchmakingStatusChange) == XblMultiplayerSessionChangeTypes::MatchmakingStatusChange)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::MatchmakingStatusChange", sessionName.c_str());
        HandleMatchmakingStatusChange();
    }

    if ((diff & XblMultiplayerSessionChangeTypes::MemberListChange) == XblMultiplayerSessionChangeTypes::MemberListChange)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::MemberListChange", sessionName.c_str());
        HandleMemberListChange();
    }

    if ((diff & XblMultiplayerSessionChangeTypes::MemberStatusChange) == XblMultiplayerSessionChangeTypes::MemberStatusChange)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::MemberStatusChange", sessionName.c_str());
    }

    if ((diff & XblMultiplayerSessionChangeTypes::SessionJoinabilityChange) == XblMultiplayerSessionChangeTypes::SessionJoinabilityChange)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::SessionJoinabilityChange", sessionName.c_str());
    }

    if ((diff & XblMultiplayerSessionChangeTypes::CustomPropertyChange) == XblMultiplayerSessionChangeTypes::CustomPropertyChange)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::CustomPropertyChange", sessionName.c_str());
    }

    if ((diff & XblMultiplayerSessionChangeTypes::MemberCustomPropertyChange) == XblMultiplayerSessionChangeTypes::MemberCustomPropertyChange)
    {
        DEBUGLOG("SessionManager::ProcessSessionChange: SessionName: %s XblMultiplayerSessionChangeTypes::MemberCustomPropertyChange", sessionName.c_str());
    }
}

void SessionManager::HandleInitilizationStateChanged()
{
    DEBUGLOG("SessionManager::HandleInitilizationStateChanged:");
}

void SessionManager::HandleMatchmakingStatusChange()
{
    DEBUGLOG("SessionManager::HandleMatchmakingStatusChange:");

    if (m_currentSessionHandle == nullptr)
    {
        DEBUGLOG("SessionManager::HandleMatchmakingStatusChange: m_m_currentSessionHandle was null");
        return;
    }

    const XblMultiplayerMatchmakingServer* matchmakingServer = XblMultiplayerSessionMatchmakingServer(m_currentSessionHandle);
    if (matchmakingServer == nullptr)
    {
        DEBUGLOG("SessionManager::HandleMatchmakingStatusChange: matchmakingServer was null");
        return;
    }

    XblMatchmakingStatus matchState = matchmakingServer->Status;

    switch (matchState)
    {
    case XblMatchmakingStatus::Unknown:
    {
        DEBUGLOG("SessionManager::HandleMatchmakingStatusChange: MatchStatus = Unknown");
        m_matchmakingState = MatchmakingState::Unknown;
        break;
    }
    case XblMatchmakingStatus::None:
    {
        DEBUGLOG("SessionManager::HandleMatchmakingStatusChange: MatchStatus = None");
        m_matchmakingState = MatchmakingState::None;
        break;
    }
    case XblMatchmakingStatus::Searching:
    {
        DEBUGLOG("SessionManager::HandleMatchmakingStatusChange: MatchStatus = Expired");
        m_matchmakingState = MatchmakingState::Searching;
        break;
    }
    case XblMatchmakingStatus::Expired:
    {
        DEBUGLOG("SessionManager::HandleMatchmakingStatusChange: MatchStatus = Expired");
        m_matchmakingState = MatchmakingState::Expired;
        break;
    }
    case XblMatchmakingStatus::Found:
    {
        DEBUGLOG("SessionManager::HandleMatchmakingStatusChange: MatchStatus = Found");
        m_matchmakingState = MatchmakingState::Found;
        break;
    }
    case XblMatchmakingStatus::Canceled:
    {
        DEBUGLOG("SessionManager::HandleMatchmakingStatusChange: MatchStatus = Canceled");
        m_matchmakingState = MatchmakingState::Canceled;
        break;
    }
    }

    if (OnMatchmakingChanged)
    {
        OnMatchmakingChanged(m_matchmakingState);
    }
}

void SessionManager::HandleMemberListChange()
{
    DEBUGLOG("SessionManager::ProcessMemberListChange:");

    const XblMultiplayerSessionMember* sessionMembers = nullptr;
    size_t numSessionMembers = 0;
    XblMultiplayerSessionMembers(m_currentSessionHandle, &sessionMembers, &numSessionMembers);

    std::vector<uint64_t> memberXuids;

    //check for any new players
    for (size_t i = 0; i < numSessionMembers; ++i)
    {
        const XblMultiplayerSessionMember& member = sessionMembers[i];
        memberXuids.emplace_back(member.Xuid);

        //xuid we haven't seen before means a player joined
        if (std::find(m_sessionXuids.begin(), m_sessionXuids.end(), member.Xuid) == m_sessionXuids.end())
        {
            m_sessionXuids.emplace_back(member.Xuid);

            DEBUGLOG("SessionManager::ProcessMemberListChange: Player joined the session Xuid: %llu, Gamertag: %s", member.Xuid, member.Gamertag);

            if (OnPlayerJoinedSession)
            {
                OnPlayerJoinedSession(member.Xuid);
            }
        }
    }

    //check for any removed players
    for (auto it = m_sessionXuids.begin(); it != m_sessionXuids.end();)
    {
        if (std::find(memberXuids.begin(), memberXuids.end(), *it) == memberXuids.end())
        {
            uint64_t xuid = *it;

            it = m_sessionXuids.erase(it);

            if (OnPlayerLeftSession)
            {
                OnPlayerLeftSession(xuid);
            }
        }
        else
        {
            ++it;
        }
    }
}

void SessionManager::CreateSession(const std::string& sessionTemplatName)
{
    DEBUGLOG("SessionManager::CreateSession:");

    if (sessionTemplatName.empty() == true)
    {
        DEBUGLOG("SessionManager::CreateSession: sessionTemplatName was invalid");

        if (OnCreateSessionCompleted)
        {
            OnCreateSessionCompleted(false);
        }
        
        return;
    }

    m_currentSessionTemplateName = sessionTemplatName;
    m_currentSessionName = DX::GuidUtil::NewGuid();

    if (m_currentSessionHandle)
    {
        XblMultiplayerSessionCloseHandle(m_currentSessionHandle);
    }

    const char* scid = nullptr;
    XblGetScid(&scid);

    //Create the new session
    XblMultiplayerSessionReference createdSessionRef = XblMultiplayerSessionReferenceCreate(scid, m_currentSessionTemplateName.c_str(), m_currentSessionName.c_str());

    DEBUGLOG("SessionManager::CreateSession: createdSessionRef: %s", SessionReferenceToString(createdSessionRef).c_str());

    if (m_liveContext == nullptr)
    {
        DEBUGLOG("SessionManager::CreateSession: m_liveContext was null");

        if (OnCreateSessionCompleted)
        {
            OnCreateSessionCompleted(false);
        }

        return;
    }

    uint64_t xuid = 0;
    HRESULT hr = XblContextGetXboxUserId(m_liveContext, &xuid);
    if (FAILED(hr))
    {
        DEBUGLOG("SessionManager::CreateSession: XblContextGetXboxUserId failed with HRESULT = 0x%08x", hr);

        if (OnCreateSessionCompleted)
        {
            OnCreateSessionCompleted(false);
        }

        return;
    }

    std::vector<uint64_t> InitiatorXuids;
    InitiatorXuids.push_back(xuid);

    XblMultiplayerSessionInitArgs initArgs;
    initArgs.MaxMembersInSession = 8; //This matches our session template
    initArgs.Visibility = XblMultiplayerSessionVisibility::Open; //The session is open and can be joined by anyone.
    initArgs.InitiatorXuids = InitiatorXuids.data();
    initArgs.InitiatorXuidsCount = InitiatorXuids.size();
    initArgs.CustomJson = nullptr;

    m_currentSessionHandle = XblMultiplayerSessionCreateHandle(xuid, &createdSessionRef, &initArgs);
    if (m_currentSessionHandle == nullptr)
    {
        DEBUGLOG("SessionManager::CreateSession: XblMultiplayerSessionCreateHandle failed");

        if (OnCreateSessionCompleted)
        {
            OnCreateSessionCompleted(false);
        }

        return;
    }

    XblMultiplayerSessionPropertiesSetJoinRestriction(m_currentSessionHandle, XblMultiplayerSessionRestriction::Followed);
    XblMultiplayerSessionPropertiesSetReadRestriction(m_currentSessionHandle, XblMultiplayerSessionRestriction::Followed);

    //Join the new session
    hr = XblMultiplayerSessionJoin(m_currentSessionHandle, nullptr, true, true);
    if (FAILED(hr))
    {
        DEBUGLOG("SessionManager::CreateSession: XblMultiplayerSessionJoin failed with HRESULT = 0x%08x", hr);

        if (OnCreateSessionCompleted)
        {
            OnCreateSessionCompleted(false);
        }

        return;
    }

    RegisterSessionChangedEvent();

    WriteSession(XblMultiplayerSessionWriteMode::CreateNew, [this](bool success)
    {
        if (success)
        {
            SetSessionActivity();
            HandleMemberListChange();
        }

        if (OnCreateSessionCompleted)
        {
            OnCreateSessionCompleted(success);
        }
    });
}

void SessionManager::SetHostDeviceToken()
{
    DEBUGLOG("SessionManager::SetHostDeviceToken:");

    if (m_currentSessionHandle == nullptr)
    {
        DEBUGLOG("SessionManager::SetHostDeviceToken: m_currentSessionHandle is null");
        return;
    }

    const XblMultiplayerSessionReference* sessionRef = XblMultiplayerSessionSessionReference(m_currentSessionHandle);
    if (sessionRef == nullptr)
    {
        DEBUGLOG("SessionManager::SetHostDeviceToken: sessionRef is null");
        return;
    }

    if(m_liveContext == nullptr)
    {
        DEBUGLOG("SessionManager::SetHostDeviceToken: m_liveContext was null");
        return;
    }

    uint64_t xuid = 0;
    HRESULT hr = XblContextGetXboxUserId(m_liveContext, &xuid);
    if (FAILED(hr))
    {
        DEBUGLOG("SessionManager::SetHostDeviceToken: XblContextGetXboxUserId failed with HRESULT = 0x%08x", hr);
        return;
    }

    const XblMultiplayerSessionMember* sessionMembers = nullptr;
    size_t numSessionMembers = 0;
    XblMultiplayerSessionMembers(m_currentSessionHandle, &sessionMembers, &numSessionMembers);

    for (size_t i = 0; i < numSessionMembers; ++i)
    {
        const XblMultiplayerSessionMember& member = sessionMembers[i];
        if (member.Xuid == xuid)
        {
            XblMultiplayerSessionSetHostDeviceToken(m_currentSessionHandle, member.DeviceToken);
            return;
        }
    }

    DEBUGLOG("SessionManager::SetHostDeviceToken: could not set the host device token");
}

void SessionManager::StartMatchmaking(bool allowCrossGen)
{
    DEBUGLOG("SessionManager::StartMatchmaking:");

    if (m_currentSessionHandle == nullptr)
    {
        DEBUGLOG("SessionManager::StartMatchmaking: m_currentSessionHandle was null");
        if (OnMatchmakingChanged)
        {
            OnMatchmakingChanged(MatchmakingState::Failed);
        }

        return;
    }

    const XblMultiplayerSessionReference* sessionRef = XblMultiplayerSessionSessionReference(m_currentSessionHandle);
    if (sessionRef == nullptr)
    {
        DEBUGLOG("SessionManager::StartMatchmaking: sessionRef was null");
        if (OnMatchmakingChanged)
        {
            OnMatchmakingChanged(MatchmakingState::Failed);
        }

        return;
    }

    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*

        if (auto pThis = reinterpret_cast<SessionManager*>(asyncBlock->context))
        {
            XblCreateMatchTicketResponse createTicketResponse;
            HRESULT hr = XblMatchmakingCreateMatchTicketResult(asyncBlockPtr.get(), &createTicketResponse);
            if (hr == S_OK)
            {
                pThis->m_matchmakingInProgress = true;
                pThis->m_matchmakingState = MatchmakingState::Searching;

                pThis->m_matchTicketId = createTicketResponse.matchTicketId;
            }
            else
            {
                DEBUGLOG("SessionManager::JoinSession: XblMatchmakingCreateMatchTicketResult failed with HRESULT = 0x%08x", hr);
                if (pThis->OnMatchmakingChanged)
                {
                    pThis->OnMatchmakingChanged(MatchmakingState::Failed);
                }
            }
        }
    };

    m_hopperName = allowCrossGen ? "GameHopperCrossGen" : "GameHopper";
    std::string ticketAttributes; //empty for now

    const char* scid = nullptr;
    XblGetScid(&scid);

    HRESULT hr = XblMatchmakingCreateMatchTicketAsync(m_liveContext, *sessionRef, scid, m_hopperName.c_str(), MATCH_MAKING_DURATION, XblPreserveSessionMode::Never, ticketAttributes.c_str(), asyncBlock.get());
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        DEBUGLOG("SessionManager::StartMatchmaking: XblMatchmakingCreateMatchTicketAsync failed with HRESULT = 0x%08x", hr);
        if (OnMatchmakingChanged)
        {
            OnMatchmakingChanged(MatchmakingState::Failed);
        }
    } 
}

void SessionManager::CancelMatchmaking()
{
    DEBUGLOG("SessionManager::CancelMatchmaking:");

    if (m_matchmakingInProgress == false)
    {
        DEBUGLOG("SessionManager::CancelMatchmaking: m_matchmakingInProgress was null");

        if (OnCancelMatchmakingCompleted)
        {
            OnCancelMatchmakingCompleted(false);
        }
        
        return;
    }

    if (m_hopperName.empty())
    {
        DEBUGLOG("SessionManager::CancelMatchmaking: m_hopperName was empty");

        if (OnCancelMatchmakingCompleted)
        {
            OnCancelMatchmakingCompleted(false);
        }

        return;
    }

    if (m_matchTicketId.empty())
    {
        DEBUGLOG("SessionManager::CancelMatchmaking: m_matchTicketId was empty");

        if (OnCancelMatchmakingCompleted)
        {
            OnCancelMatchmakingCompleted(false);
        }

        return;
    }
        
    m_matchmakingInProgress = false;
    m_matchmakingState = MatchmakingState::None;

    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        XAsyncGetStatus(asyncBlock, false);

        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*

        if (auto pThis = reinterpret_cast<SessionManager*>(asyncBlock->context))
        {
            pThis->m_matchmakingInProgress = false;
            pThis->m_matchmakingState = MatchmakingState::None;

            pThis->m_hopperName.clear();
            pThis->m_matchTicketId.clear();

            if (pThis->OnCancelMatchmakingCompleted)
            {
                pThis->OnCancelMatchmakingCompleted(true);
            }
        }
    };

    const char* scid = nullptr;
    XblGetScid(&scid);

    HRESULT hr = XblMatchmakingDeleteMatchTicketAsync(m_liveContext, scid, m_hopperName.c_str(), m_matchTicketId.c_str(), asyncBlock.get());
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        DEBUGLOG("SessionManager::StartMatchmaking: XblMatchmakingCreateMatchTicketAsync failed with HRESULT = 0x%08x", hr);
        if (OnCancelMatchmakingCompleted)
        {
            OnCancelMatchmakingCompleted(false);
        }
    }
}

void SessionManager::JoinSession(const std::string& handle)
{
    DEBUGLOG("SessionManager::JoinSession:");

    if (handle.empty() == true)
    {
        DEBUGLOG("SessionManager::JoinSession: handle was empty");
        return;
    }

    m_joinSessionHandle = handle;

    if (m_currentSessionHandle)
    {
        XblMultiplayerSessionCloseHandle(m_currentSessionHandle);
    }

    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; // Take over ownership of the XAsyncBlock*

        if (auto pThis = reinterpret_cast<SessionManager*>(asyncBlock->context))
        {
            HRESULT hr = XblMultiplayerGetSessionByHandleResult(asyncBlockPtr.get(), &(pThis->m_currentSessionHandle));
            if (FAILED(hr))
            {
                DEBUGLOG("SessionManager::JoinSession: XblMultiplayerGetSessionByHandleResult failed with HRESULT = 0x%08x", hr);

                if (pThis->OnJoinSessionCompleted)
                {
                    pThis->OnJoinSessionCompleted(false);
                }
                
                return;
            }

            pThis->InternalJoinSession();
        }
    };

    //Get the session handle from the string handle passed in
    HRESULT hr = XblMultiplayerGetSessionByHandleAsync(m_liveContext, m_joinSessionHandle.c_str(), asyncBlock.get());
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        DEBUGLOG("SessionManager::JoinSession: XblMultiplayerGetSessionByHandleAsync failed with HRESULT = 0x%08x", hr);

        if (OnJoinSessionCompleted)
        {
            OnJoinSessionCompleted(false);
        }
    }
}

void SessionManager::InternalJoinSession()
{
    DEBUGLOG("SessionManager::InternalJoinSession:");

    HRESULT hr = XblMultiplayerSessionJoin(m_currentSessionHandle, nullptr, true, true);
    if (FAILED(hr))
    {
        DEBUGLOG("SessionManager::InternalJoinSession: XblMultiplayerSessionJoin failed with HRESULT = 0x%08x", hr);

        if (OnJoinSessionCompleted)
        {
            OnJoinSessionCompleted(false);
        }

        return;
    }

    RegisterSessionChangedEvent();

    hr = XblMultiplayerSessionCurrentUserSetStatus(m_currentSessionHandle, XblMultiplayerSessionMemberStatus::Active);
    if (FAILED(hr))
    {
        DEBUGLOG("SessionManager::InternalJoinSession: XblMultiplayerSessionCurrentUserSetStatus failed with HRESULT = 0x%08x", hr);

        if (OnJoinSessionCompleted)
        {
            OnJoinSessionCompleted(false);
        }

        return;
    }

    WriteSessionByHandle(XblMultiplayerSessionWriteMode::UpdateExisting, [this](bool success)
    {
        SetSessionActivity();

        if (OnJoinSessionCompleted)
        {
            OnJoinSessionCompleted(success);
        }
    });
}

void SessionManager::LeaveSession()
{
    DEBUGLOG("SessionManager::LeaveSession:");

    XblMultiplayerRemoveSessionChangedHandler(m_liveContext, m_sessionChangedHandlerContext);

    ClearSessionActivity();

    HRESULT hr = XblMultiplayerSessionLeave(m_currentSessionHandle);
    if (FAILED(hr))
    {
        DEBUGLOG("SessionManager::LeaveSession: XblMultiplayerSessionLeave failed with HRESULT = 0x%08x", hr);
        return;
    }

    WriteSession(XblMultiplayerSessionWriteMode::SynchronizedUpdate, [this](bool success)
    {
        if (success)
        {
            m_sessionXuids.clear();
        }

        if (OnLeaveSessionCompleted)
        {
            OnLeaveSessionCompleted(success);
        }
    });
}

void SessionManager::SetSessionActivity()
{
    DEBUGLOG("SessionManager::SetSessionActivity:");

    if (m_currentSessionHandle == nullptr)
    {
        DEBUGLOG("SessionManager::SetSessionActivity: m_sessionHandle was null");
        return;
    }

    const XblMultiplayerSessionReference* sessionRef = XblMultiplayerSessionSessionReference(m_currentSessionHandle);
    if (sessionRef == nullptr)
    {
        DEBUGLOG("SessionManager::SetSessionActivity: sessionRef was null");
        return;
    }

    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = nullptr;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        XAsyncGetStatus(asyncBlock, false);

        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; //Take over ownership of the XAsyncBlock*
    };

    HRESULT hr = XblMultiplayerSetActivityAsync(m_liveContext, sessionRef, asyncBlock.get());
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        DEBUGLOG("SessionManager::SetSessionActivity: XblMultiplayerSetActivityAsync failed with HRESULT = 0x%08x", hr);
    }
}

void SessionManager::ClearSessionActivity()
{
    DEBUGLOG("SessionManager::ClearSessionActivity:");

    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = nullptr;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        XAsyncGetStatus(asyncBlock, false);

        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; //Take over ownership of the XAsyncBlock*
    };

    const char* scid = nullptr;
    XblGetScid(&scid);

    HRESULT hr = XblMultiplayerClearActivityAsync(m_liveContext, scid, asyncBlock.get());
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        DEBUGLOG("SessionManager::ClearSessionActivity: XblMultiplayerClearActivityAsync failed with HRESULT = 0x%08x", hr);
    }
}

void SessionManager::ShowPlatformInviteUI(XUserHandle inUser)
{
    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = this;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        XGameUiShowSendGameInviteResult(asyncBlock);

        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; //Take over ownership of the XAsyncBlock*
    };

    const char* scid = nullptr;
    XblGetScid(&scid);

    auto hr = XGameUiShowSendGameInviteAsync(asyncBlock.get(), inUser, scid, m_currentSessionTemplateName.c_str(), m_currentSessionName.c_str(), nullptr, nullptr);
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        DEBUGLOG("SessionManager::ShowPlatformInviteUI: XGameUiShowSendGameInviteAsync failed with HRESULT = 0x%08x", hr);
    }
}

void SessionManager::WriteSession(XblMultiplayerSessionWriteMode writeMode, std::function<void(bool)> onCompleted)
{
    DEBUGLOG("SessionManager::WriteSession:");

    auto inSessionWriteContext = new SessionWriteContext{ this, writeMode, onCompleted };

    if (inSessionWriteContext == nullptr)
    {
        return;
    }

    if (m_currentSessionHandle == nullptr)
    {
        DEBUGLOG("SessionManager::WriteSession: m_currentSessionHandle was null");

        if (inSessionWriteContext && inSessionWriteContext->m_onWriteSessionComplete)
        {
            inSessionWriteContext->m_onWriteSessionComplete(false);
            delete inSessionWriteContext;
        }

        return;
    }

    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = inSessionWriteContext;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; //Take over ownership of the XAsyncBlock*

        std::unique_ptr<SessionWriteContext> pContext{ reinterpret_cast<SessionWriteContext*>(asyncBlock->context) };
        if (pContext && pContext->m_sessionManager)
        {
            SessionManager* pSessionManager = pContext->m_sessionManager;

            XblMultiplayerSessionCloseHandle(pSessionManager->m_currentSessionHandle); //Close Original Handle

            XblMultiplayerSessionHandle createdHandle;
            HRESULT hr = XblMultiplayerWriteSessionResult(asyncBlock, &createdHandle);
            if (hr == S_OK)
            {
                XblWriteSessionStatus status = XblMultiplayerSessionWriteStatus(createdHandle);
                if (status == XblWriteSessionStatus::OutOfSync)
                {
                    // Use the updated handle and write the session again
                    pSessionManager->m_currentSessionHandle = createdHandle;
                    pSessionManager->WriteSession(pContext->m_writeMode, pContext->m_onWriteSessionComplete);
                }
                else
                {
                    pSessionManager->m_currentSessionHandle = createdHandle;

                    if (pContext->m_onWriteSessionComplete)
                    {
                        pContext->m_onWriteSessionComplete(true);
                    }
                }
            }
            else
            {
                // Handle failure
                DEBUGLOG("SessionManager::WriteSession: XblMultiplayerWriteSessionResult failed with HRESULT = 0x%08x", hr);

                XblMultiplayerSessionCloseHandle(createdHandle);

                if (pContext->m_onWriteSessionComplete)
                {
                    pContext->m_onWriteSessionComplete(false);
                }
            }
        }
    };

    //Write the session
    auto hr = XblMultiplayerWriteSessionAsync(m_liveContext, m_currentSessionHandle, inSessionWriteContext->m_writeMode, asyncBlock.get());
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        DEBUGLOG("SessionManager::WriteSession: XblMultiplayerWriteSessionAsync failed with HRESULT = 0x%08x", hr);

        if (inSessionWriteContext->m_onWriteSessionComplete)
        {
            inSessionWriteContext->m_onWriteSessionComplete(false);
            delete inSessionWriteContext;
        }
    }
}

void SessionManager::WriteSessionByHandle(XblMultiplayerSessionWriteMode writeMode, std::function<void(bool)> onCompleted)
{
    DEBUGLOG("SessionManager::WriteSessionByHandle:");

    auto inSessionWriteContext = new SessionWriteContext{ this, writeMode, onCompleted };
    if (inSessionWriteContext == nullptr)
    {
        return;
    }

    if (m_currentSessionHandle == nullptr)
    {
        DEBUGLOG("SessionManager::WriteSession: m_sessionHandle was null");

        if (inSessionWriteContext->m_onWriteSessionComplete)
        {
            inSessionWriteContext->m_onWriteSessionComplete(false);
            delete inSessionWriteContext;
        }
        
        return;
    }

    if (m_joinSessionHandle.empty() == true)
    {
        DEBUGLOG("SessionManager::WriteSession: m_joinSessionHandle was empty");

        if (inSessionWriteContext->m_onWriteSessionComplete)
        {
            inSessionWriteContext->m_onWriteSessionComplete(false);
            delete inSessionWriteContext;
        }

        return;
    }

    auto asyncBlock = std::make_unique<XAsyncBlock>();
    asyncBlock->queue = m_taskQueue;
    asyncBlock->context = inSessionWriteContext;
    asyncBlock->callback = [](XAsyncBlock* asyncBlock)
    {
        std::unique_ptr<XAsyncBlock> asyncBlockPtr{ asyncBlock }; //Take over ownership of the XAsyncBlock*

        std::unique_ptr<SessionWriteContext> pContext{ reinterpret_cast<SessionWriteContext*>(asyncBlock->context) };
        if (pContext && pContext->m_sessionManager)
        {
            SessionManager* sessionManager = pContext->m_sessionManager;
        
            XblMultiplayerSessionCloseHandle(sessionManager->m_currentSessionHandle); //Close Original Handle

            XblMultiplayerSessionHandle createdHandle;
            HRESULT hr = XblMultiplayerWriteSessionResult(asyncBlock, &createdHandle);
            if (hr == S_OK)
            {
                sessionManager->m_currentSessionHandle = createdHandle;

                if (pContext->m_onWriteSessionComplete)
                {
                    pContext->m_onWriteSessionComplete(true);
                }
            }
            else if (hr == HTTP_E_STATUS_PRECOND_FAILED)
            {
                //get the updated handle and try again
                sessionManager->m_currentSessionHandle = createdHandle;
                sessionManager->WriteSession(pContext->m_writeMode, pContext->m_onWriteSessionComplete);
            }
            else
            {
                // Handle failure
                DEBUGLOG("SessionManager::WriteSessionByHandle: XblMultiplayerWriteSessionResult failed with HRESULT = 0x%08x", hr);

                XblMultiplayerSessionCloseHandle(createdHandle);

                if (pContext->m_onWriteSessionComplete)
                {
                    pContext->m_onWriteSessionComplete(false);
                }
            }
        }
    };

    //Write the session
    auto hr = XblMultiplayerWriteSessionByHandleAsync(m_liveContext, m_currentSessionHandle, inSessionWriteContext->m_writeMode, m_joinSessionHandle.c_str(), asyncBlock.get());
    if (SUCCEEDED(hr))
    {
        // The call succeeded, so release the std::unique_ptr ownership of XAsyncBlock* since the callback will take over ownership.
        // If the call fails, the std::unique_ptr will keep ownership and delete the XAsyncBlock*
        asyncBlock.release();
    }
    else
    {
        DEBUGLOG("SessionManager::WriteSessionByHandle: XblMultiplayerWriteSessionAsync failed with HRESULT = 0x%08x", hr);

        if (inSessionWriteContext)
        {
            inSessionWriteContext->m_onWriteSessionComplete(false);
            delete inSessionWriteContext;
        }
    }
}
