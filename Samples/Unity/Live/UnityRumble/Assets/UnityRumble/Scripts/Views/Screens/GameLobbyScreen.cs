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

        PlayerPrefs.SetInt($"{XboxLive.MyXUID}.ShipIndex", _currentShipImageIndex);

        RefreshShipAndColor();

        Networking.SendMessageToAll(new PlayerLobbyState(ReadyToggle.isOn, _currentShipImageIndex, _currentShipColorIndex));
    }

    public void HandleColorButtonPressed()
    {
        _currentShipColorIndex += 1;
        _currentShipColorIndex %= TheGameAssetManager.PlayerShipColorChoices.Length;

        PlayerPrefs.SetInt($"{XboxLive.MyXUID}.ColorIndex", _currentShipColorIndex);

        RefreshShipAndColor();

        Networking.SendMessageToAll(new PlayerLobbyState(ReadyToggle.isOn, _currentShipImageIndex, _currentShipColorIndex));
    }

    public void HandleReadyTogglePressed(bool isOn)
    {
        Debug.LogFormat("GameLobbyScreen.HandleReadyTogglePressed({0})", ReadyToggle.isOn);

        Networking.SendMessageToAll(new PlayerLobbyState(ReadyToggle.isOn, _currentShipImageIndex, _currentShipColorIndex));

        if (ReadyToggle.isOn)
        {
            LobbyMembersList.MakeMemberReady(XboxLive.MyXUID);
        }
        else
        {
            LobbyMembersList.MakeMemberNotReady(XboxLive.MyXUID);
        }
    }

    public void HandleLeaveButtonPressed()
    {
        Debug.LogFormat("GameLobbyScreen.HandleLeaveButtonPressed()");

        Disable();

        Networking.OnNetworkStopped += HandleNetworkStopped;
        AsyncOpUI.Started(@"Stopping the session network...");
        Networking.StopNetworking();
    }

    public void HandleInviteButtonPressed()
    {
        XboxLive.InviteToLobby(null);
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
        LobbyMembersList.AddMember(XboxLive.MyXUID, XboxLive.MyGamerTag, XboxLive.IsHost);
        LobbyMembersList.AllMembersReady += HandleAllMembersReady;

        XboxLive.OnMembersAdded += HandleLobbyMembersAdded;
        XboxLive.OnMembersRemoved += HandleLobbyMembersRemoved;
        XboxLive.OnSessionPropertiesChanged += HandleSessionPropertiesChanged;
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

        XboxLive.OnMembersAdded -= HandleLobbyMembersAdded;
        XboxLive.OnMembersRemoved -= HandleLobbyMembersRemoved;
        XboxLive.OnSessionPropertiesChanged -= HandleSessionPropertiesChanged;
        XboxLive.OnSocialProfileObtained -= HandleSocialProfileObtained;

        Networking.OnNetworkMessage_HelloNetwork_Received -= HandleHelloNetworkReceived;
        Networking.OnNetworkMessage_PlayerLobbyState_Received -= HandlePlayerLobbyStateReceived;
        Networking.OnNetworkMessage_LobbyState_Received -= HandleLobbyStateReceived;
    }

    private void RefreshInviteButton()
    {
        var hasMaxMembers = LobbyMembersList.HasMaxMembers();
        var isHost = XboxLive.IsHost;
        var isHostedLobby = XboxLive.CurrentSessionState == XboxLiveLogic.SessionState.InHostedLobby;
        InviteButton.interactable = IsEnabled() && !hasMaxMembers;

        InviteButton.gameObject.SetActive(isHost && isHostedLobby);
    }

    private void RefreshShipAndColor()
    {
        _currentShipImageIndex = PlayerPrefs.GetInt($"{XboxLive.MyXUID}.ShipIndex", _currentShipImageIndex);
        ShipButtonImage.sprite = TheGameAssetManager.PlayerShipSprites[_currentShipImageIndex];

        _currentShipColorIndex = PlayerPrefs.GetInt($"{XboxLive.MyXUID}.ColorIndex", _currentShipColorIndex);
        ColorButtonImage.color = TheGameAssetManager.PlayerShipColorChoices[_currentShipColorIndex];

        LobbyMembersList.SetMemberShipIndex(XboxLive.MyXUID, TheGameAssetManager, _currentShipImageIndex);
        LobbyMembersList.SetMemberColorIndex(XboxLive.MyXUID, TheGameAssetManager, _currentShipColorIndex);
    }

    private void ReconcileMemberList()
    {
        ulong[] members = null;

        if (XboxLive.CurrentSessionState == XboxLiveLogic.SessionState.InHostedLobby)
        {
            members = XboxLive.GetLobbyMembers();
        }
        else if(XboxLive.CurrentSessionState == XboxLiveLogic.SessionState.InMatchedLobby)
        {
            members = XboxLive.GetGameMembers();
        }

        var currentlyDisplayedMembers = LobbyMembersList.GetMemberXuids();

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
            (XboxLive.CurrentSessionState == XboxLiveLogic.SessionState.InHostedGame) ||
            (XboxLive.CurrentSessionState == XboxLiveLogic.SessionState.InMatchedGame);
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

        Debug.LogFormat("GameLobbyScreen.__Countdown() is completed, session state is {0}", XboxLive.CurrentSessionState);

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
        // Initialize game controller early since players may get into game well ahead of us...
        TheGameController.Initialize(_currentShipImageIndex, _currentShipColorIndex);

        KillExistingCountdown();
        _countdownCoroutine = StartCoroutine(Countdown());
        XboxLive.StartGame();
    }

    private void ResetReadyState()
    {
        ReadyToggle.isOn = false;
        LobbyMembersList.MakeMemberNotReady(XboxLive.MyXUID);
    }

    private void HandleAllMembersReady()
    {
        Debug.LogFormat("GameLobbyScreen.__HandleAllMembersReady()");

        Disable();

        XboxLive.OnMembersAdded -= HandleLobbyMembersAdded;
        XboxLive.OnMembersRemoved -= HandleLobbyMembersRemoved;

        LobbyMembersList.AllMembersReady -= HandleAllMembersReady;

        if (XboxLive.IsHost)
        {
            Debug.LogFormat(">>> notifying members that lobby is ready...");

            // send out the lobby state as being ready
            Networking.SendMessageToAll(new LobbyState(true));
            XboxLive.OnMultiplayerError += HandleGameStartError;
            TransitionToGame();
        }
    }

    private void HandleGameStartError(string errorMessage, int hresult)
    {
        ResetReadyState();

        XboxLive.ClearMultiplayerEventHandlers();

        KillExistingCountdown();

        XboxLive.OnMembersAdded += HandleLobbyMembersAdded;
        XboxLive.OnMembersRemoved += HandleLobbyMembersRemoved;

        LobbyMembersList.AllMembersReady += HandleAllMembersReady;

        var errorText = string.Format("Error: {0}, hresult = {1}", errorMessage, hresult.ToString("X8"));

        Debug.Log(errorText);
        StatusBarText.text = errorText;

        Enable();
    }

    private void HandleHelloNetworkReceived(ulong senderXuid)
    {
        Debug.LogFormat("GameLobbyScreen.__HandleHelloNetworkReceived() from {0}", senderXuid);

        Networking.SendMessageToMember(senderXuid, new PlayerLobbyState(ReadyToggle.isOn, _currentShipImageIndex, _currentShipColorIndex));
    }

    private void HandlePlayerLobbyStateReceived(ulong senderXuid, PlayerLobbyState playerLobbyState)
    {
        Debug.LogFormat("GameLobbyScreen.__HandlePlayerLobbyStateReceived() from {0}", senderXuid);

        if (playerLobbyState.IsReady)
        {
            LobbyMembersList.MakeMemberReady(senderXuid);
        }
        else
        {
            LobbyMembersList.MakeMemberNotReady(senderXuid);
        }

        LobbyMembersList.SetMemberShipIndex(senderXuid, TheGameAssetManager, playerLobbyState.ShipIndex);
        LobbyMembersList.SetMemberColorIndex(senderXuid, TheGameAssetManager, playerLobbyState.ColorIndex);
    }

    private void HandleLobbyStateReceived(ulong senderXuid, LobbyState lobbyState)
    {
        Debug.LogFormat("GameLobbyScreen.__HandleLobbyStateReceived()");

        // if the lobby is ready, proceed to the game start countdown!
        if (lobbyState.IsReady)
        {
            XboxLive.OnMultiplayerError += HandleGameStartError;
            TransitionToGame();
        }
    }

    private void HandleSocialProfileObtained(ulong xuid, XboxLiveLogic.SocialProfile profile)
    {
        Debug.LogFormat("GameLobbyScreen.__HandleSocialProfileObtained()");

        if (LobbyMembersList.HasMember(xuid))
        {
            LobbyMembersList.UpdateMemberGamertag(xuid, profile.GamerTag);
        }
    }

    private void HandleLobbyMembersAdded(ulong[] xuids)
    {
        Debug.LogFormat("GameLobbyScreen.__HandleLobbyMembersAdded()");

        foreach (var xuid in xuids)
        {
            if (xuid != XboxLive.MyXUID && !LobbyMembersList.HasMember(xuid))
            {
                if (XboxLive.HasOnlineInTitleFriend(xuid))
                {
                    var friend = XboxLive.GetOnlineInTitleFriend(xuid);
                    LobbyMembersList.AddMember(friend.XUID, friend.GamerTag, XboxLive.HostXuid == xuid);
                }
                else
                {
                    // get the user's profile asynchronously and then update that
                    // member when the load has completed
                    LobbyMembersList.AddMember(xuid, xuid.ToString(), XboxLive.HostXuid == xuid);
                    XboxLive.GetSocialProfile(xuid);
                }
            }
        }

        RefreshInviteButton();
    }

    private void HandleLobbyMembersRemoved(ulong[] xuids)
    {
        Debug.LogFormat("GameLobbyScreen.__HandleLobbyMembersRemoved()");

        foreach (var xuid in xuids)
        {
            if (xuid != XboxLive.MyXUID && LobbyMembersList.HasMember(xuid))
            {
                LobbyMembersList.RemoveMember(xuid);
            }
        }

        RefreshInviteButton();
    }

    private void HandleNetworkStopped(string networkId)
    {
        Debug.LogFormat("GameLobbyScreen.__HandleNetworkStopped()");

        AsyncOpUI.Finished();
        Networking.OnNetworkStopped -= HandleNetworkStopped;

        // try to leave the current lobby session

        XboxLive.OnLobbyLeft += HandleLobbyLeft;
        AsyncOpUI.Started(@"Leaving the current game lobby session...");
        XboxLive.LeaveLobby();
    }

    private void HandleLobbyLeft()
    {
        Debug.LogFormat("GameLobbyScreen.__HandleLobbyLeft()");

        AsyncOpUI.Finished();
        XboxLive.OnLobbyLeft -= HandleLobbyLeft;;
        XboxLive.ClearMultiplayerEventHandlers();

        LeaveLobbyCompleted?.Invoke();
    }

    private void HandleSessionPropertiesChanged(SessionDocument sessionDocument)
    {
        if (LobbyMembersList.HasMember(XboxLive.HostXuid))
        {
            LobbyMembersList.UpdateMemberHost(XboxLive.HostXuid, true);
        }

        RefreshInviteButton();
    }

}
