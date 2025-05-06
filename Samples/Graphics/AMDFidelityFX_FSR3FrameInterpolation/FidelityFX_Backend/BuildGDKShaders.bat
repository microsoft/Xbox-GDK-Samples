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

:: Frame interpolation shaders
set FI_API_BASE_ARGS=%FFX_API_BASE_ARGS% -DFFX_FRAMEINTERPOLATION_EMBED_ROOTSIG=1
set FI_BASE_ARGS=-DFFX_FRAMEINTERPOLATION_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF=0 -DFFX_FRAMEINTERPOLATION_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF=0 -DFFX_FRAMEINTERPOLATION_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF=1 -DFFX_FRAMEINTERPOLATION_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF=0 -DFFX_FRAMEINTERPOLATION_OPTION_UPSAMPLE_USE_LANCZOS_TYPE=2 %FFX_BASE_ARGS%
set FI_PERMUTATION_ARGS=-DFFX_FRAMEINTERPOLATION_OPTION_LOW_RES_MOTION_VECTORS={0,1} -DFFX_FRAMEINTERPOLATION_OPTION_JITTER_MOTION_VECTORS={0,1} -DFFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH={0,1}
set FI_INCLUDE_ARGS=%FFX_BASE_INCLUDE_ARGS% -I "%sdkdir%\include\FidelityFX\gpu\frameinterpolation"
set FI_SC_ARGS=%FI_BASE_ARGS% %FI_API_BASE_ARGS% %FI_PERMUTATION_ARGS%

:: ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass.hlsl"
:: ffx_frameinterpolation_compute_inpainting_pyramid_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_compute_inpainting_pyramid_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_compute_inpainting_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_compute_inpainting_pyramid_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_compute_inpainting_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_compute_inpainting_pyramid_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_compute_inpainting_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_compute_inpainting_pyramid_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_compute_inpainting_pyramid_pass.hlsl"
:: ffx_frameinterpolation_debug_view_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_debug_view_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_debug_view_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_debug_view_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_debug_view_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_debug_view_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_debug_view_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_debug_view_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_debug_view_pass.hlsl"
:: ffx_frameinterpolation_disocclusion_mask_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_disocclusion_mask_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_disocclusion_mask_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_disocclusion_mask_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_disocclusion_mask_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_disocclusion_mask_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_disocclusion_mask_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_disocclusion_mask_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_disocclusion_mask_pass.hlsl"
:: ffx_frameinterpolation_game_motion_vector_field_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_game_motion_vector_field_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_game_motion_vector_field_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_game_motion_vector_field_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_game_motion_vector_field_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_game_motion_vector_field_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_game_motion_vector_field_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_game_motion_vector_field_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_game_motion_vector_field_pass.hlsl"
:: ffx_frameinterpolation_inpainting_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_inpainting_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_inpainting_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_inpainting_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_inpainting_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_inpainting_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_inpainting_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_inpainting_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_inpainting_pass.hlsl"
:: ffx_frameinterpolation_optical_flow_vector_field_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_optical_flow_vector_field_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_optical_flow_vector_field_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_optical_flow_vector_field_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_optical_flow_vector_field_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_optical_flow_vector_field_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_optical_flow_vector_field_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_optical_flow_vector_field_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_optical_flow_vector_field_pass.hlsl"
:: ffx_frameinterpolation_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_pass.hlsl"
::ffx_frameinterpolation_reconstruct_and_dilate_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_reconstruct_and_dilate_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_reconstruct_and_dilate_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_reconstruct_and_dilate_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_reconstruct_and_dilate_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_reconstruct_and_dilate_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_reconstruct_and_dilate_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_reconstruct_and_dilate_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_reconstruct_and_dilate_pass.hlsl"
:: ffx_frameinterpolation_reconstruct_previous_depth_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_reconstruct_previous_depth_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_reconstruct_previous_depth_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_reconstruct_previous_depth_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_reconstruct_previous_depth_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_reconstruct_previous_depth_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_reconstruct_previous_depth_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_reconstruct_previous_depth_pass.hlsl"
:: ffx_frameinterpolation_setup_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_setup_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_setup_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_setup_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_setup_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_setup_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_setup_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FI_SC_ARGS% -name=ffx_frameinterpolation_setup_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FI_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\frameinterpolation\ffx_frameinterpolation_setup_pass.hlsl"

