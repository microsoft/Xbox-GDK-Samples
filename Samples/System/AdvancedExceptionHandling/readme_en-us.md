  ![](./media/image1.png)

#   AdvancedExceptionHandling Sample

*This sample is compatible with the Microsoft Game Development Kit (June
2020)*

# Description

This sample shows several advanced ways handle exceptions that can occur
in your title.

-   Using a separate process to save crash dumps -- Demonstrates how to
    use a separate process to create crash dumps. This is the
    recommended pattern for creating crash dumps.

-   Adding custom data to Windows Error Reporting -- Demonstrates how to
    add data to the Windows Error Reporting system that is uploaded
    along with crash dumps to the Microsoft servers for later analysis.

-   Uploading Crash Dumps -- Demonstrates how to upload crash dumps to
    your own servers so they don\'t interfere with title execution and
    possibly cause more exceptions.

-   Handling Exceptions during Suspend/Resume (PLM) -- Demonstrates how
    to handle exceptions that happen during the PLM Suspend/Resume path.

-   Full Exception System -- Putting all the pieces together into a full
    exception system.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

If using the Desktop, set the active solution platform to `Gaming.Desktop.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

Press the corresponding button on the controller for each demonstration.
The display will show the order of operations that happen in the code
when an exception is raised.

# Implementation notes

All the examples are contained in the Examples folder. They are heavily
documented with details on each system and how they work.

# Update history

Initial release June 2021

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
