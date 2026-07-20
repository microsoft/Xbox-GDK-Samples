using System;
using System.Linq;
using Unity.XGamingRuntime;
using UnityEngine;
using UnityEngine.Networking;
using PlayFab;

public partial class XboxLiveMPALogic : BaseMultiplayerLogic.UserActivityAndInviteMechanism
{
    private XGameInviteRegistrationToken _inviteRegistrationToken;
    private XboxLiveLogic LiveLogic { get; }
    private bool AllowCrossPlatformJoin { get; }

    public XboxLiveMPALogic(XboxLiveLogic liveLogic, bool allowCrossPlatformJoin)
    {
        LiveLogic = liveLogic;
        AllowCrossPlatformJoin = allowCrossPlatformJoin;
    }

    public override void Initialize()
    {
        var hr = SDK.XUserCheckPrivilege(
            LiveLogic.MyUserHandle,
            XUserPrivilegeOptions.None,
            XUserPrivilege.Multiplayer,
            out bool hasMultiplayerPrivileges,
            out XUserPrivilegeDenyReason reason);

        Debug.LogFormat($"XUserCheckPrivilege() returned HRESULT:{hr:X8}");

        if (HR.FAILED(hr))
        {
            InvokeOnGeneralError(string.Format($"XUserCheckPrivilege() failed with HRESULT:{hr:X8}"), hr);
            return;
        }

        HasMultiplayerPrivileges = hasMultiplayerPrivileges;

        if (HasMultiplayerPrivileges)
        {
            RegisterForInviteEvents();
        }
        else
        {
            RequestMultiplayerPrivileges();
        }
    }

    public override void Cleanup()
    {
        if (_inviteRegistrationToken != null)
        {
            SDK.XGameInviteUnregisterForEvent(_inviteRegistrationToken);
            _inviteRegistrationToken = null;
        }
    }

    public override void SendInvitesToFriends(string inviteData)
    {
        Debug.LogFormat($"XboxLiveMPALogic.SendInvitesToFriends() inviteData:{inviteData}");

        if(!string.IsNullOrEmpty(inviteData))
        {
            SDK.XGameUiShowMultiplayerActivityGameInviteAsync(LiveLogic.MyUserHandle, (Int32 hr) =>
            {
                if (HR.SUCCEEDED(hr))
                {
                    InvokeOnInvitesSent(HR.SUCCEEDED(hr));
                }
                else
                {
                    Debug.LogFormat($"XGameUiShowMultiplayerActivityGameInvite failed with HRESULT:{hr:X8}");
                    InvokeOnInvitesSent(false);
                }
            });
        }
        else
        {
            Debug.LogFormat($"XboxLiveMPALogic.SendInvitesToFriends() inviteData was invalid");
        }
    }

    private void RegisterForInviteEvents()
    {
        Debug.Log("XboxLiveMPALogic.RegisterForInviteEvents()");

        if (_inviteRegistrationToken == null)
        {
            var hr = SDK.XGameInviteRegisterForEvent(InviteEventHandler, out _inviteRegistrationToken);
            if (HR.FAILED(hr))
            {
                InvokeOnGeneralError(string.Format($"XGameInviteRegisterForEvent() failed with HRESULT:{hr:X8}"), hr);
            }
        }
    }

    private void InviteEventHandler(IntPtr context, string inviteUri)
    {
        Debug.LogFormat($"XboxLiveMPALogic.InviteEventHandler() inviteUri:{inviteUri}");

        if(!string.IsNullOrEmpty(inviteUri))
        {
            const string KEY = "connectionString=";
            var start = inviteUri.IndexOf(KEY) + KEY.Length;
            var end = inviteUri.IndexOf("&", start);

            if(end <= start)
            {
                end = inviteUri.Length;
            }

            var connectionString = inviteUri.Substring(start, end - start);

            if (connectionString.Contains("%3A"))
            {
                connectionString = UnityWebRequest.UnEscapeURL(connectionString);
            }

            Debug.LogFormat($"XboxLiveMPALogic.InviteEventHandler() received invite with connection string:{connectionString}");

            MostRecentReceivedInviteData = connectionString;

            InvokeOnInviteReceived();
        }
        else
        {
            Debug.Log("XboxLiveMPALogic.InviteEventHandler() inviteUri was invalid");
        }
    }

    public override void RequestMultiplayerPrivileges()
    {
        SDK.XUserResolvePrivilegeWithUiAsync(
            LiveLogic.MyUserHandle,
            XUserPrivilegeOptions.None,
            XUserPrivilege.Multiplayer,
            (hr) =>
            {
                if (HR.SUCCEEDED(hr))
                {
                    HasMultiplayerPrivileges = true;
                    RegisterForInviteEvents();
                    InvokeOnRequestPrivilegesComplete();
                }
                else
                {
                    InvokeOnGeneralError(
                        "Failure to resolve MP privilege with XUserResolvePrivilegeWithUiAsync()",
                        hr);
                }
            });
    }