:: FSR3 shaders
set FSR3_API_BASE_ARGS=%FFX_API_BASE_ARGS% -DFFX_FSR3UPSCALER_EMBED_ROOTSIG=1
set FSR3_BASE_ARGS=-DFFX_FSR3UPSCALER_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF=0 -DFFX_FSR3UPSCALER_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF=0 -DFFX_FSR3UPSCALER_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF=1 -DFFX_FSR3UPSCALER_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF=0 -DFFX_FSR3UPSCALER_OPTION_UPSAMPLE_USE_LANCZOS_TYPE=2 %FFX_BASE_ARGS%
set FSR3_BASE_ARGS=%FSR3_BASE_ARGS% -DFFX_FSR3_GDK_VERSION=%GDKEDITION%
set FSR3_PERMUTATION_ARGS=-DFFX_FSR3UPSCALER_OPTION_REPROJECT_USE_LANCZOS_TYPE={0,1} -DFFX_FSR3UPSCALER_OPTION_HDR_COLOR_INPUT={0,1} -DFFX_FSR3UPSCALER_OPTION_LOW_RESOLUTION_MOTION_VECTORS={0,1} -DFFX_FSR3UPSCALER_OPTION_JITTERED_MOTION_VECTORS={0,1} -DFFX_FSR3UPSCALER_OPTION_INVERTED_DEPTH={0,1} -DFFX_FSR3UPSCALER_OPTION_APPLY_SHARPENING={0,1}
set FSR3_INCLUDE_ARGS=%FFX_BASE_INCLUDE_ARGS% -I "%sdkdir%\include\FidelityFX\gpu\fsr3" -I "%sdkdir%\include\FidelityFX\gpu\fsrupscaler"
set FSR3_SC_ARGS=%FSR3_BASE_ARGS% %FSR3_API_BASE_ARGS% %FSR3_PERMUTATION_ARGS%

::ffx_fsr3upscaler_accumulate_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_accumulate_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_accumulate_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_accumulate_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_accumulate_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_accumulate_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_accumulate_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_accumulate_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_accumulate_pass.hlsl"

::ffx_fsr3upscaler_autogen_reactive_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_autogen_reactive_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_autogen_reactive_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_autogen_reactive_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_autogen_reactive_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_autogen_reactive_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_autogen_reactive_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_autogen_reactive_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_autogen_reactive_pass.hlsl"

::ffx_fsr3upscaler_debug_view_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_debug_view_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_debug_view_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_debug_view_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_debug_view_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_debug_view_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_debug_view_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_debug_view_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_debug_view_pass.hlsl"

::ffx_fsr3upscaler_luma_instability_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_luma_instability_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_luma_instability_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_luma_instability_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_luma_instability_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_luma_instability_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_luma_instability_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_luma_instability_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_luma_instability_pass.hlsl"

::ffx_fsr3upscaler_luma_pyramid_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_luma_pyramid_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_luma_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_luma_pyramid_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_luma_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_luma_pyramid_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_luma_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_luma_pyramid_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_luma_pyramid_pass.hlsl"

::ffx_fsr3upscaler_prepare_inputs_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_prepare_inputs_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_prepare_inputs_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_prepare_inputs_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_prepare_inputs_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_prepare_inputs_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_prepare_inputs_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_prepare_inputs_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_prepare_inputs_pass.hlsl"

::ffx_fsr3upscaler_prepare_reactivity_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_prepare_reactivity_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_prepare_reactivity_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_prepare_reactivity_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_prepare_reactivity_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_prepare_reactivity_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_prepare_reactivity_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_prepare_reactivity_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_prepare_reactivity_pass.hlsl"

::ffx_fsr3upscaler_rcas_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_rcas_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_rcas_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_rcas_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_rcas_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_rcas_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_rcas_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_rcas_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_rcas_pass.hlsl"

