using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using Unity.XGamingRuntime;

using HR = Unity.XGamingRuntime.HR;
using XblMatchTicket = Unity.XGamingRuntime.XblCreateMatchTicketResponse;
using XblMultiplayerSessionChangeEventArgs = Unity.XGamingRuntime.XblMultiplayerSessionChangeEventArgs;
using XblMultiplayerSessionHandle = Unity.XGamingRuntime.XblMultiplayerSessionHandle;
using XblMultiplayerSessionInitArgs = Unity.XGamingRuntime.XblMultiplayerSessionInitArgs;
using XblMultiplayerSessionReference = Unity.XGamingRuntime.XblMultiplayerSessionReference;

namespace GdkSample_SimpleMPSD
{
    public sealed class SessionManager : MonoBehaviour
    {
        public event Action<bool> OnCreateSessionCompleted;
        public event Action<bool> OnJoinSessionCompleted;
        public event Action<bool> OnFindSessionCompleted;
        public event Action<bool> OnLeaveSessionCompleted;
        public event Action<string> OnSessionInviteReceived;
        public event Action<ulong> OnPlayerJoinedSession;
        public event Action<ulong> OnPlayerLeftSession;

        private bool _eventsRegistered = false;
        private XGameInviteRegistrationToken _gameInviteEventToken;

        private string _joinSessionHandle;
        private XblMatchTicket _findMatchTicket;

        private XblMultiplayerSessionHandle _currentSessionHandle = null;
        private string _currentSessionTemplateName;
        private string _currentSessionName;
        private ulong _lastSessionChangeNumber;

        private readonly List<ulong> _sessionMemberXuids = new();

        public void Init()
        {
            RegisterForEvents();
        }

        public void OnDestroy()
        {
            Cleanup();
        }

        public void Cleanup()
        {
            UnregisterForEvents();

            OnCreateSessionCompleted = null;
            OnJoinSessionCompleted = null;
            OnFindSessionCompleted = null;
            OnLeaveSessionCompleted = null;
            OnSessionInviteReceived = null;
            OnPlayerJoinedSession = null;
            OnPlayerLeftSession = null;
        }

        public void RegisterForEvents()
        {
            if (_eventsRegistered == false)
            {
                Debug.Log("SessionManager.RegisterForEvents:");

                RegisterSubscriptionLostEvent();
                RegisterForConnectionIdChangedEvent();
                RegisterForMPSDEvents();
                RegisterForInvites();

                _eventsRegistered = true;
            }
        }

        public void UnregisterForEvents()
        {
            if (_eventsRegistered)
            {
                var hr = SDK.XBL.XblMultiplayerSetSubscriptionsEnabled(
                    XboxManager.Instance.ContextHandle,
                    false);

                if (HR.FAILED(hr))
                {
                    Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSetSubscriptionsEnabled)} returned 0x{hr:X8}");
                }

                XboxManager.Instance.ContextHandle.XblMultiplayerSessionSubscriptionLost -= OnSubscriptionLost;
                XboxManager.Instance.ContextHandle.XblMultiplayerConnectionIdChanged -= OnConnectionIdChanged;

                SDK.XGameInviteUnregisterForEvent(_gameInviteEventToken);

