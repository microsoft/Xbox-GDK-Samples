using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Unity.XGamingRuntime;
using XBL = Unity.XGamingRuntime.SDK.XBL;

namespace GdkSample_Achievements
{
    public sealed class AchievementsSceneManager : MonoBehaviour
    {
        [SerializeField] GameObject MainMenu;
        [SerializeField] Button FirstSelectedButton;
        [SerializeField] Button ExitButton;
        [SerializeField] Button GetAllAchievementsButton;
        [SerializeField] Button GetAchievement1Button;
        [SerializeField] Button GetAchievement2Button;
        [SerializeField] Button UpdateAchievement1_25Button;
        [SerializeField] Button UpdateAchievement1_100Button;
        [SerializeField] Button UpdateAchievement2_25Button;
        [SerializeField] Button UpdateAchievement2_100Button;

        private List<Button> _buttonList;

        public static AchievementsSceneManager Instance { get; private set; }

        private void Awake()
        {
            if (Instance != null)
            {
                Destroy(this);
                return;
            }

            Instance = this;
        }

        // Start is called before the first frame update.
        void Start()
        {
            // event handlers for user sign in/out
            XboxManager.Instance.UserSignedIn += OnSignInDetected;
            XboxManager.Instance.UserSignedOut += OnUserChangeDetected;
            XboxManager.Instance.UserSignInStarted += OnUserChangeDetected;
            XboxManager.Instance.UserSignOutStarted += OnUserChangeDetected;

            // button handlers for onlick events
            GetAllAchievementsButton.onClick.AddListener(GetAllAchievements);
            GetAchievement1Button.onClick.AddListener(() => GetAchievement("1"));
            GetAchievement2Button.onClick.AddListener(() => GetAchievement("2"));
            UpdateAchievement1_25Button.onClick.AddListener(() => UpdateAchievement("1", 25));
            UpdateAchievement1_100Button.onClick.AddListener(() => UpdateAchievement("1", 100));
            UpdateAchievement2_25Button.onClick.AddListener(() => UpdateAchievement("2", 25));
            UpdateAchievement2_100Button.onClick.AddListener(() => UpdateAchievement("2", 100));

            _buttonList = new List<Button>(MainMenu.GetComponentsInChildren<Button>());
            DisableButtons();

            // Initialize GDK runtime and Xbox Live
            if (GDKGameRuntime.TryInitialize())
            {
                XboxManager.Instance.InitializeAndAddUser();
            }
        }

        public void GetAchievement(string achievementId)
        {
            Debug.Log($"Retrieving achievement status for '{achievementId}'.");

            // Requests an achievement from Xbox Services
            XBL.XblAchievementsGetAchievementAsync(
                XboxManager.Instance.ContextHandle,     // Xbox Services context handle
                XboxManager.Instance.UserId,            // Xbox user id
                GDKGameRuntime.GameConfigScid,          // Xbox Services configuration id
                achievementId,                          // id of achievement to retrieve
                (int hr, XblAchievementsResultHandle result) =>
                {
                    Debug.Log($"{nameof(XBL)}.{nameof(XBL.XblAchievementsGetAchievementAsync)}.Callback()");

                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(XBL.XblAchievementsGetAchievementAsync)} failed - 0x{hr:X8}.");
                        return;
                    }

                    Logger.Instance.Log($"Successfully retrieved achievement status for achievement '{achievementId}'.", LogColor.Success);

                    // Retrieves the array of fetched Xbox Services achievements
                    hr = XBL.XblAchievementsResultGetAchievements(result, out XblAchievement[] achievements);
                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(XBL.XblAchievementsResultGetAchievements)} failed - 0x{hr:X8}.");
                        return;
                    }

