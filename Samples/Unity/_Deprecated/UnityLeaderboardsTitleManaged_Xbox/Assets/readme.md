  ![ATG Xbox and Windows logos](./media/image1.png)

#   Unity Title-Managed Leaderboards for Xbox

This sample is compatible with:

- [Microsoft GDKX](https://www.microsoft.com/en-us/software-download/gdk) October 2023 Update 4 & Later

- [Unity Editor 2022.3.28f1](https://forum.unity.com/threads/unity-2022-3-28f1-6bae5ce6b222.1591389/) & Later

- [Unity GameCore v1.2.0](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

- [Unity GXDKInput v1.0.1](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk):*
![Image showing download option for older version of Unity Samples](./media/UnityGDKSamplesDownload.png)

#

# Description

The Unity Title-Managed Leaderboards sample demonstrates the usage of
Xbox Live Title-Managed Leaderboards using the Unity game engine. You
can update user statistics, query the user's statistics, and query a
social and global leaderboard for the numbered statistic.

![Graphical user interface, application Description automatically generated](./media/image3.png)

# Noteable Code Files

**XboxManager.cs**: Contains initialization of the Xbox GDK & Xbox Live Services APIs,
along with signing in a user & querying various information, such as Sandbox
& Title ID.

**XboxLeaderboards.cs**: Contains queries & updates for the Xbox
Live leaderboard APIs.

# Building the Sample

**IMPORTANT:** This sample **requires** the *GameCore* and *GXDKInput* packages provided by Unity, which can be found in the [Unity
Forums](https://forum.unity.com/threads/unity-for-game-core-downloads.837508/).
However, these packages are currently under **NDA** status.
You will need to join [ID@Xbox](https://www.xbox.com/en-us/Developers/id) to gain access.

After opening the project, you **must** add the following components via Unity's **Package Manager** to resolve the errors:

- com.unity.gamecore-1.2.0 (or later)

- com.unity.inputsystem.gxdk-1.0.1 (or later)

![Image showing the Game Core and Input packages in Package Manager](./media/packageManager.png)

You will also need to switch to the target Xbox platform in Unity's
**Build Settings** page.
Afterwards, you can use '*Build and Run*' to deploy to the default console that you set via [**Xbox Manager**](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xbom).

- Game Core -- Xbox One

- Game Core -- Xbox Series

![Image showing build settings for Xbox Series](./media/buildSettings.png)

For more information, see **Set up Unity for Xbox development** in Unity's Game Core documentation (Help > Unity Manual GameCoreScarlettSupport).

# Running the Sample

You will need an Xbox Live test account signed in to execute updates and
retrieval commands.

The console's sandbox **must** be set to XDKS.1.

There are two configured statistics available for this sample:

-   'ANumberStat'

-   'AStringStat'

Both achievement sections act in the same manner for updates &
retrievals.

*Query commands:*

-   '*Query Leaderboard: Global*' -- Queries the global leaderboard for
    the title using the 'ANumberStat' statistic.

-   '*Query Leaderboard: Social*' -- Queries the social/friend
    leaderboard for the title using the 'ANumberStat' statistic.

-   '*Query Statistic: Number*' -- Queries the title's last stored value
    for 'ANumberStat' for the current signed in user.

-   '*Query Statistic: String*' -- Queries the title's last stored value
    for 'AStringStat' for the current signed in user.

**IMPORTANT:** Once a statistic has received a higher numeric value, the
statistic will persist for that user in the leaderboards. This statistic
will, however, be updated accordingly & overwrite the previous value.
Additionally,

*Update command:*

-   '*Update Statistics'* -- Updates both 'ANumberStat' & 'AStringStat'
    to be a random value between 0 & 100 and a random pre-determined
    string value, respectively.

**IMPORTANT:** String values cannot be used for leaderboards as their
comparison for greater value cannot be determined.

*Additional Commands:*

-   '*Clear Logs'* -- Clears the console of all existing logs.

-   '*Close' --* Closes the sample.

# Implementation notes

- The Title-managed leaderboard API works differently from the Event-based leaderboard API. Due to these differences, it is recommended for your title to use Event-based stats if possible.
  - For general documentation on the reasoning and differences between Title-managed stats and Event-based stats, refer to this [documentation](https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/live/features/player-data/stats-leaderboards/live-stats-eb-vs-tm).
  - Event-based stats/leaderboards implementation difference in Unity:
    - Use `XblEventsWriteInGameEvent()` to update statistics.
    - When using `XblLeaderboardQuery.Create()` Set the leaderboard type to `XblLeaderboardQueryType.UserStatBacked`.
  - __IMPORTANT:__ For Title-managed leaderboards, it's important to be aware that a player's statistic (accessible via `XblUserStatisticsGet*`) can differ from the value stored on the global leaderboard, resulting in mismatched outputs. For example, a mismatch can happen when updating a stat with a lower value when a global leaderboard will only maintain the highest recorded value it. Further reading is available on the page for [XblLeaderboardQuery](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xblleaderboardquery).
- The sample also demonstrates querying of stat values directly.

# Sample Setup in Partner Center

Featured Stats & Leaderboards can be found in '*Gameplay Settings'*
underneath the '*Xbox Services'* section in your title's overview bar.
Leaderboards can be added by selecting the '*New Leaderboards'* button.

![A screenshot of a computer Description automatically generated with medium confidence](./media/image6.png)

**IMPORTANT:** In order to enable '*Featured Leaderboards & Stat,'* the
'*Data Platform Setting'* must be set to '*Title-Managed Stats´* in the
*'Xbox Settings'* tab, found underneath the '*Xbox Services'* section in
your title's overview bar.

![Text Description automatically generated](./media/image7.png)

Once inside, Leaderboards can be customized depending on your needs.
Achievements can have:

-   Unique names

-   Identifier for the stat name to track

-   Data type to store (string, integer, decimal, time, etc.)

-   Default sorting type

![Graphical user interface, application Description automatically generated](./media/image8.png)

# Known Issues

The sample was developed & tested against the packages & versions in
this document. It is highly recommended to use the most recent releases
for the GameCore plugin & GXDK Input package released by Unity.

# Privacy Statement

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History
| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of sample and README. Includes build requirements, usage details, notes and issues. | September 2022 | 1.0
| Added Notable Code section. | May 2023 | 1.0
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) Unity GameCore packages. | June 2024 | 1.1
| Updated readme with additional notes on Title-managed vs Event-based stats | March 2025 | 1.1
