//--------------------------------------------------------------------------------------
// XboxLiveMultiplayerLogic.cs
//
// Xbox Live logic that handles general Multiplayer Manager events and updates.
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
// to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

using System;
using System.Linq;
using UnityEngine;
using Unity.XGamingRuntime;
using HR = Unity.XGamingRuntime.Interop.HR;

public partial class XboxLiveLogic
{
    public event Action OnMultiplayerInitialized;
    public event Action OnInviteReceived;
    public event Action OnLobbyCreated;
    public event Action OnLobbyJoined;
    public event Action OnLobbyLeft;
    public event Action OnGameJoined;
    public event Action OnGameLeft;
    public event Action OnMatchLeft;
    public event Action OnHostChanged;
    public event Action<ulong[]> OnMembersAdded; // parameters are: xuids
    public event Action<ulong[]> OnMembersRemoved; // parameters are: xuids
    public event Action<string, int> OnMultiplayerError; // parameters are: error message, hresult

    public bool HasMultiplayerPrivileges { get; private set; }
    public bool HasMultiplayerInvite { get { return !string.IsNullOrEmpty(ReceivedInviteSessionHandleId); } }
    public string ReceivedInviteSessionHandleId { get; private set; }

    public bool IsMatchmaking { get; private set; }
    public bool IsHost { get; private set; }
    public ulong HostXuid { get; private set; }

    private const string HANDLE_KEY = "handle=";

    private XGameInviteRegistrationToken _inviteRegistrationToken;
    private XblMultiplayerSessionReference _sessionReference;

    public void InitializeMultiplayer()
    {
        Debug.LogFormat("XboxLiveMultiplayerLogic.InitializeMultiplayer()");

        ClearSessionState();

        var hresult = SDK.XUserCheckPrivilege(
            MyUserHandle, 
            XUserPrivilegeOptions.None, 
            XUserPrivilege.Multiplayer, 
            out bool hasMultiplayerPrivileges, 
            out XUserPrivilegeDenyReason reason);
        if (HR.FAILED(hresult))
        {
            OnGeneralError?.Invoke(
                string.Format("XUserCheckPrivilege() failed with hresult = ", hresult.ToString("X8")),
                hresult);
            return;
        }

        HasMultiplayerPrivileges = hasMultiplayerPrivileges;

        // Note: it is good practice to notify the user that they do
        // not possess adequate MP privileges by displaying a popup, and then allow the user
        // to remedy that through a user-directed Shell UI call like below.

        if (!HasMultiplayerPrivileges)
        {
            SDK.XUserResolvePrivilegeWithUiAsync(
                MyUserHandle,
                XUserPrivilegeOptions.None,
                XUserPrivilege.Multiplayer,
                (hr) => 
                { 
                    if (HR.SUCCEEDED(hr))
                    {
                        HasMultiplayerPrivileges = true;
                        InitializeMultiplayerManager();
                    }
                    else
                    {
                        OnGeneralError?.Invoke(
                            "Failure to resolve MP privilege with XUserResolvePrivilegeWithUiAsync()",
                            hr);
                    }
                });
        }
        else
        {
            InitializeMultiplayerManager();
        }
    }

    public void CleanupMultiplayer()
    {
        Debug.LogFormat("XboxLiveLogic.CleanupMultiplayer()");

        if (null != _inviteRegistrationToken)
        {
            SDK.XGameInviteUnregisterForEvent(_inviteRegistrationToken);
            _inviteRegistrationToken = null;
        }

        SDK.XBL.XblMultiplayerManagerLobbySessionRemoveLocalUser(MyUserHandle);

        ClearSessionState();
    }

    public void ClearMultiplayerEventHandlers()
    {
        OnMultiplayerInitialized = null;
        OnInviteReceived = null;
        OnLobbyCreated = null;
        OnLobbyJoined = null;
        OnLobbyLeft = null;
        OnGameJoined = null;
        OnGameLeft = null;
        OnMatchLeft = null;
        OnHostChanged = null;
        OnMembersAdded = null;
        OnMembersRemoved = null;
        OnSessionPropertiesChanged = null;
        OnSessionMatchMade = null;
        OnSessionMatchMakeCancelled = null;
        OnMultiplayerError = null;
    }

