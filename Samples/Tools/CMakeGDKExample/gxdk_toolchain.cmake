#
# grdk_toolchain.cmake : CMake Toolchain file for Gaming.Xbox.XboxOne.x64
#
# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

mark_as_advanced(CMAKE_TOOLCHAIN_FILE)

if(_GXDK_TOOLCHAIN_)
  return()
endif()

# Microsoft Game Development Kit
set(XdkEditionTarget "251000" CACHE STRING "Microsoft GDK Edition")

message("XdkEditionTarget = ${XdkEditionTarget}")

set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES XdkEditionTarget)

set(CMAKE_SYSTEM_NAME WINDOWS)
set(CMAKE_SYSTEM_VERSION 10.0)
set(XBOX_CONSOLE_TARGET "xboxone" CACHE STRING "")

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
    
    # Store MSVC toolset directory for later use
    set(MSVC_TOOLSET_ROOT "${MSVC_TOOLSET_DIR}" CACHE INTERNAL "MSVC toolset root directory")
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
        set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/um\" /I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/shared\" /I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/ucrt\" /I\"${WindowsSdkRoot}/Include/${WindowsSdkVersion}/winrt\"" CACHE STRING "" FORCE)
        
        # Add Windows SDK library paths
        foreach(t EXE SHARED MODULE)
            string(APPEND CMAKE_${t}_LINKER_FLAGS_INIT " /LIBPATH:\"${WindowsSdkRoot}/Lib/${WindowsSdkVersion}/um/x64\" /LIBPATH:\"${WindowsSdkRoot}/Lib/${WindowsSdkVersion}/ucrt/x64\"")
        endforeach()
        
        # Add MSVC include path if available
        if(MSVC_TOOLSET_ROOT AND EXISTS "${MSVC_TOOLSET_ROOT}/include")
            set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /I\"${MSVC_TOOLSET_ROOT}/include\"" CACHE STRING "" FORCE)
            
            # Add MSVC library paths
            foreach(t EXE SHARED MODULE)
                string(APPEND CMAKE_${t}_LINKER_FLAGS_INIT " /LIBPATH:\"${MSVC_TOOLSET_ROOT}/lib/x64\"")
            endforeach()
        endif()
    endif()
endif()

set(CMAKE_GENERATOR_PLATFORM "Gaming.Xbox.XboxOne.x64" CACHE STRING "" FORCE)
set(CMAKE_VS_PLATFORM_NAME "Gaming.Xbox.XboxOne.x64" CACHE STRING "" FORCE)

# Ensure our platform toolset is x64
set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE "x64" CACHE STRING "" FORCE)

# Let the GDK MSBuild rules decide the WindowsTargetPlatformVersion
set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "" CACHE STRING "" FORCE)

# Propagate GDK version to MSBuild
set(CMAKE_VS_GLOBALS "XdkEditionTarget=${XdkEditionTarget}" CACHE STRING "" FORCE)

if(${CMAKE_VERSION} GREATER_EQUAL "3.30")
    set(CMAKE_VS_USE_DEBUG_LIBRARIES "$<CONFIG:Debug>" CACHE STRING "" FORCE)
endif()

# Sets platform defines
set(CMAKE_CXX_FLAGS_INIT "$ENV{CFLAGS} ${CMAKE_CXX_FLAGS_INIT} -D_GAMING_XBOX -D_GAMING_XBOX_ONE -DWINAPI_FAMILY=WINAPI_FAMILY_GAMES -D_ATL_NO_DEFAULT_LIBS -D__WRL_NO_DEFAULT_LIB__ -D_CRT_USE_WINAPI_PARTITION_APP -D_UITHREADCTXT_SUPPORT=0 -D__WRL_CLASSIC_COM_STRICT__ /arch:AVX /favor:AMD64" CACHE STRING "" FORCE)

# Set platform libraries
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "xgameplatform.lib" CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD_LIBRARIES ${CMAKE_CXX_STANDARD_LIBRARIES_INIT} CACHE STRING "" FORCE)

 foreach(t EXE SHARED MODULE)
    # Prevent accidental use of libraries that are not supported by Game Core on Xbox
    string(APPEND CMAKE_${t}_LINKER_FLAGS_INIT " /NODEFAULTLIB:advapi32.lib /NODEFAULTLIB:comctl32.lib /NODEFAULTLIB:comsupp.lib /NODEFAULTLIB:dbghelp.lib /NODEFAULTLIB:gdi32.lib /NODEFAULTLIB:gdiplus.lib /NODEFAULTLIB:guardcfw.lib /NODEFAULTLIB:kernel32.lib /NODEFAULTLIB:mmc.lib /NODEFAULTLIB:msimg32.lib /NODEFAULTLIB:msvcole.lib /NODEFAULTLIB:msvcoled.lib /NODEFAULTLIB:mswsock.lib /NODEFAULTLIB:ntstrsafe.lib /NODEFAULTLIB:ole2.lib /NODEFAULTLIB:ole2autd.lib /NODEFAULTLIB:ole2auto.lib /NODEFAULTLIB:ole2d.lib /NODEFAULTLIB:ole2ui.lib /NODEFAULTLIB:ole2uid.lib /NODEFAULTLIB:ole32.lib /NODEFAULTLIB:oleacc.lib /NODEFAULTLIB:oleaut32.lib /NODEFAULTLIB:oledlg.lib /NODEFAULTLIB:oledlgd.lib /NODEFAULTLIB:oldnames.lib /NODEFAULTLIB:runtimeobject.lib /NODEFAULTLIB:shell32.lib /NODEFAULTLIB:shlwapi.lib /NODEFAULTLIB:strsafe.lib /NODEFAULTLIB:urlmon.lib /NODEFAULTLIB:user32.lib /NODEFAULTLIB:userenv.lib /NODEFAULTLIB:wlmole.lib /NODEFAULTLIB:wlmoled.lib /NODEFAULTLIB:onecore.lib")
 endforeach()

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
  GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\GDK;InstallPath]" ABSOLUTE CACHE)

  find_program(
        GDK_DXCTool
        NAMES dxc
        PATHS "${Console_SdkRoot}/${XdkEditionTarget}/xbox/bin/gen8"
        )

  mark_as_advanced(GDK_DXCTool)
  endif()
  
  # Add GDK include paths
  GET_FILENAME_COMPONENT(Console_SdkRoot "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\GDK;InstallPath]" ABSOLUTE CACHE)
  if(Console_SdkRoot AND XdkEditionTarget)
      if(EXISTS "${Console_SdkRoot}/${XdkEditionTarget}")
          set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} /I\"${Console_SdkRoot}/${XdkEditionTarget}/xbox/include\" /I\"${Console_SdkRoot}/${XdkEditionTarget}/xbox/include/gen8\"" CACHE STRING "" FORCE)
          
          # Add GDK library paths (gen8 for Xbox One)
          foreach(t EXE SHARED MODULE)
              string(APPEND CMAKE_${t}_LINKER_FLAGS_INIT " /LIBPATH:\"${Console_SdkRoot}/${XdkEditionTarget}/xbox/lib/x64\" /LIBPATH:\"${Console_SdkRoot}/${XdkEditionTarget}/xbox/lib/gen8\"")
          endforeach()
      endif()
  endif()
  
  # Fix issue with Threads package
set(CMAKE_USE_WIN32_THREADS_INIT 1)
set(Threads_FOUND TRUE)

set(_GXDK_TOOLCHAIN_ ON)
