  ![](./media/image1.png)

#   DevkitToolLauncher Sample

*This sample is compatible with the Microsoft Game Development Kit (March 2022)*

# Description

Xbox devkit machines are powerful pieces of hardware that can be
utilized for more than simply running and testing game titles. An Xbox
Game Core application unlocks the full processing resources of the
devkit and can run Win32 applications that restrict their API usage to
the WINAPI_FAMILY_GAMES subset.

The DevkitToolLauncher Sample provides an Xbox application that
facilitates running processes in the devkit's title partition for
tooling purposes. In addition, two example processes are provided to
show how both Win32 console CPU-only processes and Xbox D3D12 GPU
processes can be used.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Project Scarlett, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

*For more information, see* __Running samples__, *in the GDK documentation.*

Several scripts have also been provided to facilitate command-line usage
of the sample, such as building, deploying, and running. To use a
script, open an "*Xbox \[One/Scarlett\] VS \[2017/2019\] Gaming Command
Prompt*" and then navigate to the sample directory. The scripts are
explained in the table below:

| Script          |  Usage                                              |
|-----------------|----------------------------------------------------|
| Build.bat  |  Build.bat \[Configuration\] \[Platform\]<br /><br />Builds DevkitToolLauncher.exe, CPUTool.exe, andGPUTool.exe for the specified Configuration(Debug, Release) and Platform (XboxOne, Scarlett). CPUTool.exe is always built with the x64 configuration as a simple console Win32 application.                                       |
| Deploy.bat  |  Deploy.bat \[Platform\]<br /><br />Deploys the loose build of DevkitToolLauncher.exethat was previously built to the default devkit.   |
| DeployExampleTools.bat  | DeployExampleTools.bat \[Configuration\]\[Platform\]<br /><br />Copies CPUTool.exe and GPUTool.exe to theSystemScratch drive on the default devkit. Theseapplications are deployed to the"d:\\DevkitToolLauncherExampleTools\\\[CPU/GPU\]Tool"folder.                                            |
| BuildAndDeploy.bat  | BuildAndDeploy.bat \[Configuration\] \[Platform\]<br /><br />Combines Build.bat, Deploy.bat, andDeployExampleTools.bat into one script.            |
| Run.bat  | Run.bat \[CommandLine\]<br /><br />Launches a previously-deployedDevkitToolLauncher.exe and passes all of thescript's parameters to the command line forDevkitToolLauncher.exe. See the [Command LineUsage](#Command_Line_Usage) section below for more information on using the command line parameters.  |

# Using the sample

The sample provides 3 processes: DevkitToolLauncher.exe, CPUTool.exe,
and GPUTool.exe. DevkitToolLauncher.exe is the main sample process and
the other two processes are provided as an example of CPU-only and
GPU-utilizing processes that can be run by DevkitToolLauncher.exe.

## Sample Functionality

The DevkitToolLauncher process is split into 3 screens: Tool Browser,
Launch Settings, and Runtime Log. The different screens allow you to
find a process to run, setup that process's startup information, and
view the console output of the running process.

### Tool Browser

![Text Description automatically generated](./media/image3.png)

The Tool Browser provides a file browser interface over the System
Scratch drive (d:\\) of the devkit. Using this browser, you can navigate
to an executable to run and select it. After selecting the executable,
the sample proceeds to the "*Launch Settings"* screen.

### Launch Settings

![Graphical user interface, text Description automatically generated](./media/image4.png)

The "*Launch Settings*" screen allows for setting up the launch behavior
of the selected tool executable. Using this screen, you can setup
command-line parameters, the working directory, and whether the sample
should launch with or without GPU support.

Selecting any of the text inputs will bring up an on-screen keyboard to
enter the parameters. The commandline parameters and the working
directory are saved to a cache file at the root of the System Scratch
drive called "DevkitToolLauncherParameterCache.json". You can delete
this cache to reset saved parameters.

