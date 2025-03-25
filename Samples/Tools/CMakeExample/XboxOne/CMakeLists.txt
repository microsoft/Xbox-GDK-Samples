# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.20)

project(CMakeExampleXboxOne
  DESCRIPTION "CMake example for a Microsoft GDK Gaming.Xbox.XboxOne.x64 project"
  HOMEPAGE_URL "https://github.com/microsoft/Xbox-GDK-Samples"
  LANGUAGES CXX)

# See the BWOIExample for more information
option(BUILD_USING_BWOI "Enable Build With/Out Install" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

include(../CMake/CMakeGDKXboxOne.cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/Gaming.Xbox.XboxOne.x64")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/Gaming.Xbox.XboxOne.x64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Gaming.Xbox.XboxOne.x64")

add_executable(${PROJECT_NAME} WIN32
    ../Game.cpp
    ../Game.h
    ../DeviceResources.cpp
    ../DeviceResources.h
    ../Main.cpp
    ../StepTimer.h
    ../pch.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ./ ../ ${Console_SdkIncludeRoot})
target_link_directories(${PROJECT_NAME} PUBLIC ${VC_OneCore_LibPath} PUBLIC ${Console_SdkLibPath})

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
set(TDF_NAME "XboxOne")
configure_file(../MicrosoftGameConfig.mgc MicrosoftGame.config @ONLY)

add_custom_target(mgccompile)

add_custom_command(TARGET mgccompile
                   COMMAND ${MAKEPKG_TOOL} localize /d "${CMAKE_BINARY_DIR}" /pd "${CMAKE_BINARY_DIR}/bin/Gaming.Xbox.XboxOne.x64"
                   MAIN_DEPENDENCY ../MicrosoftGameConfig.mgc
                   COMMENT "makepkg localize MicrosoftGame.config"
                   WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                   VERBATIM)

add_dependencies(${PROJECT_NAME} mgccompile shaders)

target_compile_definitions(${PROJECT_NAME} PRIVATE $<IF:$<CONFIG:DEBUG>,_DEBUG,NDEBUG> ${Console_Defines})

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

target_link_libraries(${PROJECT_NAME} PRIVATE ${Console_Libs} Xbox::XSAPI Xbox::XCurl)
# uuid.lib if using Windows Imaging Component (WIC)
# dloadhelper.lib if using /DELAYLOAD
# optional additional extension libraries: Xbox::GameChat2 Xbox::PlayFabMultiplayer Xbox::PlayFabParty Xbox::PlayFabPartyLIVE Xbox::PlayFabServices

# Optional precompiled header switches:
# /Ycpch.h (Create Precompiled Header File)
# /Yupch.h (Use Precompiled Header File)
# /Fp (Name .pch file)
# /FI (Name Force Include File)
target_precompile_headers(${PROJECT_NAME} PRIVATE ../pch.h)

target_link_options(${PROJECT_NAME} PRIVATE ${Console_LinkOptions})

target_compile_options(${PROJECT_NAME} PRIVATE ${Console_ArchOptions})

# /ZH:SHA_256 (secure source hashing)
if((MSVC_VERSION GREATER_EQUAL 1924)
  AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)))
  target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   # /Zc:inline (Remove unreferenced COMDAT)
   target_compile_options(${PROJECT_NAME} PRIVATE /Zc:inline)

   # /analyze (Code analysis)
   if(ENABLE_CODE_ANALYSIS)
      target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
   endif()

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
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5105)
   endif()

   # /Zc:lambda (Enable updated lambda processor)
   if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:lambda)
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

       # /Zc:checkGwOdr (Enforce Standard C++ ODR violations under /Gw)
       if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
           target_compile_options(${PROJECT_NAME} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/Zc:checkGwOdr>)
       endif()

       # /LTCG (Link-time Code Generation)
       # Ignores warning from CMake generator use of /INCREMENTAL
       target_link_options(${PROJECT_NAME} PRIVATE /IGNORE:4075 ${Console_ArchOptions_LTCG})
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

# Copy config, images, VC Runtime, Game OS, and Extension Library DLLs
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
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CppRuntimeFilesPath}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
  COMMAND ${CMAKE_COMMAND} -E copy
    ${GameOSFilePath}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if((CMAKE_BUILD_TYPE MATCHES "Debug") OR is_multi_config)
    add_custom_command(
      TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        ${Console_UCRTRedistDebug}/ucrtbased.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${DebugCppRuntimeFilesPath}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
endif()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
      $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
      COMMAND_EXPAND_LISTS
      )
endif()

# Use of OpenMP requires also copying the files from Microsoft.VC14?.OpenMP
# See ${OpenMPRuntimeFilesPath}, ${DebugOpenMPRuntimeFilesPath} in CMakeGDKXboxOne.cmake

# Packaging using the makepkg tool in the GDK:
#    cd CMakeExample\XboxOne\out\build\<config>\bin
#    makepkg genmap /f chunks.xml /d Gaming.Xbox.XboxOne.x64
#    makepkg pack /f chunks.xml /lt /d Gaming.Xbox.XboxOne.x64 /pd .
