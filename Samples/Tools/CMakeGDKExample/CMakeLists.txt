# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

project(CMakeGDKExample
  DESCRIPTION "CMake example for a Microsoft GDK Gaming.*.x64 project"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${CMAKE_VS_PLATFORM_NAME}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${CMAKE_VS_PLATFORM_NAME}")
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

if(NOT (DEFINED XBOX_CONSOLE_TARGET))
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
  add_custom_command(TARGET shaders POST_BUILD
                     COMMAND ${GDK_DXCTool} /nologo /Emain /T${shadertype}_6_0 $<$<CONFIG:DEBUG>:/Od> /Zi /Fo ${CMAKE_CURRENT_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)
endforeach(FILE)

add_executable(${PROJECT_NAME} WIN32 ${GAME_SOURCES} ${GAME_ASSETS})

add_dependencies(${PROJECT_NAME} shaders)

# Standard Debug vs. Release preprocessor definitions
# _DEBUG (Debug)
# NDEBUG (Release without asserts)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG>)

# Build as Unicode (see UTF-8 Everywhere article's Win32 recommendations)
target_compile_definitions(${PROJECT_NAME} PRIVATE _UNICODE UNICODE)

if(NOT MSVC)
  message(FATAL_ERROR "ERROR: This CMake file requires MSVC or MSVC-like compiler.")
endif()

# Recommended compiler switches:
# /EHsc (C++ exception handling)
# /fp:fast (Fast floating-point optimizations)
# /GR (RTTI): if not using typeid or dynamic_cast, we can disable RTTI to save binary size using /GR-
# /GS (buffer security check)
# /Gy (enable function-level linking)
# /Zi (Debug Information Format)
target_compile_options(${PROJECT_NAME} PRIVATE /W4 /EHsc /GR- /fp:fast /GS /Gy)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # /Zc:inline (Remove unreferenced COMDAT)
    target_compile_options(${PROJECT_NAME} PRIVATE /Zc:inline)

    # /permissive- (C++ conformance mode enabled)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.10)
        target_compile_options(${PROJECT_NAME} PRIVATE /permissive-)
    endif()

    # /Zc:__cplusplus (Enable updated __cplusplus macro)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.14)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus)
    endif()

    # /Zc:preprocessor (C99 conformant preprocessor)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor)
    endif()

    # /Zc:lambda (Enable updated lambda processor)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:lambda)
    endif()
endif()

# Required linker switches:
# /MACHINE:X64 /SUBSYSTEM:WINDOWS
# /DYNAMICBASE
# /NXCOMPAT
target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT)

# /ZH:SHA_256 (secure source hashing)
if((MSVC_VERSION GREATER_EQUAL 1924)
  AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)))
  target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
endif()

if (CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE OR CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO)
    # /GL (Whole Program Optimization)
    # /Gw (Optimize Global Data)
    target_compile_options(${PROJECT_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/Gw>)

    # /Zc:checkGwOdr (Enforce Standard C++ ODR violations under /Gw)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
        target_compile_options(${PROJECT_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/Zc:checkGwOdr>)
    endif()

    # /LTCG (Link-time Code Generation)
    # Ignores warning from CMake generator use of /INCREMENTAL
    target_link_options(${PROJECT_NAME} PRIVATE /IGNORE:4075)
endif()

# Set link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE xgameruntime.lib)

if((NOT (DEFINED XBOX_CONSOLE_TARGET)) AND (XdkEditionTarget GREATER_EQUAL 220600))
    target_link_libraries(${PROJECT_NAME} PRIVATE gameinput.lib)
endif()

# Add uuid.lib if using Windows Imaging Component (WIC) or other COM objects

# Set the correct graphics link libraries
if(DEFINED XBOX_CONSOLE_TARGET)
    if(XBOX_CONSOLE_TARGET STREQUAL "scarlett")
        target_link_libraries(${PROJECT_NAME} PRIVATE pixevt.lib d3d12_xs.lib xmem.lib xg_xs.lib)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE pixevt.lib d3d12_x.lib xmem.lib xg_x.lib)
    endif()
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE dxguid.lib d3d12.lib dxgi.lib)
endif()

target_precompile_headers(${PROJECT_NAME} PRIVATE pch.h)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/VertexShader.cso $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/PixelShader.cso $<TARGET_FILE_DIR:${PROJECT_NAME}>
  )

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_SOLUTION_DEPLOY ON)

# Extension libraries
# Xbox.Services.API.C Xbox.XCurl.API Xbox.Game.Chat.2.Cpp.API PlayFab.Multiplayer.Cpp PlayFab.Party.Cpp PlayFab.PartyXboxLive.Cpp PlayFab.Services.C
set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_GDKExtLibNames Xbox.Services.API.C)

# VC++ binary compatibility means we can use the VS 2019 version with VS 2019 or VS 2022
set(ExtensionPlatformToolset 142)

target_link_libraries(${PROJECT_NAME} PRIVATE "Microsoft.Xbox.Services.${ExtensionPlatformToolset}.GDK.C.lib" winhttp.lib crypt32.lib)

if(XdkEditionTarget GREATER_EQUAL 240600)
    target_link_libraries(${PROJECT_NAME} PRIVATE "libHttpClient.GDK.lib")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE "libHttpClient.${ExtensionPlatformToolset}.GDK.C.lib")
endif()

# target_link_libraries(${PROJECT_NAME} PRIVATE XCurl.lib GameChat2.lib PlayFabMultiplayerGDK.lib Party.lib PartyXboxLive.lib PlayFabCore.GDK.lib PlayFabServices.GDK.lib)

if(NOT (DEFINED XBOX_CONSOLE_TARGET))
    target_link_libraries(${PROJECT_NAME} PRIVATE appnotify.lib)
endif()

# Set global VS properties
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

if((${CMAKE_VERSION} LESS "3.30") AND (CMAKE_CONFIGURATION_TYPES MATCHES "Debug"))
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_UseDebugLibraries "true")
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_USER_PROPS "${CMAKE_CURRENT_LIST_DIR}/BasicChecksWorkaround.props")
endif()

if(DEFINED XBOX_CONSOLE_TARGET)
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_XdkEditionTarget ${XdkEditionTarget})
    set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_VC_Target_Library_Platform "OneCore")
endif()

# If using a side-by-side MSVC toolset, set the desired toolset version number and uncomment the following statements
# https://devblogs.microsoft.com/cppblog/side-by-side-minor-version-msvc-toolsets-in-visual-studio-2019/

#set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_ClearDevCommandPromptEnvVars "false")
#set_property(TARGET ${PROJECT_NAME} PROPERTY VS_GLOBAL_VCToolsVersion "14.20.27508")
