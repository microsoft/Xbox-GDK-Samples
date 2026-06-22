  ![ATG Xbox and Windows logos](./media/image1.png)

#   Unity Social Manager for Xbox

This sample is compatible with:

- [Microsoft GDKX](https://www.microsoft.com/en-us/software-download/gdk) October 2023 Update 4 & Later

- [Unity Editor 2022.3.28f1](https://forum.unity.com/threads/unity-2022-3-28f1-6bae5ce6b222.1591389/) & Later

- [Unity GameCore v1.2.0](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

- [Unity GXDKInput v1.0.1](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk):*
![Image showing download option for older version of Unity Samples](./media/UnityGDKSamplesDownload.png)

#

# Description

The social manager sample demonstrates the usage of Xbox Live Social
Manager using the Unity game engine. You can alter friend groups based
on different levels of presence & relationships to find a specific group
of users from the Xbox Services API.

![](./media/image3.png)

# Noteable Code Files

**XboxManager.cs**: Contains initialization of the Xbox GDK & Xbox Live Services APIs,
along with signing in a user & querying various information, such as Sandbox
& Title ID.

**XboxSocialManager.cs**: Contains APIs demonstrating the usage of the
Xbox Live Social Manager.

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

You will need an Xbox Live test account signed in to execute social
group changes & retrieving user statuses.

The console's sandbox **must** be set to XDKS.1.

**IMPORTANT:** In order to see results in the 'Found Xbox Users'
section, you must have at least one friend linked to the Xbox user
logged in, via Friends or Favorites. A refresh is automatically
triggered on some Xbox Services events, such as presence changes or
social group updates.

*Social Group commands:*

-   '*All Friends'*

    -   XblPresenceFilter = XblPresenceFilter.All

    -   XblRelationshipFilter = XblRelationshipFilter.Friends

-   'All Favorites'

    -   XblPresenceFilter = XblPresenceFilter.All

    -   XblRelationshipFilter = XblRelationshipFilter.Favorite

-   *'All Online Friends'*

    -   XblPresenceFilter = XblPresenceFilter.AllOnline

    -   XblRelationshipFilter = XblRelationshipFilter.Friends

-   *'Title Online Friends'*

    -   XblPresenceFilter = XblPresenceFilter.TitleOnline

    -   XblRelationshipFilter = XblRelationshipFilter.Friends

**IMPORTANT:** These use a pre-defined combination of Xbox Services
presence & relationship filters to determine the retrieved Xbox users
related to the calling Xbox user.

*Found Xbox User commands:*

-   '*Gamer Tag ... Status'* -- displays the gamer tag & connection
    status of the first five users found that meet the social criteria
    of the current social group setting. Upon selection, also displays
    the user's Xbox profile, via Xbox UI.

**IMPORTANT:** These are only displayed if there is a user that meets
the given criteria from the pre-defined filters. Otherwise, they remain
blank.

*Additional commands:*

-   '*Sign In'* -- allows for a new user to sign in using the Xbox user
    selection UI.

-   '*Refresh' ­*-- manually refreshes the user list of Xbox users.

-   '*Clear Logs'* -- Clears the console of all existing logs.

-   '*Close' --* Closes the sample.

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