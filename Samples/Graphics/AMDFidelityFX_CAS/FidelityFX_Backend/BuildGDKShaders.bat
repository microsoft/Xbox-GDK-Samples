set "projectdir=%1"
set "platformname=%2"

:: remove all quotes because arguments can contain them
set projectdir=%projectdir:"=%
set platformname=%platformname:"=%

:: get the absolute path to the FidelityFX directory to avoid issues with long relative path names
pushd %projectdir%..\..\..\..\Kits\AMDTK\fidelityFX
set sdkdir=%cd%
popd

:: set additional variables with quotes to support paths with spaces
set outputdir="%sdkdir%\sc_bin\%platformname%"

set FFX_BASE_INCLUDE_ARGS=-I "%sdkdir%\include\FidelityFX\gpu"
set FFX_SC="%sdkdir%\tools\binary_store\FidelityFX_SC.exe"
set FFX_API_BASE_ARGS=-embed-arguments -E CS -Wno-for-redefinition -Wno-ambig-lit-shift -DFFX_HLSL=1
if %platformname% == Gaming.Desktop.x64 (
	set FFX_API_BASE_ARGS=-E CS -Wno-for-redefinition -Wno-ambig-lit-shift -DFFX_HLSL=1
)

set FFX_BASE_ARGS=-reflection -deps=gcc -DFFX_GPU=1

set HLSL_WAVE64_ARGS=-DFFX_PREFER_WAVE64="[WaveSize(64)]" -DFFX_HLSL_SM=66 -T cs_6_6
set HLSL_WAVE32_ARGS=-DFFX_HLSL_SM=62 -T cs_6_2
set HLSL_16BIT_ARGS=
set FFX_GDK_OPTION=

if %platformname% == Gaming.Desktop.x64 (
    set HLSL_16BIT_ARGS=-DFFX_HALF=1 -enable-16bit-types
    set FFX_GDK_OPTION=-compiler=dxc -Zs
)
if %platformname% == Gaming.Xbox.Scarlett.x64 (
    set HLSL_16BIT_ARGS=-DFFX_HALF=1 -enable-16bit-types
    set FFX_GDK_OPTION=-compiler=gdk.scarlett.x64 -Zs
)
if %platformname% == Gaming.Xbox.XboxOne.x64 (
    set FFX_GDK_OPTION=-compiler=gdk.xboxone.x64 -Zs
)

:: CAS shaders
set CAS_API_BASE_ARGS=%FFX_API_BASE_ARGS% -DFFX_CAS_EMBED_ROOTSIG=1
if %platformname% == Gaming.Desktop.x64 (
	set CAS_API_BASE_ARGS=%FFX_API_BASE_ARGS%
)
set CAS_BASE_ARGS=%FFX_BASE_ARGS%
set CAS_PERMUTATION_ARGS=-DFFX_CAS_OPTION_SHARPEN_ONLY={0,1} -DFFX_CAS_COLOR_SPACE_CONVERSION={0,1,2,3,4}
set CAS_INCLUDE_ARGS=%FFX_BASE_INCLUDE_ARGS% -I "%sdkdir%\include\FidelityFX\gpu\cas"
set CAS_SC_ARGS=%CAS_BASE_ARGS% %CAS_API_BASE_ARGS% %CAS_PERMUTATION_ARGS%

set BACKEND_SHADER_DIR=%sdkdir%\src\backends\gdk\shaders
if %platformname% == Gaming.Desktop.x64 (
	set BACKEND_SHADER_DIR=%sdkdir%\src\backends\dx12\shaders
)

:: ffx_cas_sharpen_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CAS_SC_ARGS% -name=ffx_cas_sharpen_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CAS_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cas\ffx_cas_sharpen_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CAS_SC_ARGS% -name=ffx_cas_sharpen_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CAS_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cas\ffx_cas_sharpen_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CAS_SC_ARGS% -name=ffx_cas_sharpen_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CAS_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cas\ffx_cas_sharpen_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CAS_SC_ARGS% -name=ffx_cas_sharpen_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CAS_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cas\ffx_cas_sharpen_pass.hlsl"

exit /b 0