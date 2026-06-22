//--------------------------------------------------------------------------------------
// RenderToneMapperLUT.hlsl
//
// Pixel shader to render the game's own defined tone mapper into a 3D LUT, used in
// the SetHDRToneMapper() API, when the game uses HDR auto tone mapping.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Render3DTexture.hlsli"
#include "HDRCommon.hlsli"
#include "ToneMappers.hlsli"

// The game's own tone mapper. This can include operations like color grading, contrast adjustment, etc.
float3 Tonemap(float3 linearHDR)
{
    float3 linearSDR = ToneMapFilmic(linearHDR);

    // NOTE: This has to output linear SDR, i.e. not gamma corrected.
    return linearSDR;
}

// Calculate color value from UVW texture coordinates. The scale and offset is
// needed to ensure we operate on the center of the pixels in the LUT
float3 UVWToColor(float2 UV, float index, float LUTSize)
{
    UV = UV - float2(0.5f / LUTSize, 0.5f / LUTSize);
    return float3(UV * LUTSize / (LUTSize - 1), index / (LUTSize - 1));
}

// The sample uses 200 nits paper white, but a game could use something different,
// so make sure to use your game's definition of paperwhite
static const float g_PaperWhiteNits = 200.0f;

// This is the shader code defined for generating the 3D LUT used for SetHDRToneMapper. We recommend that you just
// copy and paste this code and add your own tonemapper in the ToneMap() function.
// NOTE: Apply the tone mapper in the bigger Rec.2020 color space and then rotate to Rec.709, otherwise color shifts
// might be introduced.
[RootSignature(Render3DTextureRS)]
float4 main(PS_IN In) : SV_Target0
{
    float3 hdr10 = UVWToColor(In.TexCoord, In.LayerIndex, 32);                  // Calculate HDR10 color values from the UVW coordinates
    float3 normalizedLinear = ST2084ToLinear(hdr10);                            // Normalized linear value in Rec.2020
    float3 linearHDR = normalizedLinear * g_MaxNitsFor2084 / g_PaperWhiteNits;  // Linear HDR value in Rec.2020
    float3 linearSDR = Tonemap(linearHDR);                                      // Linear SDR value in Rec.2020
    linearSDR = mul(from2020to709, linearSDR);                                  // Linear SDR value in Rec.709
    return float4(linearSDR, 1.0f);
}
