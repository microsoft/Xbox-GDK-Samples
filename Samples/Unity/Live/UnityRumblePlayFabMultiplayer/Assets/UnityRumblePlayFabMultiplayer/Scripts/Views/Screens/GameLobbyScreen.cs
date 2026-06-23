//--------------------------------------------------------------------------------------
// GameLobbyScreen.cs
//
// The UI class for the game lobby screen and functionality.
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
using System.Linq;
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UI;

public class GameLobbyScreen : BaseUiScreen
{
    public event Action LeaveLobbyCompleted;
    public event Action TransitionToGameCompleted;

    public Image ShipButtonImage;
    public Image ColorButtonImage;

    public Button ShipButton;
    public Button ColorButton;
    public Toggle ReadyToggle;
    public Button LeaveButton;
    public Button InviteButton;
    public LobbyMembersView LobbyMembersList;
    public GameController TheGameController;
    public GameAssetManager TheGameAssetManager;

    Coroutine _countdownCoroutine;
    int _currentShipImageIndex = 0;
    int _currentShipColorIndex = 0;

    public void HandleShipButtonPressed()
    {
        _currentShipImageIndex += 1;
        _currentShipImageIndex %= TheGameAssetManager.PlayerShipSprites.Length;

        PlayerPrefs.SetInt($"{MultiplayerLogic.MyUserID.ID}.ShipIndex", _currentShipImageIndex);

        RefreshShipAndColor();

        Networking.SendMessageToAll(new PlayerLobbyState(ReadyToggle.isOn, _currentShipImageIndex, _currentShipColorIndex));
    }

    public void HandleColorButtonPressed()
    {
        _currentShipColorIndex += 1;
        _currentShipColorIndex %= TheGameAssetManager.PlayerShipColorChoices.Length;

        PlayerPrefs.SetInt($"{MultiplayerLogic.MyUserID.ID}.ColorIndex", _currentShipColorIndex);

        RefreshShipAndColor();

        Networking.SendMessageToAll(new PlayerLobbyState(ReadyToggle.isOn, _currentShipImageIndex, _currentShipColorIndex));
    }

    public void HandleReadyTogglePressed(bool isOn)
    {
        Debug.LogFormat($"GameLobbyScreen.HandleReadyTogglePressed({ReadyToggle.isOn})");

        Networking.SendMessageToAll(new PlayerLobbyState(ReadyToggle.isOn, _currentShipImageIndex, _currentShipColorIndex));

        if (ReadyToggle.isOn)
        {
            LobbyMembersList.MakeMemberReady(MultiplayerLogic.MyUserID);
        }
        else
        {
            LobbyMembersList.MakeMemberNotReady(MultiplayerLogic.MyUserID);
        }
    }

    public void HandleLeaveButtonPressed()
    {
        Debug.Log("GameLobbyScreen.HandleLeaveButtonPressed()");

        Disable();

        Networking.OnNetworkStopped += HandleNetworkStopped;
        AsyncOpUI.Started(@"Stopping the session network...");
        Networking.StopNetworking();
    }

    public void HandleInviteButtonPressed()
    {
        MultiplayerLogic.SendInvitesToFriends();
    }

    protected override void OnValidate()
    {
        base.OnValidate();

        Assert.IsNotNull(ShipButton);
        Assert.IsNotNull(ColorButton);
        Assert.IsNotNull(ReadyToggle);
        Assert.IsNotNull(LeaveButton);
        Assert.IsNotNull(InviteButton);
        Assert.IsNotNull(LobbyMembersList);
        Assert.IsNotNull(TheGameController);
        Assert.IsNotNull(TheGameAssetManager);
    }

