using System;
using System.Collections.Generic;
using UnityEngine;
using System.Collections;
using UnityEngine.Networking;
using Unity.XGamingRuntime;

using XBL = Unity.XGamingRuntime.SDK.XBL;

namespace GdkSample_SocialManager
{
    // Logic for usage of the Xbox Services' Social Manager API.
    public sealed class XboxSocialManager : MonoBehaviour
    {
        public static XboxSocialManager Instance { get; private set; }

        public SocialGroupType CurrentSocialGroup { get; private set; }
        private Dictionary<SocialGroupType, XblSocialManagerUserGroupHandle> m_socialManagerGroups;


        // predefined combination of XblPresenceFilter & XblRelationshipFilter
        public enum SocialGroupType
        {
            AllFavorites,
            AllFriends,
            AllOnlineFriends,
            TitleOnlineFriends
        }

        private Dictionary<ulong, byte[]> m_gamerPicCache = new();

        public IEnumerator GetGamerpic(XblSocialManagerUser user, System.Action<string, byte[]> completionCallback)
        {
            byte[] gamerPicData = null;
            if (!m_gamerPicCache.TryGetValue(user.XboxUserId, out gamerPicData))
            {
                var webRequest = UnityWebRequest.Get(user.DisplayPicUrlRaw);
                yield return webRequest.SendWebRequest();

                if (webRequest.result == UnityWebRequest.Result.ConnectionError)
                {
                    Debug.LogError($"Error while sending: {webRequest.error}");
                }
                else
                {
                    gamerPicData = webRequest.downloadHandler.data;

                    // Cache the image data
                    m_gamerPicCache[user.XboxUserId] = gamerPicData;
                }
            }
            if (gamerPicData != null)
                completionCallback?.Invoke(null, gamerPicData);
            else
                Debug.LogError($"Failed to obtain gamerpic data for {user.Gamertag}");
        }