To support tools that have GPU functionality, the "*Launch (CPU & GPU)"*
button will suspend rendering on the sample before launching the tool.
Once the tool executable exits or is terminated, rendering will
automatically be spun up again. See the [GPU Processes on an Xbox
Devkit](#_Hlk70930525) section for more information on how GPU processes
are supported by this sample.

Selecting *"Launch (CPU Only)"* will progress to the "*Runtime Log*"
screen without suspending rendering.

If the selected tool executable has GPU functionality and "*Launch (CPU
Only)*" was selected, the console will encounter undefined behaviorial
issues as a result of two processes trying to use the GPU. For example,
the application may crash, the screen may see artifacts, or the console
may shut down. See [GPU Processes on an Xbox Devkit](#_Hlk70930525) for
more information.

If the tool executable being run needs to allow unsolicited inbound
connections, then those ports have to be opened via the
MicrosoftGame.config file. See [Ports and the Xbox
Firewall](#Ports_Firewall) for more information.

### Runtime Log

![Text Description automatically generated](./media/image5.png)

The Runtime Log screen is the final screen that is brought up when
selecting one of the launch options on the "*Launch Settings*" screen.

This screen will start the selected executable as a sub-process and pipe
its stdout and stderr writes to the DevkitToolLauncher process. These
piped outputs are displayed on the screen. You can navigate the
on-screen log with a gamepad.

If the tool subprocess uses the GPU and rendering was suspended via the
"*Launch (CPU & GPU)*" button in the "*Launch Settings*" screen, then
this screen will not be shown. However, the sample is still piping the
outputs and managing the running process. When the tool subprocess
exits, this screen will be brought up automatically with all the logging
that happened.

You can always terminate the tool subprocess by holding \[B\] on a
gamepad regardless of whether rendering was suspended or not. Once the
tool subprocess is no longer running, you can press \[B\] to return the
the "*Tool Browser*" screen and start over.

## Command Line Usage

It's not required to use the provided scripts or the command-line to use
the sample. However, they were provided to show how tools can be
launched via the command-line using the DevkitToolLauncher sample.

To begin, open one of the Xbox Gaming Command Prompts. Then, navigate to
the sample directory with the "*cd /D \[SamplePath\]*" command.

### Building

Building can be done using the Build.bat script provided in the sample's
directory. The command line parameters are:

```
Build.bat [Configuration] [Platform]
```

For example, to build for Scarlett, use the command "*build.bat release
scarlett*".

### Deploying

To deploy, another script has been provided called Deploy.bat. Deploying
the DevkitToolLauncher sample will internally use the "*xbapp deploy*"
command to copy the loose build over to the devkit.

```
Deploy.bat [Platform]
```

To deploy GPUTool and CPUTool, use the DeployExampleTools.bat script.
This script copies those example tools to the SystemScratch drive in a
folder called "DevkitToolLauncherExampleTools". These tools are deployed
to the SystemScratch drive to allow them to be discovered by the *Tool
Browser* screen of the sample instead of being within the Sample's main
install folder.

```
DeployExampleTools.bat [Configuration] [Platform]
```

### Building & Deploying in One Step

The BuildAndDeploy.bat script combines the three scripts above into one
script for simplicity.

BuildAndDeploy.bat \[Configuration\] \[Platform\]

### Running

DevkitToolLauncher.exe contains a command line setup to allow skipping
the "*Tool Browser*" and "*Launch Settings*" screens, launching a tool
process, and navigate directly to the "*Runtime Log*" screen. The
command-line setup is as follows:

DevkitToolLauncher.exe \[-gpu/processUsesGpu\] \[-workingDir \"path\"\]
\[\-- \"process\" \[commandline\]\]

This command line allows you to set the same information that you could
set on the "*Launch Settings"* screen:

| Parameter      |  Description                                         |
|----------------|-----------------------------------------------------|
| -gpu or - processUsesGpu |  Optional. If specified, the DevkitToolLauncher.exe sample will suspend rendering for the duration of the subprocess. |
| -workingDir  |  Required if specifying a process to run. Specify the working directory to be set for the subprocess.                                         |
| \--  |  Optional. If the "\--\" token appears on the command line, then the sample interprets the following parameter as the subprocess to run. The subprocess must be a full path to a process. Every parameter after the process is passed on as the command line for that subprocess.                                         |

For example, to run the example CPUTool that was copied to the System
Scratch drive with DeployExampleToole.bat for 5 seconds, you would use
this command:

```
Run.bat -workingDir "d:\DevkitToolLauncherExampleTools\CPUTool" --"d:\DevkitToolLauncherExampleTools\CPUTool\CPUTool.exe" 5
```

To run the example GPUTool that has rendering, you would use this
command:

```
Run.bat -gpu -workingDir "d:\DevkitToolLauncherExampleTools\GPUTool"
-- "d:\DevkitToolLauncherExampleTools\GPUTool\GPUTool.exe"
```

Note: The GPUTool.exe process renders a triangle on the screen and
doesn't exit automatically. You can terminate GPUTool.exe (when launched
by DevkitToolLauncher.exe) by holding \[B\] on a gamepad.

### Terminating

The Run.bat command line starts the DevkitToolLauncher sample using
*xbapp* and passes a command-line. If you want to terminate the sample
from the command line, you can run "*xbapp terminate*".

## Your Own Tools

The DevkitToolLauncher sample is intended to be a complete hosting
offering for running your own tools as well as an education resource on
how to use the devkit for such cases. If you have your own tools or plan
on writing some, you can use the Run.bat steps above to host them.

See [Win32 Applications on an Xbox Devkit](#Win32_Info) for more
information about console application that can be run as tools. See [GPU
Processes on an Xbox Devkit](#_Hlk70930525) for more information about
tools that can utilize the GPU of a devkit.

You will need to place your tool on the System Scratch drive of the
devkit to be able to be hosted by DevkitToolLauncher. To do this, you
can use the "*xbcp.exe*" tool:

```
xbcp [ToolLocationPathOnLocalPC] xd:\[Folder]
```

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
Title Partition has to be active first. The DevkitToolLauncher
application takes care of this because the system will ensure that the
Title Partition is active when launching DevkitToolLauncher.exe. Then,
DevkitToolLauncher launches the tool or process with
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
      <DebugNetworkPort>4601</DebugNetworkPort>
    </DebugNetworkPortList>
  </DevelopmentOnly>
</Game>
```

A different *DebugNetworkPort* entry should be added to the
*DebugNetworkPortList* for each port that will allow unsolicited inbound
connections. This is necessary for cases such as P2P tools that would
communicate with each other directly via TCP.

Once the MicrosoftGame.config file has been updated, the title should be
re-deployed to pickup on the updates. A simple way to do this for the
DevkitToolLauncher Sample would be to run the *BuildAndDeploy.bat*
script again.

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
#include <Wwindows.h>
```

The CPUTool project is a very simple example of this. It builds a
console Win32 application restricted to *WINAPI_FAMILY_GAMES.*
CPUTool.exe can be run on both Windows PCs and Xbox consoles as a result
successfully.

Note: These application should be console applications only and not use
rendering libraries. For rendering, Xbox D3D12 can be used, explained in
the next section.

## GPU Processes on an Xbox Devkit

There are more heavy restrictions on D3D GPU usage with processes
running in the Title Partition:

* Only 1 process in the Title Partition can utilize Direct3D at a time.
* You can use the Xbox-specific D3D12 device methods *SuspendX/ResumeX* to manage multiple processes that utilize D3D. Two processes must not actively utilize D3D simultaneously.
* The Xbox-specific D3D headers and libraries must be used.

Improper handling of these considerations can cause your application to
crash, corrupt rendering, or even cause the console to crash. The
DevkitToolLauncher sample handles these cases by using *SuspendX* before
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

The DevkitToolLauncher sample uses GameInput to provide input handling
and as a result, tool subprocesses cannot currently utilize GameInput.

## Sample CPU Usage Overhead

In general, the DevkitToolLauncher sample has very low CPU overhead. It
ensures to keep its usage low by limiting frame rate to 30FPS with
[Sleep](https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep).
Most of the sample functionality when it's not sleeping is very quick.
However, the rendering of UITK in the Debug configuration can
potentially be problematic if the a tool subprocess needs to get as much
processing time of all CPU cores as possible.

To get the best tool subprocess performance in a Debug configuration,
you can use the "*Launch (CPU & GPU)*" (or -gpu when using the command
line) with any process, including non-gpu processes. Since this suspends
the sample rendering, the overhead of the DevkitToolLauncher sample is
mostly removed.

The performance impact of UITK is also effectively removed when building
with the Release configuration.

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
