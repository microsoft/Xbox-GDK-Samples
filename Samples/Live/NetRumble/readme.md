NetRumble Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

This sample demonstrates multiplayer functionality using the 2015
Multiplayer APIs to build a complete end-to-end game experience. This
sample currently only support Gaming.Xbox.x64.

This sample provides options for the following multiplayer scenarios.

-   **Creating joinable sessions**

> Create a lobby session which is advertised through the platform UI as
> joinable.
>
> Create a game session which is linked to the lobby session.

-   **Inviting friends and party members to your session**

> Open the System UI to invite friends to your session. Uses a custom
> invite string.
>
> Send invites to party members.

-   **Joining a friend's session in progress**

> Enumerate and display sessions being played by friends, and join them
> directly. Joining can also be done from the shell on Xbox consoles or
> from the Xbox app on Windows via a friend's gamer card or activity
> feed, or from the party app if a party member is in a game session.

-   **Matchmaking**

> Create and register a match session and submit a matchmaking ticket.
> Matchmaking will return a game session when successful.

-   **Handling host migration\
    **Set the host of each type of session and migrate host duties when
    the host leaves the session.

-   **Active gameplay**

> Sending and receiving data across all session members for state and
> gameplay scenarios for up to 8 players at once. In this case gameplay
> is an amazing twin-stick shooter!

# Known issues

-   There is a race condition that may cause one user to not be visible
    in the Lobby Screen. You will need to back out of the match and try
    again.

-   Audio has not yet been fully implemented in this Project Antibes
    port.

-   In game chat has not yet been implemented.

-   If building using clang/LLVM rather than Visual C++, add
    -fno-pch-instantiate-templates

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using a Xbox Series X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update history

**Initial Release:** April 2019

**Update:** June 2022 -- Compatibility with March 2022 GDK (and newer)

# Using the sample

**Xbox Live Sandbox Requirements**

-   Xbox One devkit: set the console's sandbox to XDKS.1

-   Windows 10: use the default retail sandbox OR set to
    XDKS.1 for cross-play with Xbox One (see the docs included with the
    Xbox One SDK for instructions on how to change the Xbox Live sandbox
    in Windows)

-   NOTE: when using XDKS.1 in Windows 10, you will need
    to sign in using a developer account that was created in XDP. You
    can sign in to this account from the game menus, or from the Xbox
    App on Windows.

**Multiplayer Page**

| Action                      |  Gamepad            |  Keyboard          |
|-----------------------------|--------------------|-------------------|
| Select between matchmaking, creating a session, or joining an existing session if one exists |  D-Pad Up/Down  |  Arrow Up/Down |
| Select menu item            |  A button           |  Enter             |
| Go back                     |  B button           |  Esc or Backspace  |

![](./media/image1.png)

**Lobby Screen -- Matchmaking in Progress**

| Action                      |  Gamepad             |  Keyboard         |
|-----------------------------|---------------------|------------------|
| Cancel matchmaking          |  X button            |  X                |
| Go back                     |  B button            |  Esc or Backspace |

![](./media/image2.png)

****

**Lobby Screen -- Pre-Game**

| Action                      |  Gamepad            |  Keyboard         |
|-----------------------------|--------------------|------------------|
| Mark your player state as ready |  X button  |  X |
| Open party app              |  Y button           |  (N/A)            |
| Start next round (if host)  |  A button           |  A                |
| Change ship color           |  LB button          |  Left Arrow       |
| Change ship appearance      |  RB button          |  Right Arrow      |
| Open/Close lobby options    |  Menu button        |  Tab              |
| Open/Close user options  |  Left thumbstick button |  Shift+Tab |
| Go back                     |  B button           |  Esc or Backspace |

![](./media/image3.png)

****

**Lobby Screen Options Menu**

| Action                      |  Gamepad             |  Keyboard         |
|-----------------------------|---------------------|------------------|
| Select between setting game options, sending invites, and setting a join restriction |  D-Pad Up/Down  |  Arrow Up/Down |
| Adjust winning score and join restriction  |  D-Pad Left/Right  |  Arrow Left/Right       |
| Select menu item            |  A button            |  Enter            |
| Close menu  |  Menu button or B button |  Tab or Backspace |

![](./media/image4.png)

**Gameplay Screen**

| Action                      |  Gamepad             |  Keyboard         |
|-----------------------------|---------------------|------------------|
| Move ship                   |  Left thumbstick     |  W,A,S,D          |
| Fire primary weapon         |  Right thumbstick    |  Arrow keys       |
| Drop mines                  |  Right trigger       |  Space            |
| Return to lobby             |  B button            |  Backspace        |

![](./media/image5.png)

# 

# Implementation notes

This sample is a showcase game that demonstrates how to implement Xbox
Live multiplayer gameplay using the Xbox Services API
multiplayer_manager class.

The concept of mandatory Xbox One G*ame Parties* has been retired with
the new multiplayer APIs. Titles are free to manage their sessions how
they see fit, and advertise which of those sessions they would like to
be joinable or visible from within the Xbox One shell. Refer to
*SetLobbySessionJoinRestrictionAsync*, *SetLobbySessionClosedAsync*,
*AdvertiseGameSessionAsync*, and *GetGameTransferHandle* in the sample.

*Note:* *Legacy titles built on previous XDKs that utilize
game parties will still be supported but are not shown in this sample.*

The matchmaking flow for this sample is a very simple matching
implementation with a single default hopper and no back filling of open
slots. Please refer to the multiplayer XDK documentation and Multiplayer
2015 Developer Flowcharts whitepaper for help with more complex
matchmaking scenarios.

This sample uses the new Multiplayer 2015 API
*get_activities_for_social_group* to render the joinable sessions list
in game, which is not included in the multiplayer manager. It can be
found in the *MultiplayerMenuScreen*.

In this sample QoS is handled through auto-evaluation via the match
session template, which you can refer to in the Service Configuration
document provided along with the sample. The three values measured are
*BandwidthUp*, *BandwidthDown*, and *LatencyAverage*.

This sample implements a simpler host migration design than what is
described in the Multiplayer 2015 Developer Flowcharts in that whomever
is in slot 0 of the member list becomes the new host. Refer to the
flowchart for a more complete QoS based solution.

# Known issues

-   The network mesh provided with this sample is to be used as a
    baseline for your own implementation. [The mesh should not be
    considered resilient enough for production use.]{.underline}

-   The network mesh provided does not provide for scenarios where
    players are on strict NAT types, or host migration might occur to a
    strict host. Your mesh will need to handle these cases.

-   This sample does not take into account the scenario of handling
    multiple local players. Titles looking to have both local and online
    multiplayer mixed will need to cache the local players involved in
    the session.

-   QoS requirements are specified in the match session template for
    auto evaluation. Title-evaluated QoS is currently beyond the scope
    of this sample.

-   The multiplayer manager does not currently look for stale sessions
    to cleanup. It is a best practice during development for titles to
    find and leave all old sessions, which may be lingering from an
    earlier crash for example, when starting up multiplayer.

-   *CheckPrivilegeAsync* is not available yet in the Xbox Live SDK for
    UWP, so the privilege check is skipped when running the Windows
    version of the sample. It is expected to be available in a future
    release.

# Update history

**Initial Release for GDK:** April 2019

**Updated to utilize new Text Chat filtering available in PlayFab Party
starting with the April 2021 GDK**: April: 2021

**Updated to include peer-to-peer and relayed network type
configuration, defaulting to peer-to-peer:** March 2022

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
App.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