    public void UpdateMultiplayer()
    {
        var hr = SDK.XBL.XblMultiplayerManagerDoWork(out XblMultiplayerEvent[] multiplayerEvents);

        if (HR.FAILED(hr))
        {
            OnMultiplayerError?.Invoke("XblMultiplayerManagerDoWork() failed", hr);
            return;
        }

        if (null == multiplayerEvents)
        {
            return;
        }

        XblMultiplayerManagerMember[] multiplayerMembers;

        XblMultiplayerMatchStatus matchStatus;
        XblMultiplayerMeasurementFailure failureCause;

        foreach (var mpEvent in multiplayerEvents)
        {
            switch (mpEvent.EventType)
            {
                case XblMultiplayerEventType.UserAdded:
                    Debug.Log("XblMultiplayerEventType::UserAdded");

                    if (HR.FAILED(mpEvent.Result))
                    {
                        OnMultiplayerError?.Invoke("UserAdded FAILURE", mpEvent.Result);
                        break;
                    }

                    // If host, write Network ID to session document
                    if (IsHost)
                    {
                        var result = SDK.XBL.XblMultiplayerManagerLobbySessionSessionReference(out _sessionReference);

                        if (HR.FAILED(result))
                        {
                            OnMultiplayerError?.Invoke("XblMultiplayerManagerLobbySessionSessionReference() failed", result);
                        }
                        else
                        {
                            Debug.LogFormat("My Hosted Session Name: {0}", _sessionReference.SessionName);
                        }

                        Debug.LogFormat(">>> changing session state from {0} to InHostedLobby", CurrentSessionState);
                        CurrentSessionState = SessionState.InHostedLobby;

                        SetSessionProperties(new SessionDocument(){ HostMemberID = MyXUID });

                        OnLobbyCreated?.Invoke();
                    }
                    else if(IsMatchmaking)
                    {
                        StartMatchmakingProcess();
                    }

                    break;

                case XblMultiplayerEventType.UserRemoved:
                    Debug.Log("XblMultiplayerEventType::UserRemoved");

                    if (HR.FAILED(mpEvent.Result))
                    {
                        OnMultiplayerError?.Invoke("UserRemoved FAILURE", mpEvent.Result);
                    }

                    if (!IsMatchmaking)
                    {
                        OnLobbyLeft?.Invoke();
                    }
                    else
                    {
                        IsMatchmaking = false;
                        OnSessionMatchMakeCancelled?.Invoke();
                    }

                    ClearSessionState();

                    break;
 
                case XblMultiplayerEventType.MemberJoined: // Indicates a new member has joined the session. 
                    Debug.Log("XblMultiplayerEventType::MemberJoined");

                    // You can call XblMultiplayerEventArgsMembersCount, XblMultiplayerEventArgsMembers to get the members who joined.
                    hr = SDK.XBL.XblMultiplayerEventArgsMembers(mpEvent.EventArgsHandle, out multiplayerMembers);
                    if (HR.SUCCEEDED(hr))
                    {
                        var xuids = multiplayerMembers.Select(member => { return member.Xuid; }).ToArray();
                        // Note: we do not distinguish between session type since the
                        // session state tracks which session the user is in
                        OnMembersAdded?.Invoke(xuids);
                    }

                    break;

                case XblMultiplayerEventType.MemberLeft: // Indicates a member has left the session.
                    Debug.Log("XblMultiplayerEventType::MemberLeft");

                    // You can call XblMultiplayerEventArgsMembersCount, XblMultiplayerEventArgsMembers to get the members who left. 
                    hr = SDK.XBL.XblMultiplayerEventArgsMembers(mpEvent.EventArgsHandle, out multiplayerMembers);
                    if (HR.SUCCEEDED(hr))
                    {
                        var xuids = multiplayerMembers.Select(member => { return member.Xuid; }).ToArray();
                        // Note: we do not distinguish between session type since the
                        // session state tracks which session the user is in
                        OnMembersRemoved?.Invoke(xuids);

                        // If current host left, choose another host
                        if (xuids.Contains(HostXuid))
                        {
                            ChooseNewHost();
                        }
                    }

                    break;

                case XblMultiplayerEventType.SessionPropertyChanged:
                    Debug.Log("XblMultiplayerEventType::SessionPropertyChanged");
                    {
                        // Update our copy of the session properties
                        SDK.XBL.XblMultiplayerEventArgsPropertiesJson(mpEvent.EventArgsHandle, out string propertiesJson);
                        UpdateSessionProperties(propertiesJson);
                    }

                    break;

                case XblMultiplayerEventType.MemberPropertyChanged:
                    Debug.Log("XblMultiplayerEventType::MemberPropertyChanged");
                    break;

                case XblMultiplayerEventType.JoinLobbyCompleted:
                    Debug.Log("XblMultiplayerEventType::JoinLobbyCompleted");

                    if (HR.FAILED(mpEvent.Result))
                    {
                        OnMultiplayerError?.Invoke("JoinLobbyCompleted FAILURE", mpEvent.Result);
                        break;
                    }

                    // If guest, read Network ID from session document
                    if (!IsHost)
                    {
                        var result = SDK.XBL.XblMultiplayerManagerLobbySessionSessionReference(out _sessionReference);

                        if (HR.FAILED(result))
                        {
                            OnMultiplayerError?.Invoke("XblMultiplayerManagerLobbySessionSessionReference() failed", result);
                        }
                        else
                        {
                            Debug.LogFormat("My Joined Session Name: {0}", _sessionReference.SessionName);
                        }

                        var sessionJson = SDK.XBL.XblMultiplayerManagerLobbySessionPropertiesJson();
                        UpdateSessionProperties(sessionJson);

                        Debug.LogFormat(">>> changing session state from {0} to InHostedLobby", CurrentSessionState);
                        CurrentSessionState = SessionState.InHostedLobby;
                        OnLobbyJoined?.Invoke();
                    }

                    break;

                case XblMultiplayerEventType.JoinGameCompleted:
                    Debug.Log("XblMultiplayerEventType::JoinGameCompleted");

                    if (HR.FAILED(mpEvent.Result))
                    {
                        OnMultiplayerError?.Invoke("JoinGameCompleted FAILURE", mpEvent.Result);
                        break;
                    }

                    Debug.LogFormat(">>> changing session state from {0} to InHostedGame", CurrentSessionState);
                    CurrentSessionState = SessionState.InHostedGame;

                    if (IsHost)
                    {
                        SetSessionProperties(CurrentSessionDocument);
                    }

                    OnGameJoined?.Invoke();

                    break;

                case XblMultiplayerEventType.LeaveGameCompleted:
                    if (mpEvent.SessionType == XblMultiplayerSessionType.GameSession)
                    {
                        OnGameLeft?.Invoke();
                    }
                    else if (mpEvent.SessionType == XblMultiplayerSessionType.MatchSession)
                    {
                        OnMatchLeft?.Invoke();
                    }

                    break;

                case XblMultiplayerEventType.HostChanged:
                    Debug.Log("XblMultiplayerEventType::HostChanged");
                    break;

                case XblMultiplayerEventType.ClientDisconnectedFromMultiplayerService:
                    Debug.Log("XblMultiplayerEventType::ClientDisconnectedFromMultiplayerService");
                    break;

                case XblMultiplayerEventType.JoinabilityStateChanged:
                    Debug.Log("XblMultiplayerEventType::JoinabilityStateChanged");
                    break;

                case XblMultiplayerEventType.InviteSent:
                    Debug.Log("XblMultiplayerEventType::InviteSent");
                    break;

                case XblMultiplayerEventType.ArbitrationComplete:
                    Debug.Log("XblMultiplayerEventType::ArbitrationComplete");
                    break;

                case XblMultiplayerEventType.SynchronizedHostWriteCompleted:
                    Debug.Log("XblMultiplayerEventType::SynchronizedHostWriteCompleted");
                    break;

                case XblMultiplayerEventType.SessionPropertyWriteCompleted:
                    Debug.Log("XblMultiplayerEventType::SessionPropertyWriteCompleted");
                    break;

                case XblMultiplayerEventType.SessionSynchronizedPropertyWriteCompleted:
                    Debug.Log("XblMultiplayerEventType::SessionSynchronizedPropertyWriteCompleted");
                    break;

                case XblMultiplayerEventType.LocalMemberPropertyWriteCompleted:
                    Debug.Log("XblMultiplayerEventType::LocalMemberPropertyWriteCompleted");
                    break;

                case XblMultiplayerEventType.FindMatchCompleted:
                    Debug.Log("XblMultiplayerEventType::FindMatchCompleted");
                    
                    var hresult = SDK.XBL.XblMultiplayerEventArgsFindMatchCompleted(
                             mpEvent.EventArgsHandle,
                             out matchStatus,
                             out failureCause);

                    if (HR.FAILED(hresult))
                    {
                        OnMultiplayerError?.Invoke("XblMultiplayerEventArgsFindMatchCompleted() failed.", hresult);
                    }
                    else
                    {
                        HandleMatchmakingProcessStatus(matchStatus, failureCause);
                    }

                    break;

                default:
                    throw new NotImplementedException(string.Format("XblMultiplayerEventType of {0} is not implemented.", mpEvent.EventType.ToString()));
            }
        }
    }

