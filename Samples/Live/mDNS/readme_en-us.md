  ![](./media/image1.png)

#   mDNS Sample

*This sample is compatible with the Microsoft GDKX (March 2022)*

# Description

This sample demonstrates using mDNS to register a game service and
broadcasting it across your local network, as well as demonstrating
network discovery & resolving.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using an Xbox Series X|S devkit, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

This sample does not utilize any Xbox Live functionality and should not
require the configuration of anything related to sandboxes or titles.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

When the sample is run, can perform the following actions:

-   **Register DNS:** Register local device for network discovery

-   **De-Register DNS:** De-register a previously created registration

-   **Start Browse:** Begin continuous network discovery of registered
    devices (including self)

-   **Stop Browse:** Cancel a previously started DNS browse.

Upon selecting an option, you will see output in the console showing the
result of the action requested.

When 'Start Browse' is selected, you will periodically see the endpoint
of discovered services which were resolved successfully.

This feature and sample function cross-device, for example, when running
the desktop equivalent of this sample, an Xbox and PC will be able to
discover each other.

# Update history

February 2020 - Initial release February 2020

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
