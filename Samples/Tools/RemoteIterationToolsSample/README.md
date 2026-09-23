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

## Description

A C# WPF sample for tool developers integrating the **Xbox PC Remote Iteration
API**. Use it to deploy a build, launch and control a game, and delete remote
content. The UI provides a way to exercise the API; the [code map](#code-map)
points to the integration patterns.

## Building the Sample

Use Windows with Visual Studio WPF support or the .NET 8 SDK. Open
`RemoteIterationToolsSample.sln` and select **x64** or **ARM64**, or build from
this directory:

```shell
dotnet build .\RemoteIterationToolsSample.csproj -c Release -p:Platform=x64
```

Use `-p:Platform=ARM64` for ARM64. Run the executable under
`bin\<platform>\Release\net8.0-windows`, or start the project from Visual Studio.
The project restores the matching API NuGet package and generates C# bindings
from its WinMD metadata using CsWin32.

## Using the Sample

First follow the [Remote Tools setup and pairing guide](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/pc-dev/overviews/remote-win-gamedev).
The endpoint must be running on a paired target; the sample does not pair devices.

### Deploy and launch

For a local build containing `C:\Builds\MyGame\MyGame.exe`, enter:

| Field | Example |
|-------|---------|
| Device | `MyDevPC` (a resolvable host name or IP address) |
| Source Folder (this PC) | `C:\Builds\MyGame` |
| Destination Folder (device) | `MyGame` |
| Executable to launch (device) | `MyGame\MyGame.exe` |

A **Common Root** is the endpoint's base directory for relative remote paths.
With the default configuration, this example copies to:

```text
C:\Builds\MyGame\MyGame.exe
    --Deploy--> <default Common Root>\MyGame\MyGame.exe
```

Destination and executable paths resolve independently from that root, not from
each other. Absolute device paths also work. See [Common Root configuration](#common-root-configuration)
to change the base directory.

1. Leave **Advanced Options** at their defaults and choose **Deploy**. Wait for
   **"Deployment completed; remote path validated."** The validation API,
   `WdRegisterRemoteXboxGame`, checks the remote path; it does not install or
   register a game.
2. Choose **Launch**, or **Launch Suspended** followed by **Resume**. Deployment
   and launch are independent actions.
3. Choose **Terminate** to stop the last game launched through the API on that
   device. Resume also operates on the last suspended launch, not an arbitrary PID.

Read final results and HRESULTs in the status area and **Command Output**, not
just the progress indicator. Each new action clears the output.

### Filters

**Advanced Options** filters deployment; **Delete filters** applies to folder
deletion. Name patterns accept `*` and `?`; file patterns can be separated with
`;`. Directory exclusions match names, not paths. Exclusions take precedence.

Attribute checkboxes select **Read-only**, **Hidden**, **System**, and **Archive**.
An include group matches any selected attribute; an exclude group rejects any
selected attribute. No selections means no attribute filtering. For example,
checking **Hidden** and **System** under **Exclude file attributes** excludes
either kind of file. The sample combines their flags into mask `6`, preserving
the API's `uint` copy fields and `ulong` delete fields.

See [WdCopySearchOptions](https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/remoting/structs/wdcopysearchoptions)
and [WdDeleteSearchOptions](https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/remoting/structs/wddeletesearchoptions)
for filtering semantics. [File Attribute Constants](https://learn.microsoft.com/en-us/windows/win32/fileio/file-attribute-constants)
explains the flags; RIT supports the four offered here, not every Windows attribute.

### Delete remote content

Enter a remote path in **Remote deletion**, then choose **Delete...** and review
the confirmation. Root-folder removal is off by default and only succeeds if the
folder is left empty. Deletion is separate from Deploy.

Use disposable data without directory junctions or symbolic links: deletion has
no undo, and linked directories can lead outside the target. Prepare these
example folders on the device, then inspect the remaining contents:

| Scenario | Remote path | Settings |
|----------|-------------|----------|
| Clear folder contents | `RitDeleteFixture\clear` | Leave filters at defaults. Check **Remove root folder if empty** only to remove the folder too. |
| Clean selected artifacts | `RitDeleteFixture\artifacts` | Include `*.log;*.tmp`, exclude `keep.log`, exclude directory `keep`. |
| Delete one file | `RitDeleteFixture\one\trace.log` | Filters and root-removal settings are ignored for a single-file target. |

The result **"Delete request completed; some items may remain."** reflects
best-effort deletion, not a guarantee of an empty folder. There is no per-item
progress. See [WdDeleteRemoteFiles](https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/remoting/functions/wddeleteremotefiles)
for the full contract.

### Cancellation and operation lifetime

**Cancel** requests copy cancellation. **Stop waiting** ends the client wait for
delete; remote deletion may continue. Controls remain disabled until the client
call returns. No follow-on operation starts automatically. The delete status
describes the previous request and remains until another delete starts.

The sample tracks cancellation intent separately from HRESULT: a success return
after cancellation does not prove a complete copy or delete. Cancelled copies
skip path validation. A tool using delete must decide when conflicting remote
work is safe; see [WdCancelRemoteDelete](https://learn.microsoft.com/en-us/gaming/gdk/docs/reference/remoting/functions/wdcancelremotedelete).

Closing is blocked during an active call to preserve native resource lifetime.
For a stalled operation, request cancellation when available. Ending
`RemoteIterationToolsSample.exe` in Task Manager is a last resort; it does not
undo completed work or confirm that the remote operation stopped.

### Common Root configuration

Without configured roots, relative paths use `%ProgramData%\Microsoft GDK\gameroot`.
Deploy and Launch use the default Common Root; Delete can select an existing
alias. Absolute remote paths ignore the alias.

On the **target device**, add `gameRoots` to
`%ProgramData%\Microsoft GDK\wdEndpoint\wdEndpoint.json`, preserving other settings:

```json
{
  "gameRoots": [
    { "root": "D:\\Games", "alias": "games", "default": true }
  ]
}
```

Restart the endpoint after editing. With this configuration, `MyGame` resolves to
`D:\Games\MyGame`. Add entries for other aliases as needed. The sample selects roots;
it does not configure them. See the [Common Root configuration guide](https://learn.microsoft.com/en-us/gaming/gdk/docs/tools/tools-pc/commandlinetools/gr-wdremote#deploy,-launch,-and-terminate-applications).

## Implementation notes

### Code map

| Pattern | Entry points |
|---------|--------------|
| Package and generated bindings | [Project](RemoteIterationToolsSample.csproj#L3-L37), [projection requests](NativeMethods.txt) |
| Deploy and validate the remote path | [`Deploy_Click`](MainWindow.xaml.cs#L162-L266), [`CopyAsync` / `RegisterRemoteXboxGameAsync`](RemoteIteration.cs#L16-L80) |
| Launch, resume, terminate | [UI handlers](MainWindow.xaml.cs#L297-L381), [native wrappers](RemoteIteration.cs#L82-L144) |
| Delete and stop waiting | [UI handlers and confirmation](MainWindow.Delete.xaml.cs#L8-L138), [`DeleteAsync` / `RequestDeleteStopWaiting`](RemoteIteration.Delete.cs#L10-L62) |
| Attribute flags | [`FileAttributeSelector.Mask`](FileAttributeSelector.xaml.cs#L22-L37), [copy model](CopySearchOptions.cs#L3-L13), [delete model](DeleteSearchOptions.cs#L3-L13) |
| Single active operation and handle lifetime | [`BeginOperation` / `EndOperation` / closing guard](MainWindow.xaml.cs#L36-L83), [operation state](OperationState.cs#L10-L188) |
| Copy cancellation and final results | [Result handling](MainWindow.xaml.cs#L208-L263), [`Cancel_Click`](MainWindow.xaml.cs#L268-L295) |
| Responsive callback processing | [Bounded UI draining](MainWindow.xaml.cs#L85-L160), [callback capture and ownership](OperationState.cs#L42-L188) |

Callbacks copy native data into operation-owned queues; the UI drains bounded
batches and yields between them. Callback-processing failures are reported
separately from the native HRESULT. A failed callback return does not abort copy.
