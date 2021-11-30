# escape=`

# This Dockerfile sets up an environment that can build the BWOIExample project, or other GDK projects that are configured
# to build without installing the GDK.
# This file should be placed in a parent directory containing both an extracted SDKs directory and the project directory.
# To build the container (change arguments as necessary):
#    docker build -t gdkbwoi:latest -m 2GB --build-arg ExtractedSDKDir="sdks" --build-arg ProjectDir="BWOIExample" --build-arg GDKVer="211000" .
# To run the container:
#    docker run -it gdkbwoi
# See the readme for more details.

# Use the latest Windows Server Core image with .NET Framework 4.8.
FROM mcr.microsoft.com/dotnet/framework/sdk:4.8-windowsservercore-ltsc2019
ARG ExtractedSDKDir=sdks
ARG ProjectDir=BWOIExample
ARG GDKVer=211000
# The below arguments can optionally be used when invoking a build automatically. See the comment at the end of the file.
ARG ProjectName=BWOIExample
ARG BuildPlatform=Gaming.Xbox.Scarlett.x64
ARG BuildConfig=Debug

# Restore the default Windows shell for correct batch processing.
SHELL ["cmd", "/S", "/C"]

RUN `
    # Download the VS 2019 Build Tools bootstrapper.
    # This could also be changed to copy a cached version from an internal server.
    curl -SL --output vs_buildtools.exe https://aka.ms/vs/16/release/vs_buildtools.exe `
    `
    # Install Build Tools with the workloads needed for the GDK, explicitly excluding workloads and components with known issues.
    # See https://docs.microsoft.com/en-us/visualstudio/install/build-tools-container for more information.
    && (start /w vs_buildtools.exe --quiet --wait --norestart --nocache modify `
        --installPath "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\BuildTools" `
        --add Microsoft.VisualStudio.Workload.VCTools `
        --add Microsoft.VisualStudio.Component.Windows10SDK.19041 `
        --remove Microsoft.VisualStudio.Component.Windows10SDK.10240 `
        --remove Microsoft.VisualStudio.Component.Windows10SDK.10586 `
        --remove Microsoft.VisualStudio.Component.Windows10SDK.14393 `
        --remove Microsoft.VisualStudio.Component.Windows81SDK `
        # The below line is needed to build VS 2019 projects with MSVC or Clang.
        --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        # The below line is only needed to build VS 2017 projects.
        --add Microsoft.VisualStudio.Component.VC.v141.x86.x64 `
        # The below line is only needed to build with Clang.
        --add Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang `
        || IF "%ERRORLEVEL%"=="3010" EXIT 0) `
    `
    # Cleanup
    && del /q vs_buildtools.exe

COPY [ "$ProjectDir", "c:/Project" ]
COPY [ "$ExtractedSDKDir", "c:/sdks" ]

# Merge the VCTargets
WORKDIR c:/Project
RUN `
    c:\Project\setenv.cmd vs2019 %GDKVer% c:\sdks & `
    c:\Project\vctargets.cmd

# This entrypoint sets the environment variables necessary to build.
# Using cmd /k leaves the container running with the command prompt active. Otherwise, it will exit after running the command.
ENV GDKVer=${GDKVer}
ENTRYPOINT ["cmd", "/k", "C:\\Project\\setenv.cmd", "vs2019", "%GDKVer%", "c:\\sdks"]

# Alternately, this entrypoint can be used to automatically build the project when the container is started.
# ENV GDKVer=${GDKVer} ProjectName=${ProjectName} BuildConfig=${BuildConfig} BuildPlatform=${BuildPlatform}
# ENTRYPOINT [ "C:\\Project\\setenv.cmd", "vs2019", "%GDKVer%", "c:\\sdks", "&&", `
#    "cmd", "/k", "msbuild", "%ProjectName%.vcxproj", "/p:Configuration=%BuildConfig%", "/p:Platform=%BuildPlatform%" ]