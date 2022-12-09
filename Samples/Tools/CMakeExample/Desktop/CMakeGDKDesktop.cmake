#
# CMakeGDKDesktop.cmake : CMake definitions for Microsoft GDK targeting PC
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

if(_GDK_DESKTOP_TOOLCHAIN_)
  return()
endif()

#--- Microsoft Game Development Kit
set(XdkEditionTarget "220300" CACHE STRING "Microsoft GDK Edition")

message("XdkEditionTarget = ${XdkEditionTarget}")

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES XdkEditionTarget BUILD_USING_BWOI)

#--- Windows SDK
if (NOT SDKVersion)
    set(SDKVersion 10.0.19041.0)
endif()

set(CMAKE_SYSTEM_NAME WINDOWS)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION ${SDKVersion})

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "ERROR: Gaming.Desktop.x64 requires 64-bit")
endif()

#--- Locate Software Development Kits
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
else()
    GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\GDK;GRDKInstallPath]" ABSOLUTE CACHE)
    set(DurangoXdkInstallPath "${Console_SdkRoot}/${XdkEditionTarget}")

    set(WindowsSdkDir "$ENV{ProgramFiles\(x86\)}/Windows Kits/10")
endif()

if(EXISTS ${DurangoXdkInstallPath})
    message("Microsoft GDK = ${DurangoXdkInstallPath}")
else()
    message(FATAL_ERROR "ERROR: Cannot locate Microsoft Game Development Kit (GDK) - ${XdkEditionTarget}")
endif()

if(EXISTS "${WindowsSdkDir}/Include/${SDKVersion}" )
    message("Windows SDK = v${SDKVersion} in ${WindowsSdkDir}")
else()
    message(FATAL_ERROR "ERROR: Cannot locate Windows SDK (${SDKVersion})")
endif()

set(ExtensionPlatformToolset 142)

message("Extension Platform Toolset = ${ExtensionPlatformToolset}")

#--- Headers
set(Console_EndpointIncludeRoot
    "${DurangoXdkInstallPath}/GRDK/gameKit/Include")
set(Console_WindowsIncludeRoot ${WindowsSdkDir}/Include/${SDKVersion})
set(Console_SdkIncludeRoot
    "${Console_EndpointIncludeRoot}"
    "${Console_WindowsIncludeRoot}/um"
    "${Console_WindowsIncludeRoot}/shared"
    "${Console_WindowsIncludeRoot}/winrt"
    "${Console_WindowsIncludeRoot}/cppwinrt"
    "${Console_WindowsIncludeRoot}/ucrt")

#--- Libraries
set(Console_LibRoot ${WindowsSdkDir}/Lib/${SDKVersion})
set(Console_EndpointLibRoot
    "${DurangoXdkInstallPath}/GRDK/gameKit/Lib/amd64")
set(Console_SdkLibPath
    "${Console_EndpointLibRoot}"
    "${Console_LibRoot}/ucrt/x64"
    "${Console_LibRoot}/um/x64")

set(Console_Libs dxguid.lib d3d12.lib dxgi.lib xgameruntime.lib)

if (XdkEditionTarget GREATER_EQUAL 220600)
    list(APPEND Console_Libs "gameinput.lib")
endif()

#--- Extension Libraries
set(Console_GRDKExtLibRoot "${DurangoXdkInstallPath}/GRDK/ExtensionLibraries")

# XCurl
add_library(Xbox::XCurl SHARED IMPORTED)
set_target_properties(Xbox::XCurl PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/Redist/CommonConfiguration/neutral/XCurl.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/DesignTime/CommonConfiguration/neutral/lib/XCurl.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/DesignTime/CommonConfiguration/neutral/Include")

# Xbox.Services.API.C (requires XCurl)
add_library(Xbox::XSAPI STATIC IMPORTED)
set_target_properties(Xbox::XSAPI PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Release/v${ExtensionPlatformToolset}/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_LOCATION_DEBUG "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Debug/v${ExtensionPlatformToolset}/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Include"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

add_library(Xbox::HTTPClient STATIC IMPORTED)
set_target_properties(Xbox::HTTPClient PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Release/v${ExtensionPlatformToolset}/libHttpClient.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_LOCATION_DEBUG "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Debug/v${ExtensionPlatformToolset}/libHttpClient.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

target_link_libraries(Xbox::XSAPI INTERFACE Xbox::HTTPClient Xbox::XCurl appnotify.lib winhttp.lib crypt32.lib)

# GameChat2
add_library(Xbox::GameChat2 SHARED IMPORTED)
set_target_properties(Xbox::GameChat2 PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/Redist/CommonConfiguration/neutral/GameChat2.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/DesignTime/CommonConfiguration/neutral/lib/GameChat2.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/DesignTime/CommonConfiguration/neutral/Include")

# PlayFab Multiplayer (requires XCurl)
add_library(Xbox::PlayFabMultiplayer SHARED IMPORTED)
set_target_properties(Xbox::PlayFabMultiplayer PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/Redist/CommonConfiguration/neutral/PlayFabMultiplayerGDK.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/DesignTime/CommonConfiguration/neutral/Lib/PlayFabMultiplayerGDK.lib"
    IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::XCurl
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/DesignTime/CommonConfiguration/neutral/Include")

target_link_libraries(Xbox::PlayFabMultiplayer INTERFACE Xbox::XCurl)

# PlayFab Party
add_library(Xbox::PlayFabParty SHARED IMPORTED)
set_target_properties(Xbox::PlayFabParty PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/Redist/CommonConfiguration/neutral/Party.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/DesignTime/CommonConfiguration/neutral/Lib/Party.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/DesignTime/CommonConfiguration/neutral/Include")

# PlayFab Party Xbox LIVE (requires PlayFab Party)
add_library(Xbox::PlayFabPartyLIVE SHARED IMPORTED)
set_target_properties(Xbox::PlayFabPartyLIVE PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/Redist/CommonConfiguration/neutral/PartyXboxLive.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/DesignTime/CommonConfiguration/neutral/Lib/PartyXboxLive.lib"
    IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::PlayFabParty
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/DesignTime/CommonConfiguration/neutral/Include")

target_link_libraries(Xbox::PlayFabPartyLIVE INTERFACE Xbox::PlayFabParty)

#--- Tools
set(DXCToolPath ${WindowsSdkDir}/bin/${SDKVersion}/x64)
if(NOT EXISTS ${DXCToolPath}/dxc.exe)
    message(FATAL_ERROR "ERROR: Cannot locate dxc.exe in Windows SDK (${SDKVersion})")
endif()

set(MGCToolPath "${Console_SdkRoot}/bin")

#--- Build options
# Required preprocessor defines
# WIN32
# _WINDOWS

# Standard Debug vs. Release preprocessor definitions
# (automatically defined by MSVC and MSVC-like compilers)
# _DEBUG (Debug)
# NDEBUG (Release without asserts)

# Build as Unicode (see UTF-8 Everywhere article's Win32 recommendations)
set(Console_Defines _UNICODE UNICODE)

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
