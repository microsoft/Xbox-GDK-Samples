# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# This is intended for test scenarios only using classic printf-style output.
# Use of Direct3D is not supported, and the resulting EXE is not supported for retail.
#

cmake_minimum_required (VERSION 3.13)

project(CMakeXboxConsoleApp
  DESCRIPTION "CMake example for a Microsoft GDK 'console' project"
  LANGUAGES CXX)

option(BUILD_USING_LTCG "Enable Whole Program Optimization" ON)

# See the BWOIExample for more information
option(BUILD_USING_BWOI "Enable Build With/Out Install" OFF)

option(OPTIMIZE_FOR_SCARLETT "Optimize for the AMD Hercules CPU" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

include(CMakeGDKXbox.cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Console")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Console")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Console")

add_executable(${PROJECT_NAME} Main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${Console_SdkIncludeRoot} PUBLIC ${Console_ExtIncPath})
target_link_directories(${PROJECT_NAME} PUBLIC ${VC_OneCore_LibPath} PUBLIC ${Console_SdkLibPath} PUBLIC ${Console_ExtLibPath})

# Use Warning Level 4
string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

# If not using typeid or dynamic_cast, we can disable RTTI to save binary size
string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

# Visual C++ console projects set this symbol
target_compile_definitions(${PROJECT_NAME} PRIVATE _CONSOLE)

# See the CMakeExample for more information
target_compile_definitions(${PROJECT_NAME} PRIVATE ${Console_Defines})

target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast /GS /Gy ${Console_ArchOptions})

target_link_libraries(${PROJECT_NAME} PRIVATE ${Console_Libs} ${XSAPI_Libs})

target_link_options(${PROJECT_NAME} PRIVATE ${Console_LinkOptions})

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus)

   if(ENABLE_CODE_ANALYSIS)
      target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
   endif()

   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor)
   endif()

   if((NOT CMAKE_BUILD_TYPE MATCHES "Debug") AND (BUILD_USING_LTCG))
      target_compile_options(${PROJECT_NAME} PRIVATE /GL /Gw)
      target_link_options(${PROJECT_NAME} PRIVATE /IGNORE:4075 /LTCG)
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

# Copy VC Runtime
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

# Use of OpenMP requires also copying the files from Microsoft.VC14?.OpenMP
# See ${OpenMPRuntimeFilesPath}, ${DebugOpenMPRuntimeFilesPath} in CMakeGDKXbox.cmake

# Deploy using:
#   xbcp bin\Console\*.exe xd:\
#   xbcp bin\Console\*.dll xd:\
#
# Run using:
#   xbrun /O D:\CMakeXboxConsoleApp.exe

