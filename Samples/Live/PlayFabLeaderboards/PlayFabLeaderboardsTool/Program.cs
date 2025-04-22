using System;
using System.Diagnostics;
using System.Numerics;
using System.Reflection.Metadata.Ecma335;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using System.Xml.Linq;
using PlayFab;
using PlayFab.AuthenticationModels;
using PlayFab.ClientModels;
using PlayFab.ProfilesModels;
using PlayFab.ProgressionModels;
using static System.Formats.Asn1.AsnWriter;
using static System.Runtime.InteropServices.JavaScript.JSType;

public static class Program
{
    //Note: This is merely a tool sample for creating and writing to the PlayFab leaderboard. Please make calls specifying the DeveloperSecretKey through the web service
    private const string DEVELOPER_SECRETKEY = ""; // Please set this value to your own DeveloperSecretKey from PlayFab Game Manager
    private const string PLAYFAB_TITLE_ID = ""; // Please set this value to your own titleId from PlayFab Game Manager

    private const int DEFAULT_SCORE = 1000;
    private const int DEFAULT_RANK = 100;
    private const string PLAYER_ENTITY_ID = "PlayerEntityId";
    private static readonly List<string> LEADERBOARDS_NAMES = new List<string> { "SampleLeaderboard1", "SampleLeaderboard2" };
    private const int ENTRY_NUMBER = 0;
    private const int LEADERBOARD_VERSION = 1;

    static void Main(string[] args)
    {
        string playerName = PLAYER_ENTITY_ID;
        List<string> leaderboardsNames = LEADERBOARDS_NAMES;
        List<string> entityIdsToDelete = new List<string>();
        int[] scores = new int[1000];
        int entryNumber = ENTRY_NUMBER;
        int leaderboardVersion = LEADERBOARD_VERSION;
        scores[0] = DEFAULT_SCORE;
        scores[1] = DEFAULT_RANK;

        if (args.Length == 0)
        {
            ShowHelp();
            return;
        }
        for (int i = 0; i < args.Length; i++)
        {
            switch (args[i])
            {
                case "/h":
                case "-h":
                    ShowHelp();
                    return;
                case "/playername":
                case "--playername":
                    if (i + 1 < args.Length)
                    {
                        playerName = args[i + 1];
                        i++;
                        entryNumber = i;
                    }
                    else
                    {
                        Console.WriteLine("Error: /playername requires a value.");
                        return;
                    }
                    break;
                case "/score":
                case "-score":
                    if (i + 1 < args.Length && int.TryParse(args[i + 1], out int score))
                    {
                        for (int j = 0; j < scores.Length; j++)
                        {
                            scores[0] = score;
                        }
                        i++;
                    }
                    else
                    {
                        Console.WriteLine("Error: /score requires a numeric value.");
                        return;
                    }
                    break;
                case "/rank":
                case "-rank":
                    if (i + 1 < args.Length && int.TryParse(args[i + 1], out int score2))
                    {
                        scores[1] = score2;
                        i++;
                    }
                    else
                    {
                        Console.WriteLine("Error: /score2 requires a numeric value.");
                        return;
                    }
                    break;
                case "/number":
                case "-number":
                    if (i + 1 < args.Length && int.TryParse(args[i + 1], out entryNumber))
                    {
                        i++;
                    }
                    else
                    {
                        Console.WriteLine("Error: /number requires a numeric value.");
                        return;
                    }
                    break;
                case "/version":
                case "-version":
                    if (i + 1 < args.Length && int.TryParse(args[i + 1], out leaderboardVersion))
                    {
                        i++;
                    }
                    else
                    {
                        Console.WriteLine("Error: /version requires a numeric value.");
                        return;
                    }
                    break;
                default:
                    Console.WriteLine($"Unknown argument: {args[i]}");
                    return;
            }
        }
        PlayFabSettings.staticSettings.TitleId = PLAYFAB_TITLE_ID;
        PlayFabSettings.staticSettings.DeveloperSecretKey = DEVELOPER_SECRETKEY;

        var titleEntityTask = LoginAsTitleEntity();
        titleEntityTask.Wait();
        var titleEntityContext = titleEntityTask.Result;

        for (int i = 0; i < leaderboardsNames.Count; i++)
        {
            var createLeaderboardTask = CreateLeaderboardDefinitionAsync(titleEntityContext, leaderboardsNames[i], leaderboardVersion);
            createLeaderboardTask.Wait();
        }

        Random random = new Random();

        if (entryNumber < 2)
        {
            for (int i = 0; i < leaderboardsNames.Count; i++)
            {
                var updateLeaderboardTask = UpdateLeaderboardForPlayer(titleEntityContext, leaderboardsNames[i], playerName, scores[0], scores[1]);
                updateLeaderboardTask.Wait();
            }
            Console.Write($"Progress: 100%");
        }
        else {

            for (int i = 0; i < entryNumber; i++)
            {
                for (int j = 0; j < leaderboardsNames.Count; j++)
                {
                    int randomScore = random.Next(10, 10001);
                    int randomScore2 = random.Next(1, 1001);
                    var updateLeaderboardTask = UpdateLeaderboardForPlayer(titleEntityContext, leaderboardsNames[j], $"{playerName}{i + 1}", randomScore, randomScore2);
                    updateLeaderboardTask.Wait();
                }
                int progress = (i + 1) * 100 / entryNumber;
                Console.SetCursorPosition(0, Console.CursorTop);
                Console.Write($"Progress: {progress}%");
            }
        }
    }

