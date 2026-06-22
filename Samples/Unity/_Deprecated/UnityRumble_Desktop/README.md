  ![](./media/image1.png)

#   Desktop Unity GDK Rumble Sample

*\* This sample has been developed with Unity 2020.3.12f1 and the
Microsoft GDK Feb QFE2 2021 on Win 10.*

# 

# Description

This sample is a port of the NetRumble sample that is commonly included
on the Microsoft GDK download portal page which looks like the
following:

![A picture containing text Description automatically generated](./media/image3.png)

It is a simple multiplayer game which demonstrates Xbox Live, PlayFab,
and PlayFab Party APIs that a developer would use to perform the
following functionalities:

-   Login into Xbox Live services & PlayFab services

-   Retrieve Xbox Live friend profile information

-   Start an Xbox Live multiplayer session using MPM

-   Invite Xbox Live friends to join your multiplayer session

-   Join a friend's Xbox Live multiplayer session

-   Matchmake Xbox Live users into a SmartMatch multiplayer session

-   Enter into a Party network which supports voice chat

-   Engage in gameplay using reliable and unreliable messaging

# 

# Building the Sample

This sample was developed and tested against the Feb 2021 Microsoft GDK
installation, and Unity version 2020.3.12f1. The sample depends on
several SDKS, all of which have snapshots provided in ZIP files (found
in the SDKs/ folder) that can be unpacked into the sample's Assets/
folder to fulfill the sample's necessary SDK dependencies. The sample
consists of only one scene named "SampleScene.unity" found within the
Assets/Sample/Scenes folder.

All of the Unity, Xbox Live, and PlayFab configuration required to build
the sample should already be set once all of the SDKs have been
unpacked, and the sample loaded into Unity. The sample should be built
within the Unity IDE using the "Build" button:

![Graphical user interface Description automatically generated](./media/image4.png)

... the sample currently provides a "Builds" folder and expects the
build output to go into a folder named "Builds/PC/Loose" like so:

![Graphical user interface Description automatically generated with medium confidence](./media/image5.png)

The "MicrosoftGame.config" file, and the five PNG files, can be found
within the "GdkMetadata" folder which provide the supporting files
needed by a GDK application.

Once Unity has completed its build process, and the metadata files
copied to the build output folder, there are two possible ways to "side
load" the application into Windows:

1.  Using a "Gaming VS 2019 Command Prompt" one can execute the "wdapp
    register Builds\\PC\\Loose" command from the sample root folder.

2.  Using a "Gaming VS 2019 Command Prompt" one can run the
    "Builds\\package.bat" script to make a packaged build that is
    installed using the followup command "wdapp install
    Builds\\PC\\Package\\\<name_of_app_identity\>.MSIXVC

# Running the sample

If you have managed to successfully build, possibly package, and side
load the app onto your development Windows 10 PC, you should see an app
icon within the Windows 10 Start menu that is named "UnityRumbleGDK."
Clicking on the app icon should launch the sample into a 1920x1080 HD
window that is non-resizable.

To successfully run, it is important that the PC is running in the Xbox
Live sandbox "XDKS.1" (or your own sandbox) which the title has been
configured and deployed to. A GDK tool named "XBLPCSandbox" -- run from
a Gaming VS 2019 Command Prompt" \-- can assist with switching the
active sandbox. You will also need a batch of test users who can login
to the sandbox.

**Game overview of UnityRumbleGDK on Partner Center**

![A screenshot of a computer Description automatically generated](./media/image6.png)

The above screenshot shows the current state of the UnityRumbleGDK game
title on Partner Center. The title has been fully configured, packaged,
deployed, and published to the XDKS.1 sandbox. We can see that there are
a total of three multiplayer session template(s), and one SmartMatch
hopper(s), configured for the title.

**Multiplayer session templates**

![Text Description automatically generated](./media/image7.png)

**The LobbySessionTemplate**

{

\"constants\": {

\"system\": {

\"version\": 1,

\"maxMembersCount\": 5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": true

},

\"memberInitialization\": {

\"membersNeededToStart\": 1

}

},

\"custom\": {}

}

}

