#
# GDK-targets.cmake : Defines library imports for the Microsoft GDK shared libraries
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

if(_GDK_TARGETS_)
  return()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "ERROR: Microsoft GDK only supports 64-bit")
endif()

if(NOT XdkEditionTarget)
    message(FATAL_ERROR "ERROR: XdkEditionTarget must be set")
endif()

#--- Locate Microsoft GDK
if(BUILD_USING_BWOI)
    if(DEFINED ENV{ExtractedFolder})
        cmake_path(SET ExtractedFolder "$ENV{ExtractedFolder}")
    else()
        set(ExtractedFolder "d:/xtrctd.sdks/BWOIExample/")
    endif()

    if(NOT EXISTS ${ExtractedFolder})
        message(FATAL_ERROR "ERROR: BWOI requires a valid ExtractedFolder (${ExtractedFolder})")
    endif()

    set(Console_SdkRoot "${ExtractedFolder}/Microsoft GDK")
else()
    GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\GDK;InstallPath]" ABSOLUTE CACHE)
endif()

if(NOT EXISTS "${Console_SdkRoot}/${XdkEditionTarget}")
    message(FATAL_ERROR "ERROR: Cannot locate Microsoft Game Development Kit (GDK) - ${XdkEditionTarget}")
endif()

if(XdkEditionTarget GREATER_EQUAL 251000)
   # New layout available starting with October 2025 release
   if(_GDK_XBOX_)
      set(PlatformRoot "${Console_SdkRoot}/${XdkEditionTarget}/xbox")
   else()
      set(PlatformRoot "${Console_SdkRoot}/${XdkEditionTarget}/windows")
   endif()
else(XdkEditionTarget LESS 241000)
   message(FATAL_ERROR "ERROR: This file supports October 2024 GDK or later.")
endif()

