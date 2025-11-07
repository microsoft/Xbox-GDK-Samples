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
if %platformname% == x64 (
	set FFX_API_BASE_ARGS=-E CS -Wno-for-redefinition -Wno-ambig-lit-shift -DFFX_HLSL=1
)

set FFX_BASE_ARGS=-reflection -deps=gcc -DFFX_GPU=1

set HLSL_WAVE64_ARGS=-DFFX_PREFER_WAVE64="[WaveSize(64)]" -DFFX_HLSL_SM=66 -T cs_6_6
set HLSL_WAVE32_ARGS=-DFFX_HLSL_SM=62 -T cs_6_2
set HLSL_16BIT_ARGS=
set FFX_GDK_OPTION=

if %platformname% == x64 (
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

:: CACAO shaders
set CACAO_API_BASE_ARGS=%FFX_API_BASE_ARGS% -DFFX_CACAO_EMBED_ROOTSIG=1
if %platformname% == x64 (
    set CACAO_API_BASE_ARGS=%FFX_API_BASE_ARGS%
)
set CACAO_BASE_ARGS=%FFX_BASE_ARGS%
set CACAO_PERMUTATION_ARGS=-DFFX_CACAO_OPTION_APPLY_SMART={0,1}
set CACAO_INCLUDE_ARGS=%FFX_BASE_INCLUDE_ARGS% -I "%sdkdir%\include\FidelityFX\gpu\cacao"
set CACAO_SC_ARGS=%CACAO_BASE_ARGS% %CACAO_API_BASE_ARGS% %CACAO_PERMUTATION_ARGS%

set BACKEND_SHADER_DIR=%sdkdir%\src\backends\gdk\shaders
if %platformname% == x64 (
	set BACKEND_SHADER_DIR=%sdkdir%\src\backends\dx12\shaders
)

:: ffx_cacao_apply_non_smart_half_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_non_smart_half_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_non_smart_half_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_non_smart_half_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_non_smart_half_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_non_smart_half_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_non_smart_half_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_non_smart_half_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_non_smart_half_pass.hlsl"

::ffx_cacao_apply_non_smart_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_non_smart_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_non_smart_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_non_smart_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_non_smart_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_non_smart_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_non_smart_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_non_smart_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_non_smart_pass.hlsl"

::ffx_cacao_apply_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_apply_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_apply_pass.hlsl"

::ffx_cacao_clear_load_counter_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_clear_load_counter_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_clear_load_counter_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_clear_load_counter_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_clear_load_counter_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_clear_load_counter_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_clear_load_counter_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_clear_load_counter_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_clear_load_counter_pass.hlsl"

::ffx_cacao_edge_sensitive_blur_1_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_1_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_1_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_1_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_1_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_1_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_1_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_1_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_1_pass.hlsl"

::ffx_cacao_edge_sensitive_blur_2_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_2_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_2_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_2_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_2_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_2_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_2_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_2_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_2_pass.hlsl"

::ffx_cacao_edge_sensitive_blur_3_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_3_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_3_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_3_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_3_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_3_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_3_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_3_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_3_pass.hlsl"

::ffx_cacao_edge_sensitive_blur_4_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_4_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_4_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_4_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_4_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_4_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_4_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_4_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_4_pass.hlsl"

::ffx_cacao_edge_sensitive_blur_5_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_5_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_5_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_5_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_5_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_5_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_5_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_5_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_5_pass.hlsl"

::ffx_cacao_edge_sensitive_blur_6_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_6_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_6_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_6_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_6_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_6_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_6_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_6_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_6_pass.hlsl"

::ffx_cacao_edge_sensitive_blur_7_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_7_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_7_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_7_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_7_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_7_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_7_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_7_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_7_pass.hlsl"

::ffx_cacao_edge_sensitive_blur_8_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_8_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_8_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_8_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_8_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_8_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_8_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_edge_sensitive_blur_8_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_edge_sensitive_blur_8_pass.hlsl"

::ffx_cacao_generate_importance_map_a_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_a_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_a_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_a_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_a_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_a_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_a_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_a_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_a_pass.hlsl"

::ffx_cacao_generate_importance_map_b_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_b_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_b_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_b_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_b_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_b_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_b_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_b_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_b_pass.hlsl"

::ffx_cacao_generate_importance_map_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_importance_map_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_importance_map_pass.hlsl"

::ffx_cacao_generate_q0_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q0_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q0_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q0_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q0_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q0_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q0_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q0_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q0_pass.hlsl"

::ffx_cacao_generate_q1_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q1_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q1_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q1_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q1_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q1_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q1_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q1_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q1_pass.hlsl"

::ffx_cacao_generate_q2_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q2_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q2_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q2_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q2_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q2_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q2_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q2_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q2_pass.hlsl"

::ffx_cacao_generate_q3_base_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q3_base_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q3_base_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q3_base_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q3_base_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q3_base_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q3_base_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q3_base_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q3_base_pass.hlsl"

::ffx_cacao_generate_q3_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q3_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q3_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q3_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q3_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q3_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q3_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_generate_q3_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_generate_q3_pass.hlsl"

::ffx_cacao_prepare_downsampled_depths_and_mips_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_and_mips_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_and_mips_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_and_mips_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_and_mips_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_and_mips_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_and_mips_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_and_mips_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_and_mips_pass.hlsl"

::ffx_cacao_prepare_downsampled_depths_half_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_half_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_half_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_half_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_half_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_half_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_half_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_half_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_half_pass.hlsl"

::ffx_cacao_prepare_downsampled_depths_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_depths_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_depths_pass.hlsl"

::ffx_cacao_prepare_downsampled_normals_from_input_normals_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_normals_from_input_normals_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_normals_from_input_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_normals_from_input_normals_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_normals_from_input_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_normals_from_input_normals_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_normals_from_input_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_normals_from_input_normals_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_normals_from_input_normals_pass.hlsl"

::ffx_cacao_prepare_downsampled_normals_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_normals_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_normals_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_normals_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_downsampled_normals_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_downsampled_normals_pass.hlsl"

::ffx_cacao_prepare_native_depths_and_mips_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_and_mips_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_and_mips_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_and_mips_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_and_mips_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_and_mips_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_and_mips_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_and_mips_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_and_mips_pass.hlsl"

::ffx_cacao_prepare_native_depths_half_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_half_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_half_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_half_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_half_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_half_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_half_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_half_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_half_pass.hlsl"

::ffx_cacao_prepare_native_depths_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_depths_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_depths_pass.hlsl"

::ffx_cacao_prepare_native_normals_from_input_normals_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_normals_from_input_normals_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_normals_from_input_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_normals_from_input_normals_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_normals_from_input_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_normals_from_input_normals_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_normals_from_input_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_normals_from_input_normals_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_normals_from_input_normals_pass.hlsl"

::ffx_cacao_prepare_native_normals_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_normals_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_normals_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_normals_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_normals_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_prepare_native_normals_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_prepare_native_normals_pass.hlsl"

::ffx_cacao_upscale_bilateral_5x5_pass.hlsl
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_upscale_bilateral_5x5_pass -DFFX_HALF=0 %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_upscale_bilateral_5x5_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_upscale_bilateral_5x5_pass_wave64 -DFFX_HALF=0 %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_upscale_bilateral_5x5_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_upscale_bilateral_5x5_pass_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE32_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_upscale_bilateral_5x5_pass.hlsl"
%FFX_SC% %FFX_GDK_OPTION% %CACAO_SC_ARGS% -name=ffx_cacao_upscale_bilateral_5x5_pass_wave64_16bit %HLSL_16BIT_ARGS% %HLSL_WAVE64_ARGS% %CACAO_INCLUDE_ARGS% -output=%outputdir% "%BACKEND_SHADER_DIR%\cacao\ffx_cacao_upscale_bilateral_5x5_pass.hlsl"

exit /b 0