  ![](./media/image1.png)

#   Title Storage Sample

*This sample is compatible with the Microsoft GDKX (August 2020)*

# Description

This sample demonstrates Title Storage API provided by the Microsoft
Game Development Kit (GDK). The sample includes scenarios for:

-   Enumerating and downloading Global Storage data

-   Enumerating, uploading, downloading and deleting Universal Storage
    data

-   Enumerating, uploading, downloading and deleting Trusted Platform
    Storage data

-   Retrieving Quota information

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using an Xbox Series X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Running the sample

-   You will need a signed-in Xbox Live test account

-   Xbox One devkit: set the console's sandbox to XDKS.1

# Using the sample

## 

![](./media/image3.png)

| Action                                 |  Gamepad                     |
|----------------------------------------|-----------------------------|
| Select the storage and scenario to run |  D-Pad Up/Down               |
| Confirm the storage and run a scenario |  A Button                    |
| View button                            |  Exit                        |

The black area (list window) will display Blob Path, Blob Type, Display
Name, Length (data size) and XUID of the uploaded user. For Global
Storage, you can only enumerate, download and display data which is
configured in MPC in advance. In this sample, there is data already
uploaded to Global Storage of this product in XDKS.1. For other
storages, in addition to those, you can also upload and delete data.

# Scenarios to try

-   Selecting storage

    -   Pressing A button when focusing on "Select Storage Location"
        will display the list window of the corresponding storage. You
        can move to the list window with D-Pad and by pressing A button
        you can download its data. By default, Global Storage data is
        being displayed in the list window when you launch this sample.

> ![](./media/image4.png)

-   Uploading data

    -   After selecting any storage with A button other than Global
        Storage in "Select Storage Location", you can upload data in any
        format by moving to "Upload" and then pressing A button.

> ![](./media/image5.png)

-   Downloading and deleting Title Storage data

    -   When the selected storage has data, the data will be enumerated
        in the list window automatically. By selecting any data with A
        button, you can download and delete the data.

> ![](./media/image6.png)

# Update history

**Initial Release**: November 2020

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
