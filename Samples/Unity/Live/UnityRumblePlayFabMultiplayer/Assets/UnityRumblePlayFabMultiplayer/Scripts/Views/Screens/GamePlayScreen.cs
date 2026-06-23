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
        Debug.Log("GameLobbyScreen.HandleQuitButtonPressed()");

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

        var myUserID = MultiplayerLogic.MyUserID;

        GameMembersList.ClearAllMembers();
        GameMembersList.AddMember(myUserID, XboxLive.MyGamerTag, MultiplayerLogic.IsHost);
        GameMembersList.MemberReachedMaxKills += HandleMemberReachedMaxKills;

        MultiplayerLogic.OnHostChanged += HandleHostChanged;
        MultiplayerLogic.OnMembersAdded += HandleGameMembersAdded;
        MultiplayerLogic.OnMembersRemoved += HandleGameMembersRemoved;
        MultiplayerLogic.OnSessionPropertiesChanged += HandleSessionPropertiesChanged;
        XboxLive.OnSocialProfileObtained += HandleSocialProfileObtained;

        if (Networking.PFMultiplayerManager != null)
        {
            Networking.PFMultiplayerManager.OnRemotePlayerLeft += HandleRemotePlayerLeft;
        }

        ReconcileMemberList();

        TheGameController.gameObject.SetActive(true);
        TheGameController.OnLocalPlayerRequestQuit += HandleLocalPlayerRequestQuit;
        TheGameController.OnShipDestroyed += HandleShipDestroyed;
        TheGameController.SpawnLocalShip(myUserID);
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

        MultiplayerLogic.OnHostChanged -= HandleHostChanged;
        MultiplayerLogic.OnMembersAdded -= HandleGameMembersAdded;
        MultiplayerLogic.OnMembersRemoved -= HandleGameMembersRemoved;
        MultiplayerLogic.OnSessionPropertiesChanged -= HandleSessionPropertiesChanged;
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
        Debug.Log("GamePlayScreen.CountdownAndRespawn()");

        var remainingCountdownSeconds = Configuration.COUNTDOWN_SECONDS;

        while (remainingCountdownSeconds > 0.0F)
        {
            StatusBarText.text = remainingCountdownSeconds.ToString();
            yield return new WaitForSeconds(1.0f);
            remainingCountdownSeconds -= 1;
        }

        Debug.Log("GamePlayScreen.CountdownAndSpawn() is completed");

        StatusBarText.text = string.Empty;

        TheGameController.SpawnLocalShip(MultiplayerLogic.MyUserID);

        yield break;
    }

    private void KillExistingCountdown()
    {
        Debug.LogFormat($"GamePlayScreen.KillExistingCountdown()");

        if (null != _countdownCoroutine)
        {
            StopCoroutine(_countdownCoroutine);
            _countdownCoroutine = null;
        }
    }

    private void RespawnLocalPlayerShip()
    {
        Debug.LogFormat($"GamePlayScreen.RespawnLocalPlayerShip()");

        KillExistingCountdown();
        _countdownCoroutine = StartCoroutine(CountdownAndRespawn());
    }

    private void ReconcileMemberList()
    {
        var members = MultiplayerLogic.GetGameMembers();

#if !TRY_JOIN_GAME
        // NOTE: because of the JoinGameFromLobby() API bug, we may need to get the
        // members from the lobby still
        if (null == members || members.Length == 0)
        {
            members = MultiplayerLogic.GetLobbyMembers();
        }
#endif

        var currentlyDisplayedMembers = GameMembersList.GetMemberUserIDs();

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
        if (MultiplayerLogic.IsHost)
        {
            TheGameController.NowIsHost();
        }
    }

    private void HandleMemberReachedMaxKills(UserID memberUserID)
    {
        Debug.LogFormat($"GamePlayScreen.HandleMemberReachedMaxKills({memberUserID})");

        var member = GameMembersList.GetMember(memberUserID);
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
        Debug.Log("GamePlayScreen.HandleLocalPlayerRequestQuit()");

        HandleQuitButtonPressed();
    }

    private void HandleShipDestroyed(UserID destroyedShipUserID, UserID destroyerShipUserID)
    {
        Debug.LogFormat($"GamePlayScreen.HandleShipDestroyed(destroyed={destroyedShipUserID}, destroyer={destroyerShipUserID})");

        var isLocalShipDestroyed = MultiplayerLogic.MyUserID == destroyedShipUserID;

        // update the scoring
        GameMembersList.IncrementMemberDeathCount(destroyedShipUserID);

        // do not get credit for killing your own ship
        if (destroyedShipUserID != destroyerShipUserID)
        {
            GameMembersList.IncrementMemberKillCount(destroyerShipUserID);
        }

        // if the game is over, then we should not respawn
        if (TheGameController.isActiveAndEnabled && isLocalShipDestroyed)
        {
            RespawnLocalPlayerShip();
        }
    }

    private void HandleSocialProfileObtained(ulong xuid, XboxLiveLogic.SocialProfile profile)
    {
        UserID id = MultiplayerLogic.GetUserIDforXuid(xuid);

        if (GameMembersList.HasMember(id))
        {
            GameMembersList.UpdateMemberGamertag(id, profile.GamerTag);
        }
    }

    private void HandleRemotePlayerLeft(object sender, PlayFabPlayer player)
    {
        var userID = new UserID(player.EntityKey.Id);

        HandleGameMembersRemoved(new UserID[] { userID });
    }

    private void HandleGameMembersAdded(UserID[] userIDs)
    {
        Debug.Log("GamePlayScreen.HandleGameMembersAdded()");

        foreach (var userID in userIDs)
        {
            if (userID != MultiplayerLogic.MyUserID && !GameMembersList.HasMember(userID))
            {
                ulong xuid = MultiplayerLogic.GetXuidForUserID(userID);
                bool isHost = MultiplayerLogic.HostID == userID;

                if (XboxLive.HasOnlineInTitleFriend(xuid))
                {
                    var friend = XboxLive.GetOnlineInTitleFriend(xuid);
                    GameMembersList.AddMember(userID, friend.GamerTag, isHost);
                }
                else
                {
                    // get the user's profile asynchronously and then update that
                    // member when the load has completed
                    GameMembersList.AddMember(userID, userID.ToString(), isHost);
                    XboxLive.GetSocialProfile(xuid);
                }
            }
        }
    }

    private void HandleGameMembersRemoved(UserID[] userIDs)
    {
        Debug.Log("GamePlayScreen.HandleLobbyMembersRemoved()");

        foreach (var userID in userIDs)
        {
            if (GameMembersList.HasMember(userID))
            {
                GameMembersList.RemoveMember(userID);
            }
        }
    }

    private void HandleNetworkStopped(string networkId)
    {
        Debug.Log("GamePlayScreen.HandleNetworkStopped()");

        AsyncOpUI.Finished();
        Networking.OnNetworkStopped -= HandleNetworkStopped;

        // try to leave the current lobby session

        MultiplayerLogic.OnGameLeft += HandleGameLeft;
        MultiplayerLogic.OnMatchLeft += HandleMatchLeft;
        AsyncOpUI.Started(@"Quitting the current game play session...");

        MultiplayerLogic.LeaveGame();

        // NOTE: because of a JoinGameFromLobby() API bug, if we are
        // in a hosted game, then we need to just leave the lobby directly...
        if (MultiplayerLogic.CurrentSessionState == BaseMultiplayerLogic.SessionState.InHostedGame)
        {
            HandleGameLeft();
        }
    }

    private void HandleGameLeft()
    {
        Debug.Log("MainMenuScreen.HandleGameLeft()");

        MultiplayerLogic.OnLobbyLeft += HandleGameLobbyLeft;
        MultiplayerLogic.LeaveLobby();
    }

    private void HandleGameLobbyLeft()
    {
        Debug.Log("MainMenuScreen.HandleGameLobbyLeft()");

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        // clear our activity
        MultiplayerLogic.InviteAndActivity.SetUserActivity(string.Empty, 0);

        QuitGameCompleted?.Invoke();
    }

    private void HandleMatchLeft()
    {
        Debug.Log("MainMenuScreen.HandleMatchLeft()");

        MultiplayerLogic.OnLobbyLeft += HandleMatchLobbyLeft;
        MultiplayerLogic.LeaveLobby();
    }

    private void HandleMatchLobbyLeft()
    {
        Debug.Log("MainMenuScreen.HandleMatchLobbyLeft()");

        AsyncOpUI.Finished();
        MultiplayerLogic.ClearEventHandlers();

        QuitMatchCompleted?.Invoke();
    }

    private void HandleSessionPropertiesChanged(SessionDocument sessionDocument)
    {
        if (GameMembersList.HasMember(MultiplayerLogic.HostID))
        {
            GameMembersList.UpdateMemberHost(MultiplayerLogic.HostID, true);
        }
    }
}
