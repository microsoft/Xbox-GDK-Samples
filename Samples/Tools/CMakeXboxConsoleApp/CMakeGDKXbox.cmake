#
# CMakeGDKXbox.cmake : CMake definitions for Microsoft GDK
#
# This version does not include the XboxOne or Scarlett include/lib paths required for Direct3D graphics,
# and uses /SUBSYSTEM:CONSOLE.
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

if(_GDK_XBOX_TOOLCHAIN_)
  return()
endif()

#--- Microsoft Game Development Kit
set(XdkEditionTarget "220300" CACHE STRING "Microsoft GDK Edition")

message("XdkEditionTarget = ${XdkEditionTarget}")

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES XdkEditionTarget BUILD_USING_BWOI)

#--- Windows SDK
set(SDKVersion 10.0.19041.0)

set(CMAKE_SYSTEM_NAME WINDOWS)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION ${SDKVersion})

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "ERROR: Xbox OS requires 64-bit")
endif()

#--- Locate Visual Studio (needed for VC Runtime DLLs)

if (NOT DEFINED VCInstallDir AND DEFINED ENV{VCINSTALLDIR})
    set(VCInstallDir $ENV{VCINSTALLDIR})
endif()

if (NOT DEFINED VCInstallDir)
    set(GDK_VS_EDITIONS "Community" "Professional" "Enterprise" "Preview" "BuildTools")

    if (MSVC_TOOLSET_VERSION MATCHES "143")
        foreach(vsedition IN LISTS GDK_VS_EDITIONS)
            set(VCInstallDir "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/${vsedition}/VC")
            if(EXISTS ${VCInstallDir})
                break()
            endif()
        endforeach()
    else()
        foreach(vsedition IN LISTS GDK_VS_EDITIONS)
            set(VCInstallDir "$ENV{ProgramFiles\(x86\)}/Microsoft Visual Studio/2019/${vsedition}/VC")
            if(EXISTS ${VCInstallDir})
                break()
            endif()
        endforeach()

        if (NOT EXISTS ${VCInstallDir})
            foreach(vsedition IN LISTS GDK_VS_EDITIONS)
                set(VCInstallDir "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/${vsedition}/VC")
                if(EXISTS ${VCInstallDir})
                    break()
                endif()
            endforeach()
        endif()
    endif()
endif()

if(EXISTS ${VCInstallDir})
    message("VCInstallDir = ${VCInstallDir}")
else()
    message(FATAL_ERROR "ERROR: Failed to locate Visual Studio 2019 or 2022 install")
endif()

# Find VC toolset/runtime versions
file(STRINGS "${VCInstallDir}/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" VCToolsVersion)
message("VCToolsVersion = ${VCToolsVersion}")

file(STRINGS "${VCInstallDir}/Auxiliary/Build/Microsoft.VCRedistVersion.default.txt" VCToolsRedistVersion)
message("VCToolsRedistVersion = ${VCToolsRedistVersion}")

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
    GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\GDK;InstallPath]" ABSOLUTE CACHE)
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
    "${DurangoXdkInstallPath}/GXDK/gameKit/Include"
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
# Don't link with onecore.lib, kernel32.lib, etc.
set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "")

# Need to link with "onecore" versions of Visual C++ libraries ("msvc_x64_x64" environment uses desktop libpath)
set(VC_OneCore_LibPath "${VCInstallDir}/Tools/MSVC/${VCToolsVersion}/lib/onecore/x64")
if(NOT EXISTS ${VC_OneCore_LibPath}/msvcrt.lib)
    message(FATAL_ERROR "ERROR: Cannot locate msvcrt.lib for the Visual C++ toolset (${VCToolsVersion})")
endif()

set(Console_LibRoot ${WindowsSdkDir}/Lib/${SDKVersion})
set(Console_EndpointLibRoot
    "${DurangoXdkInstallPath}/GXDK/gameKit/Lib/amd64"
    "${DurangoXdkInstallPath}/GRDK/gameKit/Lib/amd64")
set(Console_SdkLibPath
    "${Console_EndpointLibRoot}"
    "${Console_LibRoot}/ucrt/x64"
    "${Console_LibRoot}/um/x64")

set(Console_Libs xgameplatform.lib xgameruntime.lib)

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