    public override void SetUserActivity(string activityData, int totalPlayers)
    {
        Debug.LogFormat($"XboxLiveMPALogic.SetActivity({activityData}, {totalPlayers})");

        if (string.IsNullOrEmpty(activityData))
        {
            SDK.XBL.XblMultiplayerActivityDeleteActivityAsync(
                LiveLogic.MyContextHandle,
                (Int32 hr) =>
                {
                    InvokeOnActivitySet(HR.SUCCEEDED(hr));
                });
        }
        else
        {
            var newActivity = new XblMultiplayerActivityInfo()
            {
                Xuid = LiveLogic.MyXUID,
                ConnectionString = activityData,
                JoinRestriction = XblMultiplayerActivityJoinRestriction.Followed,
                MaxPlayers = 5,
                CurrentPlayers = (uint)totalPlayers, //note: update this as more players join
                GroupId = PlayFabSettings.staticSettings.TitleId
            };

            Debug.LogFormat($"XboxLiveMPALogic.SetActivity: GroupId: {newActivity.GroupId} Xuid: {newActivity.Xuid} ConnectionString {newActivity.ConnectionString} Players: {newActivity.CurrentPlayers} of {newActivity.MaxPlayers}");

            SDK.XBL.XblMultiplayerActivitySetActivityAsync(
                LiveLogic.MyContextHandle,
                newActivity,
                AllowCrossPlatformJoin,
                (Int32 hr) =>
                {
                    InvokeOnActivitySet(HR.SUCCEEDED(hr));
                });
        }
    }

    public override void GetFriendActivities()
    {
        Debug.Log("XboxLiveMPALogic.GetFriendActivities()");

        XboxLiveLogic.SocialFriend[] friends;
        LiveLogic.GetOnlineInTitleFriends(out friends);

        if(friends != null && friends.Length > 0)
        {
            Debug.LogFormat($"XboxLiveMPALogic.GetFriendActivities() found {friends.Length} online in-title friends");
            
            SDK.XBL.XblMultiplayerActivityGetActivityAsync(
                LiveLogic.MyContextHandle,
                friends.Select(friend => friend.XUID).ToArray(),
                HandleFriendActivitiesObtained);
        }
        else
        {
            Debug.Log("XboxLiveMPALogic.GetFriendActivities() found no online in-title friends");
            InvokeOnSocialFriendActivitiesObtained();
            return;
        }
    }

    private void HandleFriendActivitiesObtained(Int32 hr, XblMultiplayerActivityInfo[] result)
    {
        Debug.Log("XboxLiveMPALogic.HandleFriendActivitiesObtained()");

        if (HR.FAILED(hr))
        {
            InvokeOnSocialActivityError($"XblMultiplayerGetActivitiesWithPropertiesForSocialGroupAsync() failed with HRESULT:{hr.ToString("X8")}", hr);
        }

        foreach (var friend in LiveLogic.GetOnlineInTitleFriendsMap())
        {
            friend.Value.CurrentActivityInfo = null;
        }

        Debug.Log("XboxLiveMPALogic.HandleFriendActivitiesObtained()");
        Debug.LogFormat($"XboxLiveMPALogic.HandleFriendActivitiesObtained() XblMultiplayerActivityGetActivityAsync returned {result.Length} activities");

        foreach (var activity in result)
        {
            // make sure we have the friend tracked
            if (!LiveLogic.GetOnlineInTitleFriendsMap().ContainsKey(activity.Xuid))
            {
                Debug.LogFormat($"XboxLiveMPALogic.HandleFriendActivitiesObtained() found non-friend xuid: {activity.Xuid}");
                continue;
            }

            // check the title ID
            if (activity.GroupId != PlayFabSettings.staticSettings.TitleId)
            {
                Debug.LogFormat($">>> found non-matching title {activity.GroupId} for xuid: {activity.Xuid}");
                continue;
            }

            // check the join restriction
            if (activity.JoinRestriction != XblMultiplayerActivityJoinRestriction.Followed &&
                activity.JoinRestriction != XblMultiplayerActivityJoinRestriction.Public)
            {
                Debug.LogFormat($">>> found restricted activity for xuid: {activity.Xuid}");
                continue;
            }

            // check the member count against the max member count
            if (activity.CurrentPlayers == activity.MaxPlayers)
            {
                Debug.LogFormat($"XboxLiveMPALogic.HandleFriendActivitiesObtained()  found maxed out activity for xuid: {activity.Xuid}");
                continue;
            }

            Debug.LogFormat($"XboxLiveMPALogic.HandleFriendActivitiesObtained() found a joinable activity for xuid: {activity.Xuid}");
            LiveLogic.GetOnlineInTitleFriendsMap()[activity.Xuid].CurrentActivityInfo = activity;
        }

        InvokeOnSocialFriendActivitiesObtained();
    }

    public override void UpdateRecentPlayers(ulong metPlayerXuid)
    {
        Debug.Log("XboxLiveMPALogic.UpdateRecentPlayers()");

        if (metPlayerXuid == 0)
        {
            Debug.Log("XboxLiveMPALogic.UpdateRecentPlayers: metPlayerXuid was 0");
            return;
        }

        XblMultiplayerActivityRecentPlayerUpdate recentPlayerUpdate = new XblMultiplayerActivityRecentPlayerUpdate();
        recentPlayerUpdate.Xuid = metPlayerXuid;
        recentPlayerUpdate.EncounterType = XblMultiplayerActivityEncounterType.Default;

        var hr = SDK.XBL.XblMultiplayerActivityUpdateRecentPlayers(LiveLogic.MyContextHandle, new[] { recentPlayerUpdate });
        if (HR.FAILED(hr))
        {
            InvokeOnGeneralError(string.Format($"XblMultiplayerActivityUpdateRecentPlayers() failed with HRESULT:{hr:X8}"), hr);
        }
    }
}
