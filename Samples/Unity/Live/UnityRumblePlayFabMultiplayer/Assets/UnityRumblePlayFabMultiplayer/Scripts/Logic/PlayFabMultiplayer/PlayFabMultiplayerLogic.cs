//--------------------------------------------------------------------------------------
// PlayFabMultiplayerLogic.cs
//
// PlayFab logic that handles general Multiplayer Manager events and updates.
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
using PlayFab;
using PlayFab.Multiplayer;
using System.Collections.Generic;
using PlayFab.ClientModels;

public partial class PlayFabMultiplayerLogic : BaseMultiplayerLogic
{
    #region MemberVariables
    public XboxLiveLogic LiveLogic;
    public bool AllowCrossPlatformInvites = true;

    private const int _MAX_SEARCH_RESULTS_ = 24;

    private Lobby myLobby;
    private BaseMultiplayerLogic.UserActivityAndInviteMechanism _inviteAndActivity;
    private PlayFabAuthenticationContext _playFabAuthContext;
    private Dictionary<UserID, ulong> xuidMap = new Dictionary<UserID, ulong>();
    private Dictionary<ulong, UserID> userIDMap = new Dictionary<ulong, UserID>();

    private MatchmakingTicket _activeMatchmakeTicket;
    private Lobby _matchmakingLobby;
    #endregion

    #region InitializationAndCleanup
    public override void InitializeMultiplayer()
    {
        Debug.Log("PlayFabMultiplayerLogic.InitializeMultiplayer()");

        ClearSessionState();

        InviteAndActivity.Initialize();

        PlayFabMultiplayer.LogLevel = LogLevelType.Verbose;
        PlayFabMultiplayer.OnError += this.OnError;

        PlayFabMultiplayer.OnLobbyCreateAndJoinCompleted += this.OnLobbyCreateAndJoinCompleted;
        PlayFabMultiplayer.OnLobbyJoinCompleted += this.OnLobbyJoinCompleted;
        PlayFabMultiplayer.OnLobbyMemberAdded += this.OnLobbyMemberAdded;
        PlayFabMultiplayer.OnLobbyMemberRemoved += this.OnLobbyMemberRemoved;
        PlayFabMultiplayer.OnLobbyUpdated += this.OnLobbyUpdated;
        PlayFabMultiplayer.OnLobbyDisconnected += this.OnLobbyDisconnected;
        PlayFabMultiplayer.OnLobbyLeaveCompleted += this.OnLobbyLeaveCompleted;
        PlayFabMultiplayer.OnLobbyJoinArrangedLobbyCompleted += this.OnLobbyJoinArrangedLobbyCompleted;
        PlayFabMultiplayer.OnLobbyFindLobbiesCompleted += this.OnLobbyFindLobbiesCompleted;
        PlayFabMultiplayer.OnLobbyPostUpdateCompleted += this.OnLobbyPostUpdateCompleted;
        PlayFabMultiplayer.OnMatchmakingTicketCompleted += this.OnMatchmakingTicketCompleted;
        PlayFabMultiplayer.OnMatchmakingTicketStatusChanged += this.OnMatchmakingTicketStatusChanged;

        LoginToPlayFab();
    }

    public override void CleanupMultiplayer()
    {
        Debug.Log("PlayFabMultiplayerLogic.CleanupMultiplayer()");

        InviteAndActivity.Cleanup();

        ClearSessionState();
    }

    public override void ClearSessionState()
    {
        CurrentSessionState = SessionState.NoSession;
        IsHost = false;
        HostID = default(UserID);
        CurrentSessionDocument = new SessionDocument();
    }

    private void ClearMatchMakeLobby()
    {
        CurrentSessionState = SessionState.NoSession;
        IsMatchmaking = false;

        _activeMatchmakeTicket = null;
        _matchmakingLobby = null;
    }
    #endregion

    #region LoginToPlayFab
    private void LoginToPlayFab()
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.LoginToPlayFab() PlayFabTitleId: {PlayFabSettings.staticSettings.TitleId}");