#--- Binaries
set(Console_UCRTRedistDebug ${WindowsSdkDir}/bin/${SDKVersion}/x64/ucrt)
if(NOT EXISTS ${Console_UCRTRedistDebug}/ucrtbased.dll)
    message(FATAL_ERROR "ERROR: Cannot locate ucrtbased.dll in the Windows SDK (${SDKVersion})")
endif()

set(CRTPlatformToolset 143)
if (NOT EXISTS "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/x64/Microsoft.VC${CRTPlatformToolset}.CRT")
    set(CRTPlatformToolset 142)
endif()

message("CRT Platform Toolset = ${CRTPlatformToolset}")

set(CppRuntimeFilesPath "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/x64/Microsoft.VC${CRTPlatformToolset}.CRT")
set(OpenMPRuntimeFilesPath "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/x64/Microsoft.VC${CRTPlatformToolset}.OpenMP")
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(DebugCppRuntimeFilesPath "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/Debug_NonRedist/x64/Microsoft.VC${CRTPlatformToolset}.DebugCRT")
    set(DebugOpenMPRuntimeFilesPath "${VCInstallDir}/redist/MSVC/${VCToolsRedistVersion}/onecore/Debug_NonRedist/x64/Microsoft.VC${CRTPlatformToolset}.DebugOpenMP")
endif()

#--- Build options
# Required preprocessor defines
# WIN32
# _WINDOWS

# Traditional indicator of a 'Win32 console' application
set(Console_Defines ${Console_Defines} _CONSOLE)

# Standard Debug vs. Release preprocessor definitions
# (automatically defined by MSVC and MSVC-like compilers)
# _DEBUG (Debug)
# NDEBUG (Release without asserts)

# Build as Unicode (see UTF-8 Everywhere article's Win32 recommendations)
set(Console_Defines ${Console_Defines} _UNICODE UNICODE)

# Game Core on Xbox preprocessor definitions
set(Console_Defines ${Console_Defines} WIN32_LEAN_AND_MEAN _GAMING_XBOX WINAPI_FAMILY=WINAPI_FAMILY_GAMES)

# Additional recommended preprocessor defines
set(Console_Defines ${Console_Defines} _CRT_USE_WINAPI_PARTITION_APP _UITHREADCTXT_SUPPORT=0 __WRL_CLASSIC_COM_STRICT__)

# Default library controls
set(Console_Defines ${Console_Defines} _ATL_NO_DEFAULT_LIBS __WRL_NO_DEFAULT_LIB__)

set(UnsupportedLibs advapi32.lib comctl32.lib comsupp.lib dbghelp.lib gdi32.lib gdiplus.lib guardcfw.lib kernel32.lib mmc.lib msimg32.lib msvcole.lib msvcoled.lib mswsock.lib ntstrsafe.lib ole2.lib ole2autd.lib ole2auto.lib ole2d.lib ole2ui.lib ole2uid.lib ole32.lib oleacc.lib oleaut32.lib oledlg.lib oledlgd.lib oldnames.lib runtimeobject.lib shell32.lib shlwapi.lib strsafe.lib urlmon.lib user32.lib userenv.lib wlmole.lib wlmoled.lib onecore.lib)

# Required compiler switches:
# /MD or /MDd (VC Runtime DLL)
# /O? or /Od (Optimize code)

# Required linker switches:
# /MACHINE:X64 /SUBSYSTEM:CONSOLE
# /DYNAMICBASE
# /NXCOMPAT
set(Console_LinkOptions "/SUBSYSTEM:CONSOLE,10.0" "/DYNAMICBASE" "/NXCOMPAT")

# Prevent accidental use of libraries that are not supported by Game Core on Xbox
foreach(arg ${UnsupportedLibs})
    list(APPEND Console_LinkOptions "/NODEFAULTLIB:${arg}")
endforeach()

if (OPTIMIZE_FOR_SCARLETT)
    message("Optimizing code for Xbox Series X|S (won't run on Xbox One)")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   set(Console_ArchOptions /favor:AMD64 $<IF:$<BOOL:${OPTIMIZE_FOR_SCARLETT}>,/arch:AVX2,/arch:AVX>)
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # -march=btver2 to target AMD Jaguar CPU; -march=znver2 to target AMD Hercules CPU (requires clang v9; otherwise use znver1)
   set(Console_ArchOptions -march=$<IF:$<BOOL:${OPTIMIZE_FOR_SCARLETT}>,$<IF:$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,9.0>,znver2,znver1>,btver2>)
endif()

set(_GDK_XBOX_TOOLCHAIN_ ON)
