# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

project(CMakeExampleDesktop
  DESCRIPTION "CMake example for a Microsoft GDK x64 project"
  HOMEPAGE_URL "https://github.com/microsoft/Xbox-GDK-Samples"
  LANGUAGES CXX)

# https://docs.microsoft.com/windows/win32/apiindex/umbrella-lib-onecore
option(BUILD_USING_ONECORE "Build for Windows 10/Windows 11 only" OFF)

# See the BWOIExample for more information
option(BUILD_USING_BWOI "Enable Build With/Out Install" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

include(../CMake/CMakeGDKDesktop.cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/x64")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/x64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/x64")

# Libraries
# kernel32.lib user32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib are the libs for Win32 desktop DirectX apps
# alternatively you can use onecore.lib for Win32 desktop apps that only support Windows 10/Windows 11

if(BUILD_USING_ONECORE)
    set(CMAKE_CXX_STANDARD_LIBRARIES "onecore_apiset.lib user32.lib")
    set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "onecore_apiset.lib user32.lib")
endif()

# WARNING: Only ever use _one_ of: kernel32.lib, onecore*.lib, onecoreuap*.lib, WindowsApp.lib, xgameplatform.lib

add_executable(${PROJECT_NAME} WIN32
    ../Game.cpp
    ../Game.h
    ../DeviceResources.cpp
    ../DeviceResources.h
    ../Main.cpp
    ../StepTimer.h
    ../pch.h
    d3dx12.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ./ ../)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG> _GAMING_DESKTOP WINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP _UNICODE UNICODE __WRL_NO_DEFAULT_LIB__)

target_link_libraries(${PROJECT_NAME} PRIVATE dxguid.lib d3d12.lib dxgi.lib Xbox::GameRuntime Xbox::XSAPI Xbox::XCurl)
if (XdkEditionTarget GREATER_EQUAL 220600)
    target_link_libraries(${PROJECT_NAME} PRIVATE Xbox::GameInput)
endif()
# uuid.lib if using Windows Imaging Component (WIC) or other COM objects
# delayimp.lib if using /DELAYLOAD (or use dloadhelper.lib if BUILD_USING_ONECORE is ON)
# optional additional extension libraries: Xbox::GameChat2 Xbox::PlayFabMultiplayer Xbox::PlayFabParty Xbox::PlayFabPartyLIVE Xbox::PlayFabServices

# Build HLSL shaders
add_custom_target(shaders)

set_source_files_properties(../VertexShader.hlsl PROPERTIES ShaderType "vs")
set_source_files_properties(../PixelShader.hlsl PROPERTIES ShaderType "ps")

foreach(FILE ../VertexShader.hlsl ../PixelShader.hlsl)
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  get_source_file_property(shadertype ${FILE} ShaderType)
  add_custom_command(TARGET shaders
                     COMMAND ${DIRECTX_DXC_TOOL} /nologo /Emain /T${shadertype}_6_0 $<IF:$<CONFIG:DEBUG>,/Od,/O3> /Zi /Fo ${CMAKE_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
                     MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)
endforeach(FILE)

# Microsoft Config File
set(TDF_NAME "PC")
configure_file(../MicrosoftGameConfig.mgc MicrosoftGame.config @ONLY)

add_custom_target(mgccompile)

add_custom_command(TARGET mgccompile
                   COMMAND ${MAKEPKG_TOOL} localize /d "${CMAKE_BINARY_DIR}" /pd "${CMAKE_BINARY_DIR}/bin/x64"
                   MAIN_DEPENDENCY ../MicrosoftGameConfig.mgc
                   COMMENT "makepkg localize MicrosoftGame.config"
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   VERBATIM)

add_dependencies(${PROJECT_NAME} mgccompile shaders)

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
target_compile_options(${PROJECT_NAME} PRIVATE /W4 /GR- /fp:fast /GS /Gy)

# Optional precompiled header switches:
# /Ycpch.h (Create Precompiled Header File)
# /Yupch.h (Use Precompiled Header File)
# /Fp (Name .pch file)
# /FI (Name Force Include File)
target_precompile_headers(${PROJECT_NAME} PRIVATE ../pch.h)

target_link_options(${PROJECT_NAME} PRIVATE /DYNAMICBASE /NXCOMPAT)

# For the Windows platform, Control Flow Guard is recommended but may require updated middleware
target_compile_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
target_link_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")

if(BUILD_USING_ONECORE)
    target_link_options(${PROJECT_NAME} PRIVATE "/SUBSYSTEM:WINDOWS,10.0" "/NODEFAULTLIB:kernel32.lib")
endif()

# /ZH:SHA_256 (secure source hashing)
if((MSVC_VERSION GREATER_EQUAL 1924)
  AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)))
  target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