    protected override void OnShown()
    {
        KillExistingCountdown();

        ReadyToggle.isOn = false;

        LobbyMembersList.ClearAllMembers();
        LobbyMembersList.AddMember(MultiplayerLogic.MyUserID, XboxLive.MyGamerTag, MultiplayerLogic.IsHost);
        LobbyMembersList.AllMembersReady += HandleAllMembersReady;

        MultiplayerLogic.OnMembersAdded += HandleLobbyMembersAdded;
        MultiplayerLogic.OnMembersRemoved += HandleLobbyMembersRemoved;
        MultiplayerLogic.OnSessionPropertiesChanged += HandleSessionPropertiesChanged;
        XboxLive.OnSocialProfileObtained += HandleSocialProfileObtained;

        Networking.OnNetworkMessage_HelloNetwork_Received += HandleHelloNetworkReceived;
        Networking.OnNetworkMessage_PlayerLobbyState_Received += HandlePlayerLobbyStateReceived;
        Networking.OnNetworkMessage_LobbyState_Received += HandleLobbyStateReceived;

        ReconcileMemberList();
        RefreshShipAndColor();

        base.OnShown();
    }

    protected override void OnEnabled()
    {
        ShipButton.interactable = true;
        ColorButton.interactable = true;
        ReadyToggle.interactable = true;
        LeaveButton.interactable = true;

        RefreshInviteButton();

        base.OnEnabled();
    }

    protected override void OnDisabled()
    {
        base.OnDisabled();

        ShipButton.interactable = false;
        ColorButton.interactable = false;
        ReadyToggle.interactable = false;
        LeaveButton.interactable = false;
        InviteButton.interactable = false;
    }

    protected override void OnHidden()
    {
        base.OnHidden();

        KillExistingCountdown();

        LobbyMembersList.AllMembersReady -= HandleAllMembersReady;

        MultiplayerLogic.OnMembersAdded -= HandleLobbyMembersAdded;
        MultiplayerLogic.OnMembersRemoved -= HandleLobbyMembersRemoved;
        MultiplayerLogic.OnSessionPropertiesChanged -= HandleSessionPropertiesChanged;
        XboxLive.OnSocialProfileObtained -= HandleSocialProfileObtained;

        Networking.OnNetworkMessage_HelloNetwork_Received -= HandleHelloNetworkReceived;
        Networking.OnNetworkMessage_PlayerLobbyState_Received -= HandlePlayerLobbyStateReceived;
        Networking.OnNetworkMessage_LobbyState_Received -= HandleLobbyStateReceived;
    }

    private void RefreshInviteButton()
    {
        var hasMaxMembers = LobbyMembersList.HasMaxMembers();
        var isHost = MultiplayerLogic.IsHost;
        var isHostedLobby = MultiplayerLogic.CurrentSessionState == BaseMultiplayerLogic.SessionState.InHostedLobby;
        InviteButton.interactable = IsEnabled() && !hasMaxMembers;

        InviteButton.gameObject.SetActive(isHost && isHostedLobby);
    }

    private void RefreshShipAndColor()
    {
        var userID = MultiplayerLogic.MyUserID;

        _currentShipImageIndex = PlayerPrefs.GetInt($"{userID.ID}.ShipIndex", _currentShipImageIndex);
        ShipButtonImage.sprite = TheGameAssetManager.PlayerShipSprites[_currentShipImageIndex];
        LobbyMembersList.SetMemberShipIndex(userID, TheGameAssetManager, _currentShipImageIndex);

        _currentShipColorIndex = PlayerPrefs.GetInt($"{userID.ID}.ColorIndex", _currentShipColorIndex);
        ColorButtonImage.color = TheGameAssetManager.PlayerShipColorChoices[_currentShipColorIndex];
        LobbyMembersList.SetMemberColorIndex(userID, TheGameAssetManager, _currentShipColorIndex);
    }

    private void ReconcileMemberList()
    {
        UserID[] members = null;

        if (MultiplayerLogic.CurrentSessionState == BaseMultiplayerLogic.SessionState.InHostedLobby)
        {
            members = MultiplayerLogic.GetLobbyMembers();
        }
        else if (MultiplayerLogic.CurrentSessionState == BaseMultiplayerLogic.SessionState.InMatchedLobby)
        {
            members = MultiplayerLogic.GetLobbyMembers();
        }

        var currentlyDisplayedMembers = LobbyMembersList.GetMemberUserIDs();

        foreach (var currentMember in currentlyDisplayedMembers)
        {
            if (!members.Contains(currentMember))
            {
                LobbyMembersList.RemoveMember(currentMember);
            }
        }

        HandleLobbyMembersAdded(members);
    }

