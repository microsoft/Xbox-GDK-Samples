# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# This is intended for test scenarios only using classic printf-style output.
# Use of Direct3D is not supported, and the resulting EXE is not supported for retail.
#

cmake_minimum_required (VERSION 3.20)

project(CMakeXboxConsoleApp
  DESCRIPTION "CMake example for a Microsoft GDK 'console' project"
  LANGUAGES CXX)

# See the BWOIExample for more information
option(BUILD_USING_BWOI "Enable Build With/Out Install" OFF)

option(OPTIMIZE_FOR_SCARLETT "Optimize for the AMD Hercules CPU" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)

include(CMakeGDKXbox.cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Console")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Console")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Console")

add_executable(${PROJECT_NAME} Main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${Console_SdkIncludeRoot})
target_link_directories(${PROJECT_NAME} PUBLIC ${VC_OneCore_LibPath} PUBLIC ${Console_SdkLibPath})

# Visual C++ console projects set this symbol
target_compile_definitions(${PROJECT_NAME} PRIVATE _CONSOLE)

# See the CMakeExample for more information
target_compile_definitions(${PROJECT_NAME} PRIVATE ${Console_Defines})

# If not using typeid or dynamic_cast, we can disable RTTI to save binary size
target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast /W4 /GR- /GS /Gy ${Console_ArchOptions})

target_link_libraries(${PROJECT_NAME} PRIVATE ${Console_Libs})
# optional extension libraries: Xbox::XSAPI Xbox::XCurl Xbox::GameChat2 Xbox::PlayFabMultiplayer Xbox::PlayFabParty Xbox::PlayFabPartyLIVE Xbox::PlayFabServices

target_link_options(${PROJECT_NAME} PRIVATE ${Console_LinkOptions})

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus /Zc:inline)

   if(ENABLE_CODE_ANALYSIS)
      target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
   endif()

   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
      target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
   endif()

   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor)
   endif()

   if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:lambda)
   endif()

   if((NOT (CMAKE_BUILD_TYPE MATCHES "Debug")) AND CMAKE_INTERPROCEDURAL_OPTIMIZATION)
      target_compile_options(${PROJECT_NAME} PRIVATE /Gw)

       if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
           target_compile_options(${PROJECT_NAME} PRIVATE /Zc:checkGwOdr)
       endif()

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

   target_link_options(${PROJECT_NAME} PRIVATE /IGNORE:4078)
endif()

# Copy VC Runtime and Extension Library DLLs
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CppRuntimeFilesPath}
    $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )

if(CMAKE_BUILD_TYPE MATCHES "Debug")
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

# Uncomment if using any extension libraries
#if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21")
#   add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#      COMMAND ${CMAKE_COMMAND} -E copy
#      $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
#      COMMAND_EXPAND_LISTS
#      )
#endif()

# Use of OpenMP requires also copying the files from Microsoft.VC14?.OpenMP
# See ${OpenMPRuntimeFilesPath}, ${DebugOpenMPRuntimeFilesPath} in CMakeGDKXbox.cmake

# Deploy using:
#   xbcp bin\Console\*.exe xd:\
#   xbcp bin\Console\*.dll xd:\
#
# Run using:
#   xbrun /O D:\CMakeXboxConsoleApp.exe