                    // print each achievement to the Xbox console - this should only be one
                    foreach (XblAchievement achievement in achievements)
                    {
                        Debug.Log($"Posting found achievement...\n\n" +
                            $"        {nameof(achievement.Id)}: {achievement.Id}\n" +
                            $"        {nameof(achievement.Name)}: {achievement.Name}\n" +
                            $"        {nameof(achievement.Type)}: {achievement.Type}\n" +
                            $"        {nameof(achievement.ProgressState)}: {achievement.ProgressState}\n" +
                            $"        Percent: {(achievement.ProgressState != XblAchievementProgressState.Achieved ? $"{achievement.Progression.Requirements[0].CurrentProgressValue}%" : "100%")}\n" +
                            $"        {nameof(achievement.IsSecret)}: {achievement.IsSecret}\n" +
                            $"        {nameof(achievement.LockedDescription)}: {achievement.LockedDescription}\n" +
                            $"        {nameof(achievement.UnlockedDescription)}: {achievement.UnlockedDescription}\n");
                    }
                });
        }

        public void GetAllAchievements()
        {
            Logger.Instance.Log($"Retrieving all achievements for Title Id '0x{GDKGameRuntime.GameConfigTitleId:X8}'.", LogColor.Event);

            const bool c_unlocked_only = false;
            const int c_skipped_achievements = 0;
            const int c_number_of_achievements = int.MaxValue;

            int hr = SDK.XGameGetXboxTitleId(out uint titleId);
            if (hr != 0)
            {
                Debug.LogError($"Failed to get title id. Error: 0x{hr:X8}");
                return;
            }

            // Requests a range of achievements from Xbox Services
            XBL.XblAchievementsGetAchievementsForTitleIdAsync(
                XboxManager.Instance.ContextHandle, // Xbox Services context handle
                XboxManager.Instance.UserId,        // Xbox user id
                titleId,                            // Xbox title id converted to HEX
                XblAchievementType.All,             // Xbox Services achievement type
                c_unlocked_only,                    // select unlocked
                XblAchievementOrderBy.DefaultOrder, // ordering of achievements
                c_skipped_achievements,             // number of achievements to skip (used for initial index)
                c_number_of_achievements,           // number of achievements to return from starting index
                (int hr, XblAchievementsResultHandle result) =>
                {
                    Debug.Log($"{nameof(XBL)}.{nameof(XBL.XblAchievementsGetAchievementsForTitleIdAsync)}.Callback()");

                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(XBL.XblAchievementsGetAchievementsForTitleIdAsync)} failed - 0x{hr:X8}.");
                        return;
                    }

                    Debug.Log($"Successfully retrieved achievements for Title Id '0x{GDKGameRuntime.GameConfigTitleId:X8}'.");

                    // Retrieves the array of fetched Xbox Services achievements
                    hr = XBL.XblAchievementsResultGetAchievements(result, out XblAchievement[] achievements);
                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(XBL.XblAchievementsResultGetAchievements)} failed - 0x{hr:X8}.");
                        return;
                    }

                    // print each achievement to the Xbox console
                    Logger.Instance.Log($"Total Achievements Found: {achievements.Length}.", LogColor.Success);
                    for (int i = 0; i < achievements.Length; i++)
                    {
                        XblAchievement achievement = achievements[i];

                        Debug.Log($"Posting found achievement...\n\n" +
                                                 $"        {nameof(achievement.Id)}: {achievement.Id}\n" +
                                                 $"        {nameof(achievement.Name)}: {achievement.Name}\n" +
                                                 $"        {nameof(achievement.Type)}: {achievement.Type}\n" +
                                                 $"        {nameof(achievement.ProgressState)}: {achievement.ProgressState}\n" +
                                                 $"        Percent: {(achievement.ProgressState != XblAchievementProgressState.Achieved ? $"{achievement.Progression.Requirements[0].CurrentProgressValue}%" : "100%")}\n" +
                                                 $"        {nameof(achievement.IsSecret)}: {achievement.IsSecret}\n" +
                                                 $"        {nameof(achievement.LockedDescription)}: {achievement.LockedDescription}\n" +
                                                 $"        {nameof(achievement.UnlockedDescription)}: {achievement.UnlockedDescription}\n");
                    }
                });
        }

        public void UpdateAchievement(string achievementId, uint percentComplete)
        {
            Logger.Instance.Log($"Updating achievement status of achievement '{achievementId}' to {percentComplete}%.", LogColor.Event);

            // Updates an achievement's completion percentage
            SDK.XBL.XblAchievementsUpdateAchievementAsync(
                XboxManager.Instance.ContextHandle,  // Xbox Services context handle
                XboxManager.Instance.UserId,         // Xbox user id
                achievementId,                       // id of achievement to update
                percentComplete,                     // percent to set the achievement to
                (int hr) =>
                {
                    Debug.Log($"{nameof(XBL)}.{nameof(XBL.XblAchievementsUpdateAchievementAsync)}.Callback()");

                    if (HR.FAILED(hr))
                    {
                        if (hr == -2145844944) // HTTP_ERROR_STATUS_NOT_MODIFIED
                        {
                            Debug.LogWarning($"Values cannot be modified to be less than or equal to the current achievement value. Achievement '{achievementId}' has not been updated.");
                        }
                        else
                        {
                            Debug.LogError($"{nameof(XBL.XblAchievementsUpdateAchievementAsync)} failed - 0x{hr:X8}.");
                        }
                        return;
                    }

                    Logger.Instance.Log($"Successfully updated achievement status for achievement '{achievementId}' to {percentComplete}%.", LogColor.Success);
                });
        }

        private void OnSignInDetected()
        {
            EnableButtons();
            EventSystem.current.SetSelectedGameObject(FirstSelectedButton.gameObject);
        }

        private void OnUserChangeDetected()
        {
            DisableButtons();
        }

        private void EnableButtons()
        {
            foreach(Button button in _buttonList)
            {
                button.interactable = true;
            }   
        }

        private void DisableButtons()
        {
            foreach(Button button in _buttonList)
            {
                button.interactable = false;
            }

            // Exit button should always be enabled
            ExitButton.interactable = true;
        }

    }
}