::ffx_fsr3upscaler_shading_change_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_shading_change_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_shading_change_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_shading_change_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_shading_change_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_shading_change_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_shading_change_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_shading_change_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_shading_change_pass.hlsl"

::ffx_fsr3upscaler_shading_change_pyramid_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_shading_change_pyramid_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_shading_change_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_shading_change_pyramid_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_shading_change_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_shading_change_pyramid_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_shading_change_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %FSR3_SC_ARGS% -name=ffx_fsr3upscaler_shading_change_pyramid_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %FSR3_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\fsr3upscaler\ffx_fsr3upscaler_shading_change_pyramid_pass.hlsl"

:: Optical flow shaders
set OF_API_BASE_ARGS=%FFX_API_BASE_ARGS% -DFFX_OPTICALFLOW_EMBED_ROOTSIG=1
set OF_BASE_ARGS=%FFX_BASE_ARGS%
set OF_PERMUTATION_ARGS=-DFFX_OPTICALFLOW_OPTION_HDR_COLOR_INPUT={0,1}
set OF_INCLUDE_ARGS=%FFX_BASE_INCLUDE_ARGS% -I "%sdkdir%\include\FidelityFX\gpu\opticalflow"
set OF_SC_ARGS=%OF_BASE_ARGS% %OF_API_BASE_ARGS% %OF_PERMUTATION_ARGS%

:: ffx_opticalflow_compute_luminance_pyramid_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_luminance_pyramid_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_luminance_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_luminance_pyramid_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_luminance_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_luminance_pyramid_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_luminance_pyramid_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_luminance_pyramid_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_luminance_pyramid_pass.hlsl"

:: ffx_opticalflow_compute_optical_flow_advanced_pass_v5.hlsl
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_optical_flow_advanced_pass_v5 -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_optical_flow_advanced_pass_v5.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_optical_flow_advanced_pass_v5_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_optical_flow_advanced_pass_v5.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_optical_flow_advanced_pass_v5_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_optical_flow_advanced_pass_v5.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_optical_flow_advanced_pass_v5_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_optical_flow_advanced_pass_v5.hlsl"

:: ffx_opticalflow_compute_scd_divergence_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_scd_divergence_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_scd_divergence_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_scd_divergence_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_scd_divergence_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_scd_divergence_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_scd_divergence_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_compute_scd_divergence_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_compute_scd_divergence_pass.hlsl"

:: ffx_opticalflow_filter_optical_flow_pass_v5.hlsl
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_filter_optical_flow_pass_v5 -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_filter_optical_flow_pass_v5.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_filter_optical_flow_pass_v5_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_filter_optical_flow_pass_v5.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_filter_optical_flow_pass_v5_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_filter_optical_flow_pass_v5.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_filter_optical_flow_pass_v5_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_filter_optical_flow_pass_v5.hlsl"

:: ffx_opticalflow_generate_scd_histogram_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_generate_scd_histogram_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_generate_scd_histogram_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_generate_scd_histogram_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_generate_scd_histogram_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_generate_scd_histogram_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_generate_scd_histogram_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_generate_scd_histogram_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_generate_scd_histogram_pass.hlsl"

:: ffx_opticalflow_prepare_luma_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_prepare_luma_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_prepare_luma_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_prepare_luma_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_prepare_luma_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_prepare_luma_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_prepare_luma_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_prepare_luma_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_prepare_luma_pass.hlsl"

:: ffx_opticalflow_scale_optical_flow_advanced_pass_v5.hlsl
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_scale_optical_flow_advanced_pass_v5 -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_scale_optical_flow_advanced_pass_v5.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_scale_optical_flow_advanced_pass_v5_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_scale_optical_flow_advanced_pass_v5.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_scale_optical_flow_advanced_pass_v5_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_scale_optical_flow_advanced_pass_v5.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %OF_SC_ARGS% -name=ffx_opticalflow_scale_optical_flow_advanced_pass_v5_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %OF_INCLUDE_ARGS% -output=%outputdir% "%sdkdir%\src\backends\gdk\shaders\opticalflow\ffx_opticalflow_scale_optical_flow_advanced_pass_v5.hlsl"

exit /b 0