    private bool HasGameSessionStarted()
    {
        return
            MultiplayerLogic.CurrentSessionState == BaseMultiplayerLogic.SessionState.InHostedGame ||
            MultiplayerLogic.CurrentSessionState == BaseMultiplayerLogic.SessionState.InMatchedGame;
    }

    private IEnumerator Countdown()
    {
        var remainingCountdownSeconds = Configuration.COUNTDOWN_SECONDS;

        while (remainingCountdownSeconds > 0.0F || !HasGameSessionStarted())
        {
            remainingCountdownSeconds -= Time.deltaTime;
            remainingCountdownSeconds = Math.Max(0.0F, remainingCountdownSeconds);
            StatusBarText.text = Math.Ceiling(remainingCountdownSeconds).ToString();
            yield return 0;
        }

        Debug.LogFormat($"GameLobbyScreen.Countdown() is completed, session state is {MultiplayerLogic.CurrentSessionState}");

        StatusBarText.text = string.Empty;

        TransitionToGameCompleted?.Invoke();

        yield break;
    }

    private void KillExistingCountdown()
    {
        if (null != _countdownCoroutine)
        {
            StopCoroutine(_countdownCoroutine);
            _countdownCoroutine = null;
        }
    }

    private void TransitionToGame()
    {
        // we initialize the game controller early since players may get into
        // the game well ahead of us...
        TheGameController.Initialize(_currentShipImageIndex, _currentShipColorIndex);

        KillExistingCountdown();
        _countdownCoroutine = StartCoroutine(Countdown());
        MultiplayerLogic.StartGame();
    }

    private void ResetReadyState()
    {
        ReadyToggle.isOn = false;
        LobbyMembersList.MakeMemberNotReady(MultiplayerLogic.MyUserID);
    }

    private void HandleAllMembersReady()
    {
        Debug.Log("GameLobbyScreen.HandleAllMembersReady()");

        Disable();

        MultiplayerLogic.OnMembersAdded -= HandleLobbyMembersAdded;
        MultiplayerLogic.OnMembersRemoved -= HandleLobbyMembersRemoved;

        LobbyMembersList.AllMembersReady -= HandleAllMembersReady;

        if (MultiplayerLogic.IsHost)
        {
            Debug.Log(">>> notifying members that lobby is ready...");

            // send out the lobby state as being ready
            Networking.SendMessageToAll(new LobbyState(true));
            MultiplayerLogic.OnMultiplayerError += HandleGameStartError;
            TransitionToGame();
        }
    }

    private void HandleGameStartError(string errorMessage)
    {
        ResetReadyState();

        MultiplayerLogic.ClearEventHandlers();

        KillExistingCountdown();

        MultiplayerLogic.OnMembersAdded += HandleLobbyMembersAdded;
        MultiplayerLogic.OnMembersRemoved += HandleLobbyMembersRemoved;

        LobbyMembersList.AllMembersReady += HandleAllMembersReady;

        var errorText = string.Format($"Error: {errorMessage}");

        Debug.Log(errorText);
        StatusBarText.text = errorText;

        Enable();
    }

    private void HandleHelloNetworkReceived(UserID senderUserID)
    {
        Debug.LogFormat($"GameLobbyScreen.HandleHelloNetworkReceived() from {senderUserID}");

        Networking.SendMessageToMember(senderUserID, new PlayerLobbyState(ReadyToggle.isOn, _currentShipImageIndex, _currentShipColorIndex));
    }

    private void HandlePlayerLobbyStateReceived(UserID senderUserID, PlayerLobbyState playerLobbyState)
    {
        Debug.LogFormat($"GameLobbyScreen.HandlePlayerLobbyStateReceived() from {senderUserID}");

        if (playerLobbyState.IsReady)
        {
            LobbyMembersList.MakeMemberReady(senderUserID);
        }
        else
        {
            LobbyMembersList.MakeMemberNotReady(senderUserID);
        }

        LobbyMembersList.SetMemberShipIndex(senderUserID, TheGameAssetManager, playerLobbyState.ShipIndex);
        LobbyMembersList.SetMemberColorIndex(senderUserID, TheGameAssetManager, playerLobbyState.ColorIndex);
    }

