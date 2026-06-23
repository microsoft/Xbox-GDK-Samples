![ATG Logo](./media/atgLogo.png)

# Unity Rumble Sample

This sample is a port of the C++ NetRumble sample provided by the Xbox ATG team and available via the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk).
It is compatible with:

- [Microsoft GDK](https://github.com/microsoft/GDK/releases/tag/June_2024) or [Microsoft GDKX](https://www.microsoft.com/en-us/software-download/gdk) June 2024 (10.0.25398.4271) & Later

- [Unity Editor](https://unity.com/releases/editor/archive) 6000.0.23f1, 2022.3.49f1, 2021.3.45f2 & Later

- [Microsoft GDK API](https://docs.unity3d.com/Packages/com.unity.microsoft.gdk@1.4/manual/index.html) 1.2.3 & Later (available via Unity's Package Manager)

- [Microsoft GDK Tools](https://docs.unity3d.com/Packages/com.unity.microsoft.gdk.tools@1.4/manual/index.html) 1.2.3 & Later (available via Unity's Package Manager)

- [Microsoft GDK Discovery](https://docs.unity3d.com/Packages/com.unity.microsoft.gdk.discovery@1.1/manual/index.html) 1.1.0 & Later (available via Unity's Package Manager)

- [PlayFab Party Unity SDK](https://github.com/PlayFab/PlayFabPartyUnity/releases) 1.10.5.0-main.0 & Later

**Important!** The first time the sample project is loaded in Unity, the 'Enter Safe Mode?' dialogue might appear due to missing PlayFab dependencies.
Select 'Ignore' and the SetupSample script will automatically import PlayFab dependencies during load.
After successfully loading the project, the sample should be ready to 'build and run'.
If any errors are encountered during load, see [**Building the sample**](#building-the-sample) for guidance.

**Note:** When buidling for console, you must use a version of the Unity Editor that supports your target GDKX version.
See [**Building for console**](#building-for-console) for more information.

*If developing with the legacy (now deprecated) Unity GameCore packages instead of the Microsoft GDK packages listed above, use the October 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk) (set file type to 'GDK' and build/version to 'Microsoft October 2024 GDKX Unity Samples)*.

#

# Description

The **Unity Rumble** sample demonstrates the usage of Xbox Multiplayer Manager APIs ([XblMutliplayerManager](https://learn.microsoft.com/en-us/gaming/gdk/docs/services/multiplayer/mpm/concepts/live-multiplayer-manager-api-overview)) using the Unity game engine.

It is a simple multiplayer game which demonstrates Xbox Live, Xbox Multiplayer Manager (MPM), and PlayFab Party to perform the following:

- Sign into Xbox Live services & PlayFab services

- Retrieve Xbox Live friend profile information

- Start an Xbox Live multiplayer session using MPM

- Invite Xbox Live friends to join your multiplayer session

- Join a friend's Xbox Live multiplayer session

- Matchmake Xbox Live users into a SmartMatch multiplayer session

- Enter into a PlayFab Party network which supports voice chat

- Engage in gameplay using reliable and unreliable messaging

![Unity Rumble main menu](./media/mainMenu.png)

# Notable Code Files

The sample code, found under the "Assets\UnityRumble\Scripts" folder, is divided primarly into Logic and View (UI) related code.
Within core logic functionality, the code is further broken down to separate the Xbox Live and multiplayer, from gameplay and other generic logic bits.

### Assets\UnityRumble\Scripts\Logic\XboxLive

- *XboxLiveLogic.cs* - provides sign-in logic and handles 'user change' events.

- *XboxLiveMatchmakingLogic.cs* - handles matchmaking progression.

- *XboxLiveMultiplayerLogic.cs* - handles multiplayer events coming from MPM.

- *XboxLiveSessionLogic.cs* - handles session state transitions, including host migration.

- *XboxLiveSocialLogic.cs* - uses XblSocialManager for tracking friends and discovering joinable lobby sessions.

### Assets\UnityRumble\Scripts\Logic\Session

- *SessionNetwork.cs* - starts and stops networking with PlayFab Party.

### Assets\Plugins\Editor

- *SetupSample.cs* - automatically imports PlayFab dependencies during project load.

# Building the Sample

For general guidance on using the GDK with Unity, see [Unity GDK integration for PC: get started](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/pc-dev/tutorials/get-started-with-unity-pc/gdk-unity-end-to-end-guide).

The following steps are provided to help troubleshoot common issues when 'build and run' does not work as expected:

1. After opening the project, you may need to add the following components via Unity's **Package Manager** to resolve errors:

     - Microsoft GDK API (com.unity.microsoft.gdk) - version 1.2.3 (or later).

     - Microsoft GDK Tools (com.unity.microsoft.gdk.tools) - version 1.2.3 (or later).

     - Microsoft GDK Discovery (com.unity.microsoft.gdk.discovery) - version 1.1.0 (or later).

     - Input System (com.unity.inputsystem) - version 1.7.0 (or later).
     Switching to the new Input System will require an Editor restart.

     - TextMesh Pro - TMP Essentials needs to be included in the project (Project Settings > TextMesh Pro > Import TMP Essentials).

2. Add PlayFab Party for Unity SDK - version 1.10.5.0-main.0 (or later).

    - When the project loads, an Editor script (Assets > Plugins > Editor > SetupSample.cs) will automatically import the PlayFab and PlayFab Party SDKs included with the project (Assets > SDKs > playfab-party_rumble-sample.unityproject).

    - After a successful import, the SetupSample script will attempt to copy PlayFab Party DLLs from the latest version of the GDK installed on your machine into a 'GameCore' folder in the project.
    If there are any issues, follow the instructions in the [**PlayFab + GDK**](#playfab--gdk) section of this document.

![Image of GDK Settings for Unity Rumble](./media/playfabDlls.png)

3. Set the sample's Input Action Asset to be used as the project-wide input actions (Assets\UnityRumble\Input\SampleControls.inputactions).

4. Activate the sample's GDK Settings Asset (double-click on Assets\UnityRumble\GDKSettings.asset).

5. Open 'Edit > Project Settings > Microsoft GDK' to confirm 'Build for Microsoft GDK', 'Create Microsoft Store Package' and 'Sideloadable Package' options are checked:

![Image of GDK Settings for Unity Rumble](./media/gdkSettingsAsset.png)

6. Open 'File > Build Settings' to confirm that the sample scene (Assets\UnityRumble\Scenes\UnityRumble.unity) is added to the 'Scenes In Build' section.

### Building for PC

On the **Build Settings** page, set Platform to 'Windows, Mac, Linux', target platform to 'Windows' and architecture to 'Intel 64-bit'.
Use 'Build' to build the project, or 'Build and Run' to build and deploy the package to your development PC.

Find and launch the game from the Start menu (if not launched from the Start menu, the player will be unable to sign into Xbox Live and/or handle game invites).

### Building for Console

There are additional requirements when building for console.
The following packages are available from the [Unity Xbox Forums](https://discussions.unity.com/t/unity-for-game-core-downloads/778704).
However, they are under **NDA** status and you will need to join [ID@Xbox](https://www.xbox.com/en-us/Developers/id) to gain access.

- Unity Game Core Series (Scarlett) Add-on (version depends on Unity Editor and GDKX combo)

- Unity Game Core Xbox One Add-on (version depends on Unity Editor and GDKX combo)

- [Microsoft GDK Tools for Xbox](https://discussions.unity.com/t/microsoft-gdk-tools-xbox-1-0-2-is-now-available/1531731) (com.unity.microsoft.gdk.tools.xbox) - version 1.0.2 (or later)

- [GXDK Input System](https://discussions.unity.com/t/gxdk-input-system-1-0-2-bc4cf1f9/1587298) (com.unity.inputsystem.gxdk) - version 1.0.2 (or later)

To build for console, switch Unity's build target on the **Build Settings** page to 'Xbox One' or 'Xbox Series' and set 'Deploy Method' to 'Package'.
Afterwards, you can use '*Build and Run*' to deploy to the default console that you set via [Xbox Manager](https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/tools-console/xbox-tools-and-apis/xbom/xbom).

# Running the sample

You will need an Xbox Live test account signed in to use this sample.
For most multiplayer options to succeed, you need to run this sample on two different devices using two different test accounts that are friends with each other.

The sandbox **must** be set to XDKS.1 for all devices running the sample.

The next sections will cover the sample functionality, and explain how the sample is expected to behave in a correctly working GDK development environment.

### Start Screen

![Unity Rumble start screen](./media/startMenu.png)

The above image shows the expected first screen when the sample is launched. The "Start" button will launch a user login flow for Xbox Live and PlayFab.
If any of the login steps fail, a failure message will be presented at the bottom of the screen, and the user remains on the start screen.

Common failure situations might be that either the user does not have
access to the current sandbox, internet connectivity is currently down, or Xbox Live services or PlayFab services are experiencing a service disruption.

### Main Menu Screen

![Unity Rumble main menu](./media/mainMenu.png)

Once a test user has successfully signed into both Xbox Live and PlayFab, the sample's main menu screen will appear.
The three main functionalities presented are:

1. Match my user with other user(s) running the title using SmartMatch.

2. Have my user host a game lobby session.

3. Attempt to join a friend's currently active game lobby, or directly join a session associated with an invitation that was already received and accepted by the test user.

A failure to perform any of the three capabilities above will display at the bottom of the screen and result in the user remaining at the main menu screen.

### Finding a SmartMatch

![Unity Rumble matchmaking in progress](./media/findMatch.png)

When choosing the "Find Match" option from the main menu, the
sample will immediately attempt a matchmake through a matchmaking ticket managed by the Multiplayer Manager APIs.
The matchmake ticket will generally lead to one of two common results:

1. The matchmake ticket could be fulfilled and the matchmake session (using the match session template presented earlier) was created,    and all of the members that were match-made together placed into the new session.

2. The matchmake ticket could not be fulfilled and timed out.
If that is the result, the error is presented at the bottom of the screen, and the available main menu options are re-enabled.

A matchmaking ticket can also be cancelled so long as it is sill active.
If the matchmaking ticket is cancelled, then the main menu options are re-enabled as if the matchmake request failed.

### Joining a Friend

#### Find an Open Friend Session

When choosing the "Join Friend" button from the main menu, at most three buttons for open friend lobbies will be displayed, along with the option to return to the main menu.
If no friends are currently running a Unity Rumble game lobby, then the list will be empty along with a message that no friend lobbies could be found.

![Unity Rumble find a friend session to join](./media/joinFriend.png)

#### Join via Invite

Upon accepting an invitation sent by a host user, the "Join Invite" button will be enabled for the player and can be used to join the host's active lobby session.

![Unity Rumble invite recieved](./media/inviteRecieved.png)

![Unity Rumble invite accepted](./media/inviteAccepted.png)

### Game Lobby Screen

Once the test user has either: successfully been match-made, successfully joined a friend's lobby session through the "Join Friend" or "Join Invite" main menu options, or successfully started to host their own session, the user is presented with the game lobby screen.

![Unity Rumble Game Lobby Screen](./media/gameLobby.png)

The UI screen will show the members that are present within the lobby session.
The "Leave" option allows the user to abort being in the lobby, and if chosen by the user, will result in that member being removed from the lobby member list showing on the left of the screen.

The ship and color icon buttons above the "Ready" button allows the user to choose what ship style and color they wish to play the game with, and that selection is synchronized with all of the other lobby members as displayed within the lobby members list next to their gamertag.
The icon to the left of the user gamertag also shows who is the host of the session, and their "ready" status.
When all of the members have toggled their ready status to "on" by the "Ready" button, then the lobby will initiate a launch countdown at the bottom of the screen.

#### Host Invite Friend into Lobby

![Unity Rumble game host sending invite](./media/inviteFriend.png)

If the test user is hosting the game lobby through the "Host Game" main menu option, then the "Invite" button will be displayed on the game lobby screen.
Choosing the "Invite" button will present the user with a Shell UI screen that presents a list of friends that can be given invites to the lobby session.

Behind the scenes, the invite APIs are invoked to handle the selection of friends and the server-side invitation protocol mechanics.
If the invitation is not cancelled, then the recipient users, whether in the app or not, will receive a Shell UI notification that they they have a game invitation and they can either accept that invitation, or dismiss it.
If it is accepted, the app is launched (if not already running) and a new button "Join Invite" is enabled on the main menu.

#### The Lobby - All Ready

Once all of the lobby members have set their state to "Ready", the host will signal everyone that the game is ready to launch.
At this point, the session network is used for synchronization, and a countdown timer displays at the bottom of the lobby screen.

![Unity Rumble ready state](./media/lobbyReady.png)

If a user terminates the app, then that member will be removed from the game and disappear from the session network.
Once the countdown completes, the user will be entered into the game play screen.

### Game Play Screen

![Unity Rumble game play](./media/gamePlay.png)

The game play screen shows the active participant members along the left side, which includes their "kill" and "death" counts next to their member gamertag.
The host is indicated by the Xbox icon on the left of the host's name.
If a user quits the game prematurely through the "Quit Game" button, they will disappear from all other member's lists.

The game will naturally conclude once a player has reached a total of five kills.
At that point, just as if the player had chosen to quit the game, the user is taken back to the main menu.

# Partner Center Configuration

![Unity Rumble Partner Center configuration](./media/gameOverview.png)

The above screenshot shows the current state of the UnityRumbleGDK title on Partner Center.
The title has been fully configured, packaged, deployed, and published to the XDKS.1 sandbox.
We can see that there are a total of three multiplayer session template(s), and one SmartMatch hopper(s), configured for the title.

### Multiplayer session templates

![Session Templates for Unity Rumble in Partner Center](./media/MPSessionTemplates.png)

In a common Multiplayer Manager scenario (consult the GDK documentation for further details), we maintain at least 3 session templates: one for the lobby, one for hosted gameplay, and one for matchmade gameplay.

### The LobbySessionTemplate

With the lobby template, we limit the number of participants that a
lobby can have to 5.
The invite protocol specifies that the invitation should be given to the game application. Also, connectivity is required of the members, and this is managed through the RTA (real time activity) service.

```JSON
{
   "constants": {
        "system": {
            "version": 1,
            "maxMembersCount": 5,
            "visibility": "open",
            "inviteProtocol": "game",
            "capabilities": {
                "connectivity": true,
                "connectionRequiredForActiveMembers": true,
                "gameplay" : true,
                "crossPlay": true,
                "userAuthorizationStyle": true,
                "searchable": true
            },
            "memberInitialization":
            {
                "membersNeededToStart": 1
            }
        },
        "custom": {}
    }
}
```

We have elected to make the lobby session public and searchable, even though the sample does not demonstrate any search capability.
Finally, the number of members needed to start the lobby session is exactly one, which will be the starting host of the session.

### GameSessionTemplate

```JSON
{
   "constants": {
        "system": {
            "version": 1,
            "maxMembersCount": 5,
            "visibility": "open",
            "inviteProtocol": "game",
            "capabilities": {
                "connectivity": true,
                "connectionRequiredForActiveMembers": true,
                "gameplay" : true,
                "crossPlay": true,
                "userAuthorizationStyle": true,
                "searchable": false
            },
        },
        "custom": {}
    }
}
```

### MatchSessionTemplate and SmartMatch Hopper

```JSON
{
   "constants": {
        "system": {
            "version": 1,
            "maxMembersCount": 5,
            "visibility": "open",
            "inviteProtocol": "game",
            "capabilities": {
                "connectivity": true,
                "connectionRequiredForActiveMembers": true,
                "gameplay" : true,
                "crossPlay": true,
                "userAuthorizationStyle": true,
                "searchable": false
            },
            "memberInitialization":
            {
                "membersNeededToStart": 2
            }
        },
        "custom": {}
    }
}
```

![SmartMatch hopper configuration in Partner Center](./media/smartMatchHoppers.png)

# PlayFab + GDK

The PlayFab and PlayFabParty SDKs provided with the sample were exported from the 1.10.5.0-main.0 version of the PlayFabPartyUnity package (released on 3/18/25).
These SDKs are included as a convenience for running the sample and **should not** be treated as "official" SDK versions suitable for shipping a published title with.
We recommend using the latest GDK, SDKs, and packages that are supported by your Unity Editor version.

If you wish to update the sample to user a newer PlayFab version, additional steps are needed for PlayFab Party to work with the GDK:

1. Download the PlayFab Party .unitypackage from GitHub and import it into the sample project.

2. Create a new folder named "GameCore" under Assets\PlayFabPartySDK\Source\DLLs

3. Copy Party.dll and Party.pdb from *[GDK install path]\GRDK\ExtensionLibraries\PlayFab.Party.Cpp\Redist\CommonConfiguration\neutral to the 'GameCore' folder created above and then rename the files 'PartyWin.dll' and 'PartyWin.pdb'

4. Copy PartyXboxLive.dll and PartyXboxLive.pdb from *[GDK install path]\GRDK\ExensionLibraries\PlayFab.PartyXboxLive.Cpp\Redist\CommonConfiguration\neutral to the 'GameCore' folder created above.

5. In 'Project Settings > Player > Other Settings' check the 'Allow 'unsafe' Code' option (required by PlayFab Party).

Be sure to check PlayFab's documentation for updated instructions specific to the version you are targeting.

*GDK library locations might change with newer releases, check your install for updated paths.

# Known issues

- The sample was developed and tested against the packages and versions listed in this document.
Using different versions of the GDK, Microsoft GDK API package, PlayFab Party, or the Unity Editor may result in failures or incompatibilities that need to be resolved before building the project.
- The sample occasionally gets stuck when 'Waiting for designated host' or 'Stopping the session network' and will need to be terminated.
- XblMultiplayerManagerLobbySessionSetSynchronizedHost fails on PC due to an empty device token.
This can cause issues if the original host leaves the session and all remaining members do not agree on who to designate as the new host.
- Game invites might not appear if using an unregistered, loose build on PC. Side-load an MSIXVC version of the game and launch from the Windows Start menu to ensure invites appear and are actionable.

# Trademarks

This sample may contain trademarks or logos for projects, products, or services.
Authorized use of Microsoft trademarks or logos is subject to and must follow [Microsoft's Trademark & Brand Guidelines](https://www.microsoft.com/en-us/legal/intellectualproperty/trademarks/usage/general).
Use of Microsoft trademarks or logos in modified versions of this sample must not cause confusion or imply Microsoft sponsorship.
Any use of third-party trademarks or logos are subject to those third-party's policies.

# Privacy Statement

This sample adheres to general Microsoft privacy guidelines regarding the distribution of sample source code, documentation, or other material, for the sole private and individual usage by the prospective developer of the APIs referenced within.

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update history

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of README for the sample. Includes build requirements, usage details, and notes and issues. |  March 22, 2021  |  1.0 |
| Updated the sample to run on Unity 2020.3 LTS and Feb 2021 QFE2 GDK. |  June 18, 2021  |  1.1 |
| Sample now targets both PC and Xbox using the new Microsoft GDK API and tools packages (com.unity.microsoft.gdk, com.microsoft.gdk.tools, com.unity.microsoft.gdk.tools.xbox) available via Unity's Package Manager and the latest PlayFab SDKs available on GitHub. | September 2025 | 1.2 |
