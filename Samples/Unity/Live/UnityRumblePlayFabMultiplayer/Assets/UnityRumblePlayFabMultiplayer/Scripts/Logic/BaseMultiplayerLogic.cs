using System;
using System.IO;
using UnityEngine;
using UnityEngine.Assertions;

public class UserID
{
    public UserID(string inID)
    {
        ID = inID;
    }

    public override string ToString()
    { 
        return ID; 
    }
    
    public override bool Equals(object obj)
    {
        return ID == obj.ToString();
    }
    
    public override int GetHashCode()
    {
        return ID.GetHashCode();
    }

    public bool Equals(UserID other)
    {
        if (other is null)
        {
            return false;
        }

        // if they both have valid strings
        if(this.IsValid() && other.IsValid())
        {
            // if their strings are equal
            if(String.Equals(this.ID, other.ID))
            {
                return true;
            }
        }
        
        return false;
    }

    public static bool operator ==(UserID lhs, UserID rhs)
    {
        if (lhs is null)
        {
            if (rhs is null)
            {
                // null == null = true.
                return true;
            }

            // Only the left side is null.
            return false;
        }

        // Equals handles the case of null on right side.
        return lhs.Equals(rhs);
    }

    public static bool operator !=(UserID lhs, UserID rhs) => !(lhs == rhs);

    public void DeserializeFrom(BinaryReader reader)
    {
        var byteCount = reader.ReadByte();
        var bytes = reader.ReadBytes(byteCount);
        ID = System.Text.Encoding.ASCII.GetString(bytes);
    }

    public void SerializeTo(BinaryWriter writer)
    {
        var bytes = System.Text.Encoding.ASCII.GetBytes(ID);
        var byteCount = bytes.Length;
        writer.Write((byte)byteCount);
        writer.Write(bytes);
    }

    public bool IsValid()
    {
        return String.IsNullOrEmpty(ID) == false;
    }

    public string ID { get; private set; }
}

public abstract class BaseMultiplayerLogic : MonoBehaviour
{
    public abstract class UserActivityAndInviteMechanism
    {
        public delegate void ErrorCallback(string message, int hr);

        public event Action OnInviteReceived;
        public event Action<bool> OnInvitesSent;
        public event Action OnSocialFriendActivitiesObtained;
        public event Action OnRequestPrivilegesComplete;
        public event Action<bool> OnActivitySet;
        public event ErrorCallback OnGeneralError;
        public event ErrorCallback OnSocialActivityError;

        public bool HasMultiplayerPrivileges { get; protected set; }
        public bool HasMultiplayerInvite { get { return !string.IsNullOrEmpty(MostRecentReceivedInviteData); } }
        public string MostRecentReceivedInviteData { get; protected set; }

        public void ClearMostRecentInviteData() { MostRecentReceivedInviteData = null; }
        public void ClearEventHandlers()
        {
            OnInviteReceived = null;
            OnInvitesSent = null;
            OnSocialFriendActivitiesObtained = null;
            OnRequestPrivilegesComplete = null;
            OnActivitySet = null;
            OnGeneralError = null;
            OnSocialActivityError = null;
        }

        public abstract void Initialize();
        public abstract void Cleanup();
        
        public abstract void RequestMultiplayerPrivileges();
        // note: null or empty not allowed for inviteData
        // note: relies on recipient callback when invite has been received
        public abstract void SendInvitesToFriends(string inviteData);
        // note: use null to clear activity
        public abstract void SetUserActivity(string activityData, int totalPlayers);
        // note: relies on callback when activity has been received
        public abstract void GetFriendActivities();

        public abstract void UpdateRecentPlayers(UInt64 metPlayerXuid);

        // INTERNAL

        protected void InvokeOnInviteReceived() { OnInviteReceived?.Invoke(); }
        protected void InvokeOnInvitesSent(bool success) { OnInvitesSent?.Invoke(success); }
        protected void InvokeOnSocialFriendActivitiesObtained() { OnSocialFriendActivitiesObtained?.Invoke(); }
        protected void InvokeOnRequestPrivilegesComplete() { OnRequestPrivilegesComplete?.Invoke(); }
        protected void InvokeOnActivitySet(bool success) { OnActivitySet?.Invoke(success); }

        protected void InvokeOnGeneralError(string message, int hr)
        {
            OnGeneralError?.Invoke(message, hr);
        }

        protected void InvokeOnSocialActivityError(string message, int hr)
        {
            OnSocialActivityError?.Invoke(message, hr);
        }
    }

    public enum SessionState : int
    {
        NoSession = 0,
        InHostedLobby = 1,
        InHostedGame = 2,
        InMatchedLobby = 3,
        InMatchedGame = 4,
    }

