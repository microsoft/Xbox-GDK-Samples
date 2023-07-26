# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.
#
# This is a development only tool. It is not supported for use on retail kits.

cmake_minimum_required (VERSION 3.20)

project(xbcompress
  DESCRIPTION "Microsoft SZDD/KWAJ-style compression tool for Windows & Xbox"
  HOMEPAGE_URL "https://github.com/microsoft/DirectXTK12/wiki/Compressing-assets"
  LANGUAGES CXX)

if(NOT WIN32)
   message(FATAL_ERROR "This tool is compatible with Windows 10 and Xbox")
endif()

if((NOT MSVC) OR (CMAKE_SIZEOF_VOID_P EQUAL 4))
   message(FATAL_ERROR "ERROR: This tool requires MSVC or MSVC-like 64-bit compiler")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# Always use retail static CRT for this tool
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)

add_executable(${PROJECT_NAME} compresstool.cpp)

target_compile_definitions(${PROJECT_NAME} PRIVATE _CONSOLE _UNICODE UNICODE _WIN32_WINNT=0x0A00)

target_compile_options(${PROJECT_NAME} PRIVATE /W4 /GR- /fp:fast /GS /Gy)

# Don't link with kernel32.lib, etc.
set(CMAKE_CXX_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "")

# Not all APIs in onecore_apiset.lib are in xgameplatform.lib, but there's
# sufficent overlap for this tool.
target_link_libraries(${PROJECT_NAME} PRIVATE onecore_apiset.lib)

target_link_options(${PROJECT_NAME} PRIVATE /SUBSYSTEM:CONSOLE,10.0 /NODEFAULTLIB:kernel32.lib /NODEFAULTLIB:onecore.lib)

if((MSVC_VERSION GREATER_EQUAL 1924)
  AND ((NOT (CMAKE_CXX_COMPILER_ID MATCHES "Clang")) OR (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 16.0)))
  target_compile_options(${PROJECT_NAME} PRIVATE /ZH:SHA_256)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus /Zc:inline)

   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor)
   endif()

   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27)
      target_link_options(${PROJECT_NAME} PRIVATE /CETCOMPAT)
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
endif()

# Deploy using:
#   xbcp bin\*.exe xd:\
#
# Run using:
#   xbrun /O D:\xbcompress.exe
