#
# CMakeGDKDesktop.cmake : CMake definitions for Microsoft GDK targeting PC
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

if(_GDK_DESKTOP_TOOLCHAIN_)
  return()
endif()

# Microsoft Game Development Kit
set(XdkEditionTarget "220300" CACHE STRING "Microsoft GDK Edition")

message("XdkEditionTarget = ${XdkEditionTarget}")

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES XdkEditionTarget BUILD_USING_BWOI)

# Windows 10 SDK
if (NOT SDKVersion)
    set(SDKVersion 10.0.19041.0)
endif()
set(GamingTargetPlatformVersion ${SDKVersion})

set(CMAKE_SYSTEM_NAME WINDOWS)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION ${SDKVersion})

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "ERROR: Gaming.Desktop.x64 requires 64-bit")
endif()

# Locate Software Development Kits
if(BUILD_USING_BWOI)
    if (DEFINED ENV{ExtractedFolder})
        set(ExtractedFolder $ENV{ExtractedFolder})
    else()
        set(ExtractedFolder "d:/xtrctd.sdks/BWOIExample/")
    endif()

    if(NOT EXISTS ${ExtractedFolder})
        message(FATAL_ERROR "ERROR: BWOI requires a valid ExtractedFolder (${ExtractedFolder})")
    endif()

    set(Console_SdkRoot "${ExtractedFolder}/Microsoft GDK")
    set(DurangoXdkInstallPath "${ExtractedFolder}/Microsoft GDK/${XdkEditionTarget}")

    set(WindowsSdkDir "${ExtractedFolder}/Windows Kits/10")
    if (NOT EXISTS ${WindowsSdkDir})
        set(WindowsSdkDir "$ENV{ProgramFiles\(x86\)}/Windows Kits/10")
    endif()
    set(GamingWindowsSDKDir ${WindowsSdkDir})
else()
    GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\GDK;InstallPath]" ABSOLUTE CACHE)
    set(DurangoXdkInstallPath "${Console_SdkRoot}/${XdkEditionTarget}")

    set(WindowsSdkDir "$ENV{ProgramFiles\(x86\)}/Windows Kits/10")
    set(GamingWindowsSDKDir ${WindowsSdkDir})
endif()

if(EXISTS ${DurangoXdkInstallPath})
    message("Microsoft GDK = ${DurangoXdkInstallPath}")
else()
    message(FATAL_ERROR "ERROR: Cannot locate Microsoft Game Development Kit (GDK) - ${XdkEditionTarget}")
endif()

if(EXISTS "${WindowsSdkDir}/Include/${SDKVersion}" )
    message("Windows 10 SDK = ${WindowsSdkDir}")
else()
    message(FATAL_ERROR "ERROR: Cannot locate Windows 10 SDK (${SDKVersion})")
endif()

message("Gaming Windows 10 SDK = ${GamingWindowsSDKDir}")

set(ExtensionPlatformToolset 142)

message("Extension Platform Toolset = ${ExtensionPlatformToolset}")

# Headers
set(Console_EndpointIncludeRoot
    "${DurangoXdkInstallPath}/GRDK/gameKit/Include")
set(Console_WindowsIncludeRoot ${GamingWindowsSDKDir}/Include/${GamingTargetPlatformVersion})
set(Console_SdkIncludeRoot
    "${Console_EndpointIncludeRoot}"
    "${Console_WindowsIncludeRoot}/um"
    "${Console_WindowsIncludeRoot}/shared"
    "${Console_WindowsIncludeRoot}/winrt"
    "${Console_WindowsIncludeRoot}/cppwinrt"
    "${Console_WindowsIncludeRoot}/ucrt")

set(Console_LibRoot ${GamingWindowsSDKDir}/Lib/${GamingTargetPlatformVersion})
set(Console_EndpointLibRoot
    "${DurangoXdkInstallPath}/GRDK/gameKit/Lib/amd64")
set(Console_SdkLibPath
    "${Console_EndpointLibRoot}"
    "${Console_LibRoot}/ucrt/x64"
    "${Console_LibRoot}/um/x64")

set(Console_Libs dxguid.lib d3d12.lib dxgi.lib xgameruntime.lib)

# Extension Libraries (using just Xbox.Services.API.C for this example)
set(Console_GRDKExtLibRoot "${DurangoXdkInstallPath}/GRDK/ExtensionLibraries")

set(Console_ExtIncPath "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Include")

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(Console_ExtLibPath "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Debug/v${ExtensionPlatformToolset}")
else()
    set(Console_ExtLibPath "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Release/v${ExtensionPlatformToolset}")
endif()

set(XSAPI_Libs "libHttpClient.${ExtensionPlatformToolset}.GDK.C.lib" "Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib" appnotify.lib winhttp.lib crypt32.lib)

# Tools
set(DXCToolPath ${WindowsSdkDir}/bin/${SDKVersion}/x64)
if(NOT EXISTS ${DXCToolPath}/dxc.exe)
    message(FATAL_ERROR "ERROR: Cannot locate dxc.exe in Windows 10 SDK (${SDKVersion})")
endif()

set(MGCToolPath "${Console_SdkRoot}/bin")

# Required preprocessor defines
# WIN32
# _WINDOWS

# Standard Debug vs. Release preprocessor definitions
# _DEBUG (Debug)
# NDEBUG (Release without asserts)
set(Console_Defines "$<$<CONFIG:DEBUG>:_DEBUG>" "$<$<CONFIG:RELEASE>:NDEBUG>")

# Build as Unicode (see UTF-8 Everywhere article's Win32 recommendations)
set(Console_Defines ${Console_Defines} _UNICODE UNICODE)

# GDK for Desktop preprocessor definitions
set(Console_Defines ${Console_Defines} _GAMING_DESKTOP WINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP)

# Default library controls
set(Console_Defines ${Console_Defines} __WRL_NO_DEFAULT_LIB__)

# Required compiler switches:
# /MD or /MDd (VC Runtime DLL)
# /O? or /Od (Optimize code)

# Required linker switches:
# /MACHINE:X64 /SUBSYSTEM:WINDOWS
# /DYNAMICBASE
# /NXCOMPAT
set(Console_LinkOptions /DYNAMICBASE /NXCOMPAT)

set(_GDK_DESKTOP_TOOLCHAIN_ ON)
