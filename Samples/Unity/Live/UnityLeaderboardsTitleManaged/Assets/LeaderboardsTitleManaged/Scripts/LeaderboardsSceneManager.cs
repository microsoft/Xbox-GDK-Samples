using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using Unity.XGamingRuntime;
using XBL = Unity.XGamingRuntime.SDK.XBL;

namespace GdkSample_LeaderboardsTM
{
    public sealed class LeaderboardsSceneManager : MonoBehaviour
    {
        [SerializeField] GameObject MainMenu;
        [SerializeField] Button FirstSelectedButton;
        [SerializeField] Button ExitButton;
        [SerializeField] Button QueryGlobalLeaderboardButton;
        [SerializeField] Button QuerySocialLeaderboardButton;
        [SerializeField] Button QueryNumberStatisticButton;
        [SerializeField] Button QueryStringStatisticButton;
        [SerializeField] Button UpdateStatisticsButton;

        public static string NumberLeaderboard { get { return C_LEADERBOARD_NAME_NUMBER; } }
        public static string NumberStatistic { get { return C_STATISTIC_NAME_NUMBER; } }
        public static string StringStatistic { get { return C_STATISTIC_NAME_STRING; } }

        // configured leaderboard name from dashboard
        private const string C_LEADERBOARD_NAME_NUMBER = "ANumberLeaderboard";

        // configured statistic names from dashboard
        private const string C_STATISTIC_NAME_NUMBER = "ANumberStat";
        private const string C_STATISTIC_NAME_STRING = "AStringStat";

        private List<Button> _buttonList;

        public static LeaderboardsSceneManager Instance { get; private set; }

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
            XboxManager.Instance.UserSignedIn += OnSignInDetected;
            XboxManager.Instance.UserSignedOut += OnUserChangeDetected;
            XboxManager.Instance.UserSignInStarted += OnUserChangeDetected;
            XboxManager.Instance.UserSignOutStarted += OnUserChangeDetected;

            // Button handlers for onclick events
            QueryGlobalLeaderboardButton.onClick.AddListener(() => QueryLeaderboard(NumberStatistic, true));
            QuerySocialLeaderboardButton.onClick.AddListener(() => QueryLeaderboard(NumberStatistic, false));
            QueryNumberStatisticButton.onClick.AddListener(() => QueryUserStatistic(NumberStatistic));
            QueryStringStatisticButton.onClick.AddListener(() => QueryUserStatistic(StringStatistic));
            UpdateStatisticsButton.onClick.AddListener(UpdateUserStatistics);

            _buttonList = new List<Button>(MainMenu.GetComponentsInChildren<Button>());
            DisableButtons();

            // Initialize GDK runtime and Xbox Live
            if (GDKGameRuntime.TryInitialize())
            {
                XboxManager.Instance.InitializeAndAddUser();
            }
        }