    private void HandleLobbyStateReceived(UserID senderUserID, LobbyState lobbyState)
    {
        Debug.Log("GameLobbyScreen.HandleLobbyStateReceived()");

        // if the lobby is ready, proceed to the game start countdown!
        if (lobbyState.IsReady)
        {
            MultiplayerLogic.OnMultiplayerError += HandleGameStartError;
            TransitionToGame();
        }
    }

    private void HandleSocialProfileObtained(ulong xuid, XboxLiveLogic.SocialProfile profile)
    {
        Debug.Log("GameLobbyScreen.HandleSocialProfileObtained()");

        UserID userID = MultiplayerLogic.GetUserIDforXuid(xuid);

        if (LobbyMembersList.HasMember(userID))
        {
            LobbyMembersList.UpdateMemberGamertag(userID, profile.GamerTag);
        }
    }

    private void HandleLobbyMembersAdded(UserID[] userIDs)
    {
        Debug.Log("GameLobbyScreen.HandleLobbyMembersAdded()");

        foreach (var userID in userIDs)
        {
            if (!LobbyMembersList.HasMember(userID))
            {
                ulong xuid = MultiplayerLogic.GetXuidForUserID(userID);
                bool isHost = MultiplayerLogic.HostID == userID;

                if (XboxLive.HasOnlineInTitleFriend(xuid))
                {
                    var friend = XboxLive.GetOnlineInTitleFriend(xuid);
                    LobbyMembersList.AddMember(userID, friend.GamerTag, isHost);
                }
                else
                {
                    // get the user's profile asynchronously and then update that
                    // member when the load has completed
                    LobbyMembersList.AddMember(userID, userID.ToString(), isHost);
                    XboxLive.GetSocialProfile(xuid);
                }
            }
        }

        // set/update our activity
        string sessionConnectionString = MultiplayerLogic.GetSessionConnectionString();

        if(string.IsNullOrEmpty(sessionConnectionString) == false)
        {
            MultiplayerLogic.InviteAndActivity.SetUserActivity(MultiplayerLogic.GetSessionConnectionString(), LobbyMembersList.GetMemberUserIDs().Length);
        }
        else
        {
            Debug.Log("GameLobbyScreen.HandleLobbyMembersAdded: sessionConnectionString was invalid");
        }
        

        RefreshInviteButton();
    }

    private void HandleLobbyMembersRemoved(UserID[] userIDs)
    {
        Debug.Log("GameLobbyScreen.HandleLobbyMembersRemoved()");

        foreach (var userID in userIDs)
        {
            if (LobbyMembersList.HasMember(userID))
            {
                LobbyMembersList.RemoveMember(userID);
            }
        }

        // set/update our activity
        MultiplayerLogic.InviteAndActivity.SetUserActivity(MultiplayerLogic.GetSessionConnectionString(), LobbyMembersList.GetMemberUserIDs().Length);

        RefreshInviteButton();
    }

    private void HandleNetworkStopped(string networkId)
    {
        Debug.Log("GameLobbyScreen.HandleNetworkStopped()");

        AsyncOpUI.Finished();
        Networking.OnNetworkStopped -= HandleNetworkStopped;

        // try to leave the current lobby session

        MultiplayerLogic.OnLobbyLeft += HandleLobbyLeft;
        AsyncOpUI.Started(@"Leaving the current game lobby session...");
        MultiplayerLogic.LeaveLobby();
    }

    private void HandleLobbyLeft()
    {
        Debug.Log("GameLobbyScreen.HandleLobbyLeft()");

        // clear our activity
        MultiplayerLogic.InviteAndActivity.SetUserActivity(string.Empty, 0);

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        LeaveLobbyCompleted?.Invoke();
    }

    private void HandleSessionPropertiesChanged(SessionDocument sessionDocument)
    {
        if (LobbyMembersList.HasMember(MultiplayerLogic.HostID))
        {
            LobbyMembersList.UpdateMemberHost(MultiplayerLogic.HostID, true);
        }

        RefreshInviteButton();
    }
}
