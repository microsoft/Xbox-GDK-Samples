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
    public event Action LobbyBrowserOpened;

    public Button MatchButton;
    public Button CancelButton;
    public Button HostButton;
    public Button JoinButton;
    public Button JoinInviteButton;
    public Button LobbyBrowserButton;

    public void HandleFindMatchButtonPressed()
    {
        if (!MultiplayerLogic.IsMatchmaking)
        {
            Debug.Log("MainMenuScreen.HandleFindMatchButtonPressed()");

            OnDisabled();

            // try to matchmake into a new lobby session

            MultiplayerLogic.OnSessionMatchMade += HandleSessionMatchMade;
            MultiplayerLogic.OnSessionMatchMakeCancelled += HandleSessionMatchMakeCancelled;
            MultiplayerLogic.OnMultiplayerError += (message) => { HandleError($"Session matchmaking failed: {message}"); };
            AsyncOpUI.Started("Matchmaking into a new game session...");
            MultiplayerLogic.StartMatchmaking();

            CancelButton.interactable = true;
            InputEventSystem.SetSelectedGameObject(CancelButton.gameObject, null);
        }
    }

    public void HandleCancelMatchButtonPressed()
    {
        Debug.Log("MainMenuScreen.HandleCancelMatchButtonPressed()");
        Disable();

        // try to cancel matchmaking into a new lobby session
        if (MultiplayerLogic.IsMatchmaking)
        {
            MultiplayerLogic.OnMultiplayerError += (message) => { HandleError($"Cancelling session matchmaking failed: {message}"); };
            AsyncOpUI.Started("Canceling matchmaking into a new game lobby session...");
            MultiplayerLogic.CancelMatchmaking();
        }
    }

    public void HandleHostButtonPressed()
    {
        Debug.Log("MainMenuScreen.HandleHostButtonPressed()");

        Disable();

        // try to create a new lobby session

        MultiplayerLogic.OnLobbyCreated += HandleLobbyCreated;
        MultiplayerLogic.OnMultiplayerError += (message) => { HandleError($"Hosting lobby session failed: {message}"); };
        AsyncOpUI.Started(@"Hosting a new game lobby session...");
        MultiplayerLogic.CreateLobby();
    }

    public void HandleJoinButtonPressed()
    {
        Debug.Log("MainMenuScreen.HandleJoinButtonPressed()");

        Disable();

        // try to get our social group activities
        MultiplayerLogic.InviteAndActivity.OnSocialFriendActivitiesObtained += HandleFriendActivitiesObtained;
        MultiplayerLogic.InviteAndActivity.OnSocialActivityError +=
            (message, hr) =>
            {
                HandleError($"Getting friend activities failed: {message} with HRESULT:{hr.ToString("X8")}");
            };
        AsyncOpUI.Started(@"Looking for in-title friend lobbies...");
        MultiplayerLogic.InviteAndActivity.GetFriendActivities();
    }

    public void HandleJoinInviteButtonPressed()
    {
        Debug.Log("MainMenuScreen.HandleJoinInviteButtonPressed()");

        Disable();

        // try to join an invited lobby session

        MultiplayerLogic.OnLobbyJoined += HandleLobbyJoined;
        MultiplayerLogic.OnMultiplayerError += (message) => { HandleError($"Joining lobby session failed: {message}"); };
        AsyncOpUI.Started(@"Joining an invited game lobby session...");
        MultiplayerLogic.JoinInvitedLobby();
    }

    public void HandleLobbyBrowserButtonPressed()
    {
        Debug.Log("MainMenuScreen.HandleLobbyBrowserButtonPressed()");
        LobbyBrowserOpened?.Invoke();
    }

    protected override void OnValidate()
    {
        base.OnValidate();

        Assert.IsNotNull(MatchButton);
        Assert.IsNotNull(CancelButton);
        Assert.IsNotNull(HostButton);
        Assert.IsNotNull(JoinButton);
        Assert.IsNotNull(JoinInviteButton);
        Assert.IsNotNull(LobbyBrowserButton);
    }

    protected override void OnEnabled()
    {
        var hasMultiplayerPrivileges = MultiplayerLogic.InviteAndActivity.HasMultiplayerPrivileges;
        var hasMultiplayerInvite = MultiplayerLogic.InviteAndActivity.HasMultiplayerInvite;
        var isMatchmaking = MultiplayerLogic.IsMatchmaking;

        MatchButton.interactable = hasMultiplayerPrivileges;
        CancelButton.interactable = isMatchmaking;
        HostButton.interactable = hasMultiplayerPrivileges;
        JoinButton.interactable = hasMultiplayerPrivileges;
        JoinInviteButton.interactable = hasMultiplayerPrivileges && hasMultiplayerInvite;
        LobbyBrowserButton.interactable = hasMultiplayerPrivileges;

        // an invite can come at anytime while we are on this screen
        MultiplayerLogic.InviteAndActivity.OnInviteReceived += HandleInviteReceived;

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
        LobbyBrowserButton.interactable = false;

        // if we are disabled, then we need to stop caring about an invite that may come
        MultiplayerLogic.InviteAndActivity.OnInviteReceived -= HandleInviteReceived;
    }

    private void HandleFriendActivitiesObtained()
    {
        Debug.Log("MainMenuScreen.HandleFriendActivitiesObtained()");

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        FriendLobbiesObtained?.Invoke();
    }

    private void HandleSessionMatchMade()
    {
        Debug.Log("MainMenuScreen.HandleSessionMatchMade()");

        Disable();

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        if (MultiplayerLogic.IsHost)
        {
            Networking.OnNetworkStarted += HandleNetworkStarted;
            AsyncOpUI.Started(@"Starting session network...");
            Networking.StartNetworking(MultiplayerLogic.MyUserID, null);
        }
        else
        {
            WaitForSessionNetworkId();
        }
    }

    private void WaitForSessionNetworkId()
    {
        if (string.IsNullOrEmpty(MultiplayerLogic.CurrentSessionDocument.NetworkID))
        {
            AsyncOpUI.Started(@"Waiting for designated host...");
            MultiplayerLogic.OnSessionPropertiesChanged += HandleSessionPropertiesChanged;
        }
        else
        {
            HandleSessionPropertiesChanged(MultiplayerLogic.CurrentSessionDocument);
        }
    }

    private void HandleSessionMatchMakeCancelled()
    {
        Debug.Log("MainMenuScreen.HandleSessionMatchMakeCancelled()");

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        Enable();
    }

    private void HandleInviteReceived()
    {
        Debug.Log("MainMenuScreen.HandleInviteReceived()");

        // we are responding to an invite so we do not need to listen anymore
        MultiplayerLogic.InviteAndActivity.OnInviteReceived -= HandleInviteReceived;
        
        var hasMultiplayerPrivileges = MultiplayerLogic.InviteAndActivity.HasMultiplayerPrivileges;
        var hasMultiplayerInvite = MultiplayerLogic.InviteAndActivity.HasMultiplayerInvite;

        JoinInviteButton.interactable = hasMultiplayerPrivileges && hasMultiplayerInvite;        
    }

    private void HandleLobbyJoined()
    {
        Debug.Log("MainMenuScreen.HandleSessionJoined()");

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        WaitForSessionNetworkId();
    }

    private void HandleSessionPropertiesChanged(SessionDocument sessionDocument)
    {
        if (!string.IsNullOrEmpty(sessionDocument.NetworkID))
        {
            MultiplayerLogic.OnSessionPropertiesChanged -= HandleSessionPropertiesChanged;
            Networking.OnNetworkStarted += HandleNetworkStarted;
            AsyncOpUI.Started(@"Joining session network...");
            Networking.StartNetworking(MultiplayerLogic.MyUserID, sessionDocument.NetworkID);
        }
    }

    private void HandleLobbyCreated()
    {
        Debug.Log("MainMenuScreen.HandleSessionCreated()");

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        Networking.OnNetworkStarted += HandleNetworkStarted;
        AsyncOpUI.Started(@"Starting session network...");
        Networking.StartNetworking(MultiplayerLogic.MyUserID, null);
    }

    private void HandleNetworkStarted(string networkId)
    {
        Debug.Log("MainMenuScreen.HandleNetworkStarted()");

        AsyncOpUI.Finished();
        Networking.OnNetworkStarted -= HandleNetworkStarted;

        if (MultiplayerLogic.IsHost)
        {
            MultiplayerLogic.CurrentSessionDocument.NetworkID = networkId;
            MultiplayerLogic.SetSessionProperties(MultiplayerLogic.CurrentSessionDocument);

            if (MultiplayerLogic.CurrentSessionState == BaseMultiplayerLogic.SessionState.InMatchedLobby)
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
            if (MultiplayerLogic.CurrentSessionState == BaseMultiplayerLogic.SessionState.InMatchedLobby)
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
        MultiplayerLogic.ClearEventHandlers();

        Debug.LogFormat(errorMessage, args);
        StatusBarText.text = string.Format(errorMessage, args);

        Enable();
    }
}
