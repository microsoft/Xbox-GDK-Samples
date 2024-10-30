# escape=`

# This Dockerfile sets up an environment that can build the BWOIExample project, or other GDK projects that are configured
# to build without installing the GDK.
# This file should be placed in a parent directory containing both an extracted SDKs directory and a script directory.
# The script directory should contain the setenv.cmd and vctargets.cmd scripts from the BWOIExample sample.

# To build the container (change arguments as necessary):
#    docker build -t gdkbwoi:latest -m 2GB --build-arg ExtractedSDKDir="sdks" --build-arg ScriptDir="BWOIExample" --build-arg GDKVer="240300" .
# To run the container and invoke a build:
# Using cmd.exe:
#    docker run --rm -v %cd%\BWOIExample:c:\Project -w c:\Project gdkbwoi msbuild BWOIExample.vcxproj /p:Configuration=Debug /p:Platform=Gaming.Xbox.Scarlett.x64
# Using PowerShell:
#    docker run --rm -v ${pwd}\BWOIExample:c:\Project -w c:\Project gdkbwoi msbuild BWOIExample.vcxproj /p:Configuration=Debug /p:Platform=Gaming.Xbox.Scarlett.x64
# See the readme for more details.

# Use the latest Windows Server Core image with .NET Framework 4.8.
FROM mcr.microsoft.com/dotnet/framework/sdk:4.8-windowsservercore-ltsc2022
ARG ExtractedSDKDir=sdks
# ScriptDir is the directory containing the setenv.cmd and vctargets.cmd scripts from the sample
ARG ScriptDir=BWOIExample
ARG GDKVer=240300

# Restore the default Windows shell for correct batch processing.
SHELL ["cmd", "/S", "/C"]

RUN `
    # Download the VS 2022 Build Tools bootstrapper.
    # This could also be changed to copy a cached version from an internal server.
    curl -SL --output vs_buildtools.exe https://aka.ms/vs/17/release/vs_buildtools.exe `
    `
    # Install Build Tools with the workloads needed for the GDK, explicitly excluding workloads and components with known issues.
    # See https://docs.microsoft.com/en-us/visualstudio/install/build-tools-container for more information.
    && (start /w vs_buildtools.exe --quiet --wait --norestart --nocache modify `
        --installPath "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\BuildTools" `
        --add Microsoft.VisualStudio.Workload.VCTools `
        --add Microsoft.VisualStudio.Component.Windows11SDK.22000 `
        --remove Microsoft.VisualStudio.Component.Windows10SDK.10240 `
        --remove Microsoft.VisualStudio.Component.Windows10SDK.10586 `
        --remove Microsoft.VisualStudio.Component.Windows10SDK.14393 `
        --remove Microsoft.VisualStudio.Component.Windows81SDK `
        # The below line is needed to build VS 2022 projects with MSVC or Clang.
        --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        # The below line is only needed to build VS 2019 projects with MSVC or Clang.
        --add Microsoft.VisualStudio.ComponentGroup.VC.Tools.142.x86.x64 `
        # The below line is only needed to build VS 2017 projects.
        --add Microsoft.VisualStudio.Component.VC.v141.x86.x64 `
        # The below line is only needed to build with Clang.
        --add Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang `
        || IF "%ERRORLEVEL%"=="3010" EXIT 0) `
    `
    # Cleanup
    && del /q vs_buildtools.exe

COPY [ "$ExtractedSDKDir", "c:/sdks" ]
COPY [ "${ScriptDir}/*.cmd", "c:/scripts/" ]

# Merge the VCTargets
WORKDIR c:/scripts
ENV VSInstallDir="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\"
RUN `
    c:\scripts\setenv.cmd vs2022 %GDKVer% c:\sdks & `
    c:\scripts\vctargets.cmd

# This entrypoint sets the environment variables necessary to build.
# Using cmd /k leaves the container running with the command prompt active. Otherwise, it will exit after running the command.
ENV GDKVer=${GDKVer}
ENTRYPOINT ["C:\\scripts\\setenv.cmd", "vs2022", "%GDKVer%", "c:\\sdks", "&&", "powershell.exe", "-NoLogo", "-ExecutionPolicy", "Bypass"]
