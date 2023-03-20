#
# CMakeGDKDesktop.cmake : CMake definitions for Microsoft GDK targeting PC
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

if(_GDK_DESKTOP_TOOLCHAIN_)
  return()
endif()

#--- Microsoft Game Development Kit
set(XdkEditionTarget "220300" CACHE STRING "Microsoft GDK Edition")

message("XdkEditionTarget = ${XdkEditionTarget}")

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES XdkEditionTarget BUILD_USING_BWOI)

set(CMAKE_SYSTEM_NAME WINDOWS)
set(CMAKE_SYSTEM_VERSION 10.0)

#--- GameRuntime and Extension Libraries
include(${CMAKE_CURRENT_LIST_DIR}/GDK-targets.cmake)

message("Microsoft GDK = ${Console_SdkRoot}/${XdkEditionTarget}")

#--- Tools
find_program(MAKEPKG_TOOL makepkg.exe
    REQUIRED NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_DEFAULT_PATH
    HINTS "${Console_SdkRoot}/bin")

message("MGC Tool = ${MAKEPKG_TOOL}")

find_program(DIRECTX_DXC_TOOL dxc.exe REQUIRED)

message("DXC Compiler = ${DIRECTX_DXC_TOOL}")

set(_GDK_DESKTOP_TOOLCHAIN_ ON)
