//--------------------------------------------------------------------------------------
// ScreenManager.cs
//
// The UI screen class for the game play screen functionality.
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

using PlayFab.Party;
using System;
using System.Collections;
using System.Linq;
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UI;

public class GamePlayScreen : BaseUiScreen
{
    public event Action GameOver;
    public event Action QuitGameCompleted;
    public event Action QuitMatchCompleted;

    public Button QuitButton;
    public GameMembersView GameMembersList;
    public GameController TheGameController;
    public RectTransform MiniMapContainer;
    public RectTransform BackgroundPanel;

    Coroutine _countdownCoroutine;

    public void HandleQuitButtonPressed()
    {
        Debug.LogFormat("GameLobbyScreen.HandleQuitButtonPressed()");

        // show a confirmation popup
        PopupView.ShowQuitConfirmationMessage(this, () =>
        {
            Disable();

            Networking.OnNetworkStopped += HandleNetworkStopped;
            AsyncOpUI.Started(@"Stopping the session network...");
            Networking.StopNetworking();
        });
    }

    protected override void OnValidate()
    {
        base.OnValidate();

        Assert.IsNotNull(QuitButton);
        Assert.IsNotNull(GameMembersList);
        Assert.IsNotNull(TheGameController);
        Assert.IsNotNull(MiniMapContainer);
    }

    protected override void OnShown()
    {
        BackgroundPanel.gameObject.SetActive(false);

        GameMembersList.ClearAllMembers();
        GameMembersList.AddMember(XboxLive.MyXUID, XboxLive.MyGamerTag, XboxLive.IsHost);
        GameMembersList.MemberReachedMaxKills += HandleMemberReachedMaxKills;

        XboxLive.OnHostChanged += HandleHostChanged;
        XboxLive.OnMembersAdded += HandleGameMembersAdded;
        XboxLive.OnMembersRemoved += HandleGameMembersRemoved;
        XboxLive.OnSessionPropertiesChanged += HandleSessionPropertiesChanged;
        XboxLive.OnSocialProfileObtained += HandleSocialProfileObtained;

        if (Networking.PFMultiplayerManager != null)
        {
            Networking.PFMultiplayerManager.OnRemotePlayerLeft += HandleRemotePlayerLeft;
        }

        ReconcileMemberList();

        TheGameController.gameObject.SetActive(true);
        TheGameController.OnLocalPlayerRequestQuit += HandleLocalPlayerRequestQuit;
        TheGameController.OnShipDestroyed += HandleShipDestroyed;
        TheGameController.SpawnLocalShip(XboxLive.MyXUID);
        TheGameController.SpawnLocalAsteroids(Configuration.MIN_ASTEROID_COUNT, Configuration.MIN_ASTEROID_COUNT);

        MiniMapContainer.gameObject.SetActive(true);

        base.OnShown();
    }

    protected override void OnEnabled()
    {
        QuitButton.interactable = true;

        base.OnEnabled();
    }

    protected override void OnDisabled()
    {
        base.OnDisabled();

        QuitButton.interactable = false;
    }

    protected override void OnHidden()
    {
        base.OnHidden();

        MiniMapContainer.gameObject.SetActive(false);

        GameMembersList.MemberReachedMaxKills -= HandleMemberReachedMaxKills;

        XboxLive.OnHostChanged -= HandleHostChanged;
        XboxLive.OnMembersAdded -= HandleGameMembersAdded;
        XboxLive.OnMembersRemoved -= HandleGameMembersRemoved;
        XboxLive.OnSessionPropertiesChanged -= HandleSessionPropertiesChanged;
        XboxLive.OnSocialProfileObtained -= HandleSocialProfileObtained;

        if (Networking.PFMultiplayerManager != null)
        {
            Networking.PFMultiplayerManager.OnRemotePlayerLeft -= HandleRemotePlayerLeft;
        }

        TheGameController.OnLocalPlayerRequestQuit -= HandleLocalPlayerRequestQuit;
        TheGameController.OnShipDestroyed -= HandleShipDestroyed;
        TheGameController.Cleanup();
        TheGameController.gameObject.SetActive(false);

        BackgroundPanel.gameObject.SetActive(true);
    }