In a common Multiplayer Manager scenario, consult the GDK documentation
for further details, we maintain at least 3 session templates: one for
the lobby, one for hosted gameplay, and one for matchmade gameplay.

With the lobby template, we limit the number of participants that a
lobby can have to 5. The invite protocol specifies that the invitation
should be given to the game application. Also, connectivity is required
of the members, and this is managed through the RTA (real time activity)
service.

We have also elected to make the session be searchable, or public, even
though the sample does not demonstrate any search capability. Finally,
the number of members needed to start the lobby session is exactly one,
which will be the starting host of the session.

**The GameSessionTemplate**

{

\"constants\": {

\"system\": {

\"version\": 1,

\"maxMembersCount\": 5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": false

}

},

\"custom\": {}

}

}

\<blah\>.

**The MatchSessionTemplate and SmartMatch Hopper**

{

\"constants\": {

\"system\": {

\"version\": 1,

\"maxMembersCount\": 5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": false

},

\"memberInitialization\": {

\"membersNeededToStart\": 2

}

},

\"custom\": {}

}

}

![A screenshot of a computer Description automatically generated with medium confidence](./media/image8.png)

\<blah\>.

The next sections will cover the sample's UI screen by screen, and
explain how those screens are expected to function in a correctly
working GDK development environment.

## Sample Start Screen

![Graphical user interface, website Description automatically generated](./media/image9.png)

The above screenshot shows the expected first screen when the sample is
launched. The "Start" button will launch a user login flow for Xbox Live
and PlayFab. If any of the login steps fails, a failure message till be
presented at the bottom of the screen, and the user remains on the start
screen.

Common failure situations might be that either the user does not have
access to the current sandbox, internet connectivity is currently down,
or Xbox Live services or PlayFab services are experiencing a service
disruption of some sort.

## 

## Sample Main Menu Screen

![A screenshot of a computer Description automatically generated with low confidence](./media/image10.png)

Once a test user has been successfully signed into both Xbox Live and
PlayFab, the sample's main menu screen will display. The three main
functionalities presented are:

1.  Match my user with other user(s) running the title using SmartMatch.

2.  Have my user host a game lobby session.

3.  Attempt to join a friend's currently active game lobby, or directly
    join a session associated with an invitation that was already
    received and accepted by the test user.

A failure to perform any of the three capabilities above will display at
the bottom of the screen and result in the user remaining at the main
menu screen.

## 

## 

## Join Friend's Lobby Screen

![A screenshot of a computer Description automatically generated with low confidence](./media/image11.png)

When choosing the "Join Friend" button from the main menu, at most three
buttons for friend lobbies will be displayed, along with the option to
return to the main menu. If no friends are currently running a Unity
Rumble game lobby, then the list will be empty along with a message that
no friend lobbies could be found.

## 

## 

## Finding a SmartMatch Screen

![A screenshot of a computer Description automatically generated with low confidence](./media/image12.png)

When choosing the "Find Match" button option from the main menu, the
sample will immediately attempt a matchmake through a matchmaking ticket
managed by the Multiplayer Manager APIs. The matchmake ticket will
generally lead to one of two common results:

1.  The matchmake ticket could be fulfilled and the matchmake session
    (using the match session template presented earlier) was created,
    and all of the members that were match-made together placed into the
    new session.

2.  The matchmake ticket could not be fulfilled and timed out. If that
    is the result, the error is presented at the bottom of the screen,
    and the available main menu options are re-enabled.

A matchmaking ticket can also be cancelled so long as it is sill active.
If the matchmaking ticket is cancelled, then the main menu options are
re-enabled as if the matchmake request failed.

## The Game Lobby Screen

![Graphical user interface, website Description automatically generated](./media/image13.png)

Once the test user has either: successfully been match-made,
successfully joined a friend's game lobby through the "Join Friend" or
"Join Invite" main menu options, or successfully started to host their
own session, the user is presented with the game lobby screen.

The UI screen will show the members that are present within the lobby
session. The "Leave" option allows the user to abort being in the lobby,
and if chosen by the user, will result in that member being removed from
the lobby member list showing on the left of the screen.

