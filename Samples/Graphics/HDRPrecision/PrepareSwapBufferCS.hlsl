//--------------------------------------------------------------------------------------
// PrepareSwapBufferCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HDRCommon.hlsli"

static const int ColorSpace_Rec709  = 0;
static const int ColorSpace_P3      = 1;
static const int ColorSpace_Rec2020 = 2;

static const int GammaCurve_Linear  = 0;
static const int GammaCurve_ST2084  = 1;

#define ROOT_SIGNATURE \
    "RootConstants(b0, num32bitconstants=4), \
     DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL), \
     DescriptorTable(UAV(u1, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)"

RWTexture2D<float4> HDRSceneTexture : register(u0);
RWTexture2D<float4> SwapBuffer      : register(u1);

cbuffer Constants : register(b0)
{
    int RenderRamp;
    int ColorSpace;
    int GammaCurve;
    float BrightnessScale;
};

static const float g_PaperWhiteNits = 100.0f;

[numthreads(8, 8, 1)]
[RootSignature(ROOT_SIGNATURE)]
void main(uint3 id : SV_DispatchThreadID)
{
    float3 output = 0.0f;

    float3 hdrSceneValues = HDRSceneTexture[id.xy].rgb;

    // Adjust brightness since precision artifacts might be seen differently in darks vs. brights
    hdrSceneValues *= BrightnessScale;

    if (RenderRamp)
    {
        // The ramp is generated with the index value of each x value in the 4K image
        // Therefore, devide by 3890 to get normalized linear values
        hdrSceneValues.rgb /= 3839.0f;
    }

    // For HDR10, the console always has to send color values to the display using the Rec.2020 color space with the ST.2084 gamma curve.
    // But, we can decide where some of the color space and gamma space conversions take place. Some processing can take place in a shader
    // and other processing can be moved to the display hardware
    switch (ColorSpace)
    {
    case ColorSpace_Rec709:
        // Display HW rotates from Rec.709 to Rec.2020, use one of the swap buffer flags DXGI_COLOR_SPACE_RGB_FULL_*_NONE_P709
        // No need to do anything in the shader. The HDR scene values are Rec.709 and the display HW will do the
        // conversion from Rec.709 to Rec.2020 for free
        output = hdrSceneValues;
        break;

    case ColorSpace_P3:
        // Display HW rotates from P3 D65 to Rec.2020, use one of the swap buffer flags DXGI_COLOR_SPACE_RGB_FULL_*_NONE_D65P3
        // If a title renders P3 color space values, i.e. if the HDR scene values are P3, then no need to do anything here,
        // the display HW will do the conversion from P3 D65 to Rec.2020 for free. But, in this sample the HDR scene values
        // are in Rec.709, so the shader does a conversion from Rec.709 to P3, and the display HW from P3 to Rec.2020
        output = mul(from709toP3_D65, hdrSceneValues);
        break;

    case ColorSpace_Rec2020:
        // Display HW doesn't do any color conversions, use one of the swap buffer flags DXGI_COLOR_SPACE_RGB_FULL_*_NONE_P2020
        // If a title renders Rec.2020 color space values, i.e. if the HDR scene values are Rec.2020, then no need to do anything here,
        // we already have Rec.2020 values. But, in this sample, and in most titles, the HDR scene values are in Rec.709, so the
        // shader does a conversion from Rec.709 to Rec.2020, and the display HW does no color conversion
        output = mul(from709to2020, hdrSceneValues);
        break;
    }

    // Output values must always be normalized, independent of using linear / ST.2084 gamma curve
    output = NormalizeHDRSceneValue(output, g_PaperWhiteNits);

    switch (GammaCurve)
    {
    case GammaCurve_Linear:
        // Display HW applies the ST.2084 gamma curve, use one of the swap buffer flags DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_*
        // Nothing to do in the shader, saving some GPU perf. The HW will convert from linear to ST.2084
        break;

    case GammaCurve_ST2084:
        // Display HW does NOT apply the ST.2084 gamma curve, use one of the swap buffer flags DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_*
        // Apply the ST.2084 gamma curve in the shader
        output = LinearToST2084(output);
        break;
    }

    SwapBuffer[id.xy] = float4(output, 1.0f);
}
