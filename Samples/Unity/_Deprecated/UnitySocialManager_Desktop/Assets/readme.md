  ![ATG Xbox and Windows logos](./media/image1.png)

#   Unity Social Manager for Desktop

This sample is compatible with:

- [Microsoft GDK](https://github.com/microsoft/GDK/releases) or [GDKX](https://www.microsoft.com/en-us/software-download/gdk) - October 2023 Update 4 & Later

- [Unity Editor](https://unity.com/releases/editor/archive) - 2022.3.28f1 & Later

- [GDK Unity
    Package](https://github.com/microsoft/gdk-unity-package/releases) - March 2024 Update 1 & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk):*

![Image of older version of Unity Samples download option](./media/UnityGDKSamplesDownload.png)

#

# Description

The Unity Social Manager for Desktop sample demonstrates the usage of
Xbox Live Social Manager using the Unity game engine. You can alter
friend groups based on different levels of presence & relationships to
find a specific group of users from the Xbox Services API.

![Graphical user interface, application Description automatically generated](./media/image3.png)

# Noteable Code Files

**XboxManager.cs**: Contains initialization of the Xbox GDK & Xbox Live Services APIs,
along with signing in a user & querying various information, such as Sandbox
& Title ID.

**XboxSocialManager.cs**: Contains APIs demonstrating the usage of the
Xbox Live Social Manager.

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

You will need an Xbox Live test account signed in to execute social
group changes & retrieving user statuses.

The desktop's sandbox **must** be set to XDKS.1.

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
    of the current social group setting. Upon selection, it also
    displays the user's Xbox profile, via Xbox UI.

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
this document. Using any newer versions of GameCore or the Unity Editor
may result in build failures & incompatibilities.

# Privacy Statement

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History
| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of sample and README. Includes build requirements, usage details, notes and issues. | September 2022 | 1.0
| Added Notable Code section. | May 2023 | 1.0
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) GDK Unity Package on GitHub. Future versions of this sample will use the new Microsoft GDK Packages (com.unity.microsoft.gdk and com.unity.microsoft.gdk.tools) available in Package Manager. | June 2024 | 1.1