  ![](./media/image1.png)

#   InGameStore Sample (PC)

*This sample is compatible with the Microsoft Game Development Kit
(November 2019)*

# 

# Description

This sample is a port of the InGameStore (WSS) sample that demonstrates
the same operations performed through the Game Runtime XStore APIs.
(Note, some of the visual elements and window sizing are not working
properly in this sample currently. However, the functionality of the
store API's is working.)

![](./media/image3.png)

# Pre-requisites

-   Windows 10 (Version 1903, May 2019)

-   Windows 10 SDK (18362)

-   Microsoft Game Development Kit (June 2019)

-   Visual Studio 2017 (15.9 update recommended)

-   Xbox Test Account signed in first to Xbox app and then Store app in
    Sandbox XDKS.1

> Get a license for the sample in the store with the test account (run\
> ms-windows-store://pdp/?productid=9NN4ZHKML55R)

# Keyboard Controls

Each square on the screen represents a product that was found when doing
the products query. You can filter the list to product type by using the
left and right Ctrl keys. Use the arrow keys on the keyboard to move the
cursor from one product to another in the list.

-   A -- Select / Purchase highlighted item (XStoreShowPurchaseUIAsync)

-   B -- Refresh Entitlements (XStoreQueryEntitledProductsAsync)

-   X -- Refresh Products (XStoreQueryAssociatedProductsAsync)

-   Y -- License Info (XStoreQueryGameLicenseAsync)

-   Left Ctrl -- Previous product type list

-   Right Ctrl -- Next Product type list

-   ESC -- Exit Sample

# Building the sample

The sample is configured by default for VS2017 and will requires
Microsoft Game Development Kit (June 2019).

# Running the sample

A key characteristic of using the XStore API's is that they require a
valid license to function. This is verified with a call to the licensing
service at launch. Without this available, the APIs will typically
return 0x803f6107 indicating that a valid license was not found.

To obtain a valid license for your test account, run this command to
reach the Store Page for the sample's product directly:

> ms-windows-store://pdp/?productid=9NN4ZHKML55R

Select "Get" to obtain a product license for the account. It is not
necessary to wait for the download to complete. Note that you must be in
the XDKS.1 sandbox and then sign-in to the Xbox app with your test
account before also signing into the Windows Store with the same test
account.

The sample as installed from the store will be properly licensed and
function properly but may represent an older version of the sample. To
have the sample built in Visual Studio work, some additional setup is
required. Simply running the sample through F5 will not properly
register your debug version and link to the appropriate license
information.

To enable a locally built version you will need to run the wdapp
register command in the steps below. This uses the included
MicrosoftGame.config to register the built sample with the same name and
identity as the package downloaded from the store which the license is
tied to. For more info about the needed Ids to include in the
MicrosoftGame.config file for your own app, see [Configuring the sample
to run as your title in your
sandbox](#configuring-the-sample-to-run-as-your-title-in-your-sandbox)
below.

Finally, you must launch the app from the start menu (or taskbar if
pinned) for licensing to work properly. You cannot run with F5 or
executing the .exe directly, that will result in error 0x803f6107.

To set up your locally built version of the sample to run do the
following:

1.  [Switch your sandbox to
    XDKS.1](https://docs.microsoft.com/en-us/gaming/xbox-live/xbox-live-sandboxes)

2.  Log into the Xbox app with your test account (any test account
    should work in this sandbox)

3.  Log into the Windows Store app with the same test account

4.  Build the sample

5.  Open a 2017 Visual Studio Command Prompt

6.  Run the following to register the app as described above\
    wdapp register \[Absolute path to the Gaming.Desktop.x64\\Debug
    folder\]

7.  Launch the app from the Start Menu (F5 and running the .exe directly
    will result in error 0x803f6107 when checking the results of the
    Store API's)

8.  Attach the debugger if needed

# Implementation notes

In this preview version of the sample, image loading is not yet
functional and therefore will only show the products as white boxes in
the UI. The default sample has 9 products (consumables, durables, and
season pass) that will show up shortly after launching the app as seen
in the screenshot above. This indicates that the Store API's are working
appropriately.

Note that if multiple users are signed in, the StoreContext will be
assigned to the latest account in the user changed callback, which may
or may not match the account that is displayed in the sample. Store
operations really do not work well in multi-user scenarios, so assigning
the StoreContext to the account that presses A is typically appropriate.

You must launch the app from the Start Menu or else the Store API's will
return 0x803f6107 from their corresponding results API's.

When running the app from the Start Menu (required for Store API\'s to
work) the Current Working Directory will end up defaulting to
C:\\Windows\\system32 unless you overwrite it. The sample relies on the
font and image files in the .exe\'s directory and so we use the
following code to set the working directory to what we want.

char dir\[1024\];

GetModuleFileNameA(NULL, dir, 1024);

m_ExePath = dir;

m_ExePath = m_ExePath.substr(0, m_ExePath.find_last_of(\"\\\\\"));

SetCurrentDirectoryA(m_ExePath.c_str());

# Configuring the sample to run as your title in your sandbox

If configuring the sample to run as your own title in your own sandbox,
you will need to download your title's package to your PC from the
Windows Store to obtain the needed package information. This info will
be used in the MicrosoftGame.config to register the sample as your
title. Do the following to obtain these values:

1.  [Switch your sandbox to your development
    sandbox](https://docs.microsoft.com/en-us/gaming/xbox-live/xbox-live-sandboxes)

2.  Log into the Xbox app with your test account

3.  Log into the Windows Store app with your test account

4.  Obtain your
    [**TitleID**](https://docs.microsoft.com/en-us/gaming/xbox-live/xbox-live-service-configuration)
    from Partner Center (Services-\>Xbox Live-\>Xbox Live Setup)

5.  Obtain the following info from Partner Center on your app (Game
    Setup -\>Identity Details)

    a.  Store ID

    b.  Package/Identity/Name

    c.  Package/Identity/Publisher

6.  Click the **Store protocol link** (from Product Identity page above)
    to open up the Windows Store to your app's details page or run the
    command in Start-\>Run in Windows.\
    ms-windows-store://pdp/?productid=\[Your title's StoreID\]

7.  Click **Get** if your test account does not own the title yet

8.  Download is not required; the account just needs to own it through
    the store.

Do the following to configure the sample as your title and register the
built sample as your title's package:

1.  Open the MicrosoftGame.Config file and update the Identity variables
    with the same info from above (leave out ProccessArchitecture)

    a.  Name

    b.  Publisher

2.  Update the **m_liveResources** initialization in Sample::Sample() to
    use the **TitleID** obtained from Partner Center if using Xbox Live.

3.  Do a clean compile of the solution

4.  Uninstall the store-downlaoded package (if downloaded) for your
    title by right clicking on the app's start menu title and selecting
    Uninstall

```{=html}
<!-- -->
```
9.  Register the sample build as your package by navigating to the
    solution's root directory and then running the following command in
    the **Visual Studio 2017 command** window:\
    wdapp register \[Absolute path to the Gaming.Desktop.x64\\Debug
    folder\]

```{=html}
<!-- -->
```
5.  Launch the app from the start menu

6.  Attach to the app's process for debugging

# Known issues

The previous InGameStore (WSS) sample utilized downloaded store images
in the UI of the app. This functionality has not yet been ported to the
Game Runtime version of the sample and therefore all products will show
as a white square.

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