endif()

# For the Windows platform, Enable EH Continuation Metadata is recommended but may require updated middleware
if((MSVC_VERSION GREATER_EQUAL 1928)
  AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
  AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)))
  target_compile_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
  target_link_options(${PROJECT_NAME} PRIVATE "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   # /sdl (Enable Additional Security Checks)
   # /permissive- (C++ conformance mode enabled)
   # /Zc:__cplusplus (Enable updated __cplusplus macro)
   # /Zc:inline (Remove unreferenced COMDAT)
   target_compile_options(${PROJECT_NAME} PRIVATE /sdl /permissive- /Zc:__cplusplus /Zc:inline)

   # /analyze (Code analysis)
   if(ENABLE_CODE_ANALYSIS)
      target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
   endif()

   # /Zc:preprocessor (C99 conformant preprocessor)
   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5105)
   endif()

   if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:lambda)
   endif()

   # /CETCOMPAT (CET Shadow Stack compatible)
   # Note: Make sure any middleware libraries you use support CET
   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27)
      target_link_options(${PROJECT_NAME} PRIVATE /CETCOMPAT)
   endif()

   if(CMAKE_BUILD_TYPE MATCHES "Debug")
       # Optional debug compiler switches:
       # /RTC (Runtime Checks)
       # /JMC (Just My Code debugging)

       # Optional debug linker switches:
       # /INCREMENTAL
       # /DEBUG or /DEBUG:fastlink
   elseif(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
       # /GL (Whole Program Optimization)
       # /Gw (Optimize Global Data)
       target_compile_options(${PROJECT_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/Gw>)

       if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
           target_compile_options(${PROJECT_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/Zc:checkGwOdr>)
       endif()

       # /LTCG (Link-time Code Generation)
       # Ignores warning from CMake generator use of /INCREMENTAL
       target_link_options(${PROJECT_NAME} PRIVATE /IGNORE:4075)
   endif()
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
       -Wno-c++98-compat
       -Wno-c++98-compat-pedantic
       -Wno-gnu-anonymous-struct
       -Wno-language-extension-token
       -Wno-nested-anon-types
       -Wno-reserved-id-macro
       -Wno-unknown-pragmas)

    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13.0)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wno-reserved-identifier)
    endif()
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)
        target_compile_options(${PROJECT_NAME} PRIVATE -Wno-unsafe-buffer-usage)
    endif()

    target_link_options(${PROJECT_NAME} PRIVATE /IGNORE:4078)
endif()

# Copy config, images, and Extension Library DLLs
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_BINARY_DIR}/MicrosoftGame.Config
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_BINARY_DIR}/VertexShader.cso
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_BINARY_DIR}/PixelShader.cso
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../Assets
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
      COMMAND_EXPAND_LISTS
      )
endif()

# MSIXVC Packaging using the makepkg tool in the GDK:
#    cd CMakeExample\Desktop\out\build\<config>\bin
#    makepkg genmap /f chunks.xml /d x64
#    makepkg pack /f chunks.xml /d x64 /pd . /pc
