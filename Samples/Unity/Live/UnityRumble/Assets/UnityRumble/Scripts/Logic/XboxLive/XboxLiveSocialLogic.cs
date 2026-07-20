//--------------------------------------------------------------------------------------
// XboxLiveSocialLogic.cs
//
// Xbox Live logic that wraps Social Manager friend functionality.
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
using System.Linq;
using UnityEngine;
using UnityEngine.Networking;
using Unity.XGamingRuntime;
using HR = Unity.XGamingRuntime.Interop.HR;
using GdkSample_UnityRumble;

public partial class XboxLiveLogic
{
    public event Action<ulong, SocialProfile> OnSocialProfileObtained; // parameters are: xuid, profile

    public event Action OnSocialFriendActivitiesObtained;

    public event Action<string, int> OnSocialError; // parameters are: error message, hresult

    private XblSocialManagerUserGroupHandle _socialGroupHandle;

    private Dictionary<ulong, SocialFriend> _onlineInTitleFriends; // note: maps form XUID -> SocialFriend

    private Dictionary<ulong, SocialProfile> _cachedSocialProfiles; // note: maps form XUID -> SocialProfile

    private Dictionary<string, byte[]> _profilePicBytes; // note: maps from profileName -> profilePicPngBytes

    public class SocialProfile
    {
        public ulong XUID { get; internal set; }
        public string GamerTag { get; internal set; }
        public string DisplayName { get; internal set; }
        public string DisplayPicUrl { get; internal set; }
    }

    public class SocialFriend : SocialProfile
    {
        public XblMultiplayerActivityDetails CurrentActivityDetails { get; internal set; }
    }

    public void InitializeSocial()
    {
        Debug.LogFormat("XboxLiveLogic.InitializeSocial()");

        ClearSocialEventHandlers();

        _socialGroupHandle = null;
        _onlineInTitleFriends = new Dictionary<ulong, SocialFriend>();
        _cachedSocialProfiles = new Dictionary<ulong, SocialProfile>();
        _profilePicBytes = new Dictionary<string, byte[]>();

        AddLocalUser(MyUserHandle);
    }

