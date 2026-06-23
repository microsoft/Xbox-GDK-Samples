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

    private readonly List<XboxLiveLogic.SocialFriend> _socialFriendGames = new();

    public void HandleFriendGameButtonPressed(int buttonIndex)
    {
        Debug.LogFormat("MainMenuScreen.HandleJoinFriendButtonPressed()");

        Disable();

        // try to join a friend lobby session

        var friendSessionHandle = _socialFriendGames[buttonIndex].CurrentActivityDetails.HandleId;

        XboxLive.OnLobbyJoined += HandleLobbyJoined;
        XboxLive.OnMultiplayerError += (message, hresult) => { HandleError("Joining lobby session failed: {0}, {1}", message, hresult.ToString("X8")); };
        AsyncOpUI.Started(@"Joining friend's game lobby session...");
        XboxLive.JoinLobby(friendSessionHandle);
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
                if (friend.CurrentActivityDetails != null)
                {
                    _socialFriendGames.Add(friend);
                }

                if (_socialFriendGames.Count == FriendGameButtons.Length)
                {
                    break;
                }
            }
        }

        // hide no lobbies text if we at least have one joinable friend lobby
        NoInTitleFriendLobbiesText.gameObject.SetActive(_socialFriendGames.Count == 0);

        // hide friend game buttons if we do not have enough friend games to attempt a join
        var friendIndex = 0;
        while (friendIndex < _socialFriendGames.Count)
        {
            FriendGameButtons[friendIndex].gameObject.SetActive(true);
            FriendGameButtons[friendIndex].transform.GetComponentInChildren<TMPro.TMP_Text>().text =
                _socialFriendGames[friendIndex].GamerTag;
            FriendGameButtons[friendIndex].transform.GetComponentInChildren<XboxLiveProfilePicUI>().LoadProfilePic(
                _socialFriendGames[friendIndex].GamerTag);
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
        var hasMultiplayerPrivileges = XboxLive.HasMultiplayerPrivileges;

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

    private void HandleLobbyJoined()
    {
        Debug.LogFormat("JoinFriendScreen.__HandleLobbyJoined()");

        AsyncOpUI.Finished();
        XboxLive.OnLobbyJoined -= HandleLobbyJoined;

        WaitForSessionNetworkId();
    }

    private void HandleNetworkStarted(string networkId)
    {
        Debug.LogFormat("JoinFriendScreen.__HandleNetworkStarted()");

        AsyncOpUI.Finished();
        Networking.OnNetworkStarted -= HandleNetworkStarted;

        JoiningFriendCompleted?.Invoke();
    }

    private void HandleError(string errorMessage, params object[] args)
    {
        AsyncOpUI.Finished();

        if (args == null)
        {
            Debug.LogError(errorMessage);
            StatusBarText.text = errorMessage;
        }
        else
        {
            Debug.LogFormat(errorMessage, args);
            StatusBarText.text = string.Format(errorMessage, args);
        }

        Enable();
    }
}
