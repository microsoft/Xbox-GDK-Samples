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
    GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\GDK;GRDKInstallPath]" ABSOLUTE CACHE)
endif()

if(NOT EXISTS "${Console_SdkRoot}/${XdkEditionTarget}")
    message(FATAL_ERROR "ERROR: Cannot locate Microsoft Game Development Kit (GDK) - ${XdkEditionTarget}")
endif()

#--- GameRuntime Library (for Xbox these are included in the Console_Libs variable)
if(NOT _GDK_XBOX_)
    add_library(Xbox::GameRuntime STATIC IMPORTED)
    set_target_properties(Xbox::GameRuntime PROPERTIES
        IMPORTED_LOCATION "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Lib/amd64/xgameruntime.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Include"
        INTERFACE_COMPILE_FEATURES "cxx_std_11"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    if(XdkEditionTarget GREATER_EQUAL 220600)
        add_library(Xbox::GameInput STATIC IMPORTED)
        set_target_properties(Xbox::GameInput PROPERTIES
        IMPORTED_LOCATION "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Lib/amd64/gameinput.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/gameKit/Include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")
    endif()
endif()

#--- Extension Libraries
set(Console_GRDKExtLibRoot "${Console_SdkRoot}/${XdkEditionTarget}/GRDK/ExtensionLibraries")
set(ExtensionPlatformToolset 142)

# XCurl
add_library(Xbox::XCurl SHARED IMPORTED)
set_target_properties(Xbox::XCurl PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/Redist/CommonConfiguration/neutral/XCurl.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/DesignTime/CommonConfiguration/neutral/lib/XCurl.lib"
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.XCurl.API/DesignTime/CommonConfiguration/neutral/Include")

# Xbox.Services.API.C (requires XCurl)
add_library(Xbox::XSAPI STATIC IMPORTED)
set_target_properties(Xbox::XSAPI PROPERTIES
    IMPORTED_LOCATION_RELEASE "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Release/v${ExtensionPlatformToolset}/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_LOCATION_DEBUG "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Debug/v${ExtensionPlatformToolset}/Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Include"
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

add_library(Xbox::HTTPClient STATIC IMPORTED)
set_target_properties(Xbox::HTTPClient PROPERTIES
    IMPORTED_LOCATION_RELEASE "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Release/v${ExtensionPlatformToolset}/libHttpClient.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_LOCATION_DEBUG "${Console_GRDKExtLibRoot}/Xbox.Services.API.C/DesignTime/CommonConfiguration/Neutral/Lib/Debug/v${ExtensionPlatformToolset}/libHttpClient.${ExtensionPlatformToolset}.GDK.C.lib"
    IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

target_link_libraries(Xbox::XSAPI INTERFACE Xbox::HTTPClient Xbox::XCurl appnotify.lib winhttp.lib crypt32.lib)

# GameChat2
add_library(Xbox::GameChat2 SHARED IMPORTED)
set_target_properties(Xbox::GameChat2 PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/Redist/CommonConfiguration/neutral/GameChat2.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/DesignTime/CommonConfiguration/neutral/lib/GameChat2.lib"
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/Xbox.Game.Chat.2.Cpp.API/DesignTime/CommonConfiguration/neutral/Include")

# PlayFab Multiplayer (requires XCurl)
add_library(Xbox::PlayFabMultiplayer SHARED IMPORTED)
set_target_properties(Xbox::PlayFabMultiplayer PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/Redist/CommonConfiguration/neutral/PlayFabMultiplayerGDK.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/DesignTime/CommonConfiguration/neutral/Lib/PlayFabMultiplayerGDK.lib"
    IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::XCurl
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Multiplayer.Cpp/DesignTime/CommonConfiguration/neutral/Include")

target_link_libraries(Xbox::PlayFabMultiplayer INTERFACE Xbox::XCurl)

# PlayFab Services (requires XCurl)
if(XdkEditionTarget GREATER_EQUAL 230300)
    add_library(Xbox::PlayFabServices SHARED IMPORTED)
    set_target_properties(Xbox::PlayFabServices PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Services.C/Redist/CommonConfiguration/neutral/PlayFabServices.GDK.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Services.C/DesignTime/CommonConfiguration/neutral/Lib/PlayFabServices.GDK.lib"
        IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::XCurl
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Services.C/DesignTime/CommonConfiguration/neutral/Include"
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    add_library(Xbox::PlayFabCore SHARED IMPORTED)
    set_target_properties(Xbox::PlayFabCore PROPERTIES
        IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Services.C/Redist/CommonConfiguration/neutral/PlayFabCore.GDK.dll"
        IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Services.C/DesignTime/CommonConfiguration/neutral/Lib/PlayFabCore.GDK.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL ""
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX")

    target_link_libraries(Xbox::PlayFabServices INTERFACE Xbox::PlayFabCore Xbox::XCurl)
endif()

# PlayFab Party
add_library(Xbox::PlayFabParty SHARED IMPORTED)
set_target_properties(Xbox::PlayFabParty PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/Redist/CommonConfiguration/neutral/Party.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/DesignTime/CommonConfiguration/neutral/Lib/Party.lib"
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.Party.Cpp/DesignTime/CommonConfiguration/neutral/Include")

# PlayFab Party Xbox LIVE (requires PlayFab Party)
add_library(Xbox::PlayFabPartyLIVE SHARED IMPORTED)
set_target_properties(Xbox::PlayFabPartyLIVE PROPERTIES
    IMPORTED_LOCATION "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/Redist/CommonConfiguration/neutral/PartyXboxLive.dll"
    IMPORTED_IMPLIB "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/DesignTime/CommonConfiguration/neutral/Lib/PartyXboxLive.lib"
    IMPORTED_LINK_DEPENDENT_LIBRARIES Xbox::PlayFabParty
    MAP_IMPORTED_CONFIG_MINSIZEREL ""
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO ""
    INTERFACE_INCLUDE_DIRECTORIES "${Console_GRDKExtLibRoot}/PlayFab.PartyXboxLive.Cpp/DesignTime/CommonConfiguration/neutral/Include")

target_link_libraries(Xbox::PlayFabPartyLIVE INTERFACE Xbox::PlayFabParty)

set(_GDK_TARGETS_ ON)