    //
    // general purpose events
    //
    // parameters are: error message, HRESULT
    public event Action<string, int> OnGeneralError;
    //
    // multiplayer logic events
    //
    public event Action OnMultiplayerInitialized;
    public event Action OnLobbyCreated;
    public event Action OnLobbyJoined;
    public event Action OnLobbyLeft;
    public event Action OnGameJoined;
    public event Action OnGameLeft;
    public event Action OnMatchLeft;
    public event Action OnHostChanged;
    public event Action<UserID[]> OnMembersAdded;
    public event Action<UserID[]> OnMembersRemoved;
    // parameters are: error message, HRESULT
    public event Action<string> OnMultiplayerError;
    //
    // matchmaking logic events
    //
    public event Action OnSessionMatchMade;
    public event Action OnSessionMatchMakeCancelled;
    //
    // session logic events
    //
    public event Action<SessionDocument> OnSessionPropertiesChanged;

    // initialization and cleanup
    public abstract void InitializeMultiplayer();
    public abstract void CleanupMultiplayer();
    public abstract UserActivityAndInviteMechanism InviteAndActivity { get; }

    // matchmaking APIs
    public bool IsMatchmaking { get; protected set; }
    public abstract void StartMatchmaking();
    public abstract void CancelMatchmaking();
    public abstract void FindMatches();

    // session related APIs
    public SessionDocument CurrentSessionDocument { get; protected set; }
    public SessionState CurrentSessionState { get; protected set; }
    public abstract string GetSessionConnectionString();
    public abstract void SetSessionProperties(SessionDocument sessionDocument);
    public abstract UserID[] GetLobbyMembers();
    public abstract UserID[] GetGameMembers();
    public abstract void StartGame();
    public abstract void LeaveGame();
    public abstract void ClearSessionState();

    // multiplayer lobby APIs
    public bool IsHost { get; protected set; }
    public UserID HostID { get; protected set; }
    public abstract void CreateLobby();
    public abstract void JoinLobby(string lobbySessionHandle);
    public abstract void JoinInvitedLobby();
    public abstract void LeaveLobby();

    public abstract void SendInvitesToFriends();

    public UserID MyUserID { get; protected set; }

    public abstract ulong GetXuidForUserID(UserID userID);
    public abstract UserID GetUserIDforXuid(ulong xuid);

    // event related methods
    public void ClearEventHandlers()
    {
        OnGeneralError = null;
        OnMultiplayerInitialized = null;
        OnLobbyCreated = null;
        OnLobbyJoined = null;
        OnLobbyLeft = null;
        OnGameJoined = null;
        OnGameLeft = null;
        OnMatchLeft = null;
        OnHostChanged = null;
        OnMembersAdded = null;
        OnMembersRemoved = null;
        OnMultiplayerError = null;
        OnSessionMatchMade = null;
        OnSessionMatchMakeCancelled = null;
        OnSessionPropertiesChanged = null;

        InviteAndActivity.ClearEventHandlers();
    }

    // INTERNAL

    protected void InvokeOnGeneralError(string message, int hr) 
    { 
        OnGeneralError?.Invoke(message, hr); 
    }
    protected void InvokeOnMultiplayerInitialized() { OnMultiplayerInitialized?.Invoke(); }
    protected void InvokeOnLobbyCreated() { OnLobbyCreated?.Invoke(); }
    protected void InvokeOnLobbyJoined() { OnLobbyJoined?.Invoke(); }
    protected void InvokeOnLobbyLeft() { OnLobbyLeft?.Invoke(); }
    protected void InvokeOnGameJoined() { OnGameJoined?.Invoke(); }
    protected void InvokeOnGameLeft() { OnGameLeft?.Invoke(); }
    protected void InvokeOnMatchLeft() { OnMatchLeft?.Invoke(); }
    protected void InvokeOnHostChanged() { OnHostChanged?.Invoke(); }

    protected void InvokeOnMembersAdded(UserID[] addedMemberIds) 
    { 
        OnMembersAdded?.Invoke(addedMemberIds); 
    }

    protected void InvokeOnMembersRemoved(UserID[] removedMemberIds) 
    { 
        OnMembersRemoved?.Invoke(removedMemberIds); 
    }

    protected void InvokeOnMultiplayerError(string message)
    {
        OnMultiplayerError?.Invoke(message);
    }

    protected void InvokeOnSessionMatchMade() { OnSessionMatchMade?.Invoke(); }
    protected void InvokeOnSessionMatchMakeCancelled() { OnSessionMatchMakeCancelled?.Invoke(); }

    protected void InvokeOnSessionPropertiesChanged(SessionDocument revisedDocument) 
    { 
        OnSessionPropertiesChanged?.Invoke(revisedDocument); 
    }

    protected UserID ChooseDeterministicRandomXuid(UserID[] uids)
    {
        Assert.IsTrue(uids.Length > 0);

        int total = 0;

        for (var i = 0; i < uids.Length; i++)
        {
            total += DeterministicHash(uids[i].ID);
        }

        var randomGenerator = new System.Random(total);
        var randomIndex = randomGenerator.Next(0, uids.Length - 1);

        return uids[randomIndex];
    }

    private int DeterministicHash(string str)
    {
        var hashValue = 23;
        foreach (char c in str)
        {
            hashValue *= 31;
            hashValue += c;
        }
        return hashValue;
    }

    private void OnDestroy()
    {
        CleanupMultiplayer();
    }
}

