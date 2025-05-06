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
    set HLSL_WAVE32_ARGS=-DFFX_HLSL_SM=62 -T cs_6_2 -D__XBOX_ENABLE_WAVE32=1
)
if %platformname% == Gaming.Xbox.XboxOne.x64 (
    set FFX_GDK_OPTION=-compiler=gdk.xboxone.x64 -Zs
)

:: FSR1 shaders
set FSR1_API_BASE_ARGS=%FFX_API_BASE_ARGS% -DFFX_FSR1_EMBED_ROOTSIG=1
if %platformname% == Gaming.Desktop.x64 (
    set FSR1_API_BASE_ARGS=%FFX_API_BASE_ARGS%
)
set FSR1_BASE_ARGS=%FFX_BASE_ARGS%
set FSR1_PERMUTATION_ARGS=-DFFX_FSR1_OPTION_APPLY_RCAS={0,1} -DFFX_FSR1_OPTION_RCAS_PASSTHROUGH_ALPHA={0,1} -DFFX_FSR1_OPTION_SRGB_CONVERSIONS={0,1}
set FSR1_INCLUDE_ARGS=%FFX_BASE_INCLUDE_ARGS% -I "%sdkdir%\include\FidelityFX\gpu\fsr1"
set FSR1_SC_ARGS=%FSR1_BASE_ARGS% %FSR1_API_BASE_ARGS% %FSR1_PERMUTATION_ARGS%

set BACKEND_SHADER_DIR=%sdkdir%\src\backends\gdk\shaders
if %platformname% == Gaming.Desktop.x64 (
    set BACKEND_SHADER_DIR=%sdkdir%\src\backends\dx12\shaders
)

:: ffx_fsr1_easu_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR1_SC_ARGS% -name=ffx_fsr1_easu_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR1_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr1\ffx_fsr1_easu_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR1_SC_ARGS% -name=ffx_fsr1_easu_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR1_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr1\ffx_fsr1_easu_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR1_SC_ARGS% -name=ffx_fsr1_easu_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR1_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr1\ffx_fsr1_easu_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR1_SC_ARGS% -name=ffx_fsr1_easu_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR1_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr1\ffx_fsr1_easu_pass.hlsl"

:: ffx_fsr1_rcas_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR1_SC_ARGS% -name=ffx_fsr1_rcas_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR1_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr1\ffx_fsr1_rcas_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR1_SC_ARGS% -name=ffx_fsr1_rcas_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR1_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr1\ffx_fsr1_rcas_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR1_SC_ARGS% -name=ffx_fsr1_rcas_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR1_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr1\ffx_fsr1_rcas_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR1_SC_ARGS% -name=ffx_fsr1_rcas_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR1_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr1\ffx_fsr1_rcas_pass.hlsl"

:: FSR2 shaders
set FSR2_API_BASE_ARGS=%FFX_API_BASE_ARGS% -DFFX_FSR2_EMBED_ROOTSIG=1
set FSR2_BASE_ARGS=-DFFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF=0 -DFFX_FSR2_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF=0 -DFFX_FSR2_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF=1 -DFFX_FSR2_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF=0 -DFFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE=2 %FFX_BASE_ARGS%
set FSR2_BASE_ARGS=%FSR2_BASE_ARGS% -DFFX_FSR2_GDK_VERSION=%GDKEDITION%
set FSR2_PERMUTATION_ARGS=-DFFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE={0,1} -DFFX_FSR2_OPTION_HDR_COLOR_INPUT={0,1} -DFFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS={0,1} -DFFX_FSR2_OPTION_JITTERED_MOTION_VECTORS={0,1} -DFFX_FSR2_OPTION_INVERTED_DEPTH={0,1} -DFFX_FSR2_OPTION_APPLY_SHARPENING={0,1}
set FSR2_INCLUDE_ARGS=%FFX_BASE_INCLUDE_ARGS% -I "%sdkdir%\include\FidelityFX\gpu\fsr2"
set FSR2_SC_ARGS=%FSR2_BASE_ARGS% %FSR2_API_BASE_ARGS% %FSR2_PERMUTATION_ARGS%

:: ffx_fsr2_accumulate_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_accumulate_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_accumulate_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_accumulate_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_accumulate_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_accumulate_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_accumulate_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_accumulate_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_accumulate_pass.hlsl"

:: ffx_fsr2_autogen_reactive_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_autogen_reactive_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_autogen_reactive_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_autogen_reactive_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_autogen_reactive_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_autogen_reactive_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_autogen_reactive_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_autogen_reactive_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_autogen_reactive_pass.hlsl"

:: ffx_fsr2_compute_luminance_pyramid_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_compute_luminance_pyramid_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_compute_luminance_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_compute_luminance_pyramid_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_compute_luminance_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_compute_luminance_pyramid_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_compute_luminance_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_compute_luminance_pyramid_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_compute_luminance_pyramid_pass.hlsl"

:: ffx_fsr2_depth_clip_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_depth_clip_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_depth_clip_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_depth_clip_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_depth_clip_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_depth_clip_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_depth_clip_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_depth_clip_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_depth_clip_pass.hlsl"

:: ffx_fsr2_lock_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_lock_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_lock_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_lock_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_lock_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_lock_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_lock_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_lock_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_lock_pass.hlsl"

:: ffx_fsr2_rcas_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_rcas_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_rcas_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_rcas_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_rcas_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_rcas_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_rcas_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_rcas_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_rcas_pass.hlsl"

:: ffx_fsr2_reconstruct_previous_depth_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_reconstruct_previous_depth_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_reconstruct_previous_depth_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_reconstruct_previous_depth_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_reconstruct_previous_depth_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_reconstruct_previous_depth_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_reconstruct_previous_depth_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_reconstruct_previous_depth_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_reconstruct_previous_depth_pass.hlsl"

:: ffx_fsr2_tcr_autogen_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_tcr_autogen_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_tcr_autogen_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_tcr_autogen_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_tcr_autogen_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_tcr_autogen_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_tcr_autogen_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR2_SC_ARGS% -name=ffx_fsr2_tcr_autogen_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR2_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\fsr2\ffx_fsr2_tcr_autogen_pass.hlsl"

exit /b 0