    private void ClearSessionState()
    {
        CurrentSessionState = SessionState.NoSession;
        IsHost = false;
        HostXuid = 0;
        UpdateSessionProperties(string.Empty);
    }

    private void InitializeMultiplayerManager()
    {
        Debug.LogFormat("XboxLiveLogic.__InitializeMultiplayerManager()");

        var hresult = SDK.XBL.XblMultiplayerManagerInitialize(Configuration.LOBBY_SESSION_TEMPLATE_NAME);
        if (HR.SUCCEEDED(hresult))
        {
            RegisterForInviteEvents();
            OnMultiplayerInitialized?.Invoke();
        }
        else
        {
            OnGeneralError?.Invoke(
                string.Format("XblMultiplayerManagerInitialize({0}) failed with hresult = {1}.",
                    Configuration.LOBBY_SESSION_TEMPLATE_NAME,
                    hresult.ToString("X8")),
                hresult);
        }
    }

    private void RegisterForInviteEvents()
    {
        Debug.LogFormat("XboxLiveLogic.__RegisterForInviteEvents()");

        var hresult = SDK.XGameInviteRegisterForEvent(
            InviteEventHandler, 
            out _inviteRegistrationToken);

        if (HR.FAILED(hresult))
        {
            OnGeneralError?.Invoke(
                "XGameInviteRegisterForEvent() failed.",
                hresult);
        }
    }

    private void InviteEventHandler(IntPtr context, string inviteUri)
    {
        // Example invite string URI:
        // ms-xbl-76b1590e://inviteHandleAccept/?invitedXuid=2814645439730606&handle=27a705f6-f080-4aa7-8bd7-ec8c51befd3d&senderXuid=2814626418925179

        var handleStart = inviteUri.IndexOf(HANDLE_KEY) + HANDLE_KEY.Length;
        var handleEnd = inviteUri.IndexOf("&", handleStart);

        var sessionHandleId = inviteUri.Substring(handleStart, handleEnd - handleStart);

        Debug.LogFormat("Invite Received: {0}", inviteUri);
        Debug.LogFormat("==> Handle is {0}", sessionHandleId);
        ReceivedInviteSessionHandleId = sessionHandleId;

        OnInviteReceived?.Invoke();
    }
}
