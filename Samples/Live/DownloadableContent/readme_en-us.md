  ![](./media/image1.png)

#   Downloadable Content (DLC) Sample

*Windows PC: This sample is compatible with the Microsoft GDK (June
2022)*

*Xbox One/Xbox Series X|S: This sample is compatible with the Microsoft
GDKX (June 2022)*

# 

# Description

This sample demonstrates how to implement purchase, download,
enumeration and loading of downloadable content through XPackage and
XStore APIs.

![ビデオゲームの画面のスクリーンショット 低い精度で自動的に生成された説明](./media/image3.png)

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S Dev Kit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using a Windows PC, set the active solution platform to
Gaming.Desktop.x64

*For more information, see* __Running samples__, *in the GDK documentation.*

# Running the sample

This sample is configured to work in the XDKS.1 sandbox, but running it
in this sandbox in licensed mode is not strictly required.

The left side of the screen will show the packages that are installed.
You can Mount / Unmount the package, which involve checking for the
license. If you can mount the package successfully, the sample will
display an image from package. This functionality will work without
being licensed, but will require local installation of DLC packages
outlined below.

If the sample is run in XDKS.1 using a test account that is entitled to
the Store product (see below), the right side will show a list of
available Durable addons. Selecting the item will bring up the Purchase
UI if it is not owned by the account; if it is, then selecting the item
download the package. Upon completion, the package should show up on the
list on the left. This most closely represents the actual retail flow
where DLC is purchased from the Store and packages are installed from
CDN.

| Action                     |  Keyboard           |  Gamepad            |
|----------------------------|--------------------|--------------------|
| Select Package  |  Arrow keys Up and Down |  D-Pad Up and Down |
| Switch between local or store packages |  Arrow keys Left and Right |  D-Pad Left and Right              |
| Mount or Unmount Package (left column) Purchase or Download Package (right column) |  Enter  |  A button |
| Toggle XPackageEnumeratePackages kind and scope |  Page Up/Page Down  |  LB/RB |
| Uninstall Package          |  X                  |  X Button           |
| Refresh the enumerated packages |  Y  |  Y Button |
| Exit                       |  Esc                |  View button        |

XStore APIs require a valid license in order to function, as well as
applying specific configuration actions. Refer to the GDK documentation
section titled "**Enabling XStore development and testing**" for more
details.

If this is not done properly, XStore APIs will return 0x803f6107
(IAP_E\_UNEXPECTED) indicating that a valid license was not found.

# How the products are set up

The Store ID for this product is 9NQWJKKNHF1L.

To reach its Store page, from Gaming command prompt use

`xbapp launch ms-windows-store://pdp/?productid=9NQWJKKNHF1L`

or just `ms-windows-store://pdp/?productid=9NQWJKKNHF1L` on Windows.

![ロゴ が含まれている画像 自動的に生成された説明](./media/image4.jpeg)

9NQWJKKNHF1L contains three addons as of this writing, representing
common combinations of packages for available platforms:

-   9P96RFVJQ562 contains packages for Xbox Series, Xbox One GDK and PC

-   9PPJJCWPCWW4 contains an Xbox One ERA package

-   9PGJRLSPSN3V contains an Xbox One GDK package and PC

The sample running on Scarlett devkit should be able to Scarlett DLC
(9P96RFVJQ562) package, and the package that is installed from Store
should have the \_xs suffix. The sample running on Xbox One devkit
should be able to access all three packages, and for 9P96RFVJQ562 the
package would have an \_x suffix instead. The sample running on PC
should only be able to access 9P96RFVJQ562 and 9PGJRLSPSN3V packages.

The sample as installed from the store will be properly licensed and
function properly, but may represent an older version of the sample.

# Running with local packages

While this sample can be run with DLC packages downloaded and installed
from the Store, typical development will involve iteration of DLC
content locally. There are several ways to accomplish this. More
information can be found in the GDK documentation titled "**Manage and
license downloadable content**".

Included in the same are several script files use to generate packaged
versions of the sample and the DLC. For the sample (i.e. base game)
makepcpkg, makexboxonepkg, makescarlettpkg will create their respective
packages. The scripts will build the package with the correct contentID
associated with the packages submitted for 9NQWJKKNHF1L on Partner
Center.

For side-loading game package onto **Xbox One and Scarlett**, you have
to override EKBID.

`xbapp setekbid 41336MicrosoftATG.ATGDownloadableContent_2022.7.19.0_neutral\_\_dspnxghe87tn0 {00000000-0000-0000-0000-000000000001}`

Note for **Xbox One and Scarlett**, the correct **TargetDeviceFamily**
node must be inserted into the
Gaming.Xbox.\*.x64\\Layout\\Image\\Loose\\MicrosoftGame.config,
otherwise makepkg will give an error:

```xml
<ExecutableList>
    <Executable Name="DownloadableContent.exe"
        Id="Game"
        TargetDeviceFamily="Scarlett"/>
</ExecutableList>
```

For DLC, the DLCPackage directory contains all required files for

1.  Scarlett GDK DLC (\_xs.xvc)

2.  Xbox One GDK DLC (\_x.xvc)

3.  Xbox One ERA DLC (no extension)

suitable for Xbox; DLCPackagePC will contain the required files for the
PC .msixvc.

Within each are makedlcpkg commands that will generate each platform's
DLC packages.

With these, it is possible to make packaged builds for the game and DLC.
To install, use **xbapp install** on Xbox or **wdapp install** for PC,
or available equivalent tools. In this configuration, any installed DLC
should show up in the left hand side and be mountable, even if the
sample itself is not running in licensed mode.

It is also possible to run completely using loose files. To achieve
this, use **xbapp deploy** on Xbox or **wdapp register** on PC and pass
in the directory where the MicrosoftGame.config is located, e.g.

`xbapp deploy .\DLCPackage\Package_Scarlett`

`wdapp register .\DLCPackagePC\Package`

It should be possible to mix and match: packaged base game + loose DLC;
loose base game + packaged DLC, loose base game + Store DLC, etc.,
though see the Known Issues section for any problems with certain
combinations.

# Known issues

# Update history

**Initial Release:** April 2019

**Update:** March 2022

Added DLCPackagePC folder for demonstrate to create DLC on PC.

Fixed the crash when the license lost.

**Update:** June 2022

Change XPackageMount API to XPackageMountWithUiAsync API.

Add XPackageUninstallPackage API.

**Update:** July 2022

Fixed the error handling.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
