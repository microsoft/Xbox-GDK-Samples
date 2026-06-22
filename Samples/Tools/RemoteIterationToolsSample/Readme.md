# Remote Iteration Tools Sample

A WPF sample application demonstrating how to use the **Remote Iteration Client API**
(`Microsoft.GDK.RemoteIterationClientApi`) to deploy, launch, and manage game builds on
remote Xbox devices.

## What This Sample Demonstrates

- **Deploy** — Copy a local game build to a remote device using delta file transfer
- **Register** — Register a deployed game on the remote device
- **Launch** — Start an executable on the remote device (normal or suspended)
- **Resume** — Resume a suspended game process
- **Terminate** — Stop a running game on the remote device
- **Cancel** — Cancel an in-progress deployment

## Prerequisites

- Visual Studio with support for WPF applications
- .NET SDK installed
- Pair remote device using [Xbox PC Remote Tools overview](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/pc-dev/overviews/remote-win-gamedev)

## Build

Open `RemoteIterationToolsSample.sln` in Visual Studio and build the **x64** or **ARM64** configuration,
or from the command line:

```shell
dotnet build RemoteIterationToolsSample/RemoteIterationToolsSample.csproj -p:Platform=x64
dotnet build RemoteIterationToolsSample/RemoteIterationToolsSample.csproj -p:Platform=ARM64
```

> **Note:** The project references the `Microsoft.GDK.RemoteIterationClientApi` NuGet
> package.