#--- GameRuntime Library (for Xbox these are included in the Console_Libs variable)
if(NOT _GDK_XBOX_)
    add_library(Xbox::GameRuntime STATIC IMPORTED)
    if(XdkEditionTarget GREATER_EQUAL 251000)
        set_target_properties(Xbox::GameRuntime PROPERTIES
            IMPORTED_LOCATION "${PlatformRoot}/lib/x64/xgameruntime.lib"
            MAP_IMPORTED_CONFIG_MINSIZEREL ""
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
            INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include"
            INTERFACE_COMPILE_FEATURES "cxx_std_11"
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
    else()
        set_target_properties(Xbox::GameRuntime PROPERTIES
            IMPORTED_LOCATION "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Lib/amd64/xgameruntime.lib"
            MAP_IMPORTED_CONFIG_MINSIZEREL ""
            MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
            INTERFACE_INCLUDE_DIRECTORIES "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Include"
            INTERFACE_COMPILE_FEATURES "cxx_std_11"
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
    endif()

    add_library(Xbox::GameInput STATIC IMPORTED)
    if(XdkEditionTarget GREATER_EQUAL 251000)
        set_target_properties(Xbox::GameInput PROPERTIES
        IMPORTED_LOCATION "${PlatformRoot}/lib/x64/GameInput.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
    else()
        set_target_properties(Xbox::GameInput PROPERTIES
        IMPORTED_LOCATION "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Lib/amd64/gameinput.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
    endif()
endif()

#--- Extension Libraries
if(XdkEditionTarget GREATER_EQUAL 260400)
    # VC++ binary compatibility means we can use the VS 2022 version with VS 2026
    set(ExtensionPlatformToolset 143)
else()
    # VC++ binary compatibility means we can use the VS 2019 version with VS 2022
    set(ExtensionPlatformToolset 142)
endif()

if(XdkEditionTarget GREATER_EQUAL 251000)
   # These are now integrated into the platform folders.
else()
   set(Console_GRDKExtLibRoot "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/ExtensionLibraries")
endif()

# XCurl
add_library(Xbox::XCurl SHARED IMPORTED)
if(XdkEditionTarget GREATER_EQUAL 251000)
    set_target_properties(Xbox::XCurl PROPERTIES
        IMPORTED_LOCATION "${PlatformRoot}/bin/x64/XCurl.dll"
        IMPORTED_IMPLIB "${PlatformRoot}/lib/x64/XCurl.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include")
else()
    set_target_properties(Xbox::XCurl PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/Redist/x64/XCurl.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/Lib/x64/XCurl.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/Include")
endif()

# Xbox.Services.API.C (requires XCurl)
add_library(Xbox::XSAPI STATIC IMPORTED)
if(XdkEditionTarget GREATER_EQUAL 251000)
    set_target_properties(Xbox::XSAPI PROPERTIES
        IMPORTED_LOCATION_RELEASE "${PlatformRoot}/lib/x64/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.C.lib"
        IMPORTED_LOCATION_DEBUG "${PlatformRoot}/lib/x64/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.C.Debug.lib"
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
else()
    set_target_properties(Xbox::XSAPI PROPERTIES
        IMPORTED_LOCATION_RELEASE "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/Lib/x64/Release/v${ExtensionPlatformToolset}/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib"
        IMPORTED_LOCATION_DEBUG "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/Lib/x64/Debug/v${ExtensionPlatformToolset}/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib"
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/Include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
endif()

# Xbox::HTTPClient (prior to June 2024 was included as part of Xbox.Services.API.C)
if(XdkEditionTarget GREATER_EQUAL 251000)
    add_library(Xbox::HTTPClient SHARED IMPORTED)
    set_target_properties(Xbox::HTTPClient PROPERTIES
        IMPORTED_LOCATION "${PlatformRoot}/bin/x64/libHttpClient.dll"
        IMPORTED_IMPLIB "${PlatformRoot}/lib/x64/libHttpClient.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include")
else()
    add_library(Xbox::HTTPClient SHARED IMPORTED)
    set_target_properties(Xbox::HTTPClient PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.LibHttpClient/Redist/x64/libHttpClient.GDK.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.LibHttpClient/Lib/x64/libHttpClient.GDK.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.LibHttpClient/Include")
endif()

target_link_libraries(Xbox::XSAPI INTERFACE Xbox::HTTPClient Xbox::XCurl appnotify.lib winhttp.lib crypt32.lib)

# GameChat2
add_library(Xbox::GameChat2 SHARED IMPORTED)
if(XdkEditionTarget GREATER_EQUAL 251000)
    set_target_properties(Xbox::GameChat2 PROPERTIES
        IMPORTED_LOCATION "${PlatformRoot}/bin/x64/GameChat2.dll"
        IMPORTED_IMPLIB "${PlatformRoot}/lib/x64/GameChat2.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include")
else()
    set_target_properties(Xbox::GameChat2 PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/Redist/x64/GameChat2.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/Lib/x64/GameChat2.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/Include")
endif()

# PlayFab Multiplayer (requires XCurl)
add_library(Xbox::PlayFabMultiplayer SHARED IMPORTED)
if(XdkEditionTarget GREATER_EQUAL 251000)
    set_target_properties(Xbox::PlayFabMultiplayer PROPERTIES
        IMPORTED_LOCATION "${PlatformRoot}/bin/x64/PlayFabMultiplayer.dll"
        IMPORTED_IMPLIB "${PlatformRoot}/lib/x64/PlayFabMultiplayer.lib"
        IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::XCurl
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include")
else()
    set_target_properties(Xbox::PlayFabMultiplayer PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/Redist/x64/PlayFabMultiplayerGDK.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/Lib/x64/PlayFabMultiplayerGDK.lib"
        IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::XCurl
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/Include")
endif()

target_link_libraries(Xbox::PlayFabMultiplayer INTERFACE Xbox::XCurl)

# PlayFab Services (requires XCurl)
if(XdkEditionTarget GREATER_EQUAL 251000)
    add_library(Xbox::PlayFabServices SHARED IMPORTED)
    set_target_properties(Xbox::PlayFabServices PROPERTIES
        IMPORTED_LOCATION "${PlatformRoot}/bin/x64/PlayFabServices.dll"
        IMPORTED_IMPLIB "${PlatformRoot}/lib/x64/PlayFabServices.lib"
        IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::XCurl
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    add_library(Xbox::PlayFabCore SHARED IMPORTED)
    set_target_properties(Xbox::PlayFabCore PROPERTIES
        IMPORTED_LOCATION "${PlatformRoot}/bin/x64/PlayFabCore.dll"
        IMPORTED_IMPLIB "${PlatformRoot}/lib/x64/PlayFabCore.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    target_link_libraries(Xbox::PlayFabServices INTERFACE Xbox::PlayFabCore Xbox::XCurl)
else()
    add_library(Xbox::PlayFabServices SHARED IMPORTED)
    set_target_properties(Xbox::PlayFabServices PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Services.C/Redist/x64/PlayFabServices.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Services.C/Lib/x64/PlayFabServices.lib"
        IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::XCurl
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Services.C/Include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    add_library(Xbox::PlayFabCore SHARED IMPORTED)
    set_target_properties(Xbox::PlayFabCore PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Services.C/Redist/x64/PlayFabCore.GDK.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Services.C/Lib/x64/PlayFabCore.GDK.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    target_link_libraries(Xbox::PlayFabServices INTERFACE Xbox::PlayFabCore Xbox::XCurl)
endif()

# PlayFab Party
add_library(Xbox::PlayFabParty SHARED IMPORTED)
if(XdkEditionTarget GREATER_EQUAL 251000)
    set_target_properties(Xbox::PlayFabParty PROPERTIES
        IMPORTED_LOCATION "${PlatformRoot}/bin/x64/Party.dll"
        IMPORTED_IMPLIB "${PlatformRoot}/lib/x64/Party.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include")
else()
    set_target_properties(Xbox::PlayFabParty PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/Redist/x64/Party.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/Lib/x64/Party.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/Include")
endif()

# PlayFab Party Xbox LIVE (requires PlayFab Party)
add_library(Xbox::PlayFabPartyLIVE SHARED IMPORTED)
if(XdkEditionTarget GREATER_EQUAL 251000)
    set_target_properties(Xbox::PlayFabPartyLIVE PROPERTIES
        IMPORTED_LOCATION "${PlatformRoot}/bin/x64/PartyXboxLive.dll"
        IMPORTED_IMPLIB "${PlatformRoot}/lib/x64/PartyXboxLive.lib"
        IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::PlayFabParty
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${PlatformRoot}/include")
else()
    set_target_properties(Xbox::PlayFabPartyLIVE PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/Redist/x64/PartyXboxLive.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/Lib/x64/PartyXboxLive.lib"
        IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::PlayFabParty
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/Include")
endif()

target_link_libraries(Xbox::PlayFabPartyLIVE INTERFACE Xbox::PlayFabParty)

set(_GDK_TARGETS_ ON)
