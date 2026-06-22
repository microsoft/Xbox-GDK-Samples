PlayFabMultiplayerRumble Sample

*This sample is compatible with the Microsoft Game Development Kit
(June 2022)*

# Description

This sample demonstrates multiplayer functionality using the PlayFab
Multiplayer and PlayFab Party APIs to build a complete end-to-end game
experience.

This sample provides options for the following multiplayer scenarios.

-   **Creating joinable lobbies**

> Create a lobby which is advertised through the platform UI as
> joinable.

-   **Inviting friends and party members to your lobby**

> Open the System UI to invite friends to your lobby. Uses a custom
> invite string.
>
> Send invites to party members.

-   **Joining a friend's session in progress**

> Enumerate and display lobbies being played by friends and join them
> directly. Joining can also be done from the shell on Xbox consoles or
> from the Xbox app on Windows via a friend's gamer card or activity
> feed, or from the party app if a party member is in a game session.

-   **Matchmaking**

> Creating and submitting a matchmaking ticket. Matchmaking will return
> a lobby connection string when successful.

-   **Active gameplay**

> Sending and receiving data across all lobby members for state and
> gameplay scenarios for up to 4 players at once. In this case gameplay
> is an amazing twin-stick shooter!

# Building the sample

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using a Windows PC, set the active solution platform to
Gaming.Xbox.x64

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

**Xbox Live Sandbox Requirements**

-   Xbox One devkit: set the console's sandbox to XDKS.1

-   Windows 10: use the default retail sandbox OR set to
    XDKS.1 for cross-play with Xbox One (see the docs included with the
    Xbox One SDK for instructions on how to change the Xbox Live sandbox
    in Windows)

-   NOTE: when using XDKS.1 in Windows 10, you will need
    to sign in using a developer account that was created in XDP. You
    can sign into this account from the game menus, or from the Xbox App
    on Windows.

# Known issues

-   Joining a game in progress is only possible if players are currently
    waiting for the game to start while sitting on the lobby screen.

# Update history

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of README for the sample. Includes build requirements, usage details, and notes and issues. |  March, 2022  |  1.0 |
| Updated project to use PlayFabMultiplayer from the GDK |  April, 2023  |  1.1 |

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
App.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
