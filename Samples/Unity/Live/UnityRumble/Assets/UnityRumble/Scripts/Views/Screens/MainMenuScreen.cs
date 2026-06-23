//--------------------------------------------------------------------------------------
// MainMenuScreen.cs
//
// The UI class for the main menu screen and functionality.
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
using UnityEngine.Assertions;
using UnityEngine.UI;

public class MainMenuScreen : BaseUiScreen
{
    public event Action FindingGameCompleted;
    public event Action HostingGameCompleted;
    public event Action JoiningGameCompleted;
    public event Action FriendLobbiesObtained;

    public Button MatchButton;
    public Button CancelButton;
    public Button HostButton;
    public Button JoinButton;
    public Button JoinInviteButton;

    public void HandleFindMatchButtonPressed()
    {
        if (XboxLive.IsMatchmaking)
        { return; }

        Debug.LogFormat("MainMenuScreen.HandleFindMatchButtonPressed()");

        OnDisabled();

        // try to matchmake into a new lobby session

        XboxLive.OnSessionMatchMade += HandleSessionMatchMade;
        XboxLive.OnMultiplayerError += (message, hresult) => { HandleMatchmakeError("Session matchmaking failed: {0}, {1}", message, hresult.ToString("X8")); };
        AsyncOpUI.Started(@"Matchmaking into a new game session...");
        XboxLive.StartMatchmaking();

        CancelButton.interactable = true;
        InputEventSystem.SetSelectedGameObject(CancelButton.gameObject, null);
    }

    public void HandleCancelMatchButtonPressed()
    {
        if (XboxLive.IsMatchmaking)
        {
            Debug.LogFormat("MainMenuScreen.HandleCancelMatchButtonPressed()");

            Disable();

            // try to cancel matchmaking into a new lobby session

            XboxLive.OnSessionMatchMakeCancelled += HandleSessionMatchMakeCancelled;
            XboxLive.OnMultiplayerError += (message, hresult) => { HandleError("Cancelling session matchmaking failed: {0}, {1}", message, hresult.ToString("X8")); };
            AsyncOpUI.Started(@"Cancelling matchmaking into a new game lobby session...");
            XboxLive.CancelMatchmaking();
        }
    }

    public void HandleHostButtonPressed()
    {
        Debug.LogFormat("MainMenuScreen.HandleHostButtonPressed()");

        Disable();

        // try to create a new lobby session

        XboxLive.OnLobbyCreated += HandleLobbyCreated;
        XboxLive.OnMultiplayerError += (message, hresult) => { HandleError("Hosting lobby session failed: {0}, {1}", message, hresult.ToString("X8")); };
        AsyncOpUI.Started(@"Hosting a new game lobby session...");
        XboxLive.CreateLobby();
    }

    public void HandleJoinButtonPressed()
    {
        Debug.LogFormat("MainMenuScreen.HandleJoinButtonPressed()");

        Disable();

        // try to get our social group activities

        XboxLive.OnSocialFriendActivitiesObtained += HandleFriendActivitiesObtained;
        XboxLive.OnSocialError += (message, hresult) => { HandleError("Getting friend activities failed: {0}, {1}", message, hresult.ToString("X8")); };
        AsyncOpUI.Started(@"Looking for in-title friend lobbies...");
        XboxLive.GetSocialFriendActivities();
    }

    public void HandleJoinInviteButtonPressed()
    {
        Debug.LogFormat("MainMenuScreen.HandleJoinInviteButtonPressed()");

        Disable();

        // try to join an invited lobby session

        XboxLive.OnLobbyJoined += HandleLobbyJoined;
        XboxLive.OnMultiplayerError += (message, hresult) => { HandleError("Joining lobby session failed: {0}, {1}", message, hresult.ToString("X8")); };
        AsyncOpUI.Started(@"Joining an invited game lobby session...");
        XboxLive.JoinInvitedLobby();
    }

    protected override void OnValidate()
    {
        base.OnValidate();

        Assert.IsNotNull(MatchButton);
        Assert.IsNotNull(CancelButton);
        Assert.IsNotNull(HostButton);
        Assert.IsNotNull(JoinButton);
        Assert.IsNotNull(JoinInviteButton);
    }

    protected override void OnEnabled()
    {
        var hasMultiplayerPrivileges = XboxLive.HasMultiplayerPrivileges;
        var hasMultiplayerInvite = XboxLive.HasMultiplayerInvite;
        var isMatchmaking = XboxLive.IsMatchmaking;

        MatchButton.interactable = hasMultiplayerPrivileges;
        CancelButton.interactable = isMatchmaking;
        HostButton.interactable = hasMultiplayerPrivileges;
        JoinButton.interactable = hasMultiplayerPrivileges;
        JoinInviteButton.interactable = hasMultiplayerPrivileges && hasMultiplayerInvite;

        // an invite can come at anytime while we are on this screen
        XboxLive.OnInviteReceived += HandleInviteReceived;

        base.OnEnabled();
    }

    protected override void OnDisabled()
    {
        base.OnDisabled();

        MatchButton.interactable = false;
        CancelButton.interactable = false;
        HostButton.interactable = false;
        JoinButton.interactable = false;
        JoinInviteButton.interactable = false;

        // if we are disabled, then we need to stop caring about an invite that may come
        XboxLive.OnInviteReceived -= HandleInviteReceived;
    }

