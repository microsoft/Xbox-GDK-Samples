  ![ATG Xbox and Windows logos](./media/image1.png)

#   Unity Title-Managed Leaderboards for Desktop

This sample is compatible with:

- [Microsoft GDK](https://github.com/microsoft/GDK/releases) or [GDKX](https://www.microsoft.com/en-us/software-download/gdk) - October 2023 Update 4 & Later

- [Unity Editor](https://unity.com/releases/editor/archive) - 2022.3.28f1 & Later

- [GDK Unity
    Package](https://github.com/microsoft/gdk-unity-package/releases) - March 2024 Update 1 & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk):*

![Image of older version of Unity Samples download option](./media/UnityGDKSamplesDownload.png)

#

# Description

The Unity Title-Managed Leaderboards for Desktop sample demonstrates the
usage of Xbox Live Title-Managed Leaderboards using the Unity game
engine for desktop applications. You can update user statistics, query
the user's statistics, and query a social and global leaderboard for the
numbered statistic.

![Graphical user interface, website Description automatically generated](./media/image3.png)

# Noteable Code Files

**XboxManager.cs**: Contains initialization of the Xbox GDK & Xbox Live Services APIs,
along with signing in a user & querying various information, such as Sandbox
& Title ID.

**XboxLeaderboards.cs**: Contains queries & updates for the Xbox
Live leaderboard APIs.

# Building the Sample

**IMPORTANT** This sample **requires** the [GDK Unity Package](https://github.com/microsoft/gdk-unity-package/releases).
Both the *'GDK-APIs'* & *'GDK-Tools'* provided in the package must be included in order to successfully build and run the sample.

Use the following steps to import the package:

1. Download the GDK Unity Package from GitHub.

2. In Unity, use **Assets > Import Package > Custom Package** and select the GDK Unity package on your PC.

3. This sample already contains a **GDK-Tools\ProjectMetadata** folder which holds a pre-configured '*MicrosoftGame.Config*' file.
Uncheck the 'ProjectMetadata' folder to preserve the sample's configuration:

![Image showing package import](./media/packageImport.png)

**Note:** If you accidentally overwrite this file, you can copy the contents of MicrosoftGameConfig.mgc into MicrosoftGame.Config (Assets\GDK-Tools\ProjectMetadata) to restore the sample's configuration.

4. Select the '*Import*' button.
After the package import completes, your '*Project'* folder should mirror the image below:

![Image showing GDK package assets](./media/projectAssets.png)

5. Open **GDK > PC > Build and Run** and check the 'Define MICROSOFT_GAME_CORE' checkbox.
Use this page when you want to generate a packaged version of your game for testing or to upload to Partner Center.

![Image showing Build and Run option for PC](./media/gdk_pcBuildSettings.png)

6. Open **File > Build Settings** and ensure the target platform is set to 'Windows'.
Use '*Build*' or '*Build and Run*' to build the sample executable.

![Image showing Unity Build Settings](./media/buildSettings.png)

7. To run the sample within the Unity Editor, select **GDK > PC > Update Editor Game Config** and then press the '*Play*' button. For best visual results, set the Game window's display to **Full HD (1920x1080)**.

For more information, see the [*Unity End-to-End Guide*](https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/get-started-with-pc-dev/get-started-with-unity-pc/gdk-unity-end-to-end-guide) in the GDK documentation.

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

-   '*Sign In'* -- allows for a new user to sign in using the Xbox user
    selection UI.

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
this document. Using any newer versions of [GDK Unity
Package](https://github.com/microsoft/gdk-unity-package) or the Unity
Editor may result in build failures & incompatibilities.

# Privacy Statement

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History
| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of sample and README. Includes build requirements, usage details, notes and issues. | September 2022 | 1.0
| Added Notable Code section. | May 2023 | 1.0
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) GDK Unity Package on GitHub. Future versions of this sample will use the new Microsoft GDK Packages (com.unity.microsoft.gdk and com.unity.microsoft.gdk.tools) available in Package Manager. Added notes on Title-managed vs Event-based stats | June 2024 | 1.1
| Updated readme with additional notes on Title-managed vs Event-based stats | March 2025 | 1.1
