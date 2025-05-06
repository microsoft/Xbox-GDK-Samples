  ![](./media/image1.png)

#   Title-managed Leaderboard Sample

*This sample is compatible with the Microsoft Game Development Kit with
Xbox Extensions (October 2022)*

# 

# Description

The leaderboards sample demonstrates the usage of Xbox Live Leaderboards
with Title-Managed stats (previously referred to as Stats 2017).

![](./media/image3.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using an Xbox Series X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using a Desktop, set the active solution platform to `Gaming.Desktop.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Running the sample

-   You will need an Xbox Live test account signed in to send player
    stats and view *Social Leaderboards*

-   Set your system's sandbox to XDKS.1

*NOTE: You will need more than one test account, friended with each
other both having sent stats from the sample to see more than one user
listed in Social Leaderboards queries*

# Sample Setup in Partner Center

Unlike Leaderboards based on Event-Based stats, Title-Managed Featured
Stats and Leaderboards are defined at the same time.

1.  Create a new leaderboard for the stat

2.  Fill out the information for the stat

![](./media/image4.png)

The ID field is what you will use to refer to your stat when uploading
from the client.

NOTE: When selecting the String type for a stat, the sorting mode
doesn't matter as they can't be used in leaderboards. Instead, they are
just queried from the server.

Below are the stats defined for this sample.

![](./media/image5.png)

*NOTE: These images reflect the layout of Partner Center at the time
this sample was written.*

# Implementation notes

- `LeaderboardsTitleManaged.cpp` contains code relevant to producing and querying stats and leaderboards
- The Title-managed leaderboard API works differently from the Event-based leaderboard API. Due to these differences, it is recommended for your title to use Event-based stats if possible.
  - For general documentation on the reasoning and differences between Title-managed stats and Event-based stats, refer to this [documentation](https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/live/features/player-data/stats-leaderboards/live-stats-eb-vs-tm).
  - __IMPORTANT:__ For Title-managed leaderboards, it's important to be aware that a player's statistic (accessible via `XblUserStatisticsGet*`) can differ from the value stored on the global leaderboard, resulting in mismatched outputs. For example, a mismatch can happen when updating a stat with a lower value when a global leaderboard will only maintain the highest recorded value it. Further reading is available on the page for [XblLeaderboardQuery](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xblleaderboardquery).
- The sample also demonstrates querying of stat values directly.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update history

**Initial Release:** January 2021

**Update:** June 2022

**Update:** July 2022

**Update:** March 2025 - updated readme with additional notes on Title-managed vs Event-based stats
