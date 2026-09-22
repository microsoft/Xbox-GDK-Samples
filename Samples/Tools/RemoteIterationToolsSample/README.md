<!-----
---
page_type: sample
languages:
- csharp
products:
- gdk
urlFragment: "remoteiterationtoolssample"
extendedZipContent:
- path: LICENSE
  target: LICENSE
- path: Kits
  target: Kits
- path: Media
  target: Media
description: "A WPF sample application demonstrating how to use the Remote Iteration Client API"
---
----->

# Remote Iteration Tools Sample

A WPF sample application demonstrating how to use the **Xbox PC Remote Iteration API**
(`Microsoft.GDK.RemoteIterationClientApi`) to deploy, launch, and manage game builds on
remote development devices.

## What This Sample Demonstrates

- **Deploy** — Copy a local game build to a remote device using delta file transfer
- **Validate remote path** — After a successful, uncancelled copy, call
  `WdRegisterRemoteXboxGame` to resolve and validate the remote folder path.
  Despite its name, this API does not perform platform game registration.
- **Launch** — Start an executable on the remote device (normal or suspended)
- **Resume** — Resume a suspended game process
- **Terminate** — Stop the last game launched through the API on the remote device
- **Cancel** — Request cancellation of an in-progress copy, then wait for its result

The deploy and game-control actions are implemented in
[`MainWindow.xaml.cs`, lines 159–393](MainWindow.xaml.cs#L159-L393).
See also the [path-validation wrapper, lines 60–80](RemoteIteration.cs#L60-L80).

## Prerequisites

- Windows with Visual Studio WPF support, or the .NET 8 SDK for command-line builds
  ([project settings, lines 3–16](RemoteIterationToolsSample.csproj#L3-L16))
- The `Microsoft.GDK.RemoteIterationClientApi` NuGet package version referenced
  by the project, restored during the build
  ([package references, lines 30–37](RemoteIterationToolsSample.csproj#L30-L37))
- A compatible Remote Tools endpoint running on a target device paired with the
  development PC. Follow the [Xbox PC Remote Tools setup and pairing guide](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/pc-dev/overviews/remote-win-gamedev).

## Build

Open `RemoteIterationToolsSample.sln` in Visual Studio and build the **x64** or
**ARM64** configuration, or run these commands from this sample's directory:

```shell
dotnet build .\RemoteIterationToolsSample.csproj -c Release -p:Platform=x64
dotnet build .\RemoteIterationToolsSample.csproj -c Release -p:Platform=ARM64
```

The API NuGet package supplies the native binary and WinMD metadata.
CsWin32 generates the C# bindings from that metadata
([project, lines 20–37](RemoteIterationToolsSample.csproj#L20-L37)).

## Using the sample

The three numbered groups in the UI cover device selection, deployment, and game
control. These are separate actions: **Deploy does not launch the game, and Launch
does not deploy it**. Use your own paired device and a build containing a runnable
executable and its dependencies.

### 1. Choose a paired device

Complete the [Remote Tools setup and pairing workflow](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/pc-dev/overviews/remote-win-gamedev)
first, with the Remote Tools endpoint running on the target. The sample does not
discover, provision, or pair devices.

Enter the target's **IP address or resolvable host name** in **Device**, for example
`192.168.1.100` or `MyDevPC`. This field identifies the remote machine, not a local
folder or executable ([device guidance, lines 132–138](MainWindow.xaml#L132-L138)).

### 2. Deploy a local build

Suppose your local build contains `C:\Builds\MyGame\MyGame.exe`. Use a disposable
remote destination while learning the sample.

| UI field | Example value | Meaning |
|----------|---------------|---------|
| Device | `MyDevPC` | The paired target machine |
| Source Folder (this PC) | `C:\Builds\MyGame` | Local build folder; **Browse** selects a folder on this PC |
| Destination Folder (device) | `MyGame` | Remote folder relative to the endpoint's default Common Root |
| Executable to launch (device) | `MyGame\MyGame.exe` | Remote executable to use in step 3 |

The paths connect like this:

```text
This PC                              Paired target device
C:\Builds\MyGame\MyGame.exe  --Deploy-->  <default Common Root>\MyGame\MyGame.exe

Destination Folder: MyGame
Executable to launch: MyGame\MyGame.exe
```

Relative destination and executable paths start at the endpoint's **default Common
Root**. Absolute paths on the target, such as `D:\Games\MyGame` and
`D:\Games\MyGame\MyGame.exe`, can be used instead. The UI does not expose a Common Root
alias. **Executable to launch is not automatically relative to
Destination Folder**: entering just `MyGame.exe` does not prepend `MyGame`.
The sample passes the destination and executable as separate API inputs
([deployment inputs, lines 164–188](MainWindow.xaml.cs#L164-L188);
[launch inputs, lines 321–338](MainWindow.xaml.cs#L321-L338);
[destination guidance, lines 154–162](MainWindow.xaml#L154-L162);
[executable guidance, lines 217–227](MainWindow.xaml#L217-L227)).

Leave **Advanced Options** blank for the first deployment. Empty patterns use the
default selection and blank attribute masks mean zero. Choose **Deploy**. It calls
`WdRemoteCopy` and, only after successful copy without cancellation or a callback
processing failure, `WdRegisterRemoteXboxGame` to validate the remote folder path.
This is **not game installation or platform registration**.

Wait for **"Deployment completed; remote path validated."** in the status area.
Use **Command Output** to inspect the copy and path-validation HRESULTs or failure
messages; progress reaching 100% is not the authoritative result
([deployment result handling, lines 209–252](MainWindow.xaml.cs#L209-L252)).

During a copy, choose **Cancel** to request cancellation and wait for the operation
to finish. Cancellation does not establish
that the build is complete, even when the copy's HRESULT indicates success; the
sample skips path validation in that case
([cancellation, lines 280–307](MainWindow.xaml.cs#L280-L307);
[cancelled-copy result, lines 226–231](MainWindow.xaml.cs#L226-L231)).

### 3. Launch and control the remote game

After deployment, enter **`MyGame\MyGame.exe`** in **Executable to launch** for this
example. You may instead launch a build already present on the device.

Choose **Launch** to start the executable normally. Alternatively, choose
**Launch Suspended** to start it suspended, then **Resume** to let it run.
Check **Command Output** for the result and returned process/thread IDs.
Choose **Terminate** to stop the game.

Resume operates on the last game launched suspended through `WdLaunchRemoteGame`;
Terminate operates on the last game launched through that API. These buttons use
**Device**, not the executable-path field, and do not accept an arbitrary process ID
([launch actions, lines 309–350](MainWindow.xaml.cs#L309-L350);
[resume/terminate, lines 353–390](MainWindow.xaml.cs#L353-L390);
[native wrappers, lines 82–144](RemoteIteration.cs#L82-L144)).

Each new action clears **Command Output**. Copy any diagnostics you want to keep
before starting the next action
([operation start, lines 36–45](MainWindow.xaml.cs#L36-L45)).

### Common Root configuration

A **Common Root** is a named base directory on the target device for relative
remote paths. If no roots are configured, the default is
`%ProgramData%\Microsoft GDK\gameroot`. This sample uses the endpoint's default
Common Root; it does not configure roots.

To customize the roots, create or edit
`%ProgramData%\Microsoft GDK\wdEndpoint\wdEndpoint.json` **on the target device**.
Merge the `gameRoots` setting into any existing configuration, preserving other
settings. For example:

```json
{
  "gameRoots": [
    {
      "root": "D:\\Games",
      "alias": "games",
      "default": true
    }
  ]
}
```

Restart `wdEndpoint` after changing the configuration. With this example, the
sample's relative destination `MyGame` resolves to `D:\Games\MyGame`.
Additional entries define other aliases; mark the intended default with
`"default": true`. See the [Common Root configuration guide](https://learn.microsoft.com/en-us/gaming/gdk/docs/tools/tools-pc/commandlinetools/gr-wdremote#deploy,-launch,-and-terminate-applications)
for details.

## From UI action to API call

Use the walkthrough to observe the behavior, then follow these entry points to
see the integration code:

| UI action | Handler | Managed wrapper and native API |
|-----------|---------|--------------------------------|
| Deploy: copy | [`Deploy_Click`, lines 159–210](MainWindow.xaml.cs#L159-L210) | [`CopyAsync` → `WdRemoteCopy`, lines 16–58](RemoteIteration.cs#L16-L58) |
| Deploy: validate remote path | [`Deploy_Click`, lines 238–246](MainWindow.xaml.cs#L238-L246) | [`RegisterRemoteXboxGameAsync` → `WdRegisterRemoteXboxGame`, lines 60–80](RemoteIteration.cs#L60-L80) |
| Cancel | [`Cancel_Click`, lines 280–307](MainWindow.xaml.cs#L280-L307) | [`RequestCopyCancellation` → `WdCancelRemoteCopy`, lines 148–156](RemoteIteration.cs#L148-L156) |
| Launch / Launch Suspended | [`LaunchProcessAsync`, lines 309–350](MainWindow.xaml.cs#L309-L350) | [`LaunchRemoteGameAsync` → `WdLaunchRemoteGame`, lines 101–131](RemoteIteration.cs#L101-L131); suspended launch supplies `WdLaunchMode.Suspended` |
| Resume | [`Resume_Click` / `ControlGameAsync`, lines 353–390](MainWindow.xaml.cs#L353-L390) | [`ResumeGameAsync` → `WdResumeRemoteGame`, lines 133–144](RemoteIteration.cs#L133-L144) |
| Terminate | [`Terminate_Click` / `ControlGameAsync`, lines 358–390](MainWindow.xaml.cs#L358-L390) | [`TerminateRemoteGameAsync` → `WdTerminateRemoteGame`, lines 82–94](RemoteIteration.cs#L82-L94) |

For progress and diagnostic callbacks, start with
[`OperationState.CreateCopyCallbacks`, lines 79–90](OperationState.cs#L79-L90)
and [UI processing, lines 82–157](MainWindow.xaml.cs#L82-L157).

## Implementation notes

- File-attribute masks accept decimal `uint` values (`0` through `4294967295`).
  Blank means zero. Malformed and out-of-range values are rejected before work
  starts
  ([input validation, lines 173–175](MainWindow.xaml.cs#L173-L175);
  [mask parsing, lines 269–277](MainWindow.xaml.cs#L269-L277)).
- Directory exclusions match **names, not paths**, and exclusions take precedence
  over inclusions ([filter tooltips, lines 174–197](MainWindow.xaml#L174-L197)).
  The managed search model also includes `IncludeDirectoryAttributes` and
  `ExcludeDirectoryAttributes`, both zero by default and not exposed in the UI.
  All seven search fields are passed to the API
  ([model, lines 3–13](CopySearchOptions.cs#L3-L13);
  [mapping, lines 36–42](RemoteIteration.cs#L36-L42)).
- Only one action runs at a time. Device/path/filter inputs and conflicting
  actions are disabled until it returns. Closing the window is blocked while
  native work is active. If an operation appears stalled, choose **Cancel** when
  available and wait for it to return. If it remains unresponsive, end
  `RemoteIterationToolsSample.exe` in Task Manager as a last resort. Force-closing
  does not undo completed transfers or confirm that the remote operation stopped
  ([operation lifetime and closing guidance, lines 36–81](MainWindow.xaml.cs#L36-L81)).
- Cancellation is a request, not confirmation that the build is complete.
  The sample records the request separately from the copy's HRESULT and skips
  remote-path validation when cancellation was requested, even if copy returns
  success. Copy failures are still reported
  ([copy result, lines 209–237](MainWindow.xaml.cs#L209-L237);
  [cancellation, lines 280–307](MainWindow.xaml.cs#L280-L307)).
- Native callbacks copy progress and diagnostic data into operation-owned queues
  rather than updating WPF controls directly. The UI consumes bounded batches to
  stay responsive and drains remaining output before showing the final result.
  Callback-processing failures are reported separately; returning a failed
  callback HRESULT does not abort native copy
  ([bounded queue consumption, lines 38–77](OperationState.cs#L38-L77);
  [callback ownership and capture, lines 79–176](OperationState.cs#L79-L176);
  [UI processing, lines 82–157](MainWindow.xaml.cs#L82-L157)).