    private void AddLocalUser(XUserHandle userHandle)
    {
        int hr = SDK.XUserGetId(userHandle, out ulong xuid);
        if (HR.FAILED(hr))
        {
            Debug.LogError($"{nameof(SDK)}.{nameof(SDK.XUserGetId)} failed - 0x{hr:X8}.");
        }

        // Check if the user is already initialized in Social Manager
        hr = SDK.XBL.XblSocialManagerGetLocalUsers(out XUserHandle[] currentUserHandles);
        if (HR.FAILED(hr))
        {
            Debug.LogError($"{nameof(SDK.XBL)}.{nameof(SDK.XBL.XblSocialManagerGetLocalUsers)} failed - 0x{hr:X8}.");
        }

        bool initUser = true;
        foreach (XUserHandle handle in currentUserHandles)
        {
            if (SDK.XUserCompare(handle, userHandle) == 0)
            {
                Debug.Log($"User {xuid} already initialized in Social Manager.");
                initUser = false;
                break;
            }
        }

        if (initUser)
        {
            Debug.Log($"Adding user {xuid} to SocialManager\n");

            // Add the local user to Xbox Services social manager
            hr = SDK.XBL.XblSocialManagerAddLocalUser(userHandle, XblSocialManagerExtraDetailLevel.NoExtraDetail);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK.XBL)}.{nameof(SDK.XBL.XblSocialManagerAddLocalUser)} failed - 0x{hr:X8}.");
                return;
            }
        }

        // Sets whether to enable social manager to poll every 30 seconds from the presence service.
        // Removes the need to manually refresh for presence changes.
        hr = SDK.XBL.XblSocialManagerSetRichPresencePollingStatus(userHandle, true);
        if (HR.FAILED(hr))
        {
            Debug.LogError($"{nameof(SDK.XBL)} . {nameof(SDK.XBL.XblSocialManagerSetRichPresencePollingStatus)} failed - 0x{hr:X8}.");
        }
    }

    public void CleanupSocial()
    {
        Debug.LogFormat("XboxLiveLogic.CleanupSocial()");

        if (null != _socialGroupHandle)
        {
            SDK.XBL.XblSocialManagerDestroySocialUserGroup(_socialGroupHandle);
            _socialGroupHandle = null;
        }

        SDK.XBL.XblSocialManagerRemoveLocalUser(
            MyUserHandle,
            XblSocialManagerExtraDetailLevel.NoExtraDetail);

        ClearSocialEventHandlers();
    }

    public void ClearSocialEventHandlers()
    {
        OnSocialProfileObtained = null;
        OnSocialFriendActivitiesObtained = null;
        OnSocialError = null;
    }

    public void DownloadUserProfilePic(string profileName, System.Action<string, byte[]> profilePicCallback)
    {
        Debug.LogFormat("XboxLiveLogic.DownloadUserProfilePic({0})", profileName);

        if (_profilePicBytes.ContainsKey(profileName))
        {
            profilePicCallback?.Invoke(profileName, _profilePicBytes[profileName]);
        }
        else
        {
            // we can use the social manager to get the friend's profile pic URL
            // and then use UnityWebRequest to get that bytes from that URL
            if (profileName == MyGamerTag)
            {
                SDK.XUserGetGamerPictureAsync(
                    MyUserHandle, 
                    XUserGamerPictureSize.Medium, 
                    (int hresult, byte[] buffer) =>
                    {
                        if (HR.FAILED(hresult))
                        {
                            OnGeneralError?.Invoke(
                                string.Format("XUserGetGamerPictureAsync({0}) failed", profileName),
                                hresult);
                        }
                        else
                        {
                            profilePicCallback(profileName, buffer);
                        }
                    });
            }
            else
            {
                StartCoroutine(DownloadProfilePicCoroutine(profileName, profilePicCallback));
            }
        }
    }

    public void GetSocialProfile(ulong xuid)
    {
        Debug.LogFormat("XboxLiveLogic.GetSocialProfile()");

        if (_cachedSocialProfiles.ContainsKey(xuid))
        {
            OnSocialProfileObtained?.Invoke(xuid, _cachedSocialProfiles[xuid]);
        }

        SDK.XBL.XblProfileGetUserProfileAsync(
            MyContextHandle,
            xuid,
            (Int32 hresult, XblUserProfile result) =>
            {
                if (HR.FAILED(hresult))
                {
                    OnSocialError?.Invoke(string.Format("XblProfileGetUserProfileAsync({0}) failed", xuid), hresult);
                }
                else
                {
                    OnSocialProfileObtained?.Invoke(
                        xuid,
                        new SocialProfile()
                        {
                            XUID = result.XboxUserId,
                            GamerTag = result.Gamertag,
                            DisplayName = result.AppDisplayName,
                            DisplayPicUrl = result.AppDisplayPictureResizeUri
                        });
                }
            });
    }

    public bool HasOnlineInTitleFriend(ulong friendXuid)
    {
        return _onlineInTitleFriends.ContainsKey(friendXuid);
    }

    public SocialFriend GetOnlineInTitleFriend(ulong friendXuid)
    {
        _onlineInTitleFriends.TryGetValue(friendXuid, out SocialFriend friend);
        return friend;
    }

    public void GetOnlineInTitleFriends(out SocialFriend[] friends)
    {
        Debug.LogFormat("XboxLiveLogic.GetOnlineInTitleFriends()");

        if (_onlineInTitleFriends.Count == 0)
        {
            friends = null;
            return;
        }

        friends = new SocialFriend[_onlineInTitleFriends.Count];
        var index = 0;
        foreach (var friend in _onlineInTitleFriends)
        {
            friends[index] = friend.Value;
            index++;
        }

        Debug.LogFormat(">>> returned {0} friends.", friends.Length);
    }

    public void GetSocialFriendActivities()
    {
        Debug.LogFormat("XboxLiveLogic.GetSocialFriendActivities()");

        SDK.XBL.XblMultiplayerGetActivitiesWithPropertiesForSocialGroupAsync(
            MyContextHandle,
            GDKGameRuntime.GameConfigScid, //Configuration.SCID,
            MyXUID,
            "People",
            HandleFriendActivitiesObtained);
    }

    public void UpdateSocial()
    {
        int hr = SDK.XBL.XblSocialManagerDoWork(out XblSocialManagerEvent[] socialEvents);

        if (HR.FAILED(hr))
        {
            OnSocialError?.Invoke("XblSocialManagerDoWork() failed ", hr);
            return;
        }

        if (socialEvents == null || socialEvents.Length == 0)
        {
            return;
        }

        foreach (XblSocialManagerEvent socialEvent in socialEvents)
        {
            if (socialEvent == null)
            {
                continue;
            }

            if (HR.FAILED(socialEvent.Hr))
            {
                Debug.Log($"XblSocialManagerEvent failed {socialEvent.Hr}");
                continue;
            }

            switch (socialEvent.EventType)
            {
                case XblSocialManagerEventType.LocalUserAdded:
                    {
                        Debug.Log("XblSocialManagerEventType::LocalUserAdded");
                        var result = SDK.XBL.XblSocialManagerCreateSocialUserGroupFromFilters(
                            MyUserHandle,
                            XblPresenceFilter.All/*TitleOnline*/,
                            XblRelationshipFilter.Friends,
                            out _socialGroupHandle);
                        if (HR.FAILED(result))
                        {
                            OnSocialError?.Invoke("XblSocialManagerCreateSocialUserGroupFromFilters() failed", result);
                        }
                        break;
                    }

                case XblSocialManagerEventType.UsersAddedToSocialGraph:
                    Debug.Log("XblSocialManagerEventType::UsersAddedToSocialGraph");
                    break;

                case XblSocialManagerEventType.UsersRemovedFromSocialGraph:
                    Debug.Log("XblSocialManagerEventType::UsersRemovedFromSocialGraph");
                    break;

                case XblSocialManagerEventType.SocialRelationshipsChanged:
                    Debug.Log("XblSocialManagerEventType::SocialRelationshipsChanged");
                    break;

                case XblSocialManagerEventType.SocialUserGroupLoaded:
                    Debug.Log("XblSocialManagerEventType::SocialUserGroupLoaded");
                    UpdateFriendDisplayNames(socialEvent.LoadedGroup);
                    break;

                case XblSocialManagerEventType.SocialUserGroupUpdated:
                    Debug.Log("XblSocialManagerEventType::SocialUserGroupUpdated");
                    UpdateFriendDisplayNames(socialEvent.LoadedGroup);
                    break;

                case XblSocialManagerEventType.PresenceChanged:
                    Debug.Log("XblSocialManagerEventType::PresenceChanged");
                    break;

                case XblSocialManagerEventType.ProfilesChanged:
                    Debug.Log("XblSocialManagerEventType::ProfilesChanged");
                    break;

                default:
                    break;
            }
        }
    }

    private void HandleFriendActivitiesObtained(Int32 hresult, XblMultiplayerActivityDetails[] result)
    {
        Debug.LogFormat("XboxLiveLogic.__HandleFriendActivitiesObtained()");

        if (HR.FAILED(hresult))
        {
            OnSocialError?.Invoke("XblMultiplayerGetActivitiesWithPropertiesForSocialGroupAsync() failed", hresult);
        }

        foreach (var friend in _onlineInTitleFriends)
        {
            friend.Value.CurrentActivityDetails = null;
        }

        Debug.LogFormat(">>> sifting through {0} activities...", result.Length);

        foreach (var activity in result)
        {
            // make sure we have the friend tracked
            if (!_onlineInTitleFriends.ContainsKey(activity.OwnerXuid))
            {
                Debug.LogFormat(">>> found non-friend xuid: {0}", activity.OwnerXuid.ToString());
                continue;
            }

            // check the title ID
            string titleId = activity.TitleId.ToString("X");
            if (titleId != GDKGameRuntime.GameConfigTitleId)
            {
                Debug.LogFormat(">>> found non-matching title {0} for xuid: {1}", titleId, activity.OwnerXuid.ToString());
                continue;
            }

            // check the visibility
            if (activity.Visibility != XblMultiplayerSessionVisibility.Open)
            {
                Debug.LogFormat(">>> found invisible activity for xuid: {0}", activity.OwnerXuid.ToString());
                continue;
            }

            // check the join restriction
            if (activity.JoinRestriction != XblMultiplayerSessionRestriction.Followed &&
                activity.JoinRestriction != XblMultiplayerSessionRestriction.None)
            {
                Debug.LogFormat(">>> found restricted activity for xuid: {0}", activity.OwnerXuid.ToString());
                continue;
            }

            // check the closed flag
            if (activity.Closed)
            {
                Debug.LogFormat(">>> found closed activity for xuid: {0}", activity.OwnerXuid.ToString());
                continue;
            }

            // check the member count against the max member count
            if (activity.MembersCount == activity.MaxMembersCount)
            {
                Debug.LogFormat(">>> found maxed out activity for xuid: {0}", activity.OwnerXuid.ToString());
                continue;
            }

            Debug.LogFormat(">>> FOUND JOINABLE ACTIVITY for xuid: {0}", activity.OwnerXuid.ToString());
            _onlineInTitleFriends[activity.OwnerXuid].CurrentActivityDetails = activity;
        }

        OnSocialFriendActivitiesObtained?.Invoke();
    }

    private IEnumerator DownloadProfilePicCoroutine(
        string contextData,
        System.Action<string, byte[]> completionCallback)
    {
        const float FriendLoadTimeoutInS = 10F;

        SocialFriend[] friends;

        // wait for the friend data to load, up to a time limit
        var timeLeft = FriendLoadTimeoutInS;
        do
        {
            GetOnlineInTitleFriends(out friends);
            timeLeft -= Time.deltaTime;
            yield return null;
        }
        while (friends == null && timeLeft > 0F);

        if (timeLeft <= 0F && friends == null)
        {
            OnSocialError?.Invoke("Loading social friends timed out.", 0);
            yield break;
        }

        // get the friend whose name corresponds with the given context name
        var friendWithGamertag = friends.First(friend => { return friend.GamerTag == contextData; });
        if (friendWithGamertag != null)
        {
            var webRequest = UnityWebRequest.Get(friendWithGamertag.DisplayPicUrl);
            yield return webRequest.SendWebRequest();

            completionCallback?.Invoke(contextData, webRequest.downloadHandler.data);
        }
        else
        {
            OnSocialError?.Invoke($"Friend with gamertag '{contextData}' not found.", 0);
        }

        yield break;
    }

    private void UpdateFriendDisplayNames(XblSocialManagerUserGroupHandle groupHandle)
    {
        Debug.LogFormat("XboxLiveLogic.UpdateFriendDisplayNames()");


        var hr = SDK.XBL.XblSocialManagerUserGroupGetUsers(
            groupHandle,
            out XblSocialManagerUser[] users);

        if (HR.FAILED(hr))
        {
            OnSocialError?.Invoke("XblSocialManagerUserGroupGetUsers() failed", hr);
            return;
        }
        else
        {
            Debug.LogFormat("... a total of {0} users found.", users.Length);
        }

        _onlineInTitleFriends.Clear();

        foreach (var user in users)
        {
            AddOnlineInTitleFriend(user);
        }
    }

    private void AddOnlineInTitleFriend(XblSocialManagerUser user)
    {
        _onlineInTitleFriends[user.XboxUserId] = new SocialFriend()
        {
            XUID = user.XboxUserId,
            GamerTag = user.Gamertag,
            DisplayName = user.DisplayName,
            DisplayPicUrl = user.DisplayPicUrlRaw
        };
    }
}