        public void QueryLeaderboard(string statisticName, bool isGlobal)
        {
            Logger.Instance.Log($"Calling QueryLeaderboards()", LogColor.Event);

            const string c_continuation_token = "";
            const int c_max_returned_items = 10;
            const int c_skip_to_rank = 0;
            // [Title-managed Leaderboards]
            // Setting skipToXboxUserId as a non-zero value will compare the given user's stat (accessible via XblUserStatisticsGet*) against the global leaderboard.
            // Be aware that a player's stat and their score on the global leaderboard may not be the same. More info in the readme's Implementation Notes section.
            const int c_skip_to_user = 0;

            string[] leaderboardColumns = { };

            XblLeaderboardQueryType leaderboardQueryType;
            // [Title-managed Leaderboards]
            // Social queries must include a valid XUID
            // If the user has no friends or favorited people who have played this sample, only this user's results will be returned
            // Setting socialGroup as People or Favorites will compare the stats of the associated users to the player's stat. It will not use values of the global leaderboard.
            XblSocialGroupType socialGroupType;
            ulong targetXuid;

            // set required query values based on global or friends leaderboards
            if (isGlobal)
            {
                leaderboardQueryType = XblLeaderboardQueryType.TitleManagedStatBackedGlobal;
                socialGroupType = XblSocialGroupType.None;
                targetXuid = 0;
            }
            else
            {
                // social queries require a valid xuid
                // if the user has no friends or favorited people who played this sample, only this user's results will return
                leaderboardQueryType = XblLeaderboardQueryType.TitleManagedStatBackedSocial;
                socialGroupType = XblSocialGroupType.People;
                targetXuid = XboxManager.Instance.UserId;
            }

            // creates an Xbox Services leaderboard query
            int hr = XblLeaderboardQuery.Create(
                targetXuid,                                  // Xbox Services user id to query - set to 0 if global
                GDKGameRuntime.GameConfigScid,               // Xbox Services configuration id
                NumberLeaderboard,                           // leaderboard name to look up
                statisticName,                               // statistic name to look up
                socialGroupType,                             // type of social group to grab
                leaderboardColumns,                          // additional leaderboard columns to find
                XblLeaderboardSortOrder.Descending,          // sort ordering - ascending/descending
                c_max_returned_items,                        // max leaderboard items to return
                c_skip_to_user,                              // Xbox Services user id to skip to
                c_skip_to_rank,                              // skips to a specific rank
                c_continuation_token,                        // token used to get the next set of leaderboard data
                leaderboardQueryType,                        // type of leaderboard query - global/friend/self
                out XblLeaderboardQuery leaderboardQuery);   // created leaderboard query

            if (HR.FAILED(hr))
            {
                Debug.LogError($"{nameof(XblLeaderboardQuery)}.{nameof(XblLeaderboardQuery.Create)} failed - 0x{hr:X8}.");
                return;
            }

            // retrieves an Xbox Services leaderboard using the leaderboard query
            XBL.XblLeaderboardGetLeaderboardAsync(
                XboxManager.Instance.ContextHandle,  // Xbox Services context handle
                leaderboardQuery,                    // the created leaderboard query
                (int hr, XblLeaderboardResult result) =>
                {
                    Debug.Log($"{nameof(XBL)}.{nameof(XBL.XblLeaderboardGetLeaderboardAsync)}.Callback()");

                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblLeaderboardGetLeaderboardAsync)} failed - 0x{hr:X8}.");
                        return;
                    }

                    Logger.Instance.Log($"Successfully retrieved leaderboards for '{statisticName}'.", LogColor.Success);

                    // print the leaderboard to the Xbox Console
                    const int c_rank_width = -6;
                    const int c_gamertag_width = -20;
                    const int c_statistics_name_width = -24;

                    string output = $"Posting retrieved leaderboard...\n\n";
                    output += $"        {"RANK",c_rank_width}|  {"GAMER TAG",c_gamertag_width}|  {statisticName,c_statistics_name_width}\n";
                    output += $"        ---------------------------------------------------------------\n";