    private void HandleFriendActivitiesObtained()
    {
        Debug.LogFormat("MainMenuScreen.__HandleFriendActivitiesObtained()");

        AsyncOpUI.Finished();
        XboxLive.OnSocialFriendActivitiesObtained -= HandleFriendActivitiesObtained;

        FriendLobbiesObtained?.Invoke();
    }

    private void HandleSessionMatchMade()
    {
        Debug.LogFormat("MainMenuScreen.__HandleSessionMatchMade()");

        Disable();

        AsyncOpUI.Finished();
        XboxLive.OnSessionMatchMade -= HandleSessionMatchMade;

        if (XboxLive.IsHost)
        {
            Networking.OnNetworkStarted += HandleNetworkStarted;
            AsyncOpUI.Started(@"Starting session network...");
            Networking.StartNetworking(XboxLive.MyXUID, null);
        }
        else
        {
            WaitForSessionNetworkId();
        }
    }

    private void WaitForSessionNetworkId()
    {
        if (string.IsNullOrEmpty(XboxLive.CurrentSessionDocument.NetworkID))
        {
            AsyncOpUI.Started(@"Waiting for designated host...");
            XboxLive.OnSessionPropertiesChanged += HandleSessionPropertiesChanged;
        }
        else
        {
            HandleSessionPropertiesChanged(XboxLive.CurrentSessionDocument);
        }
    }

    private void HandleSessionMatchMakeCancelled()
    {
        Debug.LogFormat("MainMenuScreen.__HandleSessionMatchMakeCancelled()");

        AsyncOpUI.Finished();
        XboxLive.OnSessionMatchMakeCancelled -= HandleSessionMatchMakeCancelled;
        XboxLive.ClearMultiplayerEventHandlers();

        Enable();
    }

    private void HandleInviteReceived()
    {
        Debug.LogFormat("MainMenuScreen.__HandleInviteReceived()");

        // we are responding to an invite so we do not need to listen anymore
        XboxLive.OnInviteReceived -= HandleInviteReceived;
        
        var hasMultiplayerPrivileges = XboxLive.HasMultiplayerPrivileges;
        var hasMultiplayerInvite = XboxLive.HasMultiplayerInvite;

        JoinInviteButton.interactable = hasMultiplayerPrivileges && hasMultiplayerInvite;
    }

    private void HandleLobbyJoined()
    {
        Debug.LogFormat("MainMenuScreen.__HandleSessionJoined()");

        AsyncOpUI.Finished();
        XboxLive.OnLobbyJoined -= HandleLobbyJoined;

        WaitForSessionNetworkId();
    }

    private void HandleSessionPropertiesChanged(SessionDocument sessionDocument)
    {
        if (!string.IsNullOrEmpty(sessionDocument.NetworkID))
        {
            XboxLive.OnSessionPropertiesChanged -= HandleSessionPropertiesChanged;
            Networking.OnNetworkStarted += HandleNetworkStarted;
            AsyncOpUI.Started(@"Joining session network...");
            Networking.StartNetworking(XboxLive.MyXUID, sessionDocument.NetworkID);
        }
    }

    private void HandleLobbyCreated()
    {
        Debug.LogFormat("MainMenuScreen.__HandleSessionCreated()");

        AsyncOpUI.Finished();
        XboxLive.OnLobbyCreated -= HandleLobbyCreated;

        Networking.OnNetworkStarted += HandleNetworkStarted;
        AsyncOpUI.Started(@"Starting session network...");
        Networking.StartNetworking(XboxLive.MyXUID, null);
    }

    private void HandleNetworkStarted(string networkId)
    {
        Debug.LogFormat("MainMenuScreen.__HandleNetworkStarted()");

        AsyncOpUI.Finished();
        Networking.OnNetworkStarted -= HandleNetworkStarted;

        if (XboxLive.IsHost)
        {
            XboxLive.CurrentSessionDocument.NetworkID = networkId;
            XboxLive.SetSessionProperties(XboxLive.CurrentSessionDocument);

            if (XboxLive.CurrentSessionState == XboxLiveLogic.SessionState.InMatchedLobby)
            {
                FindingGameCompleted?.Invoke();
            }
            else
            {
                HostingGameCompleted?.Invoke();
            }
        }
        else
        {
            if (XboxLive.CurrentSessionState == XboxLiveLogic.SessionState.InMatchedLobby)
            {
                FindingGameCompleted?.Invoke();
            }
            else
            {
                JoiningGameCompleted?.Invoke();
            }
        }
    }

    private void HandleError(string errorMessage, params object[] args)
    {
        AsyncOpUI.Finished();
        XboxLive.ClearMultiplayerEventHandlers();

        Debug.LogFormat(errorMessage, args);
        StatusBarText.text = string.Format(errorMessage, args);

        Enable();
    }

    private void HandleMatchmakeError(string errorMessage, params object[] args)
    {
        AsyncOpUI.Finished();
        XboxLive.ClearMultiplayerEventHandlers();

        Debug.LogFormat(errorMessage, args);
        StatusBarText.text = string.Format(errorMessage, args);

        XboxLive.OnSessionMatchMakeCancelled += HandleSessionMatchMakeCancelled;
        XboxLive.OnMultiplayerError += (message, hresult) => { HandleError("Cancelling session matchmaking failed: {0}, {1}", message, hresult.ToString("X8")); };

        Disable();
    }
}
