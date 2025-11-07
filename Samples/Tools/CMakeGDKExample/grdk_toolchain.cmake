#
# grdk_toolchain.cmake : CMake Toolchain file for x64
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

if(_GRDK_TOOLCHAIN_)
  return()
endif()

set(CMAKE_SYSTEM_NAME WINDOWS)
set(CMAKE_SYSTEM_VERSION 10.0)

set(XdkEditionTarget "251000" CACHE STRING "Microsoft GDK Edition")

# Set explicit compiler paths using environment variables
set(PROGRAM_FILES_X86 "$ENV{ProgramFiles\(x86\)}")

# Find the latest MSVC toolset
file(GLOB MSVC_VERSIONS
    "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/*/VC/Tools/MSVC/*"
    "$ENV{ProgramFiles}/Microsoft Visual Studio/2019/*/VC/Tools/MSVC/*"
    "${PROGRAM_FILES_X86}/Microsoft Visual Studio/2019/*/VC/Tools/MSVC/*"
)

if(MSVC_VERSIONS)
    list(GET MSVC_VERSIONS -1 MSVC_TOOLSET_DIR)
    
    # Check if we're using Clang toolset
    if(CMAKE_VS_PLATFORM_TOOLSET MATCHES "ClangCL" OR CMAKE_GENERATOR_TOOLSET MATCHES "ClangCL")
        # Use Clang compiler from Visual Studio
        find_program(CLANG_CL_EXECUTABLE
            NAMES clang-cl.exe
            PATHS
                "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/*/VC/Tools/Llvm/x64/bin"
                "$ENV{ProgramFiles}/Microsoft Visual Studio/2019/*/VC/Tools/Llvm/x64/bin"
            NO_DEFAULT_PATH
        )
        
        if(CLANG_CL_EXECUTABLE)
            set(CMAKE_CXX_COMPILER "${CLANG_CL_EXECUTABLE}" CACHE FILEPATH "CXX compiler")
            set(CMAKE_C_COMPILER "${CLANG_CL_EXECUTABLE}" CACHE FILEPATH "C compiler")
        endif()
    else()
        # Use MSVC compiler
        set(CMAKE_CXX_COMPILER "${MSVC_TOOLSET_DIR}/bin/Hostx64/x64/cl.exe" CACHE FILEPATH "CXX compiler")
        set(CMAKE_C_COMPILER "${MSVC_TOOLSET_DIR}/bin/Hostx64/x64/cl.exe" CACHE FILEPATH "C compiler")
        
        # Ensure CMake recognizes this as MSVC
        set(CMAKE_CXX_COMPILER_ID "MSVC" CACHE STRING "CXX compiler ID")
        set(CMAKE_C_COMPILER_ID "MSVC" CACHE STRING "C compiler ID")
        
        # Set MSVC version information
        set(MSVC_VERSION "1944" CACHE STRING "MSVC version")
        set(CMAKE_CXX_COMPILER_VERSION "19.44.35219.0" CACHE STRING "CXX compiler version")
        set(CMAKE_C_COMPILER_VERSION "19.44.35219.0" CACHE STRING "C compiler version")
        
        # Set C++ standard defaults for MSVC
        set(CMAKE_CXX_STANDARD_COMPUTED_DEFAULT "14" CACHE STRING "Default C++ standard")
        set(CMAKE_CXX_EXTENSIONS_COMPUTED_DEFAULT "OFF" CACHE STRING "Default C++ extensions")
    endif()
    
    # Store MSVC toolset directory for later use
    set(MSVC_TOOLSET_ROOT "${MSVC_TOOLSET_DIR}" CACHE INTERNAL "MSVC toolset root directory")
endif()

set(CMAKE_GENERATOR_PLATFORM "x64" CACHE STRING "" FORCE)
set(CMAKE_VS_PLATFORM_NAME "x64" CACHE STRING "" FORCE)

# Ensure our platform toolset is x64
set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x64" CACHE STRING "" FORCE)

# Let the GDK MSBuild rules decide the WindowsTargetPlatformVersion
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "" CACHE STRING "" FORCE)

# Propagate GDK version to MSBuild
set(CMAKE_VS_GLOBALS "XdkEditionTarget=${XdkEditionTarget}" CACHE STRING "" FORCE)

if(${CMAKE_VERSION} GREATER_EQUAL "3.30")
    set(CMAKE_VS_USE_DEBUG_LIBRARIES "$<CONFIG:Debug>" CACHE STRING "" FORCE)
endif()

# Find Windows SDK using environment variables with fallback
if(DEFINED ENV{WindowsSdkDir})
    set(WindowsSdkRoot "$ENV{WindowsSdkDir}")
elseif(DEFINED ENV{ProgramFiles\(x86\)})
    set(WindowsSdkRoot "$ENV{ProgramFiles\(x86\)}/Windows Kits/10")
else()
    set(WindowsSdkRoot "C:/Program Files (x86)/Windows Kits/10")
endif()