    private IEnumerator CountdownAndRespawn()
    {
        Debug.LogFormat("GamePlayScreen.__CountdownAndRespawn()");

        var remainingCountdownSeconds = Configuration.COUNTDOWN_SECONDS;

        while (remainingCountdownSeconds > 0.0F)
        {
            remainingCountdownSeconds -= Time.deltaTime;
            remainingCountdownSeconds = Math.Max(0.0F, remainingCountdownSeconds);
            StatusBarText.text = Math.Ceiling(remainingCountdownSeconds).ToString();
            yield return 0;
        }

        Debug.LogFormat("GamePlayScreen.__CountdownAndSpawn() is completed");

        StatusBarText.text = string.Empty;

        TheGameController.SpawnLocalShip(XboxLive.MyXUID);

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

    private void RespawnLocalPlayerShip()
    {
        KillExistingCountdown();
        _countdownCoroutine = StartCoroutine(CountdownAndRespawn());
    }

    private void ReconcileMemberList()
    {
        var members = XboxLive.GetGameMembers();

        // NOTE: because of a JoinGameFromLobby() API bug, we may need to get the
        // members from the lobby still
        if (null == members || members.Length == 0)
        {
            members = XboxLive.GetLobbyMembers();
        }

        var currentlyDisplayedMembers = GameMembersList.GetMemberXuids();

        foreach (var currentMember in currentlyDisplayedMembers)
        {
            if (!members.Contains(currentMember))
            {
                GameMembersList.RemoveMember(currentMember);
            }
        }

        HandleGameMembersAdded(members);
    }

    private void HandleHostChanged()
    {
        if (XboxLive.IsHost)
        {
            TheGameController.NowIsHost();
        }
    }

    private void HandleMemberReachedMaxKills(ulong memberXuid)
    {
        Debug.LogFormat("GamePlayScreen.__HandleMemberReachedMaxKills({0})", memberXuid);

        var member = GameMembersList.GetMember(memberXuid);
        {
            TheGameController.gameObject.SetActive(false);
            PopupView.ShowGameOverMessage(
                this, 
                member.GetGamertag(),
                () => 
                {
                    Disable();

                    Networking.OnNetworkStopped += HandleNetworkStopped;
                    AsyncOpUI.Started(@"Stopping the session network...");
                    Networking.StopNetworking();

                    GameOver?.Invoke();
                });
        }
    }

    private void HandleLocalPlayerRequestQuit()
    {
        Debug.LogFormat("GamePlayScreen.__HandleLocalPlayerRequestQuit()");

        HandleQuitButtonPressed();
    }

    private void HandleShipDestroyed(ulong destroyedShipXuid, ulong destroyerShipXuid)
    {
        Debug.LogFormat("GamePlayScreen.__HandleShipDestroyed(destroyed={0}, destroyer={1})", destroyedShipXuid, destroyerShipXuid);

        var isLocalShipDestroyed = XboxLive.MyXUID == destroyedShipXuid;

        // update the scoring
        GameMembersList.IncrementMemberDeathCount(destroyedShipXuid);
        GameMembersList.IncrementMemberKillCount(destroyerShipXuid);

        // if the game is over, then we should not respawn
        if (TheGameController.isActiveAndEnabled && isLocalShipDestroyed)
        {
            RespawnLocalPlayerShip();
        }
    }

    private void HandleSocialProfileObtained(ulong xuid, XboxLiveLogic.SocialProfile profile)
    {
        if (GameMembersList.HasMember(xuid))
        {
            GameMembersList.UpdateMemberGamertag(xuid, profile.GamerTag);
        }
    }

    private void HandleRemotePlayerLeft(object sender, PlayFabPlayer player)
    {
        HandleGameMembersRemoved(new ulong[] { Networking.GetXuidForPlayer(player) });
    }

    private void HandleGameMembersAdded(ulong[] xuids)
    {
        Debug.LogFormat("GamePlayScreen.__HandleGameMembersAdded()");

        foreach (var xuid in xuids)
        {
            if (xuid != XboxLive.MyXUID && !GameMembersList.HasMember(xuid))
            {
                if (XboxLive.HasOnlineInTitleFriend(xuid))
                {
                    var friend = XboxLive.GetOnlineInTitleFriend(xuid);
                    GameMembersList.AddMember(friend.XUID, friend.GamerTag, XboxLive.HostXuid == xuid);
                }
                else
                {
                    // Get the user's profile asynchronously and then update that
                    // member when the load has completed
                    GameMembersList.AddMember(xuid, xuid.ToString(), XboxLive.HostXuid == xuid);
                    XboxLive.GetSocialProfile(xuid);
                }
            }
        }
    }

    private void HandleGameMembersRemoved(ulong[] xuids)
    {
        Debug.LogFormat("GamePlayScreen.__HandleLobbyMembersRemoved()");

        foreach (var xuid in xuids)
        {
            if (xuid != XboxLive.MyXUID && GameMembersList.HasMember(xuid))
            {
                GameMembersList.RemoveMember(xuid);
            }
        }
    }

    private void HandleNetworkStopped(string networkId)
    {
        Debug.LogFormat("GamePlayScreen.__HandleNetworkStopped()");

        AsyncOpUI.Finished();
        Networking.OnNetworkStopped -= HandleNetworkStopped;

        // try to leave the current game session

        XboxLive.OnGameLeft += HandleGameLeft;
        XboxLive.OnMatchLeft += HandleMatchLeft;
        AsyncOpUI.Started(@"Quitting the current game play session...");

        XboxLive.LeaveGame();

        // NOTE: because of a JoinGameFromLobby() API bug, if we are
        // in a hosted game, then we need to leave the lobby directly...
        if (XboxLive.CurrentSessionState == XboxLiveLogic.SessionState.InHostedGame)
        {
            HandleGameLeft();
        }
    }

    private void HandleGameLeft()
    {
        Debug.LogFormat("MainMenuScreen.__HandleGameLeft()");

        XboxLive.OnGameLeft -= HandleGameLeft;

        XboxLive.OnLobbyLeft += HandleGameLobbyLeft;
        XboxLive.LeaveLobby();
    }

    private void HandleGameLobbyLeft()
    {
        Debug.LogFormat("MainMenuScreen.__HandleGameLobbyLeft()");

        AsyncOpUI.Finished();
        XboxLive.OnLobbyLeft -= HandleGameLobbyLeft;

        QuitGameCompleted?.Invoke();
    }

    private void HandleMatchLeft()
    {
        Debug.LogFormat("MainMenuScreen.__HandleMatchLeft()");

        XboxLive.OnMatchLeft -= HandleMatchLeft;

        XboxLive.OnLobbyLeft += HandleMatchLobbyLeft;
        XboxLive.LeaveLobby();
    }

    private void HandleMatchLobbyLeft()
    {
        Debug.LogFormat("MainMenuScreen.__HandleMatchLobbyLeft()");

        AsyncOpUI.Finished();
        XboxLive.OnLobbyLeft -= HandleMatchLobbyLeft;

        QuitMatchCompleted?.Invoke();
    }

    private void HandleSessionPropertiesChanged(SessionDocument sessionDocument)
    {
        if (GameMembersList.HasMember(XboxLive.HostXuid))
        {
            GameMembersList.UpdateMemberHost(XboxLive.HostXuid, true);
        }
    }
}
