![ATG Logo](./media/image1.png)

# Unity Rumble PlayFab Multiplayer Sample

This sample is a port of the C++ PlayFabMultiplayerRumble sample provided by the Xbox ATG team and downloadable from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk). It is compatible with:

- [Microsoft GDK](https://github.com/microsoft/GDK/releases/tag/June_2024) or [Microsoft GDKX](https://www.microsoft.com/en-us/software-download/gdk) June 2024 (10.0.25398.4271) & Later

- [Unity Editor](https://unity.com/releases/editor/archive) 6000.0.23f1, 2022.3.49f1, 2021.3.45f2 & Later

- [Microsoft GDK API](https://docs.unity3d.com/Packages/com.unity.microsoft.gdk@1.4/manual/index.html) 1.2.3 & Later (available via Unity's Package Manager)

- [Microsoft GDK Tools](https://docs.unity3d.com/Packages/com.unity.microsoft.gdk.tools@1.4/manual/index.html) 1.2.3 & Later (available via Unity's Package Manager)

- [Microsoft GDK Discovery](https://docs.unity3d.com/Packages/com.unity.microsoft.gdk.discovery@1.1/manual/index.html) 1.1.0 & Later (available via Unity's Package Manager)

- [PlayFab Party Unity SDK](https://github.com/PlayFab/PlayFabPartyUnity/releases) 1.10.5.0-main.0 & Later

- [PlayFab Multiplayer Unity plugin](https://github.com/PlayFab/PlayFabMultiplayerUnity/releases) 1.7.9.0-main.0 & Later

**Important!** The first time the sample project is loaded in Unity, the 'Enter Safe Mode?' dialogue might appear due to missing PlayFab dependencies.
Select 'Ignore' and the SetupSample script will automatically import PlayFab dependencies during load.
After successfully loading the project, the sample should be ready to 'build and run'.
If any errors are encountered during load, see [**Building the sample**](#building-the-sample) for guidance.

**Note:** When buidling for console, you must use a version of the Unity Editor that supports your target GDKX version.
See [**Building for console**](#building-for-console) for more information and additional package requirements.

*If developing with the legacy (now deprecated) Unity GameCore packages instead of the Microsoft GDK packages listed above, use the October 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk) (set file type to 'GDK' and build/version to 'Microsoft October 2024 GDKX Unity Samples)*.

#

# Description

The **Unity Rumble PlayFab Multiplayer** sample is a simple multiplayer game that demonstrates usage of [XblMutliplayerActivity](https://learn.microsoft.com/en-us/gaming/gdk/docs/services/multiplayer/mpa/live-mpa-overview), [PlayFab Party](https://learn.microsoft.com/en-us/gaming/playfab/multiplayer/networking/), and [PlayFab Multiplayer](https://learn.microsoft.com/en-us/gaming/playfab/multiplayer/mpintro), using the Unity game engine. The following actions are supported:

- Sign into Xbox Live and PlayFab services

- Retrieve Xbox Live friend profile information

- Start a PlayFab Multiplayer lobby

- Invite Xbox Live friends to join your multiplayer lobby

- Join a friend's PlayFab Multiplayer lobby

- Matchmake Xbox Live users into a PlayFab Multiplayer lobby

- Enter a PlayFab Party network which supports voice chat

- Engage in gameplay using reliable and unreliable messaging

- Use Multiplayer Activity APIs for handling invites and updating joinable sessions

![Sample image](./media/Sample_MainMenu.png)

# Notable Code Files

The sample code, found under the "Assets\UnityRumblePlayFabMultiplayer\Scripts" folder, is divided primarly into Logic and View (UI) related code.
Within core logic functionality, the code is further broken down to separate Xbox Live and multiplayer, from gameplay and other generic logic bits.

### Assets\UnityRumblePlayFabMultiplayer\Scripts\Logic\XboxLive

- *XboxLiveLogic.cs* - provides sign-in logic and handles user change events.

- *XboxLiveMPALogic.cs* - uses XblMultiplayerActivity for tracking user activity changes and handling game invites.

- *XboxLiveSocialLogic.cs* - uses XblSocialManager for tracking player activity and discovering joinable friend sessions.

### Assets\UnityRumblePlayFabMultiplayer\Scripts\Logic\PlayFabMultiplayer

- *PlayFabMultiplayerLogic.cs* - includes the bulk of Multiplayer events and activities, including matchmaking, creating, joining, and leaving multiplayer sessions via PlayFabMultiplayer APIs.

### Assets\UnityRumblePlayFabMultiplayer\Scripts\Logic\Session

- *SessionNetwork.cs* - starts and stops networking with PlayFab Party.

### Assets\Plugins\Editor

- *SetupSample.cs* - automatically imports PlayFab dependencies during project load.

# Building the Sample

For general guidance on using the GDK with Unity, see [Unity GDK integration for PC: get started](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/pc-dev/tutorials/get-started-with-unity-pc/gdk-unity-end-to-end-guide).

The following steps are provided to help troubleshoot common issues when 'build and run' does not work as expected:

1. After opening the project, you may need to add the following components via Unity's **Package Manager** to resolve any errors:

     - Microsoft GDK API (com.unity.microsoft.gdk) - version 1.2.3 (or later).

     - Microsoft GDK Tools (com.unity.microsoft.gdk.tools) - version 1.2.3 (or later).

     - Input System (com.unity.inputsystem) - version 1.7.0 (or later).
     Switching to the new Input System will require an Editor restart.

     - TextMesh Pro - TMP Essentials needs to be included in the project (Project Settings > TextMesh Pro > Import TMP Essentials).

2. Add PlayFab Party for Unity SDK (version 1.10.5.0-main.0 or later) and PlayFab Multiplayer for Unity SDK (version 1.7.9.0-main.0 or later).

    - When the project loads, an Editor script (Assets > Plugins > Editor > SetupSample.cs) will automatically import the PlayFab SDKs included with the project (Assets > SDKs > playfab-multiplayer-party.unityproject).

    - After a successful import, the SetupSample script will attempt to copy PlayFab DLLs from the latest version of the GDK installed on the development PC into the project.

![Image of PlayFabParty DLLs](./media/PlayFabDLLs.png)

![Image of PlayFabMultiplayer DLL](./media/PlayFabMultiplayerDLL.png)

  If there are any issues, follow the instructions in the [**PlayFab + GDK**](#playfab--gdk) section of this document.

3. Set the sample's Input Action Asset to be used as the project-wide input actions (Assets\UnityRumblePlayFabMultiplayer\Input\SampleControls.inputactions).

4. Activate the sample's GDK Settings Asset (double-click on Assets\UnityRumblePlayFabMultiplayer\GDKSettings.asset).

5. Open 'Edit > Project Settings > Microsoft GDK' to confirm 'Build for Microsoft GDK' is checked, 'Create Microsoft Store Package' and 'Sideloadable Package' options are checked:

![Image of GDK Settings for Unity Rumble](./media/gdkSettingsAsset.png)

6. Open 'File > Build Settings' to confirm that the sample scene (Assets\UnityRumblePlayFabMultiplayer\Scenes\UnityRumblePlayFabMultiplayer.unity) is added to the 'Scenes In Build' section.

## Building for PC

On the **Build Settings** page, set Platform to 'Windows, Mac, Linux', target platform to 'Windows' and architecture to 'Intel 64-bit'.
Use 'Build' to build the project, or 'Build and Run' to build and deploy the package to your development PC.

Find and launch the game from the Window's Start menu (if not launched from the Start menu, the player will be unable to sign into Xbox Live and/or handle game invites).

## Building for Console

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

You will need an Xbox Live test account signed into the device (or Xbox App on PC) to use this sample.
For most multiplayer options to succeed, you need to run this sample on two different devices using two different test accounts that are friends with each other.

The sandbox **must** be set to XDKS.1 for all devices running the sample.

The next sections will cover the sample functionality, and explain how the sample is expected to behave in a correctly working GDK development environment.

### Start Menu

![Sample Start](./media/Sample_Start.png)

The above image shows the expected first screen when the sample is launched. The "Start" button will launch a user login flow for Xbox Live and PlayFab.
If any of the login steps fail, a failure message will be presented at the bottom of the screen, and the user remains on the start screen.

Common failure situations include: the user does not have
access to the current sandbox, internet connectivity is currently down, or Xbox Live services or PlayFab services are experiencing a service disruption, or the game was not launched from the Window's Start menu (PC).

### Main Menu

![Sample Main Menu](./media/Sample_MainMenu.png)

Once a test account has successfully signed into both Xbox Live and PlayFab, the sample's main menu screen will appear.
The following functionality is supported:

1. Match my user with other user(s) running the title.

2. Have my user host a game lobby session.

3. Attempt to join a friend's currently active game lobby, or directly join a session associated with an invitation that was already received and accepted by the player.

4. Browse for joinable sessions.

A failure to perform any of the capabilities above will display at the bottom of the screen and result in the user remaining at the main menu screen.

### Find Match

![Sample Find Match](./media/Sample_FindMatch.png)

When choosing the "Find Match" option from the main menu, the
sample will immediately attempt to matchmake through a PlayFabMultiplayer MatchmakingTicket.
The matchmake ticket will generally lead to one of two common results:

1. The matchmake ticket could be fulfilled and a matchmake session is created.
All members that were match-made are placed into this new session together.

2. The matchmake ticket could not be fulfilled and timed out.
If that is the result, an error is presented at the bottom of the screen, and the available main menu options are re-enabled.

A matchmaking ticket can also be cancelled via the 'Cancel' button so long as it is sill active.
If the matchmaking ticket is cancelled, then the main menu options are re-enabled as if the matchmake request failed.

### Join Friend

When choosing the "Join Friend" button from the main menu, XblSocialManager will be used to generate up to three buttons that represent open friend lobbies.
If no friends are currently running a joinable lobby for the game, then the list will be empty along with a message that no friend lobbies could be found.

![Sample Join Friend](./media/Sample_JoinFriend.png)

### Join Invite

Upon accepting an invitation sent by a host user, the "Join Invite" button will be enabled for the player and can be used to join the host's active lobby session.

![Sample Invite Recieved](./media/Sample_GameInviteNotification.png)

![Sample Join Invite enabled](./media/Sample_JoinInvite.png)

### Lobby Browser

When choosing the 'Lobby Browser' option, PlayfabMultiplayer.FindLobbies() will be called to search for joinable sessions.
Use the scroll arrows to browse available sessions and the 'Search' option to update results. The 'Cancel' button will return the player to the Main Menu.

![Sample Join Session](./media/Sample_SessionBrowser.png)

### Game Lobby

Once the player has either: successfully been match-made, successfully joined a friend's lobby session through the "Join Friend" or "Join Invite" main menu options, or successfully started to host their own game session, the user is presented with the Game Lobby screen.

If the current player is hosting the session, they can use the 'Invite' button to send a game invite to friends.

![Sample Host Game](./media/Sample_HostGame.png)

![Sample Send Invite](./media/Sample_SendInvite.png)

The Game Lobby screen will show the members that are present within the lobby session.
The 'Leave' option allows the user to abort being in the lobby, and if chosen by the user, will result in that member being removed from the lobby member list showing on the left of the screen.

![Sample Lobby Ready](./media/Sample_LobbyReady.png)

The ship and color icon buttons above the "Ready" button allows the user to choose what ship style and color they wish to play the game with, and that selection is synchronized with all of the other lobby members as displayed within the lobby members list next to their gamertag.
The icon to the left of the user gamertag also shows who is the host of the session, and their "ready" status.
When all of the members have toggled their ready status to "on" by the "Ready" button, then the lobby will initiate a launch countdown at the bottom of the screen.

### Game Play

After the launch countdown completes, the game begins.
The Game Play screen shows the active participant members on the left, and includes their "kill" and "death" counts next to their member gamertag.
The host is indicated by the Xbox icon on the left of the host's name.

![Sample Game Play](./media/Sample_GamePlay.png)

If a user quits the game prematurely through the 'Quit Game' button, they will disappear from all other member's lists and be returned to the Main Menu.

![Sample Quit Game](./media/Sample_QuitGame.png)

The game will naturally conclude once a player has reached a total of five kills.
After pressing 'OK' on the game over message, the player will return to the Main Menu.

![Sample Game Over](./media/Sample_GameOver.png)

# PlayFab + GDK

The PlayFab and PlayFabParty SDKs provided with the sample were exported from the 1.10.5.0-main.0 version of the PlayFabPartyUnity package (released 3/18/25).
The PlayFabMultiplayer SDK provided with the sample was exported from the 1.7.9.0-main.0 version of the PlayFabMultiplayerUnity package (released 3/18/25).
These SDKs are included as a convenience for running the sample and **should not** be treated as "official" SDK versions suitable for shipping a published title with.
We recommend using the latest GDK, SDKs, and packages that are supported by your Unity Editor version.

If you wish to update the sample to target a newer PlayFab version, the following steps are needed for PlayFab to work with the GDK:

1. Download the PlayFab Party .unitypackage from GitHub and import it into the sample project.

2. Create a new folder named "GameCore" under Assets\PlayFabPartySDK\Source\DLLs

3. Copy Party.dll and Party.pdb from *[GDK install path]\GRDK\ExtensionLibraries\PlayFab.Party.Cpp\Redist\CommonConfiguration\neutral to the 'GameCore' folder created above and then rename the files 'PartyWin.dll' and 'PartyWin.pdb'

4. Copy PartyXboxLive.dll and PartyXboxLive.pdb from *[GDK install path]\GRDK\ExensionLibraries\PlayFab.PartyXboxLive.Cpp\Redist\CommonConfiguration\neutral to the 'GameCore' folder created above.

5. Download PlayFab Multiplayer .unitypackage from GitHub and import it into the sample project.

6. Create a new folder named "GDK" under Assets\PlayFabMultiplayerSDK\Source\DLLs

7. Copy PlayFabMultiplayerGDK.dll and PlayFabMultiplayerGDK.pdb from *[GDK install path]\GRDK\ExtensionLibraries\PlayFab.Multiplayer.Cpp\Redist\CommonConfiguration\neutral" to the 'GDK' folder created above.

8. In 'Project Settings > Player > Other Settings' check the 'Allow 'unsafe' Code' option (required by PlayFab Party).

9. In 'Project Settings > Player > Other Settings' set the 'Scripting Backend' to IL2CPP (required by PlayFab Multiplayer).

10. Select 'PlayFab > MakePlayFabSharedSettings' from the 'PlayFab' menu at the top of the Unity Editor.
This will generate a 'PlayFabSharedSettings.asset' in 'Assets > PlayFabSDK > Shared > Public > Resources'.
Set the 'Title Id' for this asset to **9AD5D**, which is the PlayFab title Id for the sample.

![PlayFab Shared Settings](./media/PlayFabSharedSettings.png)

Be sure to check PlayFab's documentation for updated instructions specific to the version you are targeting.

*GDK library locations might change with newer releases, check your install for updated paths.

# Known issues

- The sample was developed and tested against the packages and versions listed in this document.
Using different versions of the GDK, Microsoft GDK API package, PlayFab Party, PlayFab Multiplayer, or the Unity Editor may result in failures or incompatibilities that need to be addressed before building the project.
- The sample can fail during Xbox Live Multiplayer initialization if the PlayFab Shared Settings asset does not exist and the Title Id has not been set.
See [**PlayFab + GDK**](#playfab--gdk) for instructions on how to fix this error.
- On PC, Xbox Live sign in will fail with error 0x89245110 (E_GAMEUSER_NO_PACKAGE_IDENTITY) when using a non-packaged build or when a packaged build is launched from the Unity build process.
To resolve, install an MSIXVC packaged version of the game and launch it from the Windows Start menu.
See [**Build the Sample**](#building-the-sample) and [**Building for PC**](#building-for-pc) for build instructions.

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
| Initial draft of sample and README using Game Core. Includes build requirements, usage details, notes and issues. |  November 3, 2021  |  1.0 |
| Sample now targets both PC and Xbox using the new Microsoft GDK API and tools packages (com.unity.microsoft.gdk, com.microsoft.gdk.tools, com.unity.microsoft.gdk.tools.xbox) available via Unity's Package Manager and the latest PlayFab SDKs available on GitHub. | September 2025 | 1.2 |