The ship and color icon buttons above the "Ready" button allows the user
to choose what ship style and color they wish to play the game with, and
that selection is synchronized with all of the other lobby members as
displayed within the lobby members list next to their gamertag. The icon
to the left of the user gamertag also shows who is the host of the
session, and their "ready" status. When all of the members have toggled
their ready status to "on" by the "Ready" button, then the lobby will
initiate a launch countdown at the bottom of the screen.

## Host Invite Friend into Lobby

![Graphical user interface, website Description automatically generated](./media/image14.png)

If the test user is hosting the game lobby through the "Host Game" main
menu option, then the "Invite" button will be displayed on the game
lobby screen. Choosing the "Invite" button will present the user with a
Shell UI screen that presents a list of friends that can be given
invites to the lobby session.

Behind the scenes, the invite APIs are invoked to handle the selection
of friends and the server-side invitation protocol mechanics. If the
invitation is not cancelled, then the recipient users -- whether in the
app or not -- will receive a Shell UI notification that they have been
given an invitation and they can either accept that invitation, or
dismiss it. If it is accepted, the app is launched -- if not already
running -- and a new button "Join Invite" is enabled on the main menu.

## Invite from Friend Received Screens

![A screenshot of a computer Description automatically generated with low confidence](./media/image15.png)

![A screenshot of a computer Description automatically generated with medium confidence](./media/image16.png)

As mentioned previously, when accepting an invitation given by a host
user, the main menu screen will display a "Join Invite" when the invite
is accepted.

**The Lobby All Ready Screen**

![Graphical user interface, website Description automatically generated](./media/image17.png)

Once all of the lobby members have "readied up" their user through the
"Ready" toggle will prompt the host to signal everyone that the game is
ready to launch. At this point, the session network is used for
synchronization, and a countdown timer displays at the bottom of the
lobby screen.

If a user terminates the app, then that member will be removed from the
game and disappear from the session network. Once the countdown
completes, the user will be entered into the game play screen.

****

**The Game Play Screen**

![Graphical user interface, application Description automatically generated](./media/image18.png)

The game play screen shows the active participant members along the left
side, which includes their "kill" and "death" counts next to their
member gamertag. The host is indicated by the Xbox icon on the left of
the host's name. If a user quits the game prematurely through the "Quit
Game" button, they will disappear from all of the other member's lists.

The game will naturally conclude once a player has reached a total of
five kills. At that point, just as if the player had chosen to quit the
game, the user is taken back to the main menu.

# Implementation notes

The sample script code, found under the "Assets/Sample/Scripts" folder,
is broken down primarily into the UI/View related code, and the core
logic. Within the core logic, the code is further broken down to
separate the Xbox Live/PlayFab/Networking specific logic, from the
gameplay and other generic logic bits.

-   Xbox Live functions for login, friends, multiplayer, etc. are in
    Assets\\Sample\\Script\\Logic\\XboxLive

-   PlayFab functions for login are in Assets\\Sample\\Logic\\PlayFab

-   Functions for networking and session document are in
    Assets\\Sample\\Script\\Logic\\Session

-   To use Unity's GameCore package, make sure to define
    "USE_UNITY_GAMECORE" and "UNITY_GAMECORE" in place of the defines
    that are there.

# Important!

The SDK snapshots provided with the sample, in the SDKS\\ folder, are
purely meant to serve as a convenience for running the sample and
**should not** be treated as "official" SDK versions suitable for
shipping a published title with. Always use the latest QFE your
development is comfortable with for your title's particular SDK
versions.

# Known issues

Not exactly a "known" issue, but the sample was developed and tested
against the SDK snapshots provided within the "SDKs" folder for the
sample. Integrating a newer version of the Microsoft Unity GDK plugin,
using Unity's GameCore package, or integrating a different version of
the PlayFab, or the PlayFab Party, Unity plugin may expose
incompatibilities with APIs that may be deprecated, or have altered
behavioral characteristics.

# 

# Update history

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of README for the sample. Includes build requirements, usage details, and notes and issues. |  March 22, 2021  |  1.0 |
| Updated the sample to run on Unity 2020.3 LTS and Feb 2021 QFE2 GDK. |  June 18, 2021  |  1.1 |

# 

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
