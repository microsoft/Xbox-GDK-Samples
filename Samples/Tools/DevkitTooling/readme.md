  ![](./media/image1.png)

#   DevkitTooling Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

Xbox devkit machines are powerful pieces of hardware that can be
utilized for more than simply running and testing game titles. An Xbox
Game Core application unlocks the full processing resources of the
devkit and can run Win32 applications that restrict their API usage to
the WINAPI_FAMILY_GAMES subset.

The DevkitTooling Sample shows how to launch CPU and GPU subprocesses
such that they can be run on Xbox Devkits as tools.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The DevkitTooling sample shows a single screen with logging output from
tools run as subprocesses.

![Text Description automatically generated](./media/image3.png)

To start an example CPU tool subprocess, press \[X\] on a gamepad. To
start an example GPU tool subprocess, press \[Y\] on a gamepad. The
example subprocesses will run for a total of 5 seconds before
automatically exiting. Their log output will be shown on screen.

When running the GPU tool subprocess, the screen will change to show the
rendering of the tool (a simple triangle) instead of the sample. Once
the GPU tool finished, the sample will begin rendering again. To support
this, rendering is suspended with *SuspendX* and resumed with *ResumeX.*
See implementation notes for more information.

![Shape Description automatically generated](./media/image4.png)

# Implementation notes

This sample uses some functionality that is uncommon with Xbox titles.
In addition, there are special requirements that must be respected to
keep the devkit from encountering unexpected errors. Each section below
talks through the different functionality, requirements, and other
considerations for running tools on a devkit.

## Xbox Title Partition

For an application running on an Xbox devkit to get full access to
hardware resources, it must run within the Title Partition. The Title
Partition is a virtual machine on the Xbox console specifically for this
purpose.

Normally, titles for an Xbox are deployed to a devkit or installed as a
package which causes a registration with the system. Then, the Xbox
system can start these titles on the Title Partition properly.

If you have a tool or other standalone process with no registration with
the system, you can still run these on the Title Partition. However, the
Title Partition has to be active first. The DevkitTooling application
takes care of this because the system will ensure that the Title
Partition is active when launching the sample. Then, DevkitTooling
launches the tool or process with
[CreateProcess](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa)
which will be within the Title Partition and have full hardware resource
access.

Note: The *xbrun.exe* tool can also launch applications in the Title
Partition. However, *xbrun.exe* does not ensure that the Title Partition
is active first. If a title is currently running on an Xbox devkit, then
the "*/x/title*" parameter to *xbrun.exe* specified that the executable
should be run in the title partition. Please see GDK documentation for
more information about *xbrun.exe*.

## Ports and the Xbox Firewall

Xbox consoles and Xbox devkits have a firewall that prevents most
unsolicited inbound connection requests by default for both TCP and UDP.
For retail titles, unsolicited inbound TCP is always blocked and there
is no way around this. UDP however is allowed to use the preferred local
UDP multiplater port which can be queried with
*XNetworkingQueryPreferredLocalUDPMultiplayerPort\[Async\]*.

Xbox devkits can be configured to allow unsolicited inbound TCP and UDP
packets via an entry in the MicrosoftGame.config file:

```
<?xml version="1.0" encoding="utf-8"?>
<Game configVersion="0">
...
  <DevelopmentOnly>
    <DebugNetworkPortList>
      <DebugNetworkPort>4600</DebugNetworkPort>
      <DebugNetworkPort>4601<DebugNetworkPort>
    </DebugNetworkPortList>
  </DevelopmentOnly>
</Game>
```

A different *DebugNetworkPort* entry should be added to the
*DebugNetworkPortList* for each port that will allow unsolicited inbound
connections. This is necessary for cases such as P2P tools that would
communicate with each other directly via TCP.

Once the MicrosoftGame.config file has been updated, the title should be
re-deployed to pickup on the updates.

## Win32 Applications on an Xbox Devkit

The Title Partition can run Win32 console applications directly that
haven't made any specific Xbox integrations. However, the Win32 API
availability is a stripped-down subset of what would normally be
available for Windows PCs.

To restrict a Win32 application to only the available APIs, you should
define the *WINAPI_FAMILY* to *WINAPI_FAMILY_GAMES* before including any
Windows headers.

```
#define WINAPI_FAMILY WINAPI_FAMILY_GAMES
#include <Windows.h>
```

The CPUTool project is a very simple example of this. It builds a
console Win32 application restricted to *WINAPI_FAMILY_GAMES.*
CPUTool.exe can be run on both Windows PCs and Xbox consoles as a result
successfully.

## GPU Processes on an Xbox Devkit

There are more heavy restrictions on D3D GPU usage with processes
running in the Title Partition:

* Only 1 process in the Title Partition can utilize Direct3D at a time.

* You can use the Xbox-specific D3D12 device methods *SuspendX/ResumeX* to manage multiple processes that utilize D3D. Two processes must not actively utilize D3D simultaneously.

* The Xbox-specific D3D headers and libraries must be used.

Improper handling of these considerations can cause your application to
crash, corrupt rendering, or even cause the console to crash. The
DevkitTooling sample handles these cases by using *SuspendX* before
creating a subprocess that utilizes the GPU. Once that subprocess has
exited fully, then *ResumeX* is used to re-enable the sample's
rendering.

The GPUTool project is a simple process that renders a triangle on the
screen. It's used to show how a GPU subprocess can be used in the Title
Partition when the GPU considerations are respected.

## GameInput with Multiple Processes

The GameInput library currently (at the creation of this sample) doesn't
support being used on more than 1 process at a time at the creation of
this sample. Additionally, it cannot be cleaned up and re-initialized
in-process or by terminating and starting the process. As a result, the
first process to initialize GameInput is the only process that can use
it. Any future process that tries to initialize will crash.

# Update history

Initial release June 2021.

# Privacy Statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
