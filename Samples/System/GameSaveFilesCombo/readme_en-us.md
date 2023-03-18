  ![](./media/image1.png)

#   GameSaveFilesCombo Sample (PC, XBOX)

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

![Graphical user interface, text Description automatically generated](./media/image3.png)

# 

# Description

A simple sample that implements many of the
[XGameSave](https://docs.microsoft.com/en-us/gaming/gdk/_content/gc/reference/system/xgamesave/xgamesave_members)
api functions. This implementation is done by modifying the data of a
single container with 2 blobs.

Important Methods

## GetFolderWithUIAsync

-   Calls the
    [XGameSaveFilesGetFolderWithUiAsync](https://docs.microsoft.com/en-us/gaming/gdk/_content/gc/reference/system/xgamesavefiles/functions/xgamesavefilesgetfolderwithuiasync)
    in order to get a folder that can be used to contain directories and
    files for storage in the cloud.

## GetRemainingQuoata

-   Calls the
    [XGameSaveFilesGetRemainingQuota](https://docs.microsoft.com/en-us/gaming/gdk/_content/gc/reference/system/xgamesavefiles/functions/xgamesavefilesgetremainingquota)
    to get the remaining storage available to save with using the
    XGameSaveFiles API.

# Building the sample

This sample supports both Xbox and Desktop.

Privacy:

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, remove ATG_ENABLE_TELEMETRY from the
C/C++ / Preprocessor / Preprocessor Definitions list in the project's
settings.

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