if(EXISTS "${WindowsSdkRoot}")
    # Find the latest Windows SDK version
    file(GLOB WindowsSdkVersions "${WindowsSdkRoot}/Include/10.0.*")
    if(WindowsSdkVersions)
        list(SORT WindowsSdkVersions)
        list(GET WindowsSdkVersions -1 WindowsSdkLatestPath)
        get_filename_component(WindowsSdkVersion ${WindowsSdkLatestPath} NAME)
        
        # Add Windows SDK include paths to compiler flags
        # Check if we're using Clang and use appropriate flag format
        if(CMAKE_VS_PLATFORM_TOOLSET MATCHES "ClangCL" OR CMAKE_GENERATOR_TOOLSET MATCHES "ClangCL")
            set(CMAKE_CXX_FLAGS_INIT "$ENV{CFLAGS} ${CMAKE_CXX_FLAGS_INIT} -D_GAMING_DESKTOP -DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP -I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/um\" -I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/shared\" -I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/ucrt\" -I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/winrt\"" CACHE STRING "" FORCE)
        else()
            set(CMAKE_CXX_FLAGS_INIT "$ENV{CFLAGS} ${CMAKE_CXX_FLAGS_INIT} -D_GAMING_DESKTOP -DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP /I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/um\" /I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/shared\" /I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/ucrt\" /I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/winrt\"" CACHE STRING "" FORCE)
        endif()
        
        # Add Windows SDK library paths
        foreach(t EXE SHARED MODULE)
            string(APPEND CMAKE_${t}_LINKER_FLAGS_INIT " /LIBPATH:\"${WindowsSdkRoot}/Lib/${WindowsSdkVersion}/um/x64\" /LIBPATH:\"${WindowsSdkRoot}/Lib/${WindowsSdkVersion}/ucrt/x64\"")
        endforeach()
        
        # Add MSVC include path if available
        if(MSVC_TOOLSET_ROOT AND EXISTS "${MSVC_TOOLSET_ROOT}/include")
            # Check if we're using Clang and use appropriate flag format
            if(CMAKE_VS_PLATFORM_TOOLSET MATCHES "ClangCL" OR CMAKE_GENERATOR_TOOLSET MATCHES "ClangCL")
                set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -I\"${MSVC_TOOLSET_ROOT}/include\"" CACHE STRING "" FORCE)
            else()
                set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /I\"${MSVC_TOOLSET_ROOT}/include\"" CACHE STRING "" FORCE)
            endif()
            
            # Add MSVC library paths
            foreach(t EXE SHARED MODULE)
                string(APPEND CMAKE_${t}_LINKER_FLAGS_INIT " /LIBPATH:\"${MSVC_TOOLSET_ROOT}/lib/x64\"")
            endforeach()
        endif()
    else()
        # Sets platform defines (fallback if no Windows SDK found)
        set(CMAKE_CXX_FLAGS_INIT "$ENV{CFLAGS} ${CMAKE_CXX_FLAGS_INIT} -D_GAMING_DESKTOP -DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP" CACHE STRING "" FORCE)
    endif()
else()
    # Sets platform defines (fallback if no Windows SDK found)
    set(CMAKE_CXX_FLAGS_INIT "$ENV{CFLAGS} ${CMAKE_CXX_FLAGS_INIT} -D_GAMING_DESKTOP -DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP" CACHE STRING "" FORCE)
endif()

# Add GDK include paths for Desktop
GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\GDK;InstallPath]" ABSOLUTE CACHE)
if(Console_SdkRoot AND XdkEditionTarget)
    if(EXISTS "${Console_SdkRoot}/${XdkEditionTarget}")
        # Check if we're using Clang and use appropriate flag format for GDK include
        if(CMAKE_VS_PLATFORM_TOOLSET MATCHES "ClangCL" OR CMAKE_GENERATOR_TOOLSET MATCHES "ClangCL")
            set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -I\"${Console_SdkRoot}/${XdkEditionTarget}/windows/include\"" CACHE STRING "" FORCE)
        else()
            set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /I\"${Console_SdkRoot}/${XdkEditionTarget}/windows/include\"" CACHE STRING "" FORCE)
        endif()
        
        # Add GDK library paths for Desktop
        foreach(t EXE SHARED MODULE)
            string(APPEND CMAKE_${t}_LINKER_FLAGS_INIT " /LIBPATH:\"${Console_SdkRoot}/${XdkEditionTarget}/windows/lib/x64\"")
        endforeach()
    endif()
endif()

# Add GDK props file
file(GENERATE OUTPUT gdk_build.props INPUT ${CMAKE_CURRENT_LIST_DIR}/gdk_build.props)

function(add_executable target_name)
  _add_executable(${target_name} ${ARGN})
  set_target_properties(${target_name} PROPERTIES VS_USER_PROPS gdk_build.props)
endfunction()

function(add_library target_name)
  _add_library(${target_name} ${ARGN})
  set_target_properties(${target_name} PROPERTIES VS_USER_PROPS gdk_build.props)
endfunction()

# Find DXC compiler
if(NOT GDK_DXCTool)
  set(GDK_DXCTool "dxc.exe")
  mark_as_advanced(GDK_DXCTool)
endif()

# The MicrosoftGame.Config file needs to be in the directory with the project to enable deploy for x64
file(GENERATE OUTPUT MicrosoftGame.Config INPUT ${CMAKE_CURRENT_LIST_DIR}/MicrosoftGameConfig.mgc)

# Fix issue with Threads package
set(CMAKE_USE_WIN32_THREADS_INIT 1)
set(Threads_FOUND TRUE)

set(_GRDK_TOOLCHAIN_ ON)
