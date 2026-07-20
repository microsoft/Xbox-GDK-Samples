//--------------------------------------------------------------------------------------
// XboxLiveMatchmakingLogic.cs
//
// Xbox Live logic for handling SmartMatch matchmaking related logic.
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
using UnityEngine;
using Unity.XGamingRuntime;
using HR = Unity.XGamingRuntime.Interop.HR;

public partial class XboxLiveLogic
{
    public event Action OnSessionMatchMade;
    public event Action OnSessionMatchMakeCancelled;

    public void StartMatchmaking()
    {
        Debug.LogFormat("XboxLive.StartMatchmaking()");

        if (IsMatchmaking)
        {
            return;
        }

        // When matchmaking a game we first join the local user to MPM and wait for the UserAdded event
        var hresult = SDK.XBL.XblMultiplayerManagerLobbySessionAddLocalUser(MyUserHandle);

        if (HR.FAILED(hresult))
        {
            Debug.LogErrorFormat("XblMultiplayerManagerLobbySessionAddLocalUser() failed with hresult = {0}.", hresult.ToString("X8"));
            OnMultiplayerError?.Invoke("XblMultiplayerManagerLobbySessionAddLocalUser() failed", hresult);
            return;
        }

        IsMatchmaking = true;
    }

    public void CancelMatchmaking()
    {
        Debug.LogFormat("XboxLive.CancelMatchmaking()");

        if (IsMatchmaking)
        {
            SDK.XBL.XblMultiplayerManagerCancelMatch();
        }
    }

    private void StartMatchmakingProcess()
    {
        Debug.LogFormat("XboxLive.__StartMatchmakingProcess()");

        var hr = SDK.XBL.XblMultiplayerManagerFindMatch(
            Configuration.MATCHMAKING_HOPPER_NAME,          // Partner Center configured match hopper name
            Configuration.MATCHMAKING_ATTRIBUTES_JSON,      // Match session attributes
            Configuration.MATCHMAKING_TIMEOUT_IN_SECONDS    // Timeout period in seconds
            );

        if (HR.FAILED(hr))
        {
            Debug.LogErrorFormat("XblMultiplayerManagerFindMatch() failed.", hr.ToString("X8"));
        }
    }

    private void HandleMatchmakingCompleted()
    {
        Debug.LogFormat("XboxLiveLogic.__HandleMatchmakingCompleted()");

        Debug.LogFormat(">>> changing session state from {0} to InMatchedLobby", CurrentSessionState);
        CurrentSessionState = SessionState.InMatchedLobby;

        ChooseNewHost();

        IsMatchmaking = false;
        OnSessionMatchMade?.Invoke();
    }

    private void HandleMatchmakingCancelled()
    {
        Debug.LogFormat("XboxLiveLogic.__HandleMatchmakingCancelled()");

        var hresult = SDK.XBL.XblMultiplayerManagerLobbySessionRemoveLocalUser(MyUserHandle);

        if (HR.FAILED(hresult))
        {
            IsMatchmaking = false;
            OnMultiplayerError?.Invoke("XblMultiplayerManagerLobbySessionRemoveLocalUser() failed.", hresult);
        }
    }

    private void HandleMatchmakingProcessStatus(XblMultiplayerMatchStatus matchStatus, XblMultiplayerMeasurementFailure failureCause)
    {
        switch (matchStatus)
        {
            // match statuses:
            //
            // None = 0,
            // SubmittingMatchTicket = 1,
            // Searching = 2,
            // Found = 3,
            // Joining = 4,
            // WaitingForRemoteClientsToJoin = 5,
            // Measuring = 6,
            // UploadingQosMeasurements = 7,
            // WaitingForRemoteClientsToUploadQos = 8,
            // Evaluating = 9,
            // Completed = 10,
            // Resubmitting = 11,
            // Expired = 12,
            // Canceling = 13,
            // Canceled = 14,
            // Failed = 15,
            case XblMultiplayerMatchStatus.Completed:
                {
                    HandleMatchmakingCompleted();
                }
                break;

            case XblMultiplayerMatchStatus.Canceled:
                {
                    HandleMatchmakingCancelled();
                }
                break;

            case XblMultiplayerMatchStatus.Expired:
                {
                    HandleMatchmakingCancelled();
                    XblMultiplayerMeasurementFailure cause = XblMultiplayerMeasurementFailure.Timeout;
                    OnMultiplayerError?.Invoke("Matchmaking expired", (int)cause);
                }
                break;

            case XblMultiplayerMatchStatus.Failed:
                {
                    // failure cases:
                    //
                    // Unknown = 0,
                    // None = 1,
                    // Timeout = 2,
                    // Latency = 3,
                    // BandwidthUp = 4,
                    // BandwidthDown = 5,
                    // Group = 6,
                    // Network = 7,
                    // Episode = 8,
                    HandleMatchmakingCancelled();
                    OnMultiplayerError?.Invoke("Matchmaking failed", (int)failureCause);
                }
                break;

            default:
                break;
        }
    }
}