    private static void OnLoginComplete(Task<PlayFabResult<LoginResult>> taskResult)
    {
        var apiError = taskResult.Result.Error;
        var apiResult = taskResult.Result.Result;

        if (apiError != null)
        {
            Console.ForegroundColor = ConsoleColor.Red; // Make the error more visible
            Console.WriteLine("Something went wrong with your first API call.  :(");
            Console.WriteLine("Here's some debug information:");
            Console.WriteLine(PlayFabUtil.GenerateErrorReport(apiError));
            Console.ForegroundColor = ConsoleColor.Gray; // Reset to normal
        }
        else if (apiResult != null)
        {
            Console.WriteLine("Congratulations, you made your first successful API call!");
        }
    }
    public static async Task CreateLeaderboardDefinitionAsync(PlayFabAuthenticationContext context, string leaderboardName, int version)
    {
        PlayFabProgressionInstanceAPI leaderboardsAPI = new PlayFabProgressionInstanceAPI(context);
        CreateLeaderboardDefinitionRequest leaderboardDefinitionRequest = new CreateLeaderboardDefinitionRequest()
        {
            AuthenticationContext = context,
            Name = leaderboardName,
            SizeLimit = 1000,
            EntityType = "title_player_account",
            VersionConfiguration = new VersionConfiguration()
            {
                MaxQueryableVersions = version,
                ResetInterval = ResetInterval.Manual,
            },
            Columns = new List<LeaderboardColumn>()
        {
            new LeaderboardColumn()
            {
                Name = "Score1",
                SortDirection = LeaderboardSortDirection.Descending,
            },
            new LeaderboardColumn()
            {
                Name = "Score2",
                SortDirection = LeaderboardSortDirection.Ascending,
            },
        }
        };

        PlayFabResult<PlayFab.ProgressionModels.EmptyResponse> createLbDefinitionResult = await leaderboardsAPI.CreateLeaderboardDefinitionAsync(leaderboardDefinitionRequest);
    }

    public static async Task DeleteLeaderboardEntries(PlayFabAuthenticationContext context, string leaderboardName, List<string> entityIds)
    {
        PlayFabProgressionInstanceAPI leaderboardsAPI = new PlayFabProgressionInstanceAPI(context);
        DeleteLeaderboardEntriesRequest leaderboardsDelReq = new DeleteLeaderboardEntriesRequest()
        {
            Name = leaderboardName,
            EntityIds = entityIds
        };

        PlayFabResult<PlayFab.ProgressionModels.EmptyResponse> delLeaderboardDefResult = await leaderboardsAPI.DeleteLeaderboardEntriesAsync(leaderboardsDelReq);
    }

