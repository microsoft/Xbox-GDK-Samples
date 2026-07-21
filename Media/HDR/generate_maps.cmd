@echo off
REM ATG Internal Use Only
REM
REM Generate radiance and irradiance maps for a given HDR image.
REM Assume CMFT Studio is installed at given paths

REM first and only parameter (%1)
set source_texture="%1"
set source_texture_base="%~n1"
if not exist %source_texture% goto need_source_texture

REM tool environment paths - change or override
REM eg set MAP_GEN_PATH="z:\cmft_win64\cmft.exe"

set map_gen_tool="%MAP_GEN_PATH%"
if not exist %map_gen_tool% goto need_map_gen

REM output textures
set output_texture_irradiance=%source_texture_base%_irradiance
set output_texture_radiance=%source_texture_base%_radiance

REM Step 1, create irradiance texture from DDS
%map_gen_tool% --input %source_texture% --filter irradiance --outputNum 1 --dstFaceSize 256 --output0 %output_texture_irradiance% --output0params dds,rgba16f,cubemap

REM Step 2, create radiance texture from DDS
%map_gen_tool% --input %source_texture% --filter radiance --lightingModel phongbrdf --outputNum 1 --mipCount 9 --glossScale 10 --glossBias 1 --srcFaceSize 512 --dstFaceSize 256 --output0 %output_texture_radiance% --output0Params dds,rgba16f,cubemap

REM Finished, all good?
exit /b

:need_map_gen
echo Map generator tool not found at following path, check MAP_GEN_PATH:
echo %map_gen_tool%
exit /b

:need_source_texture
echo Source texture not found at:
echo %source_texture%
exit /b