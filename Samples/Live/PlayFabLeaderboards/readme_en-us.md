_This sample is compatible with the October 2024 GDK._

# PlayFab Leaderboards

## Description
This is a sample that displays leaderboard data registered in PlayFab's Leaderboard (v2).

This is a simple sample application (PlayFabLeaderboards.exe) that runs on Xbox and PC. It reads and displays PlayerEntityIDs registered in PlayFab's leaderboard. The implemented features include displaying multi-column scores, metadata, and update times from multiple PlayFab leaderboards, showing 10 entries at a time. Additionally, it has the functionality to retrieve and display the rankings of players within 5 positions before and after a specified PlayerID in the code, as well as the rankings of friends.

This sample is intended to serve as a reference for loading PlayFab leaderboards. (As mentioned later, we also provide a tool that allows you to create leaderboards and write PlayerEntities to leaderboards as supplementary features.) Writing from clients using statistics requires caution from a cheating perspective, so this sample does not implement it. If there is any feedback, we will consider updating this sample in the future.

Typically, leaderboard creation is done through the PlayFab Manager (https://developer.playfab.com/) or by using the developer's web service for creation and data registration. For more details.
However, for the purpose of easily viewing leaderboard information in this sample, we provide a command-line tool (PlayFabLeaderboardsTool.exe) as a support feature.
By using this tool, you can easily register multiple PlayerEntities to PlayFab's leaderboard from the Windows command line. The tool's features include automatically creating two leaderboards and either registering the score of a specific PlayerID or randomly registering scores for a specified number of PlayerEntityIDs with numbered EntityIDs.
Please refer to the tool's option help for usage instructions. 

The application and tool are hardcoded to use the same leaderboard name. If you want to change this part, you will need to refer to and edit the code.

## Building the sample

This sample uses the PlayFab.Services.C extension library. To build this sample, you must target the October 2024 GDK (or later).And,Please use Visual Studio 2019/2022 or later. This sample was developed using Visual Studio 2022.

![Screenshot of extension](./media/playfabextension.png)

## Using the sample

![Screenshot of sample](./media/screenshot.png)
![Screenshot of tool](./media/screenshot_tool.png)

This sample is intended to run in the sandbox XDKS.1. To use the `PFLeaderboards` API, please prepare a test account that can sign in to **XDKS.1**. Additionally, you will need not only the Xbox title ID but also the PlayFab title ID. You can check the PlayFab title ID in the PlayFab Manager.

For details on creating titles and the necessary IDs, please refer to the [[Quickstart leaderboards](https://learn.microsoft.com/en-us/gaming/playfab/features/new-leaderboards-statistics/leaderboards/quickstart-leaderboards)] guid.


## Implementation details

### PlayFab authentication

You need to call the PlayFab leaderboard APIs used by the Xbox and Windows applications, as well as the provided tools.This is facilitated by the [PlayFabResources](..\..\..\Kits\PlayFabGDK\PlayFabResources.h) helper class which wraps the required [LoginWithXbox](https://learn.microsoft.com/en-us/rest/api/playfab/client/authentication/login-with-xbox) authentication that uses API provided by the PlayFab extension library.
Being able to request an XToken requires an Xbox Live enabled product that has been published via Partner Center.

After a player successfully signs into Xbox Live and PlayFab, PlayFab Leaderboards can be retrieved from the service.

## How to use

As mentioned above, data registration to the leaderboard is typically done through the developer's web service. However, in this sample, the creation and registration of the leaderboard are done using a command-line tool, and the data is then read and displayed by the Xbox and Windows client applications. If you start the client application without registering anything, the data cannot be loaded, and the leaderboard will not be displayed.

Below are the steps to display leaderboard data in the client application

  1,Open the Program.cs file in the \Live\PlayFabLeaderboards\PlayFabLeaderboardsTool folder, set the DEVELOPER_SECRETKEY and PlayFab title ID generated in the PlayFab Manager, and then build.

    （Note: Writing to the leaderboard should ideally be done from a trusted web service, so please do not hardcode the developer secret key in a client that users can edit. This tool sample is intended as an auxiliary tool for creating and writing to leaderboards. ）

  2,Open the command prompt.

  3,Run the built PlayFabLeaderboardsTool.exe.

    Example 1: PlayFabLeaderboardsTool.exe /h (Display tool help)

    Example 2: PlayFabLeaderboardsTool.exe /number 100 (Register 100 PlayerEntityIDs and random scores)

    Example 3: PlayFabLeaderboardsTool.exe /playername testplayer1 /score 1000 (Write a score of 1000 for testplayer1 to the leaderboard)
 
  4,Set the PlayFab title ID in the PLAYFAB_TITLE_ID of PlayFabLeaderboards.h file in the \Live\PlayFabLeaderboards folder.

  5,Change the title ID and SCID in the MicrosoftGameConfig.mgc file of PlayFabLeaderboards to your title's information and then build.

  6,Run the built PlayFabLeaderboards.exe on Xbox and PC.

  How to operate the client application：
    -By pressing the 'Get Leaderboard data' button on the left, you will retrieve the data registered in SampleLeaderboard1 using the tool.
    -Use the LT button and RT button to switch between SampleLeaderboard1 and SampleLeaderboard2.
    -Use the LB button and RB button to update the leaderboard in increments of 10 people.
    -When you click on 'Get surroundings of EntityID' on the left, it retrieves and displays the ranking positions of the specified PlayerID within 5 places before and after in the code.
    In the client application's code, the PlayerID is hard-coded as 'PlayerEntityId1', which is automatically generated when multiple users are registered with the tool.
    By default, this feature will not work if 'PlayerEntityId1' is not registered on the leaderboard.

    -When you click on 'Get Friend Leaderboard' on the left, it retrieves data for friends, including Xbox friends, up to a maximum of 25 people.
      You need to manually register the friends' data using the Title player account ID, which can be checked in the PlayFab Manager. The Title player account ID can be confirmed in the PlayFab Manager's Players section by signing in to the relevant PlayFab title application with an Xbox Live account. Please ensure that the Xbox accounts to be registered as friends are already added as friends beforehand.

## Limitations
In this sample, the leaderboard names are set to 'SampleLeaderboard1' and 'SampleLeaderboard2'. If you want to create them with different names, please modify the code accordingly.

If you want to use statistics, you need to enable 'Allow client to post player statistics' in PlayFab Manager > Settings > API Features. However, as mentioned above, this sample does not implement it.

This sample demonstrates how to communicate with PlayFab services via the PlayFab.Services.C extension library.
This library is not available on non-GDK platforms and only a single player login method (`PFAuthenticationLoginWithXUserAsync`) is provided.
Only client APIs are supported (those that use a player entity key) and there is no support for Economy v1 features.

With inclusion in the GDK, developers can now call PlayFab using the [XAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/async-library-xasync) calling pattern.
While most calls within this sample use a callback to determine completion, it is possible (and sometimes preferable) to call functions synchronously.
This approach is demonstrated by the call to `PFAuthenticationLoginWithXUserAsync` and [XAsyncGetStatus](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xasyncgetstatus) (where wait = true).

With the extension library, any error coming from the service is returned as an HRESULT (defined in PFErrors.h).
In many cases, the HRESULT code is not as informative as the underlying error codes that PlayFab provides.
For example, HR 0x8923541A (E_PF_SERVICEERROR) might be returned when the player has insufficient funds (PlayFab errorCode=1059) or when the database throughput has been exceeded (PlayFab errorCode=1113).
We recommend using a web-debugging tool such as [Fiddler](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/fiddler-setup-networking) to see the detailed error messages coming from the service.

## Update history
May 2025 : Initial release

## Privacy Statement
When compiling and running a sample, the file name of the sample executable will be sent to Microsoft to help track sample usage.
To opt-out of this data collection, you can remove the block of code in Main.cpp labeled “Sample Usage Telemetry”.

For more information about Microsoft’s privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).