                    for (int i = 0; i < result.Rows.Length; i++)
                    {
                        XblLeaderboardRow row = result.Rows[i];

                        output += $"        {row.Rank,c_rank_width}|  {row.Gamertag,c_gamertag_width}|  ";

                        string[] columns = row.ColumnValues;
                        for (int j = 0; j < columns.Length; j++)
                        {
                            output += $"{columns[j],c_statistics_name_width}";
                        }
                        output += $"\n";
                    }
                    output += $"\n";
                    Debug.Log(output);
                });
        }

        // [Title-managed Leaderboards]
        // Stat data can be strings or numbers, but only numbers can be used as leaderboards
        // If the sent score is lower than what is already there, the stat will update, but NOT the corresponding entry in global leaderboard, as it would remain the highest recorded value.
        public void QueryUserStatistic(string statisticName)
        {
            Logger.Instance.Log($"Calling QueryUserStatistics()", LogColor.Event);

            Debug.Log($"Retrieving user statistic for '{statisticName}'.");

            // Requests a single user statistic
            XBL.XblUserStatisticsGetSingleUserStatisticAsync(
                XboxManager.Instance.ContextHandle,     // Xbox Services context handle
                XboxManager.Instance.UserId,            // Xbox user id
                GDKGameRuntime.GameConfigScid,          // Xbox Services configuration id
                statisticName,                          // name of the statistic to retrieve
                (int hr, XblUserStatisticsResult result) =>
                {
                    Debug.Log($"{nameof(XBL)}.{nameof(XBL.XblUserStatisticsGetSingleUserStatisticAsync)}.Callback()");

                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblUserStatisticsGetSingleUserStatisticAsync)} failed - 0x{hr:X8}.");
                        return;
                    }

                    Logger.Instance.Log($"Successfully retrieved user statistics for '{statisticName}'.", LogColor.Success);

                    // print each statistic to the Xbox Console - this should only be one
                    string output = "Posting retrieved statistics...\n";
                    foreach (XblServiceConfigurationStatistic serviceConfigStatistic in result.ServiceConfigStatistics)
                    {
                        foreach (XblStatistic statistic in serviceConfigStatistic.Statistics)
                        {
                            output += $"\n";
                            output += $"        {nameof(statistic.StatisticName)}: {statistic.StatisticName}\n";
                            output += $"        {nameof(statistic.StatisticType)}: {statistic.StatisticType}\n";
                            output += $"        {nameof(statistic.Value)}: {statistic.Value}\n";
                        }
                    }
                    output += $"\n";
                    Debug.Log(output);
                });
        }

        public void UpdateUserStatistics()
        {
            Logger.Instance.Log($"Calling UpdateUserStatistics()", LogColor.Event);

            // create a random number statistic from 0 to 100
            int randomValue = Random.Range(0, 101);
            Debug.Log($"Updating '{NumberStatistic}' to '{randomValue}'.");

            // create string statistic from a random range of strings
            string[] randomStrings = { "Xbox", "Xbox Series S", "Xbox Series X", "Xbox 360", "Microsoft" };
            int randomIndex = Random.Range(0, randomStrings.Length);
            string randomString = randomStrings[randomIndex];
            Debug.Log($"Updating '{StringStatistic}' to '{randomString}'.");

            XblTitleManagedStatistic[] statistics =
            {
                new XblTitleManagedStatistic(NumberStatistic, randomValue),   // create a number statistic using a random value
                new XblTitleManagedStatistic(StringStatistic, randomString),  // create a string statistic using a random string
            };

            // Update user's leaderboard statistics
            XBL.XblTitleManagedStatsWriteAsync(
                XboxManager.Instance.ContextHandle, // Xbox Services context handle
                XboxManager.Instance.UserId,        // Xbox user id
                statistics,                         // name of the statistic to retrieve
                (int hr) =>
                {
                    Debug.Log($"{nameof(XBL)}.{nameof(XBL.XblTitleManagedStatsWriteAsync)}.Callback()");

                    if (HR.FAILED(hr))
                    {
                        Debug.LogError($"{nameof(XBL)}.{nameof(XBL.XblTitleManagedStatsWriteAsync)} failed - 0x{hr:X8}.");
                        return;
                    }

                    Logger.Instance.Log($"Successfully updated user statistics.", LogColor.Success);

                    // print updated statistics to the Xbox Console
                    string output = $"Posting updated statistics...\n";
                    foreach (XblTitleManagedStatistic statistic in statistics)
                    {
                        output += $"\n";
                        output += $"        {nameof(statistic.StatisticName)}: {statistic.StatisticName}\n";
                        output += $"        {nameof(statistic.StatisticType)}: {statistic.StatisticType}\n";

                        if (statistic.StatisticType == XblTitleManagedStatType.Number)
                        {
                            output += $"        {nameof(statistic.NumberValue)}: {statistic.NumberValue}\n";
                        }
                        else
                        {
                            output += $"        {nameof(statistic.StringValue)}: {statistic.StringValue}\n";
                        }
                    }
                    output += $"\n";
                    Debug.Log(output);
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