        public event Action<XblSocialManagerUser[]> UserListRefreshed;
        public void Refresh()
        {
            int hr;

            // Retrieve the users in the current social group from the Xbox Services Social Manager
            hr = XBL.XblSocialManagerUserGroupGetUsers(
                m_socialManagerGroups[CurrentSocialGroup],  // current social group mapping
                out XblSocialManagerUser[] users);            // retrieved users from Xbox Services Social Manager
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblSocialManagerUserGroupGetUsers)} failed - 0x{hr:X8}.");
                return;
            }

            Debug.Log($"Successfully refreshed {nameof(XboxSocialManager)} user list.");
            UserListRefreshed?.Invoke(users);
        }

        public void UpdateSocialGroup(SocialGroupType socialGroup)
        {
            if (CurrentSocialGroup != socialGroup)
            {
                Debug.Log($"Updating {nameof(XboxSocialManager)} social group to '{Enum.GetName(typeof(SocialGroupType), socialGroup)}'.");
                CurrentSocialGroup = socialGroup;
                Refresh();
            }
            else
            {
                Debug.LogWarning($"{nameof(XboxSocialManager)} social group is already set to '{Enum.GetName(typeof(SocialGroupType), socialGroup)}'.");
            }
        }

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }
            Instance = this;
        }

        private void Start()
        {
            CurrentSocialGroup = SocialGroupType.AllFriends;
            m_socialManagerGroups = new Dictionary<SocialGroupType, XblSocialManagerUserGroupHandle>();

            XboxManager.Instance.UserSignedIn += AddLocalUser;
            XboxManager.Instance.UserSignOutStarted += RemoveLocalUser;

            Instance.StartCoroutine(UpdateSocialManager());
        }

        /// <summary>
        /// This executes all Social Manager Asynchronous block work natively
        /// </summary>
        private IEnumerator UpdateSocialManager()
        {
            while (true)
            {
                int hr;

                // Retrieve social events from Xbox Services
                hr = XBL.XblSocialManagerDoWork(out XblSocialManagerEvent[] socialEvents);
                if (HR.FAILED(hr))
                {
                    Debug.Log($"{nameof(XBL)}.{nameof(XBL.XblSocialManagerDoWork)} failed - 0x{hr:X8}.");
                    yield return null;
                }

                if (socialEvents != null)
                {
                    // Process each social event retrieved
                    foreach (XblSocialManagerEvent socialEvent in socialEvents)
                    {
                        if (socialEvent == null)
                        {
                            continue;
                        }

                        if (HR.FAILED(socialEvent.Hr))
                        {
                            Debug.Log($"{nameof(XblSocialManagerEvent)} failed - 0x{hr:X8}.");
                            continue;
                        }

                        Debug.Log($"{nameof(XblSocialManagerEvent)} recorded: {Enum.GetName(typeof(XblSocialManagerEventType), socialEvent.EventType)}.");
                        switch (socialEvent.EventType)
                        {
                            case XblSocialManagerEventType.UsersAddedToSocialGraph:
                                Refresh();
                                break;
                            case XblSocialManagerEventType.UsersRemovedFromSocialGraph:
                                Refresh();
                                break;
                            case XblSocialManagerEventType.PresenceChanged:
                                Refresh();
                                break;
                            case XblSocialManagerEventType.ProfilesChanged:
                                break;
                            case XblSocialManagerEventType.SocialRelationshipsChanged:
                                break;
                            case XblSocialManagerEventType.LocalUserAdded:
                                break;
                            case XblSocialManagerEventType.SocialUserGroupLoaded:
                                Refresh();
                                break;
                            case XblSocialManagerEventType.SocialUserGroupUpdated:
                                Refresh();
                                break;
                            case XblSocialManagerEventType.UnknownEvent:
                                break;
                            default:
                                break;
                        }
                    }
                }
                yield return null;
            }
        }

        private void AddLocalUser(XUserHandle userHandle)
        {
            Debug.Log($"{nameof(XboxSocialManager)}.{nameof(XboxSocialManager.AddLocalUser)}()");

            int hr;

            hr = SDK.XUserGetId(userHandle, out ulong xuid);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(SDK.XUserGetId)} failed - 0x{hr:X8}.");
            }

            // Check if the user is already initialized in Social Manager
            hr = XBL.XblSocialManagerGetLocalUsers(out XUserHandle[] currentUserHandles);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblSocialManagerGetLocalUsers)} failed - 0x{hr:X8}.");
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
                hr = XBL.XblSocialManagerAddLocalUser(userHandle, XblSocialManagerExtraDetailLevel.NoExtraDetail);
                if (HR.FAILED(hr))
                {
                    Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblSocialManagerAddLocalUser)} failed - 0x{hr:X8}.");
                    return;
                }
            }

            // create four unique groups for social updates
            CreateSocialGroupFromFilters(userHandle, SocialGroupType.AllFriends);
            CreateSocialGroupFromFilters(userHandle, SocialGroupType.AllFavorites);
            CreateSocialGroupFromFilters(userHandle, SocialGroupType.AllOnlineFriends);
            CreateSocialGroupFromFilters(userHandle, SocialGroupType.TitleOnlineFriends);

            // Sets whether to enable social manager to poll every 30 seconds from the presence service.
            // Removes the need to manually refresh for presence changes.
            hr = XBL.XblSocialManagerSetRichPresencePollingStatus(userHandle, true);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblSocialManagerSetRichPresencePollingStatus)} failed - 0x{hr:X8}.");
            }

            Refresh();
        }

        private void RemoveLocalUser(XUserHandle userHandle)
        {
            Debug.Log($"{nameof(XboxSocialManager)}.{nameof(XboxSocialManager.RemoveLocalUser)}()");

            int hr;

            hr = SDK.XUserGetId(userHandle, out ulong xuid);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(SDK)}.{nameof(SDK.XUserGetId)} failed - 0x{hr:X8}.");
            }

            Debug.Log($"Removing user {xuid} from SocialManager\n");

            // destroy each Xbox Services Social Manager group created for the user
            foreach (XblSocialManagerUserGroupHandle group in m_socialManagerGroups.Values)
            {
                hr = XBL.XblSocialManagerDestroySocialUserGroup(group);
                if (HR.FAILED(hr))
                {
                    Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblSocialManagerDestroySocialUserGroup)} failed - 0x{hr:X8}.");
                }
            }

            m_socialManagerGroups.Clear();

            // Remove the user from Xbox Services social manager
            hr = XBL.XblSocialManagerRemoveLocalUser(userHandle, XblSocialManagerExtraDetailLevel.NoExtraDetail);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblSocialManagerRemoveLocalUser)} failed - 0x{hr:X8}.");
            }
        }

        private void CreateSocialGroupFromFilters(XUserHandle userHandle, SocialGroupType socialGroup)
        {
            Debug.Log($"{nameof(XboxSocialManager)}.{nameof(XboxSocialManager.CreateSocialGroupFromFilters)}()");

            XblPresenceFilter presenceFilter;
            XblRelationshipFilter relationshipFilter;

            // assign presence & relationship filters based on Social Group mapping
            switch (socialGroup)
            {
                case SocialGroupType.AllFavorites:
                    presenceFilter = XblPresenceFilter.All;
                    relationshipFilter = XblRelationshipFilter.Favorite;
                    break;
                case SocialGroupType.AllFriends:
                    presenceFilter = XblPresenceFilter.All;
                    relationshipFilter = XblRelationshipFilter.Friends;
                    break;
                case SocialGroupType.AllOnlineFriends:
                    presenceFilter = XblPresenceFilter.AllOnline;
                    relationshipFilter = XblRelationshipFilter.Friends;
                    break;
                case SocialGroupType.TitleOnlineFriends:
                    presenceFilter = XblPresenceFilter.TitleOnline;
                    relationshipFilter = XblRelationshipFilter.Friends;
                    break;
                default:
                    presenceFilter = XblPresenceFilter.All;
                    relationshipFilter = XblRelationshipFilter.Favorite;
                    break;
            }

            Debug.Log($"Creating {nameof(SocialGroupType)} of '{Enum.GetName(typeof(SocialGroupType), socialGroup)}'.");

            int hr;

            // Create the Xbox Services social group for the user
            hr = XBL.XblSocialManagerCreateSocialUserGroupFromFilters(
                userHandle,         // Xbox Services user handle
                presenceFilter,     // filter for presence
                relationshipFilter, // filter for relationship to the user
                out XblSocialManagerUserGroupHandle group);
            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblSocialManagerCreateSocialUserGroupFromFilters)} failed - 0x{hr:X8}.");
                return;
            }

            Debug.Log($"Successfully created {nameof(SocialGroupType)} of '{Enum.GetName(typeof(SocialGroupType), socialGroup)}'.");

            m_socialManagerGroups[socialGroup] = group;
        }
    }
}