    public static async Task<PlayFabAuthenticationContext> LoginAsTitleEntity()
    {
        GetEntityTokenRequest request = new GetEntityTokenRequest()
        {
            Entity = new PlayFab.AuthenticationModels.EntityKey()
            {
                Id = PlayFabSettings.staticSettings.TitleId,
                Type = "title",
            },
        };

        PlayFabResult<GetEntityTokenResponse> entityTokenResult = await PlayFabAuthenticationAPI.GetEntityTokenAsync(request);

        PlayFabAuthenticationContext authContext = new PlayFabAuthenticationContext
        {
            EntityToken = entityTokenResult.Result.EntityToken
        };

        return authContext;
    }

    public static async Task UpdateEntityDisplayName(PlayFabAuthenticationContext context, string customId)
    {
        SetDisplayNameRequest request = new SetDisplayNameRequest()
        {
            AuthenticationContext = context,
            DisplayName = customId,
            Entity = new PlayFab.ProfilesModels.EntityKey()
            {
                Id = context.EntityId,
                Type = context.EntityType,
            },
        };

        PlayFabResult<SetDisplayNameResponse> updateNameResult = await PlayFabProfilesAPI.SetDisplayNameAsync(request);
    }
    public static async Task<List<EntityLeaderboardEntry>> GetLeaderboard(PlayFabAuthenticationContext context, string leaderboardName)
    {
        PlayFabProgressionInstanceAPI leaderboardsAPI = new PlayFabProgressionInstanceAPI(context);
        GetEntityLeaderboardRequest getLbRequest = new GetEntityLeaderboardRequest()
        {
            LeaderboardName = leaderboardName,
            StartingPosition = 1,
            PageSize = 20,
            AuthenticationContext = context,
        };

        PlayFabResult<GetEntityLeaderboardResponse> lbResponse = await leaderboardsAPI.GetLeaderboardAsync(getLbRequest);

        return lbResponse.Result.Rankings;
    }

    public static async Task UpdateLeaderboardForPlayer(PlayFabAuthenticationContext context, string leaderboardName, string entityId, int score, int score2)
    {
        PlayFabProgressionInstanceAPI leaderboardsAPI = new PlayFabProgressionInstanceAPI(context);
        UpdateLeaderboardEntriesRequest updateLeaderboardRequest = new UpdateLeaderboardEntriesRequest()
        {
            Entries = new List<LeaderboardEntryUpdate>()
        {
            new LeaderboardEntryUpdate()
            {
                EntityId = entityId,
                Scores = new List<string> { score.ToString(), score2.ToString()},
                Metadata = GenerateRandomString(8),
            }
        },
            AuthenticationContext = context,
            LeaderboardName = leaderboardName,
        };

        PlayFabResult<PlayFab.ProgressionModels.EmptyResponse> updateResult = await leaderboardsAPI.UpdateLeaderboardEntriesAsync(updateLeaderboardRequest);
    }
    private static string GenerateRandomString(int length)
    {
        const string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        Random random = new Random();
        return new string(Enumerable.Repeat(chars, length)
            .Select(s => s[random.Next(s.Length)]).ToArray());
    }
    private static void ShowHelp()
    {
        Console.WriteLine("Usage:");
        Console.WriteLine("  /h                              Show help");
        Console.WriteLine("  /playername NAME                Set the player name on the leaderboard.");
        Console.WriteLine("                                   If you entry multiple players, a number will be added after the Playername.");
        Console.WriteLine("  /score NUMBER                   Set the score on the leaderboard. Default is 1000");
        Console.WriteLine("  /rank NUMBER                    Set the rank on the leaderboard. Default is 100");
        Console.WriteLine("                                   If there are multiple entries, a random number will be used.");
        Console.WriteLine("  /number NUMBER                  Set the number of entries to be entered on the leaderboard.");
        Console.WriteLine("                                   If not set, it defaults to 1.");
        Console.WriteLine("  /version NUMBER                 Set the Specify the maximum number of queryable versions for the leaderboard. Default is 1");
    }
}
