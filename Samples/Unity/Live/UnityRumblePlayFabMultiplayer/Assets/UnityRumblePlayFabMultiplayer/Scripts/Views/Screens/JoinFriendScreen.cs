//--------------------------------------------------------------------------------------
// JoinFriendScreen.cs
//
// The UI class for the join friend screen and functionality.
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
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UI;

public class JoinFriendScreen : BaseUiScreen
{
    public event Action JoiningFriendCompleted;
    public event Action JoiningFriendCancelled;

    public Text NoInTitleFriendLobbiesText;
    public Button[] FriendGameButtons;
    public Button CancelButton;

    private readonly List<XboxLiveLogic.SocialFriend> _socialFriendGames = new List<XboxLiveLogic.SocialFriend>();

    public void HandleFriendGameButtonPressed(int buttonIndex)
    {
        Debug.Log("MainMenuScreen.HandleJoinInviteButtonPressed()");

        Disable();

        // try to join a friend's lobby session
        var lobbyConnectionString = _socialFriendGames[buttonIndex].LobbyConnectionString;

        MultiplayerLogic.OnLobbyJoined += HandleLobbyJoined;
        MultiplayerLogic.OnMultiplayerError += (message) => { HandleError($"Joining lobby session failed: {message}"); };
        AsyncOpUI.Started(@"Joining friend's game lobby session...");
        MultiplayerLogic.JoinLobby(lobbyConnectionString);
    }

    public void HandleCancelButtonPressed()
    {
        JoiningFriendCancelled?.Invoke();
    }

    protected override void OnValidate()
    {
        base.OnValidate();

        Assert.IsNotNull(NoInTitleFriendLobbiesText);
        foreach (var friendGameButton in FriendGameButtons)
        {
            Assert.IsNotNull(friendGameButton);
        }
        Assert.IsNotNull(CancelButton);
    }

    protected override void OnShown()
    {
        base.OnShown();

        // populate the list of social friend games
        XboxLive.GetOnlineInTitleFriends(out XboxLiveLogic.SocialFriend[] socialFriends);

        _socialFriendGames.Clear();
        if (socialFriends != null)
        {
            foreach (var friend in socialFriends)
            {
                if (friend.CurrentActivityInfo != null)
                {
                    _socialFriendGames.Add(friend);
                }

                if (_socialFriendGames.Count == FriendGameButtons.Length)
                {
                    break;
                }
            }
        }

        // hide no lobbies text if we at least have one accessible friend lobby we can join
        NoInTitleFriendLobbiesText.gameObject.SetActive(_socialFriendGames.Count == 0);

        // hide friend game buttons if we do not have enough friend games to attempt to join
        var friendIndex = 0;
        while (friendIndex < _socialFriendGames.Count)
        {
            FriendGameButtons[friendIndex].gameObject.SetActive(true);
            FriendGameButtons[friendIndex].transform.GetComponentInChildren<TMPro.TMP_Text>().text = _socialFriendGames[friendIndex].GamerTag;
            FriendGameButtons[friendIndex].transform.GetComponentInChildren<XboxLiveProfilePicUI>().LoadProfilePic(_socialFriendGames[friendIndex].GamerTag);
            friendIndex++;
        }
        while (friendIndex < FriendGameButtons.Length)
        {
            FriendGameButtons[friendIndex].gameObject.SetActive(false);
            friendIndex++;
        }

        // finally set up the navigation for the buttons that we are showing
        var lastIndex = _socialFriendGames.Count - 1;
        for (var friendGameIndex = 0; friendGameIndex < _socialFriendGames.Count; friendGameIndex++)
        {
            var navigation = new Navigation() { mode = Navigation.Mode.Explicit };

            if (friendGameIndex != 0)
            {
                navigation.selectOnUp = FriendGameButtons[friendGameIndex - 1];
            }

            if (friendGameIndex == lastIndex)
            {
                navigation.selectOnDown = CancelButton;
            }
            else
            {
                navigation.selectOnDown = FriendGameButtons[friendGameIndex + 1];
            }

            FriendGameButtons[friendGameIndex].navigation = navigation;
        }

        CancelButton.navigation = new Navigation() 
        { 
            mode = Navigation.Mode.Explicit,
            selectOnUp = _socialFriendGames.Count > 0 ? FriendGameButtons[lastIndex] : null
        };
    }

    protected override void OnEnabled()
    {
        var hasMultiplayerPrivileges = MultiplayerLogic.InviteAndActivity.HasMultiplayerPrivileges;

        foreach (var friendGameButton in FriendGameButtons)
        {
            friendGameButton.interactable = hasMultiplayerPrivileges;
        }
        CancelButton.interactable = true;

        base.OnEnabled();
    }

    protected override void OnDisabled()
    {
        base.OnDisabled();

        foreach (var friendGameButton in FriendGameButtons)
        {
            friendGameButton.interactable = false;
        }
        CancelButton.interactable = false;
    }

    protected override void OnHidden()
    {
        base.OnHidden();
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

    private void HandleLobbyJoined()
    {
        Debug.Log("JoinFriendScreen.HandleSessionJoined()");

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        WaitForSessionNetworkId();
    }

    private void HandleNetworkStarted(string networkId)
    {
        Debug.Log("JoinFriendScreen.HandleNetworkStarted()");

        AsyncOpUI.Finished();
        Networking.OnNetworkStarted -= HandleNetworkStarted;

        JoiningFriendCompleted?.Invoke();
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
