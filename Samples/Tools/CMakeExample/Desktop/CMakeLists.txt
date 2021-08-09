# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

cmake_minimum_required (VERSION 3.13)

project(CMakeExampleDesktop
  DESCRIPTION "CMake example for a Microsoft GDK Gaming.Desktop.x64 project"
  LANGUAGES CXX)

option(BUILD_USING_LTCG "Enable Whole Program Optimization" ON)

# https://docs.microsoft.com/en-us/windows/win32/apiindex/umbrella-lib-onecore
option(BUILD_USING_ONECORE "Build for Windows 10 only" OFF)

# See the BWOIExample for more information
option(BUILD_USING_BWOI "Enable Build With/Out Install" OFF)

option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

include(CMakeGDKDesktop.cmake)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Gaming.Desktop.x64")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Gaming.Desktop.x64")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/Gaming.Desktop.x64")

# Libraries
# kernel32.lib user32.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib are the libs for Win32 desktop DirectX apps
# alternatively you can use onecore.lib for Win32 desktop apps that only support Windows 10

if(BUILD_USING_ONECORE)
    set(CMAKE_CXX_STANDARD_LIBRARIES "onecore.lib")
    set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "onecore.lib")
endif()

# WARNING: Only ever use _one_ of: kernel32.lib, onecore.lib, onecoreuap.lib, WindowsApp.lib, xgameplatform.lib

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

target_include_directories(${PROJECT_NAME} PUBLIC ./ ../ ${Console_SdkIncludeRoot} PUBLIC ${Console_ExtIncPath})
target_link_directories(${PROJECT_NAME} PUBLIC ${Console_SdkLibPath} PUBLIC ${Console_ExtLibPath})

# Build HLSL shaders
add_custom_target(shaders)

set_source_files_properties(../VertexShader.hlsl PROPERTIES ShaderType "vs")
set_source_files_properties(../PixelShader.hlsl PROPERTIES ShaderType "ps")

foreach(FILE ../VertexShader.hlsl ../PixelShader.hlsl)
  get_filename_component(FILE_WE ${FILE} NAME_WE)
  get_source_file_property(shadertype ${FILE} ShaderType)
  add_custom_command(TARGET shaders
                     COMMAND ${DXCToolPath}/dxc.exe /nologo /Emain /T${shadertype}_6_0 $<$<CONFIG:DEBUG>:/Od> /Zi /Fo ${CMAKE_BINARY_DIR}/${FILE_WE}.cso /Fd ${CMAKE_BINARY_DIR}/${FILE_WE}.pdb ${FILE}
                     MAIN_DEPENDENCY ${FILE}
                     COMMENT "HLSL ${FILE}"
                     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                     VERBATIM)
endforeach(FILE)

# Microsoft Config File
add_custom_target(mgccompile)

add_custom_command(TARGET mgccompile
                   COMMAND ${MGCToolPath}/makepkg.exe localize /d "${CMAKE_CURRENT_SOURCE_DIR}" /pd "${CMAKE_BINARY_DIR}/bin/Gaming.Desktop.x64"
                   MAIN_DEPENDENCY MicrosoftGame.config
                   COMMENT "makepkg localize MicrosoftGame.config"
                   WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                   VERBATIM)

add_dependencies(${PROJECT_NAME} mgccompile shaders)

# Use Warning Level 4
string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "/W3 " "/W4 " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

# If not using typeid or dynamic_cast, we can disable RTTI to save binary size
string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "/GR " "/GR- " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

target_compile_definitions(${PROJECT_NAME} PRIVATE ${Console_Defines})

# Recommended compiler switches:
# /EHsc (C++ exception handling)
# /fp:fast (Fast floating-point optimizations)
# /GS (buffer security check)
# /Gy (enable function-level linking)
# /Zi (Debug Information Format)
target_compile_options(${PROJECT_NAME} PRIVATE /fp:fast /GS /Gy)

target_link_libraries(${PROJECT_NAME} PRIVATE ${Console_Libs} ${XSAPI_Libs})
# uuid.lib if using Windows Imaging Component (WIC) or other COM objects
# delayimp.lib if using /DELAYLOAD (or use dloadhelper.lib if BUILD_USING_ONECORE is ON)

# Optional precompiled header switches:
# /Ycpch.h (Create Precompiled Header File)
# /Yupch.h (Use Precompiled Header File)
# /Fp (Name .pch file)
# /FI (Name Force Include File)
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(${PROJECT_NAME} PRIVATE ../pch.h )
endif()

target_link_options(${PROJECT_NAME} PRIVATE ${Console_LinkOptions})

if(BUILD_USING_ONECORE)
    target_link_options(${PROJECT_NAME} PRIVATE "/NODEFAULTLIB:kernel32.lib")
else()
# Could add /NODEFAULTLIB:onecore.lib for this case, but only if you use #pragma comment(lib,"onecore.lib") in your code
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
   # /permissive- (C++ conformance mode enabled)
   # /Zc:__cplusplus (Enable updated __cplusplus macro)
   target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus)

   # /analyze (Code analysis)
   if(ENABLE_CODE_ANALYSIS)
      target_compile_options(${PROJECT_NAME} PRIVATE /analyze)
   endif()

   # /Zc:preprocessor (C99 conformant preprocessor)
   if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
      target_compile_options(${PROJECT_NAME} PRIVATE /Zc:preprocessor /wd5105)
   endif()

   if(CMAKE_BUILD_TYPE MATCHES "Debug")
       # Optional debug compiler switches:
       # /RTC (Runtime Checks)
       # /JMC (Just My Code debugging)

       # Optional debug linker switches:
       # /INCREMENTAL
       # /DEBUG or /DEBUG:fastlink
   elseif(BUILD_USING_LTCG)
       # /GL (Whole Program Optimization)
       # /Gw (Optimize Global Data)
       target_compile_options(${PROJECT_NAME} PRIVATE /GL /Gw)

       # /LTCG (Link-time Code Generation)
       # Ignores warning from CMake generator use of /INCREMENTAL
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

# Copy config and images
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/MicrosoftGame.Config
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

# MSIXVC Packaging using the makepkg tool in the GDK:
#    cd CMakeExample\Desktop\out\build\<config>\bin
#    makepkg genmap /f chunks.xml /d Gaming.Desktop.x64
#    makepkg pack /f chunks.xml /d Gaming.Desktop.x64 /pd . /pc