        var request = new LoginWithXboxRequest
        {
            TitleId = PlayFabSettings.staticSettings.TitleId,
            XboxToken = LiveLogic.MyUserToken,
            CreateAccount = true
        };

        PlayFabClientAPI.LoginWithXbox(request, OnLoginToPlayFabComplete,
        error =>
        {
            Debug.LogError(error.GenerateErrorReport());
        });
    }

    private void OnLoginToPlayFabComplete(LoginResult result)
    {
        Debug.Log("PlayFabMultiplayerLogic.OnLoginToPlayFabComplete()");
        Debug.Log("Logged in a player with PlayFabId: " + result.PlayFabId);

        this._playFabAuthContext = result.AuthenticationContext;

        MyUserID = new UserID(result.AuthenticationContext.EntityId);

        PlayFabMultiplayer.SetEntityToken(result.AuthenticationContext);

        InvokeOnMultiplayerInitialized();
    }
    #endregion

    #region CreateLobby
    public override void CreateLobby()
    {
        Debug.Log("PlayFabMultiplayerLogic.CreateLobby()");

        LobbyCreateConfiguration createConfiguration = new LobbyCreateConfiguration();
        createConfiguration.MaxMemberCount = 4;
        createConfiguration.OwnerMigrationPolicy = LobbyOwnerMigrationPolicy.Automatic;
        createConfiguration.AccessPolicy = LobbyAccessPolicy.Public;

        LobbyJoinConfiguration joinConfiguration = new LobbyJoinConfiguration();
        joinConfiguration.MemberProperties["xuid"] = LiveLogic.MyXUID.ToString();

        myLobby = PlayFabMultiplayer.CreateAndJoinLobby(PlayFabSettings.staticPlayer, createConfiguration, joinConfiguration);
    }

    private void OnLobbyCreateAndJoinCompleted(Lobby lobby, int result)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnLobbyCreateAndJoinCompleted() Result: {result}");

        if (LobbyError.SUCCEEDED(result))
        {
            myLobby = lobby;
            LogLobby(myLobby);
            CurrentSessionState = SessionState.InHostedLobby;

            UpdateLocalSessionDocumentFromLobby(lobby);

            InvokeOnLobbyCreated();
        }
        else
        {
            InvokeOnMultiplayerError("PlayFabMultiplayer.CreateAndJoinLobby() failed");
        }
    }
    #endregion

    #region Invites
    public override void SendInvitesToFriends()
    {
        Debug.Log("PlayFabMultiplayerLogic.SendInvitesToFriends()");

        string connectionString = "";

        if (CurrentSessionState == SessionState.InHostedLobby && myLobby != null)
        {
            connectionString = myLobby.ConnectionString;
        }
        else if (CurrentSessionState == SessionState.InMatchedLobby && _matchmakingLobby != null) // TODO: is this the correct thing to do here?
        {
            connectionString = _matchmakingLobby.ConnectionString;
        }

        if (!string.IsNullOrEmpty(connectionString))
        {
            InviteAndActivity.SendInvitesToFriends(connectionString);
        }
        else
        {
            Debug.Log("PlayFabMultiplayerLogic.SendInvitesToFriends() connectionString was invalid");
        }
    }

    public override void JoinInvitedLobby()
    {
        Debug.Log("PlayFabMultiplayerLogic.JoinInvitedLobby()");

        if(InviteAndActivity.HasMultiplayerInvite)
        {
            JoinLobby(InviteAndActivity.MostRecentReceivedInviteData);
        }
        else
        {
            Debug.Log("PlayFabMultiplayerLogic.JoinInvitedLobby() there is not a pending invite to accept currently");
        }
    }
    #endregion

    #region JoinLobby
    public override void JoinLobby(string lobbyConnectionString)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.JoinLobby() lobbyConnectionString:{lobbyConnectionString}");

        // clean up any pending invite handle if it happens to match
        if (lobbyConnectionString == InviteAndActivity.MostRecentReceivedInviteData)
        {
            InviteAndActivity.ClearMostRecentInviteData();
        }

        if (!string.IsNullOrEmpty(lobbyConnectionString))
        {
            Dictionary<string, string> memberProperties = new Dictionary<string, string>();
            memberProperties["xuid"] = LiveLogic.MyXUID.ToString();

            PlayFabMultiplayer.JoinLobby(PlayFabSettings.staticPlayer, lobbyConnectionString, memberProperties);
        }
        else
        {
            Debug.Log("PlayFabMultiplayerLogic.JoinLobby() lobbyConnectionString was invalid");
            InvokeOnMultiplayerError("JoinLobby FAILURE");
        }
    }

    private void OnLobbyJoinCompleted(Lobby lobby, PFEntityKey newMember, int result)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnLobbyJoinCompleted() Result:{result} EntityID:{newMember.Id}");

        if (LobbyError.SUCCEEDED(result))
        {
            myLobby = lobby;
            UpdateLocalSessionDocumentFromLobby(lobby);
            CurrentSessionState = SessionState.InHostedLobby;
            InvokeOnLobbyJoined();
        }
        else
        {
            InvokeOnMultiplayerError("JoinLobbyCompleted FAILURE");
        }
    }
    #endregion

    #region LeaveLobby
    public override void LeaveLobby()
    {
        Debug.Log("PlayFabMultiplayerLogic.LeaveLobby()");

        if (CurrentSessionState == SessionState.InMatchedLobby && _matchmakingLobby != null)
        {
            _matchmakingLobby.Leave(_playFabAuthContext);
        }
        else if (myLobby != null)
        {
            myLobby.Leave(_playFabAuthContext);
        }
        else
        {
            Debug.Log("PlayFabMultiplayerLogic.LeaveLobby: could not leave lobby because it was null");
        }
    }

    private void OnLobbyLeaveCompleted(Lobby lobby, PFEntityKey newMember)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnLobbyLeaveCompleted() EntityID:{newMember.Id}");

        if (CurrentSessionState == SessionState.InMatchedLobby && _matchmakingLobby == lobby)
        {
            ClearMatchMakeLobby();
            InvokeOnLobbyLeft();
        }
        else if (myLobby == lobby)
        {
            myLobby = null;
            InvokeOnLobbyLeft();
        }
    }

    private void OnLobbyDisconnected(Lobby lobby)
    {
        Debug.Log("PlayFabMultiplayerLogic.OnLobbyDisconnected()");
    }
    #endregion

    #region LobbyUpdates
    private void OnLobbyPostUpdateCompleted(Lobby lobby, PFEntityKey member, int result)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnLobbyPostUpdateCompleted() Result: {result} EntityID:{member.Id}");

        if (LobbyError.SUCCEEDED(result))
        {
            UpdateLocalSessionDocumentFromLobby(lobby);
        }
        else
        {
            InvokeOnMultiplayerError("OnLobbyPostUpdateCompleted FAILURE");
        }
    }

    private void OnLobbyUpdated(
        Lobby lobby,
        bool ownerUpdated,
        bool maxMembersUpdated,
        bool accessPolicyUpdated,
        bool membershipLockUpdated,
        IList<string> updatedSearchPropertyKeys,
        IList<string> updatedLobbyPropertyKeys,
        IList<LobbyMemberUpdateSummary> memberUpdates)
    {
        Debug.Log("PlayFabMultiplayerLogic.OnLobbyUpdated()");

        UpdateLocalSessionDocumentFromLobby(lobby);
    }

    public void UpdateLocalSessionDocumentFromLobby(Lobby lobby)
    {
        Debug.Log("PlayFabMultiplayerLogic.UpdateLocalSessionDocumentFromLobby()");

        CurrentSessionDocument = new SessionDocument();

        if (lobby != null)
        {
            LogLobby(lobby);

            foreach (PFEntityKey member in lobby.GetMembers())
            {
                UpdateXuidCacheForLobbyMember(lobby, member);
            }

            CurrentSessionDocument.FromDictionary(lobby.GetLobbyProperties());

            PFEntityKey owner;
            if (lobby.TryGetOwner(out owner))
            {
                HostID = new UserID(owner.Id);
                CurrentSessionDocument.HostID = owner.Id;
            }

            IsHost = HostID == MyUserID;
            InvokeOnSessionPropertiesChanged(CurrentSessionDocument);
        }
        else
        {
            Debug.Log("PlayFabMultiplayerLogic.UpdateLocalSessionDocumentFromLobby: lobby was null");
        }
    }

    public override void SetSessionProperties(SessionDocument sessionDocument)
    {
        Debug.Log("PlayFabMultiplayerLogic.SetSessionProperties()");

        Assert.IsTrue(IsHost);

        CurrentSessionDocument = sessionDocument;
        HostID = new UserID(CurrentSessionDocument.HostID);

        Lobby lobbyToModify = null;

        if (CurrentSessionState == SessionState.InMatchedLobby && _matchmakingLobby != null)
        {
            lobbyToModify = _matchmakingLobby;
        }
        else if (CurrentSessionState == SessionState.InHostedLobby && myLobby != null)
        {
            lobbyToModify = myLobby;
        }

        if (lobbyToModify != null)
        {
            var lobbyUpdateData = new LobbyDataUpdate()
            {
                LobbyProperties = sessionDocument.ToDictionary()
            };

            lobbyToModify.PostUpdate(PlayFabSettings.staticPlayer, lobbyUpdateData);
        }
    }
    #endregion

    #region Matchmaking
    public override void StartMatchmaking()
    {
        Debug.Log("PlayFabMultiplayerLogic.StartMatchmaking()");

        Assert.IsFalse(IsMatchmaking);
        Assert.IsNull(_activeMatchmakeTicket);

        var localUser = new MatchUser(PlayFabSettings.staticPlayer, string.Empty);

        _activeMatchmakeTicket = PlayFabMultiplayer.CreateMatchmakingTicket(localUser, Configuration.MATCHMAKING_HOPPER_NAME, Configuration.MATCHMAKING_TIMEOUT_IN_SECONDS);

        IsMatchmaking = true;
    }

    public override void CancelMatchmaking()
    {
        Debug.Log("PlayFabMultiplayerLogic.CancelMatchmaking()");

        if (_activeMatchmakeTicket != null && IsMatchmaking)
        {
            _activeMatchmakeTicket.Cancel();
        }
        else
        {
            ClearMatchMakeLobby();
            InvokeOnSessionMatchMakeCancelled();
        }
    }

    public override void FindMatches()
    {
        Debug.Log("PlayFabMultiplayerLogic.FindMatches()");

        LobbySearchConfiguration searchConfig = new LobbySearchConfiguration();
        searchConfig.FilterString = "";
        searchConfig.SortString = "";
        searchConfig.ClientSearchResultCount = _MAX_SEARCH_RESULTS_;

        PlayFabMultiplayer.FindLobbies(PlayFabSettings.staticPlayer, searchConfig);
    }

    private void OnMatchmakingTicketCompleted(MatchmakingTicket ticket, int result)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnMatchmakingTicketCompleted() Result: {result}");

        if (LobbyError.SUCCEEDED(result) && _activeMatchmakeTicket != null && _activeMatchmakeTicket.Equals(ticket))
        {
            var details = _activeMatchmakeTicket.GetMatchDetails();

            Dictionary<string, string> memberProperties = new Dictionary<string, string>();
            memberProperties["xuid"] = LiveLogic.MyXUID.ToString();

            var config = new LobbyArrangedJoinConfiguration();
            config.MaxMemberCount = Configuration.LOBBY_MAX_MEMBERS_IN_SESSION;
            config.OwnerMigrationPolicy = LobbyOwnerMigrationPolicy.Manual;
            config.AccessPolicy = LobbyAccessPolicy.Friends;
            config.MemberProperties = memberProperties;

            _matchmakingLobby = PlayFabMultiplayer.JoinArrangedLobby(PlayFabSettings.staticPlayer, details.LobbyArrangementString, config);
        }
        else
        {
            ClearMatchMakeLobby();
            InvokeOnSessionMatchMakeCancelled();
        }
    }

    private void OnMatchmakingTicketStatusChanged(MatchmakingTicket ticket)
    {
        Debug.Log("PlayFabMultiplayerLogic.OnMatchmakingTicketStatusChanged()");
    }

    private void OnLobbyJoinArrangedLobbyCompleted(Lobby lobby, PFEntityKey newMember, int result)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnLobbyJoinArrangedLobbyCompleted() Result: {result} EntityID:{newMember.Id}");

        if (LobbyError.FAILED(result))
        {
            InvokeOnMultiplayerError($"Failed to join matchmake lobby with ID: {lobby.Id}");
            ClearMatchMakeLobby();
            return;
        }

        CurrentSessionState = SessionState.InMatchedLobby;
        IsMatchmaking = false;

        Assert.AreEqual(lobby, _matchmakingLobby);
        Assert.AreEqual(newMember.Id, PlayFabSettings.staticPlayer.EntityId);

        PFEntityKey ownerId;
        var success = lobby.TryGetOwner(out ownerId);

        if (success)
        {
            IsHost = ownerId.Id == PlayFabSettings.staticPlayer.EntityId;
            HostID = new UserID(ownerId.Id);
        }
        else
        {
            // TODO: if getting the host failed, perhaps force a deterministic 
            // random host from amongst the members and have that host explicitly
            // set themselves as the new host?
            Debug.Log($"PlayFabMultiplayerLogic.OnLobbyJoinArrangedLobbyCompleted() Lobby.TryGetOwner() FAILED.");
        }

        var currentSessionDocument = CurrentSessionDocument;

        if (IsHost)
        {
            currentSessionDocument = currentSessionDocument ?? new SessionDocument();
            currentSessionDocument.HostID = HostID.ID;
            SetSessionProperties(currentSessionDocument);
        }
        else if (null == currentSessionDocument || string.IsNullOrEmpty(currentSessionDocument.NetworkID))
        {
            UpdateLocalSessionDocumentFromLobby(_matchmakingLobby);
        }

        InvokeOnSessionMatchMade();

        // TODO: need to see if a race condition exists here...
        // if the owner sets the properties before the guest has joined, perhaps a 
        // session document change event will not happen?
    }

    private void OnLobbyFindLobbiesCompleted(IList<LobbySearchResult> searchResults, PFEntityKey newMember, int result)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnLobbyFindLobbiesCompleted() Result: {result} EntityID:{newMember.Id}");
    }
    #endregion

    #region StartGameLeaveGame
    public override void StartGame()
    {
        Debug.Log("PlayFabMultiplayerLogic.StartGame()");

        if (CurrentSessionState == SessionState.InHostedLobby)
        {
            Debug.LogFormat($"PlayFabMultiplayerLogic.StartGame() changing session state from InHostedLobby to InHostedGame");
            CurrentSessionState = SessionState.InHostedGame;
            InvokeOnGameJoined();
        }
        else if (CurrentSessionState == SessionState.InMatchedLobby)
        {
            Debug.LogFormat($"PlayFabMultiplayerLogic.StartGame() changing session state from InMatchedLobby to InMatchedGame");
            CurrentSessionState = SessionState.InMatchedGame;
            InvokeOnGameJoined();
        }
    }

    public override void LeaveGame()
    {
        Debug.Log("PlayFabMultiplayerLogic.LeaveGame()");

        if (CurrentSessionState == SessionState.InHostedGame)
        {
            Debug.LogFormat($"PlayFabMultiplayerLogic.LeaveGame() changing session state from InHostedGame to InHostedLobby");
            CurrentSessionState = SessionState.InHostedLobby;
            InvokeOnGameLeft();
        }
        else if (CurrentSessionState == SessionState.InMatchedGame)
        {
            Debug.LogFormat($"PlayFabMultiplayerLogic.LeaveGame() changing session state from InMatchedGame to InMatchedLobby");
            CurrentSessionState = SessionState.InMatchedLobby;
            InvokeOnMatchLeft();
        }
    }
    #endregion

    #region Members
    private void OnLobbyMemberAdded(Lobby lobby, PFEntityKey member)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnLobbyMemberAdded() EntityID:{member.Id}");

        UpdateXuidCacheForLobbyMember(lobby, member);

        LogLobby(lobby);

        UserID addedMemberUserID = new UserID(member.Id);

        InviteAndActivity.UpdateRecentPlayers(GetXuidForUserID(addedMemberUserID));

        InvokeOnMembersAdded(new[] { addedMemberUserID });
    }

    private void OnLobbyMemberRemoved(Lobby lobby, PFEntityKey member, LobbyMemberRemovedReason reason)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnLobbyMemberRemoved() Result:{reason} EntityID:{member.Id}");

        UserID userID = new UserID(member.Id);

        // TODO: we will need to manually choose another host for
        // whatever lobby we are currently in (if the member who left also happened to be the host) ...

        var lobbyMembers = GetLobbyMembers();

        if (userID == HostID && lobbyMembers != null && lobbyMembers.Length > 0)
        {
            var newHostID = ChooseDeterministicRandomXuid(lobbyMembers);
            if (newHostID == MyUserID)
            {
                IsHost = true;
                HostID = MyUserID;

                var lobbyUpdate = new LobbyDataUpdate();
                lobbyUpdate.NewOwner = new PFEntityKey(PlayFabSettings.staticPlayer.EntityId, PlayFabSettings.staticPlayer.EntityType);
                lobby.PostUpdate(PlayFabSettings.staticPlayer, lobbyUpdate);

                CurrentSessionDocument.HostID = newHostID.ID;
                SetSessionProperties(CurrentSessionDocument);
            }
        }

        //clear their xuid from the cache
        ulong xuid = 0;

        if (xuidMap.TryGetValue(userID, out xuid))
        {
            xuidMap.Remove(userID);
            userIDMap.Remove(xuid);
        }

        LogLobby(lobby);

        InvokeOnMembersRemoved(new[] { userID });
    }

    public override UserID[] GetLobbyMembers()
    {
        Debug.Log("PlayFabSessionLogic.GetLobbyMembers()");

        List<UserID> members = new List<UserID>();

        if (myLobby != null)
        {
            GetMembersOfPFLobby(myLobby, members);
        }
        else if (_matchmakingLobby != null)
        {
            GetMembersOfPFLobby(_matchmakingLobby, members);
        }
        else
        {
            Debug.Log("PlayFabMultiplayerLogic.GetLobbyMembers: there is no valid lobby to return members for");
        }

        return members.ToArray();
    }

    public override UserID[] GetGameMembers()
    {
        Debug.Log("PlayFabSessionLogic.GetGameMembers()");

        return GetLobbyMembers();
    }

    private void GetMembersOfPFLobby(Lobby lobby, List<UserID> members)
    {
        Debug.Log("PlayFabSessionLogic.GetMembersOfPFLobby()");

        if (lobby != null)
        {
            IList<PFEntityKey> memberEntityKeys = lobby.GetMembers();
            if (memberEntityKeys != null)
            {
                foreach (PFEntityKey entityKey in memberEntityKeys)
                {
                    members.Add(new UserID(entityKey.Id));
                }
            }
            else
            {
                Debug.Log("PlayFabSessionLogic.GetMembersOfPFLobby: memberEntityKeys was null");
            }
        }
        else
        {
            Debug.Log("PlayFabSessionLogic.GetMembersOfPFLobby: lobby was null");
        }
    }
    #endregion

    #region Xuids
    private void UpdateXuidCacheForLobbyMember(Lobby lobby, PFEntityKey member)
    {
        Debug.LogFormat($"PlayFabMultiplayerLogic.UpdateXuidCacheForLobbyMember() EntityID:{member.Id}");

        IDictionary<string, string> memberProperties = lobby.GetMemberProperties(member);
        if (memberProperties != null)
        {
            string xuidString;
            if (memberProperties.TryGetValue("xuid", out xuidString))
            {
                UserID userID = new UserID(member.Id);
                ulong xuid = Convert.ToUInt64(xuidString);

                if (xuid != 0)
                {
                    Debug.LogFormat($"PlayFabMultiplayerLogic.UpdateXuidCacheForLobbyMember: Adding new member to xuid cache: xuid:{xuid}, EntityID:{member.Id}");

                    xuidMap[userID] = xuid;
                    userIDMap[xuid] = userID;
                }
                else
                {
                    Debug.LogFormat($"PlayFabMultiplayerLogic.UpdateXuidCacheForLobbyMember: Xuid was invalid for this user EntityID:{member.Id}");
                }
            }
            else
            {
                Debug.LogFormat($"PlayFabMultiplayerLogic.UpdateXuidCacheForLobbyMember: Could not get the xuid this user EntityID:{member.Id}");
            }
        }
    }

    public override ulong GetXuidForUserID(UserID userID)
    {
        ulong xuid = 0;

        if (userID.IsValid())
        {
            xuidMap.TryGetValue(userID, out xuid);

            if (xuid == 0)
            {
                Debug.LogFormat($"PlayFabSessionLogic.GetXuidForUserID: could not find a xuid for user UserID:{userID.ToString()}");
            }
        }
        else
        {
            Debug.Log("PlayFabSessionLogic.GetXuidForUserID: serID was invalid");
        }

        return xuid;
    }

    public override UserID GetUserIDforXuid(ulong xuid)
    {
        UserID userID = default(UserID);

        if (xuid != 0)
        {
            userIDMap.TryGetValue(xuid, out userID);

            if (userID.IsValid())
            {
                Debug.LogFormat($"PlayFabSessionLogic.GetUserIDforXuid: could not find a UserID for user xuid:{xuid}");
            }
        }
        else
        {
            Debug.Log("PlayFabSessionLogic.GetUserIDforXuid: xuid was invalid");
        }

        return userID;
    }
    #endregion

    #region Helpers
    public override string GetSessionConnectionString()
    {
        return myLobby != null ? myLobby.ConnectionString : string.Empty;
    }

    private void LogLobby(Lobby lobby)
    {
        Debug.Log("\n");

        Debug.LogFormat($"ID={lobby.Id} MaxMemberCount:{lobby.MaxMemberCount} OwnerMigrationPolicy:{lobby.OwnerMigrationPolicy} AccessPolicy:{lobby.AccessPolicy} MembershipLock:{lobby.MembershipLock} #LobbyProps:{lobby.GetLobbyProperties().Count}");
        Debug.LogFormat($"ConnectionString:{lobby.ConnectionString}");
        PFEntityKey owner;
        bool hasOwner = lobby.TryGetOwner(out owner);
        string ownerId = hasOwner ? owner.Id : "n/a";
        Debug.LogFormat($"HasOwner:{hasOwner} OwnerID={ownerId}");

        int lobbyPropertyIndex = 1;
        foreach (var prop in lobby.GetLobbyProperties())
        {
            Debug.LogFormat($"     Lobby Prop {lobbyPropertyIndex}: K={prop.Key} V={prop.Value}");
            lobbyPropertyIndex++;
        }

        int index = 1;
        foreach (var member in lobby.GetMembers())
        {
            Debug.LogFormat($"Member {index}: {member.Id} {member.Type} Props: {lobby.GetMemberProperties(member).Count}");

            int propindex = 1;
            foreach (var prop in lobby.GetMemberProperties(member))
            {
                Debug.LogFormat($"     Member Prop {propindex}: K={prop.Key} V={prop.Value}");
                propindex++;
            }

            index++;
        }

        Debug.Log("\n");
    }

    public override UserActivityAndInviteMechanism InviteAndActivity
    {
        get
        {
            if (_inviteAndActivity == null)
            {
                _inviteAndActivity = new XboxLiveMPALogic(LiveLogic, AllowCrossPlatformInvites);
            }
            return _inviteAndActivity;
        }
    }

    private void OnValidate()
    {
        Assert.IsNotNull(LiveLogic);
    }

    private void OnError(PlayFabMultiplayerErrorArgs args)
    {
        string errorCode = $"0x{args.Code:X}";
        Debug.LogFormat($"PlayFabMultiplayerLogic.OnError: {errorCode} Message:{args.Message}");
    }
    #endregion
}