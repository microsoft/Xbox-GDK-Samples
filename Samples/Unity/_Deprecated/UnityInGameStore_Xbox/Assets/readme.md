  ![ATG Xbox and Windows logos](./media/logoImage.png)

# Unity In-Game Store for Xbox

This sample is compatible with:

- [Microsoft GDKX](https://www.microsoft.com/en-us/software-download/gdk) October 2023 Update 4 & Later

- [Unity Editor 2022.3.28f1](https://forum.unity.com/threads/unity-2022-3-28f1-6bae5ce6b222.1591389/) & Later

- [Unity GameCore v1.2.0](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

- [Unity GXDKInput v1.0.1](https://forum.unity.com/threads/game-core-package-1-2-0-84c9720d1886.1548836/) & Later

*If targeting older versions of the GDK and Unity Editor, use the March 2024 version of **GDKX Unity Samples** available from the [GDK Download site](https://www.microsoft.com/en-us/software-download/gdk):*
![Image showing download option for older version of Unity Samples](./media/UnityGDKSamplesDownload.png)

#

# Description

The **Unity In-Game Store for Xbox** sample demonstrates the usage of Xbox
commerce APIs ([XStore](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstore_members)) using the Unity game engine.
You can retrieve the game license, query for add-on content available for purchase, and retrieve entitled products that the user has already purchased.
This sample provides a visual store interface to display product and ownership details, and enables browsing, purchasing and licensing of content.

![Image of In-Game Store sample UI](./media/productListMenu.png)

If you are new to commerce, be sure to check out the [Commerce](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/commerce-nav) overview provided in the GDK documentation.

# Notable Code Files

**Assets\Samples\Scripts\Xbox**

- *XboxManager.cs* - provides sign-in logic and handles 'user change' events. 
The store cannot function without a user signed into Xbox.

**Assets\Samples\Scripts\XStore**

- *XStoreManager.cs* - creates the store context, initializes the store, provides access to global variables and state, and monitors for 'license lost' events.

- *XStoreDownload.cs* - provides support for downloading/installing a durable with a package.

- *XStoreLicensing.cs* - contains game license queries and support for previewing/acquiring durable licenses.

- *XStorePLM.cs* - handles 'Process Lifetime Management' events for the store.

- *XStoreQueries.cs* - calls XStoreQuery* APIs to retrieve product information from the store catalog and user entitlements (collections).

- *XStoreShowUI.cs* - calls XStoreShow* APIs to open the Microsoft Store app.

**Assets\Samples\Scripts\Menus**

- *ItemMenu.cs* - registers acquired durable licenses for 'license lost' events.

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

The console's sandbox **must** be set to XDKS.1 for the sample to work with the default configuration. All @xboxtest.com accounts have access to this sandbox.
If you want to run the sample as your own title within your development sandbox, then you can use the '*Store Association*' wizard to alter the Microsoft Game configuration.
For more information, see [**Configuring the Sample**](#configuring-the-sample).

You will need to sign in with an Xbox Live test account before [XStoreCreateContext](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstorecreatecontext) can be called to initialize the store.
A valid context is required for most store operations.

**Main Menu**

The '*Main Menu*' offers multiple options for interacting with the store at the game level.
Results of each call are displayed in the console window.

![Image showing the Main Menu for the sample](./media/mainMenu.png)

*XStore Show Commands:*

- *Show Game* -- calls [XStoreShowProductPageUIAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowproductpageuiasync) with the storeId of the base game.

- *Purchase Game* -- calls [XStoreShowPurchaseUIAsync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowpurchaseuiasync) with the storeId of the base game.

- *Show Add-ons in Store* -- calls [XStoreShowAssociatedProductsUIasync](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstoreshowassociatedproductsuiasync) with the storeId of the base game.

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

- Open **Project Settings > Player > Publishing Settings > Game Config**.

- Set the '*SCID*' to match the value for your title in Partner Center.

- Select '*Launch MicrosoftGame.config Editor*' to alter the game configuration.

![Image of Player Settings for Xbox](./media/publishingSettings.png)

Under the **General** section of the config editor:

- Set '*Publisher*' to your title's '*Package/Identity/Publisher*' value found in Partner Center.

- Select '*Associate with the Microsoft Store*' and follow the prompts to pull your title's information from Partner Center.

![Image of Microsoft Game Configuration Editor](./media/gameConfig1.png)

# Real Licenses and Trial Testing

While the majority of XStore APIs will work with the default test license, some features (trial mode and XStoreQueryAddOnLicensesAsync) will only work when the sample is configured to use a real license.
Real licenses are available for all store-installed packages, but when using development builds (loose deploy or side-loaded packages), ContentID and EKBID overrides must be set.

This sample has a usage-based trial that expires after 10 hours of title uptimes.
Trial configuration and limits are configured in Partner Center, and
requires the title to use the Restrictive Licensing policy.
Contact your Microsoft Account Representative for more details.

In order for the sample to run in trial mode, it must be configured to use a real license:

1. Ensure no other account with any license to the sample product is signed in (or present in the case the console is set as Home Xbox for any owner account).

2. Obtain a trial license with a test account (`xbapp launch ms-windows-store://pdp?productid=9NTL0QDWZ4FS`).

![Image of 'Free Trial' button in Store](./media/trialPurchase.png)

3. Download the store package fully.

4. With the package installed in step 3 above, obtain the ContentID and EKBID (`xbapp list /d`).

    - **IMPORTANT:** In this scenario it is necessary to use the
        **actual** EKBID from a package downloaded by a trial license
        owner (`xbapp getekbid`).

5. Add override values to development build's MicrosoftGameConfig.mgc

    - Open **Project Settings > Player > Publishing Settings > Game Config**.

    - Select '*Launch MicrosoftGame.config Editor*' to alter the game configuration.

    - Select **Xbox > Development only features** (if missing, use the '*Add new*' option).

    - Set the '*ContentId*' and '*EKBID*' overrides to match the values obtained in step 4 above and press '*Save*' (for this sample, set ContentId = 2797FA46-A93B-494C-AD80-B67C9FCA939F and EKBID = 37E80840-6BE0-46F8-8EDB-92F877056087).

  ![Image of development overrides in game config](./media/gameConfig2.png)

6. Uninstall the store package, build and run from Unity.

7. Observe trial attributes in the Sample UI.

    - Trial license details appear in the *ATG Console* whenever '*Query Game License*' is called.

    - An upsell offer for the game will appear in the *Product List Menu* when the user owns a trial and/or does not have a full license to the game.

    - If using a [packaged build](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/xstore-package-build-iteration), a TCUI trial notification will appear whenever the game is launched or resumes from a suspend event.

![Image of trial overlay message displayed by system](./media/trialTCUI.png)

# Known Issues

- The sample was developed and tested against the packages and versions in
this document.
Using different versions of GameCore or the Unity Editor may result in build failures and incompatibilities.

- '*Query Add-on Licenses*' (XStoreQueryAddOnLicensesAsync) will return 0 when a test license is used for the game.
To fix this, set the ContentId and EKBID overrides using the steps outlined in [**Real Licenses and Trial Testing**](#real-licenses-and-trial-testing).

# Privacy Statement

This sample adheres to general Microsoft privacy guidelines regarding the distribution of sample source code, documentation, or other material, for the sole private and individual usage by the prospective developer of the APIs referenced within.

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).

# Update History

| Description                 |  Release Date       |  Version          |
|-----------------------------|--------------------|------------------|
| Initial draft of sample and README. Includes build requirements, usage details, notes and issues. |  October 2023  |  1.0 |
| Updated the sample to detect trial->full license upgrades and provide basic store-related PLM event handling. |  March 2024  |  1.0 |
| Updated the sample to run on Unity 2022.3.28f1 and to use the latest (legacy) GameCore packages supported by Unity. Future versions of this sample will use the new Microsoft GDK Packages (com.unity.microsoft.gdk, com.unity.microsoft.gdk.tools, com.unity.microsoft.gdk.tools.xbox) available in Package Manager. |  June 2024 |  1.1 |
