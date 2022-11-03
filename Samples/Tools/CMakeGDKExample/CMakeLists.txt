# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.13)

project(CMakeGDKExample
  DESCRIPTION "CMake example for a Microsoft GDK Gaming.*.x64 project"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_VS_PLATFORM_NAME}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_VS_PLATFORM_NAME}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${CMAKE_VS_PLATFORM_NAME}")

set(GAME_SOURCES
    Game.cpp
    Game.h
    DeviceResources.cpp
    DeviceResources.h
    Main.cpp
    StepTimer.h
    pch.h
    MicrosoftGameConfig.mgc
)

if(CMAKE_VS_PLATFORM_NAME STREQUAL "Gaming.Desktop.x64")
    set(GAME_SOURCES ${GAME_SOURCES} d3dx12.h)
endif()

set(GAME_ASSETS
    Assets/Logo.png
    Assets/SmallLogo.png
    Assets/SplashScreen.png
    Assets/StoreLogo.png
)

source_group("Assets" FILES ${GAME_ASSETS})

# MicrosoftGameConfig.mgc
set_source_files_properties(MicrosoftGameConfig.mgc PROPERTIES VS_TOOL_OVERRIDE "MGCCompile")
set_source_files_properties(Assets/Logo.png PROPERTIES VS_TOOL_OVERRIDE "CopyFileToFolders")
set_source_files_properties(Assets/SmallLogo.png PROPERTIES VS_TOOL_OVERRIDE "CopyFileToFolders")
set_source_files_properties(Assets/SplashScreen.png PROPERTIES VS_TOOL_OVERRIDE "CopyFileToFolders")
set_source_files_properties(Assets/StoreLogo.png PROPERTIES VS_TOOL_OVERRIDE "CopyFileToFolders")

# Build HLSL shaders
add_custom_target(shaders)

set_source_files_properties(VertexShader.hlsl PROPERTIES ShaderType "vs")
set_source_files_properties(PixelShader.hlsl PROPERTIES ShaderType "ps")

foreach(FILE VertexShader.hlsl PixelShader.hlsl)
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  get_source_file_property(shadertype ${FILE} ShaderType)
  add_custom_command(TARGET shaders
                     COMMAND ${GDK_DXCTool} /nologo /Emain /T${shadertype}_6_0 $<$<CONFIG:DEBUG>:/Od> /Zi /Fo ${CMAKE_CURRENT_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
                     MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)
endforeach(FILE)

# Use Warning Level 4
string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

add_executable(${PROJECT_NAME} WIN32 ${GAME_SOURCES} ${GAME_ASSETS})

add_dependencies(${PROJECT_NAME} shaders)

# Standard Debug vs. Release preprocessor definitions
# _DEBUG (Debug)
# NDEBUG (Release without asserts)
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<CONFIG:DEBUG>:_DEBUG>" "$<$<CONFIG:RELEASE>:NDEBUG>")

# Build as Unicode (see UTF-8 Everywhere article's Win32 recommendations)
target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE)

# Recommended compiler switches:
# /EHsc (C++ exception handling)
# /fp:fast (Fast floating-point optimizations)
# /GS (buffer security check)
# /Gy (enable function-level linking)
# /Zi (Debug Information Format)
target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast /GS /Gy)

# /permissive- (C++ conformance mode enabled)
# /Zc:__cplusplus (Enable updated __cplusplus macro)
target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus)

# Required linker switches:
# /MACHINE:X64 /SUBSYSTEM:WINDOWS
# /DYNAMICBASE
# /NXCOMPAT
target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT)

# Set link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE xgameruntime.lib)

if((CMAKE_VS_PLATFORM_NAME STREQUAL "Gaming.Desktop.x64") AND (XdkEditionTarget GREATER_EQUAL 220600))
    target_link_libraries(${PROJECT_NAME} PRIVATE gameinput.lib)
endif()

# Add uuid.lib if using Windows Imaging Component (WIC) or other COM objects

# Set the correct graphics link libraries
if(CMAKE_VS_PLATFORM_NAME STREQUAL "Gaming.Desktop.x64")
    target_link_libraries(${PROJECT_NAME} PRIVATE dxguid.lib d3d12.lib dxgi.lib)
elseif(CMAKE_VS_PLATFORM_NAME MATCHES "Gaming.Xbox.*.x64")
    if(CMAKE_VS_PLATFORM_NAME STREQUAL "Gaming.Xbox.Scarlett.x64")
        target_link_libraries(${PROJECT_NAME} PRIVATE pixevt.lib d3d12_xs.lib xmem.lib xg_xs.lib)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE pixevt.lib d3d12_x.lib xmem.lib xg_x.lib)
    endif()
endif()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)
endif()

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/VertexShader.cso $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/PixelShader.cso $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_SOLUTION_DEPLOY ON)
endif()

# Extension libraries
# Xbox.Services.API.C Xbox.XCurl.API Xbox.Game.Chat.2.Cpp.API PlayFab.Multiplayer.Cpp PlayFab.Party.Cpp PlayFab.PartyXboxLive.Cpp
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_GDKExtLibNames Xbox.Services.API.C)

# VC++ binary compatibility means we can use the VS 2019 version with VS 2019 or VS 2022
set(ExtensionPlatformToolset 142)

target_link_libraries(${PROJECT_NAME} PRIVATE "libHttpClient.${ExtensionPlatformToolset}.GDK.C.lib" "Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib" winhttp.lib crypt32.lib)

if(CMAKE_VS_PLATFORM_NAME STREQUAL "Gaming.Desktop.x64")
    target_link_libraries(${PROJECT_NAME} PRIVATE appnotify.lib)
endif()

# Set global VS properties
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if(CMAKE_CONFIGURATION_TYPES MATCHES "Debug")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_UseDebugLibraries "true")
endif()

if(NOT CMAKE_VS_PLATFORM_NAME STREQUAL "Gaming.Desktop.x64")
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_XdkEditionTarget ${XdkEditionTarget})
endif()

# If using a side-by-side MSVC toolset, set the desired toolset version number and uncomment the following statements
# https://devblogs.microsoft.com/cppblog/side-by-side-minor-version-msvc-toolsets-in-visual-studio-2019/

#set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_ClearDevCommandPromptEnvVars "false")
#set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_VCToolsVersion "14.20.27508")