                _eventsRegistered = false;
            }
        }

        public void RegisterForMPSDEvents()
        {
            var hr = SDK.XBL.XblMultiplayerSetSubscriptionsEnabled(
                XboxManager.Instance.ContextHandle,
                true);

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSetSubscriptionsEnabled)} returned 0x{hr:X8}");
            }
        }

        public void RegisterForConnectionIdChangedEvent()
        {
            XboxManager.Instance.ContextHandle.XblMultiplayerConnectionIdChanged += OnConnectionIdChanged;
        }

        public void RegisterSessionChangedEvent()
        {
            XboxManager.Instance.ContextHandle.XblMultiplayerSessionChanged += OnSessionChanged;

            var hr = SDK.XBL.XblMultiplayerSessionSetSessionChangeSubscription(_currentSessionHandle, XblMultiplayerSessionChangeTypes.Everything);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSessionSetSessionChangeSubscription)} returned 0x{hr:X8}");
            }
            else
            {
                Debug.Log($"{nameof(SDK.XBL.XblMultiplayerSessionSetSessionChangeSubscription)} succeeded");
            }
        }

        public void RegisterSubscriptionLostEvent()
        {
            XboxManager.Instance.ContextHandle.XblMultiplayerSessionSubscriptionLost += OnSubscriptionLost;
        }

        public void RegisterForInvites()
        {
            Debug.Log($"{nameof(SessionManager.RegisterForInvites)}");

            var hr = SDK.XGameInviteRegisterForEvent(
                (IntPtr context, string inviteUri) =>
                {
                    string uri = inviteUri;

                    var pos = uri.IndexOf("handle=") + 7;
                    var end = uri.IndexOf('&', pos);

                    // If the session is at the end of the string then end will return not found.
                    if (end == -1)
                    {
                        end = uri.Length + 1;
                    }

                    string handle = uri.Substring(pos, end - pos);

                    OnSessionInviteReceived?.Invoke(handle);
                },
                out _gameInviteEventToken);

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK.XGameInviteRegisterForEvent)} returned - 0x{hr:X8}");
            }
            else
            {
                Debug.Log($"{nameof(SDK.XGameInviteRegisterForEvent)} SUCCESS");
            }
        }

        public void CreateSession(string sessionTemplateName)
        {
            Debug.Log($"{nameof(SessionManager.CreateSession)}");

            InternalCreateSession(sessionTemplateName, OnCreateSessionCompleted);
        }

        public void JoinSession(string handle)
        {
            Debug.Log($"{nameof(SessionManager.JoinSession)}");

            if (string.IsNullOrEmpty(handle) == true)
            {
                Debug.LogWarning($"{nameof(SessionManager.JoinSession)}: handle was empty");
                return;
            }

            _joinSessionHandle = handle;

            if (_currentSessionHandle != null)
            {
                SDK.XBL.XblMultiplayerSessionCloseHandle(_currentSessionHandle);
                _currentSessionHandle = null;
            }

            //Get the session handle from the string handle passed in
            SDK.XBL.XblMultiplayerGetSessionByHandleAsync(
                XboxManager.Instance.ContextHandle,
                _joinSessionHandle,
                (Int32 hresult, XblMultiplayerSessionHandle handle) =>
                {
                    if (HR.SUCCEEDED(hresult))
                    {
                        _currentSessionHandle = handle;
                        _lastSessionChangeNumber = 0;
                        InternalJoinSession();
                    }
                    else
                    {
                        Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerGetSessionByHandleAsync)} returned - 0x{hresult:X8}");
                        OnJoinSessionCompleted?.Invoke(false);
                    }
                });
        }

        public void MatchmakeSession(string sessionTemplateName, string hopperName)
        {
            Debug.Log($"{nameof(SessionManager.MatchmakeSession)}");

            // create a new lobby session
            InternalCreateSession(
                sessionTemplateName,
                (bool success) =>
                {
                    // matchmake with the session on success
                    if (success)
                    {
                        InternalMatchmakeSession(hopperName);
                    }
                    else
                    {
                        OnFindSessionCompleted?.Invoke(false);
                    }
                });
        }

        public void CancelMatchmake(string hopperName)
        {
            Debug.Log($"{nameof(SessionManager.CancelMatchmake)}");

            InternalCancelMatchmake(hopperName, (bool success) => { });
        }

        public void LeaveSession()
        {
            Debug.Log($"{nameof(SessionManager.LeaveSession)}");

            InternalLeaveSession(OnLeaveSessionCompleted);
        }

        public void SetSessionActivity()
        {
            Debug.Log($"{nameof(SessionManager.SetSessionActivity)}");

            if (_currentSessionHandle == null)
            {
                Debug.LogWarning($"{nameof(SessionManager.SetSessionActivity)} current session handle was null");
                return;
            }

            var sessionRef = SDK.XBL.XblMultiplayerSessionSessionReference(_currentSessionHandle);

            if (sessionRef == null)
            {
                Debug.LogWarning($"{nameof(SDK.XBL.XblMultiplayerSessionSessionReference)} returned a null session");
                return;
            }

            SDK.XBL.XblMultiplayerSetActivityAsync(
                XboxManager.Instance.ContextHandle,
                sessionRef,
                (int hr) =>
                {
                    if (HR.SUCCEEDED(hr))
                    {
                        Debug.Log($"{nameof(SDK.XBL.XblMultiplayerSetActivityAsync)} succeeded");
                    }
                    else
                    {
                        Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSetActivityAsync)} returned - 0x{hr:X8}");
                    }
                });
        }

        public void WriteSession(XblMultiplayerSessionWriteMode writeMode, Action<bool> onComplete)
        {
            Debug.Log($"{nameof(SessionManager.WriteSession)}");

            if (_currentSessionHandle == null)
            {
                Debug.LogWarning($"{nameof(SessionManager.WriteSession)} current session handle was null");
                return;
            }

            //Write the session
            SDK.XBL.XblMultiplayerWriteSessionAsync(
                XboxManager.Instance.ContextHandle,
                _currentSessionHandle,
                writeMode,
                (Int32 hresult, XblMultiplayerSessionHandle handle) =>
                {
                    if (HR.SUCCEEDED(hresult))
                    {
                        _currentSessionHandle = handle;
                        Debug.Log($"{nameof(SDK.XBL.XblMultiplayerWriteSessionAsync)} succeeded");
                        onComplete?.Invoke(true);
                    }
                    else
                    {
                        _currentSessionHandle = null;
                        Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerWriteSessionAsync)} returned - 0x{hresult:X8}");
                        onComplete?.Invoke(false);
                    }
                });
        }

        public void WriteSessionByHandle(XblMultiplayerSessionWriteMode writeMode, Action<bool> onComplete)
        {
            Debug.Log($"{nameof(SessionManager.WriteSessionByHandle)}");

            if (_currentSessionHandle == null)
            {
                Debug.LogWarning($"{nameof(SessionManager.WriteSessionByHandle)} current session handle was null");
                return;
            }

            //Write the session
            SDK.XBL.XblMultiplayerWriteSessionByHandleAsync(
                XboxManager.Instance.ContextHandle,
                _currentSessionHandle,
                writeMode,
                _joinSessionHandle,
                (Int32 hresult, XblMultiplayerSessionHandle handle) =>
                {
                    if (HR.SUCCEEDED(hresult))
                    {
                        _currentSessionHandle = handle;
                        Debug.Log($"{nameof(SDK.XBL.XblMultiplayerWriteSessionByHandleAsync)} succeeded");
                        onComplete?.Invoke(true);
                    }
                    else
                    {
                        _currentSessionHandle = null;
                        Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerWriteSessionByHandleAsync)} returned - 0x{hresult:X8}");
                        onComplete?.Invoke(false);
                    }
                });
        }

        public void ShowPlatformInviteUI(XUserHandle requestingUser)
        {
            Debug.Log($"{nameof(SessionManager.ShowPlatformInviteUI)}");

            SDK.XGameUiShowSendGameInviteAsync(
                requestingUser,
                SessionConfiguration.SCID,
                _currentSessionTemplateName,
                _currentSessionName,
                null,
                null,
                (Int32 hresult) =>
                {
                    if (HR.SUCCEEDED(hresult))
                    {
                        Debug.Log($"{nameof(SDK.XGameUiShowSendGameInviteAsync)} succeeded");
                    }
                    else
                    {
                        Debug.LogError($"{nameof(SDK.XGameUiShowSendGameInviteAsync)} returned - 0x{hresult:X8}");
                    }
                });
        }

        public void SetHostDeviceToken()
        {
            Debug.Log($"{nameof(SessionManager.SetHostDeviceToken)}");

            if (_currentSessionHandle == null)
            {
                Debug.LogWarning($"{nameof(SessionManager.SetHostDeviceToken)} current session handle was null");
                return;
            }

            if (XboxManager.Instance.ContextHandle == null)
            {
                Debug.LogWarning($"{nameof(SessionManager.SetHostDeviceToken)} XBL context handle was null");
                return;
            }

            XblMultiplayerSessionMember[] sessionMembers;
            SDK.XBL.XblMultiplayerSessionMembers(
                _currentSessionHandle,
                out sessionMembers);

            foreach (var sessionMember in sessionMembers)
            {
                if (sessionMember.Xuid == XboxManager.Instance.UserId)
                {
                    SDK.XBL.XblMultiplayerSessionSetHostDeviceToken(
                        _currentSessionHandle,
                        sessionMember.DeviceToken);
                    return;
                }
            }

            Debug.LogWarning($"{nameof(SessionManager.SetHostDeviceToken)} could not set host device token");
        }

        public bool IsInSession()
        {
            return _currentSessionHandle != null;
        }

        public void ProcessSessionChanged(XblMultiplayerSessionChangeEventArgs eventArgs)
        {
            // compare the change number to the last good one
            if (_lastSessionChangeNumber >= eventArgs.ChangeNumber)
            {
                return;
            }

            _lastSessionChangeNumber = eventArgs.ChangeNumber;

            // get the session for the reference
            SDK.XBL.XblMultiplayerGetSessionAsync(
                XboxManager.Instance.ContextHandle,
                eventArgs.SessionReference,
                (int hresult, XblMultiplayerSessionHandle sessionHandle) =>
                {
                    if (HR.FAILED(hresult))
                    {
                        Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerGetSessionAsync)} returned - 0x{hresult:X8}");
                    }
                    else
                    {
                        // get the session info for the new handle
                        XblMultiplayerSessionReference updatedSessionRef = SDK.XBL.XblMultiplayerSessionSessionReference(sessionHandle);

                        // compare the sessions
                        var changes = SDK.XBL.XblMultiplayerSessionCompare(sessionHandle, _currentSessionHandle);

                        // store the updated session handle
                        if (_currentSessionHandle != null)
                        {
                            SDK.XBL.XblMultiplayerSessionCloseHandle(_currentSessionHandle);
                            _currentSessionHandle = null;
                        }

                        _currentSessionHandle = sessionHandle;
                        _currentSessionName = updatedSessionRef.SessionName;
                        _currentSessionTemplateName = updatedSessionRef.SessionTemplateName;

                        // process the session changes
                        ProcessSessionChangeComplete(_currentSessionName, changes);
                    }
                });
        }

        public void ProcessSessionChangeComplete(string sessionName, XblMultiplayerSessionChangeTypes changes)
        {
            if ((changes & XblMultiplayerSessionChangeTypes.None) == XblMultiplayerSessionChangeTypes.None)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::None");
            }

            if ((changes & XblMultiplayerSessionChangeTypes.Everything) == XblMultiplayerSessionChangeTypes.Everything)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::Everything");
            }

            if ((changes & XblMultiplayerSessionChangeTypes.HostDeviceTokenChange) == XblMultiplayerSessionChangeTypes.HostDeviceTokenChange)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::HostDeviceTokenChange");
            }

            if ((changes & XblMultiplayerSessionChangeTypes.InitializationStateChange) == XblMultiplayerSessionChangeTypes.InitializationStateChange)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::InitializationStateChange");
                HandleInitializationStateChanged();
            }

            if ((changes & XblMultiplayerSessionChangeTypes.MatchmakingStatusChange) == XblMultiplayerSessionChangeTypes.MatchmakingStatusChange)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::MatchmakingStatusChange");
                HandleMatchmakingStatusChange();
            }

            if ((changes & XblMultiplayerSessionChangeTypes.MemberListChange) == XblMultiplayerSessionChangeTypes.MemberListChange)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::MemberListChange");
                HandleMemberListChanged();
            }

            if ((changes & XblMultiplayerSessionChangeTypes.MemberStatusChange) == XblMultiplayerSessionChangeTypes.MemberStatusChange)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::MemberStatusChange");
            }

            if ((changes & XblMultiplayerSessionChangeTypes.SessionJoinabilityChange) == XblMultiplayerSessionChangeTypes.SessionJoinabilityChange)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::SessionJoinabilityChange");
            }

            if ((changes & XblMultiplayerSessionChangeTypes.CustomPropertyChange) == XblMultiplayerSessionChangeTypes.CustomPropertyChange)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::CustomPropertyChange");
            }

            if ((changes & XblMultiplayerSessionChangeTypes.MemberCustomPropertyChange) == XblMultiplayerSessionChangeTypes.MemberCustomPropertyChange)
            {
                Debug.Log($"{nameof(SessionManager.ProcessSessionChangeComplete)} SessionName: {sessionName} XblMultiplayerSessionChangeTypes::MemberCustomPropertyChange");
            }
        }

        public void HandleInitializationStateChanged()
        {
            // left intentionally blank for now.
        }

        public void HandleMatchmakingStatusChange()
        {
            Debug.Log($"{nameof(SessionManager.HandleMatchmakingStatusChange)}");

            if (_currentSessionHandle == null)
            {
                Debug.LogWarning($"{nameof(SessionManager.HandleMatchmakingStatusChange)} current session handle was null");
                return;
            }

            XblMultiplayerMatchmakingServer matchmakingServer =
                SDK.XBL.XblMultiplayerSessionMatchmakingServer(_currentSessionHandle);
            if (matchmakingServer == null)
            {
                Debug.LogWarning($"{nameof(SDK.XBL.XblMultiplayerSessionMatchmakingServer)} returned null");
                return;
            }

            XblMatchmakingStatus matchState = matchmakingServer.Status;

            switch (matchState)
            {
                case XblMatchmakingStatus.Unknown:
                    {
                        Debug.Log($"{nameof(SessionManager.HandleMatchmakingStatusChange)} MatchStatus = Unknown");
                        break;
                    }
                case XblMatchmakingStatus.None:
                    {
                        Debug.Log($"{nameof(SessionManager.HandleMatchmakingStatusChange)} MatchStatus = None");
                        break;
                    }
                case XblMatchmakingStatus.Searching:
                    {
                        Debug.Log($"{nameof(SessionManager.HandleMatchmakingStatusChange)} MatchStatus = Searching");
                        break;
                    }
                case XblMatchmakingStatus.Expired:
                    {
                        Debug.Log($"{nameof(SessionManager.HandleMatchmakingStatusChange)} MatchStatus = Expired");
                        OnFindSessionCompleted?.Invoke(false);
                        break;
                    }
                case XblMatchmakingStatus.Found:
                    {
                        Debug.Log($"{nameof(SessionManager.HandleMatchmakingStatusChange)} MatchStatus = Found");
                        OnFindSessionCompleted?.Invoke(true);
                        //m_findMatchTicket = default(XblMatchTicket);
                        break;
                    }
                case XblMatchmakingStatus.Canceled:
                    {
                        Debug.Log($"{nameof(SessionManager.HandleMatchmakingStatusChange)} MatchStatus = Canceled");
                        OnFindSessionCompleted?.Invoke(false);
                        _findMatchTicket = default(XblMatchTicket);
                        break;
                    }
            }
        }

        public void HandleMemberListChanged()
        {
            // maintain a cached list of active session members and compare that
            // with an API call to get the member set so that we know who came and left

            XblMultiplayerSessionMember[] members;
            var hresult = SDK.XBL.XblMultiplayerSessionMembers(_currentSessionHandle, out members);

            if (HR.FAILED(hresult))
            {
                Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSessionMembers)} returned 0x{hresult:X8}");
                return;
            }

            var memberXuids = members.Select(x => x.Xuid).ToList();

            foreach (var memberXuid in memberXuids)
            {
                if (!_sessionMemberXuids.Contains(memberXuid))
                {
                    Debug.Log($">>> Adding user {memberXuid} to session!!!");
                    _sessionMemberXuids.Add(memberXuid);
                    OnPlayerJoinedSession?.Invoke(memberXuid);
                }
            }

            var xuidsToRemove = new List<ulong>();

            foreach (var sessionMemberXuid in _sessionMemberXuids)
            {
                if (!memberXuids.Contains(sessionMemberXuid))
                {
                    Debug.Log($"<<< Removing user {sessionMemberXuid} from session!!!");
                    xuidsToRemove.Add(sessionMemberXuid);
                    OnPlayerLeftSession?.Invoke(sessionMemberXuid);
                }
            }

            foreach (var xuidToRemove in xuidsToRemove)
            {
                _sessionMemberXuids.Remove(xuidToRemove);
            }
        }

        private void InternalCreateSession(
            string sessionTemplateName,
            Action<bool> completionCallback)
        {
            if (string.IsNullOrEmpty(sessionTemplateName))
            {
                Debug.LogWarning($"{nameof(SessionManager.CreateSession)} session template name was invalid");
                completionCallback?.Invoke(false);
                return;
            }

            if (XboxManager.Instance.ContextHandle == null)
            {
                Debug.LogWarning($"{nameof(SessionManager.CreateSession)} XBL context handle was null");
                completionCallback?.Invoke(false);
                return;
            }

            _currentSessionTemplateName = sessionTemplateName;
            _currentSessionName = Guid.NewGuid().ToString();

            if (_currentSessionHandle != null)
            {
                SDK.XBL.XblMultiplayerSessionCloseHandle(_currentSessionHandle);
                _currentSessionHandle = null;
            }

            //Create the new session
            XblMultiplayerSessionReference createdSessionRef = SDK.XBL.XblMultiplayerSessionReferenceCreate(
                SessionConfiguration.SCID,
                _currentSessionTemplateName,
                _currentSessionName);

            XblMultiplayerSessionInitArgs initArgs = new XblMultiplayerSessionInitArgs()
            {
                MaxMembersInSession = 8,//This matches our session template
                Visibility = XblMultiplayerSessionVisibility.Open, //The session is open and can be joined by anyone.
                InitiatorXuids = new ulong[] { XboxManager.Instance.UserId },
                CustomJson = null
            };

            _currentSessionHandle = SDK.XBL.XblMultiplayerSessionCreateHandle(
                XboxManager.Instance.UserId,
                createdSessionRef,
                initArgs);
            if (_currentSessionHandle == null)
            {
                Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSessionCreateHandle)} returned a null session handle");
                completionCallback?.Invoke(false);
                return;
            }
            else
            {
                Debug.Log($"{nameof(SDK.XBL.XblMultiplayerSessionCreateHandle)} succeeded");
                _lastSessionChangeNumber = 0;
            }

            SDK.XBL.XblMultiplayerSessionPropertiesSetJoinRestriction(_currentSessionHandle, XblMultiplayerSessionRestriction.Followed);
            SDK.XBL.XblMultiplayerSessionPropertiesSetReadRestriction(_currentSessionHandle, XblMultiplayerSessionRestriction.Followed);

            //Join the new session
            var hr = SDK.XBL.XblMultiplayerSessionJoin(_currentSessionHandle, null, true, true);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSessionJoin)} returned 0x{hr:X8}");
                completionCallback?.Invoke(false);
                return;
            }
            else
            {
                Debug.Log($"{nameof(SDK.XBL.XblMultiplayerSessionJoin)} succeeded");
            }

            RegisterSessionChangedEvent();

            WriteSession(
                XblMultiplayerSessionWriteMode.CreateNew,
                (bool success) =>
                {
                    if (success)
                    {
                        SetSessionActivity();
                        HandleMemberListChanged();
                    }

                    completionCallback?.Invoke(success);
                });
        }

        private void InternalCancelMatchmake(string hopperName, Action<bool> completionCallback)
        {
            if (_findMatchTicket == null || string.IsNullOrEmpty(_findMatchTicket.MatchTicketId))
            {
                Debug.LogWarning($"{nameof(SessionManager.CancelMatchmake)} MatchTicketId was null or empty");
                completionCallback?.Invoke(false);
                return;
            }

            SDK.XBL.XblMatchmakingDeleteMatchTicketAsync(
                XboxManager.Instance.ContextHandle,
                SessionConfiguration.SCID,
                hopperName,
                _findMatchTicket.MatchTicketId,
                (int hresult) =>
                {
                    if (HR.SUCCEEDED(hresult))
                    {
                        _findMatchTicket = default(XblMatchTicket);
                        completionCallback?.Invoke(true);
                    }
                    else
                    {
                        Debug.LogError($"{nameof(SDK.XBL.XblMatchmakingDeleteMatchTicketAsync)} returned 0x{hresult:X8}");
                        completionCallback?.Invoke(false);
                    }
                });
        }

        private void InternalMatchmakeSession(string hopperName)
        {
            var sessionRef = SDK.XBL.XblMultiplayerSessionSessionReference(_currentSessionHandle);

            if (sessionRef == null)
            {
                Debug.LogWarning($"{nameof(SDK.XBL.XblMultiplayerSessionSessionReference)} returned null");
                OnFindSessionCompleted?.Invoke(false);
            }

            SDK.XBL.XblMatchmakingCreateMatchTicketAsync(
                XboxManager.Instance.ContextHandle,
                sessionRef,
                SessionConfiguration.SCID,
                hopperName,
                SessionConfiguration.MATCHMAKE_TIMEOUT_MS,
                XblPreserveSessionMode.Never,
                string.Empty,
                (int hresult, XblMatchTicket matchTicket) =>
                {
                    if (HR.FAILED(hresult))
                    {
                        InternalLeaveSession(
                            (bool success) =>
                            {
                                OnFindSessionCompleted?.Invoke(false);
                            });
                    }
                    else
                    {
                        _findMatchTicket = matchTicket;
                    }
                });
        }

        private void InternalJoinSession()
        {
            var hr = SDK.XBL.XblMultiplayerSessionJoin(
                _currentSessionHandle,
                null,
                true,
                true);

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSessionJoin)} returned 0x{hr:X8}");
                OnJoinSessionCompleted?.Invoke(false);
                return;
            }

            RegisterSessionChangedEvent();

            hr = SDK.XBL.XblMultiplayerSessionCurrentUserSetStatus(
                _currentSessionHandle,
                XblMultiplayerSessionMemberStatus.Active);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSessionCurrentUserSetStatus)} returned 0x{hr:X8}");
                OnJoinSessionCompleted?.Invoke(false);
                return;
            }

            WriteSessionByHandle(
                XblMultiplayerSessionWriteMode.UpdateExisting,
                (bool success) =>
                {
                    if (success)
                    {
                        SetSessionActivity();
                        HandleMemberListChanged();
                    }

                    OnJoinSessionCompleted?.Invoke(success);
                });
        }

        private void InternalLeaveSession(Action<bool> completionCallback)
        {
            XboxManager.Instance.ContextHandle.XblMultiplayerSessionChanged -= OnSessionChanged;

            // Clear session activity
            SDK.XBL.XblMultiplayerClearActivityAsync(
            XboxManager.Instance.ContextHandle,
            SessionConfiguration.SCID,
            (int hr) =>
            {
                if (HR.SUCCEEDED(hr))
                {
                    Debug.Log($"{nameof(SDK.XBL.XblMultiplayerClearActivityAsync)} succeeded");
                }
                else
                {
                    Debug.LogWarning($"{nameof(SDK.XBL.XblMultiplayerClearActivityAsync)} returned - 0x{hr:X8}");
                }

                // Leave session
                hr = SDK.XBL.XblMultiplayerSessionLeave(_currentSessionHandle);
                if (HR.FAILED(hr))
                {
                    Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSessionLeave)} returned 0x{hr:X8}");
                    completionCallback?.Invoke(false);
                    return;
                }

                // Close session
                WriteSession(
                    XblMultiplayerSessionWriteMode.SynchronizedUpdate,
                    (bool success) =>
                    {
                        if (success)
                        {
                            _sessionMemberXuids.Clear();

                            if (_currentSessionHandle != null)
                            {
                                SDK.XBL.XblMultiplayerSessionCloseHandle(_currentSessionHandle);
                                _currentSessionHandle = null;
                            }
                        }

                        OnLeaveSessionCompleted?.Invoke(success);
                    });
            });
        }

        private void OnConnectionIdChanged()
        {
            if (_currentSessionHandle == null)
            {
                Debug.LogWarning($"{nameof(SessionManager.OnConnectionIdChanged)} current session handle was null");
                return;
            }

            var hr = SDK.XBL.XblMultiplayerSessionCurrentUserSetStatus(
                _currentSessionHandle,
                XblMultiplayerSessionMemberStatus.Active);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK.XBL.XblMultiplayerSessionCurrentUserSetStatus)} returned 0x{hr:X8}");
                return;
            }

            WriteSession(XblMultiplayerSessionWriteMode.UpdateExisting, null);
        }

        private void OnSessionChanged(XblMultiplayerSessionChangeEventArgs eventArgs)
        {
            ProcessSessionChanged(eventArgs);
        }

        private void OnSubscriptionLost()
        {
            // left intentionally blank for now.
        }

    }
}
