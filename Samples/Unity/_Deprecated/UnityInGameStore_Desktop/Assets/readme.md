  ![ATG Xbox and Windows logos](./media/logoImage.png)

# Unity In-Game Store for Desktop

This sample is compatible with:

- [Microsoft GDK](https://github.com/microsoft/GDK/releases) or [GDKX](https://www.microsoft.com/en-us/software-download/gdk) - October 2023 Update 4 & Later

- [Unity Editor](https://unity.com/releases/editor/archive) - 2022.3.28f1 & Later

- [GDK Unity
    Package](https://github.com/microsoft/gdk-unity-package/releases) - March 2024 Update 1 & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk):*

![Image of older version of Unity Samples download option](./media/UnityGDKSamplesDownload.png)

#

# Description

The **Unity In-Game Store for Desktop** sample demonstrates the usage of Xbox
commerce APIs ([XStore](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstore_members)) using the Unity game engine.
You can retrieve the game license, query for add-on content available for purchase, and retrieve entitled products that the user has already purchased.
This sample provides a visual store interface to display product and ownership details, and enables browsing, purchasing and licensing of content.

![Image of In-Game Store sample UI](./media/productListMenu.png)

If you are new to commerce, be sure to check out the [Commerce](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/commerce-nav) overview provided in the GDK documentation.

# Notable Code Files

**Assets\Samples\Scripts\Xbox**

- *XboxManager.cs* - provides sign-in logic and handles 'user change' events.
The store cannot function without a user signed into Xbox Live.

**Assets\Samples\Scripts\XStore**

- *XStoreManager.cs* - creates the store context, initializes the store, provides access to global variables and state, and monitors for 'license lost' events.

- *XStoreDownload.cs* - provides support for downloading/installing a durable with a package.

- *XStoreLicensing.cs* - contains game license queries and support for previewing/acquiring durable licenses.

- *XStorePLM.cs* - handles 'Process Lifetime Management' events for the store.

- *XStoreQueries.cs* - calls XStoreQuery* APIs to retrieve product information from the store catalog and user entitlements (collections).

- *XStoreShowUI.cs* - calls XStoreShow* APIs to open the Microsoft Store.

**Assets\Samples\Scripts\Menus**

- *ItemMenu.cs* - registers acquired durable licenses for 'license lost' events.

# Building the Sample

**IMPORTANT:** This sample **requires** the [*GDK Unity
Package*](https://github.com/microsoft/gdk-unity-package/releases).
Both the '*GDK-APIs'* & '*GDK-Tools'* provided in the package must be included in order to successfully build the sample.

Use the following steps to import the package:

1. Download the GDK Unity Package from GitHub.

2. In Unity, use **Assets > Import Package > Custom Package** and select the GDK Unity package on your PC.

3. This sample already contains a **GDK-Tools\ProjectMetadata** folder which holds a pre-configured '*MicrosoftGame.config*' file.
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

7. To run the sample within the Unity Editor, select **GDK > PC > Update Editor Game Config** and then press the '*Play*' button. For best visual results, set the Game window to display **Full HD (1920x1080)**.

For more information, see the [*Unity End-to-End Guide*](https://learn.microsoft.com/en-us/gaming/gdk/_content/gc/get-started-with-pc-dev/get-started-with-unity-pc/gdk-unity-end-to-end-guide) in the GDK documentation.

# Running the Sample

You need to sign in with an Xbox Live test account before [XStoreCreateContext](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorecreatecontext) is called to initialize the store.
A valid context is required for most store operations.
If a different account is signed into the Xbox App and/or Microsoft Store, then the sample will display a failure message during store initialization and ask you to sync the accounts.

In order for the sample to succeed during store initialization, follow these steps before launch:

1. The PC's sandbox **must** be set to XDKS.1 for the sample to work with the default configuration.
Either use **GDK > PC > Switch Sandbox** to change the sandbox to XDKS.1, or open a GDK command prompt and run 'XblPCSandbox.exe XDKS.1'.

2. Sign into the **Xbox App** with a test account that has access to the sandbox (all @xboxtest.com accounts have access to XDKS.1).

3. Sign into the **Microsoft Store** with the same account before launching the game.

For more information, see [Switching sandboxes properly for Store operations](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstore-switching-pc-sandbox-for-store).

If you want to run the sample as your own title within your own development sandbox, then you can use the '*Store Association*' wizard to alter the Microsoft Game configuration.
Follow the above steps to configure the sandbox and sign in with a test account that is enabled for your sandbox.
For more information, see [**Configuring the Sample**](#configuring-the-sample).

**Main Menu**

The '*Main Menu*' offers multiple options for interacting with the store at the game level.
Results of each call are displayed in the console window.

![Image of Main Menu](./media/mainMenu.png)

*XStore Show Commands:*

- *Show Game* -- calls [XStoreShowProductPageUIAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowproductpageuiasync) with the storeId of the base game.

- *Purchase Game* -- calls [XStoreShowPurchaseUIAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowpurchaseuiasync) with the storeId of the base game.

- *Show Add-ons in Store* -- calls [XStoreShowAssociatedProductsUIAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowassociatedproductsuiasync) with the storeId of the base game.

- *Redeem Token* -- calls [XStoreShowRedeemTokenUIAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowredeemtokenuiasync) for the user to redeem a token within the game.

- *Rate Game* -- calls [XStoreShowRateAndReviewUIAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowrateandreviewuiasync) for the user to submit a star rating and/or written review of the game.

*XStore Query Commands:*

- *Query Catalog* -- calls [XStoreQueryAssociatedProductsAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequeryassociatedproductsasync) to retrieve all add-on products that can be sold by the title.

- *Query Collections* -- calls [XStoreQueryEntitledProductsAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequeryentitledproductsasync) to retrieve all add-on products that the user has entitlements for.

- *Query Game License* -- calls [XStoreQueryGameLicenseAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequerygamelicenseasync) to retrieve a license for the game.
If the license is valid, it will call [XStoreQueryLicenseTokenAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequerylicensetokenasync) to get the license token.

- *Query Add-on Licenses* -- calls [XStoreQueryAddOnLicensesAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorequeryaddonlicensesasync) to get a list of durable add-ons that the user *might* be able to license.

*Additional Controls:*

- *In-Game Store* -- launches the '*Product List Menu*', which provides a visual display of all add-on products available to the user.
If the user does not currently have a full license to the game (owned or shared), then the base game offer will be included in the product list.

- *Sign In* -- opens the Xbox account picker.
A user is required for all store operations.
Whenever a new user is signed-in, the XStoreContext will be recreated and the products refreshed.

- *Exit Game* -- releases all license handles and exits the game.

- *Clear Logs* -- clears the console of all existing logs.

**Product List Menu**

The '*Product List Menu*' displays product details and images for all content returned from the Microsoft Store catalog and collections services that are relevant to the current user.

![Image of Product List](./media/productListMenu.png)

*Controls:*

- *Product Scroll View* -- scrollable button list for content browsing and selection.

- *Product Button* -- displays the logo icon, name, and price of an individual product.
If the product is owned by the user, a checkmark icon will be visible.
Selecting this button will open the '*Item Menu*' for the corresponding product.

- *Close* -- closes the '*Product List Menu*' and activates the '*Main Menu*'.
Pressing 'B' on the Xbox controller will also trigger this operation.

**Item Menu**

The '*Item Menu*' offers multiple options for interacting with the store at the individual add-on level.
Results of each call are displayed in the console window.
Available options are dependent on product type (consumable, durable, durable with a package).

![Image of Item Menu](./media/itemMenu.png)

*Controls:*

- *Purchase* -- launches the purchase flow for the displayed product via [XStoreShowPurchaseUIAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowpurchaseuiasync).
Enabled for all catalog products.

- *Show Product Page* -- shows the product details page in the Microsoft Store by calling [XStoreShowProductPageUIAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowproductpageuiasync). Enabled for all products (catalog + collections).

- *Download and Install* -- adds the product to the download and installation queue by calling [XStoreDownloadAndInstallPackageAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoredownloadandinstallpackagesasync).
Only enabled for durables with a package.

- *Preview License* -- checks if the durable product is licensable to the user by calling either [XStoreCanAcquireLicenseForStoreIdAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorecanacquirelicenseforstoreidasync) or [XStoreCanAcquireLicenseForPackageAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorecanacquirelicenseforpackageasync).
Enabled for durables with and without a package.

- *Acquire License* -- acquires a license for the durable item by calling either [XStoreAcquireLicenseForDurablesAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreacquirelicensefordurablesasync) or [XStoreAcquireLicenseForPackageAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreacquirelicenseforpackageasync).
If the call succeeds, it checks that the license is valid with [XStoreIsLicenseValid](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreislicensevalid). If the license is valid, it will be registered for *License Lost* events via [XStoreRegisterPacakgeLicenseLost](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreregisterpackagelicenselost).
Enabled for durables with and without a package.

- *Close* -- closes the '*Item Menu*' and activates the '*Product List Menu*'.
Pressing 'B' on the Xbox controller will also trigger this operation.

# Sample Setup in Partner Center

Game and add-on configuration in Partner Center is beyond the scope of this document.

To get started with configuring your title in Partner Center, see [Initial configuration in Partner Center](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstore-initial-configuration-in-partner-center).
For more information about supported add-on types (consumables, durables, bundles, etc.), see [Choosing the right product type](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstore-choosing-the-right-product-type).

# Configuring the Sample

This sample can run as any title that has been properly configured in Partner Center and published to the Microsoft Store.
Running this sample with your title configuration can be helpful when trying to diagnose store-related issues with your game.

To configure the sample to run as your title:

- Select **GDK > Associate with the Microsoft Store** to open the **MicrosoftGame.config Editor**.

- Set '*Publisher*' to your title's '*Package/Identity/Publisher*' value found in Partner Center.

- Select '*Associate with the Microsoft Store*' and follow the prompts to pull your title's information from Partner Center.

![Graphical user interface, application Description automatically generated](./media/gameConfig1.png)

- If the '*MSA App Id*' and '*Title Id*' fields are blank under the '*Store information*' section, then you will need to manually edit the game config file to include these values.

  - Open **Assets\GDK-Tools\ProjectMetadata\MicrosoftGame.Config**.

    - Add the *MSAAppId* and *TitleId* values for your game (these values are found in Partner Center under *Game setup > Identity details > Show details*).
    The values shown below are for the In-Game Store sample:

```xml
  <StoreId>9NTL0QDWZ4FS</StoreId>
  <MSAAppId>000000004C2690C8</MSAAppId>
  <TitleId>62ab3c24</TitleId>
```

For more information, see [Enabling XStore development and testing](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstore-product-testing-setup).

# Packaging

If a package has previously been submitted to Partner Center, the ContentId override must be present when creating a new package via **GDK > PC > Build and Run**. The ContentId override should match the ContentId of the store-installed package.

Follow these steps to find and set the ContentId override:

1. Uninstall any previous instance of the game.

2. Download the store package fully for the sandbox you are targeting (for the In-Game Store sample in XDKS.1, run `ms-windows-store://pdp/?productid=9NTL0QDWZ4FS` and click 'Install').

3. With the package installed in step 2 above, obtain the ContentId from the registry (`Computer\HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Store\ContentId`).

4. Add the ContentId override to the MicrosoftGame.Config file.

    - Open **Assets\GDK-Tools\ProjectMetadata\MicrosoftGame.Config**.

    - Set the ContentId override to the value obtained in step 3 above.
    The value shown below is for the In-Game Store sample in XDKS.1:

```xml
  <DevelopmentOnly>
    <ContentIdOverride>0B04E5FB-17E2-4CD7-9501-BB308FA9FC76</ContentIdOverride>
  </DevelopmentOnly>
```

5. Uninstall the store package, use **GDK > PC > Build and Run > Build** to generate a new package with the ContentId.

# Known Issues

- The sample was developed and tested against the packages and versions in
this document.
Using different versions of the *[GDK Unity Package](https://github.com/microsoft/gdk-unity-package/releases)* or the Unity Editor may result in build failures and incompatibilities.

- *Show Associated Products* (XStoreShowAssociatedProductsUIAsync) will result in the Microsoft Store displaying a 'No results found' message when running in sandbox or private audience on PC.
This API will only work in RETAIL with add-on content available to the public.

# Privacy Statement

This sample adheres to general Microsoft privacy guidelines regarding the distribution of sample source code, documentation, or other material, for the sole private and individual usage by the prospective developer of the APIs referenced within.

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of sample and README. Includes build requirements, usage details, notes and issues. |  October 2023  |  1.0 |
| Updated the sample to detect trial->full license upgrades. |  March 2024  |  1.0 |
| Updated the sample to run on Unity 2022.3.28f1 and the latest (legacy) GDK Unity Package on GitHub. Future versions of this sample will use the new Microsoft GDK Packages (com.unity.microsoft.gdk and com.unity.microsoft.gdk.tools) available in Package Manager. |  June 2024 |  1.1 |
