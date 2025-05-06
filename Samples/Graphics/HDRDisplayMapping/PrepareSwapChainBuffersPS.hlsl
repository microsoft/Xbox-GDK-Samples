//--------------------------------------------------------------------------------------
// PrepareSwapChainBuffersPS.hlsl
//
// This sample shows that even when rendering a HDR scene on a HDR capable TV, some tone mapping is still needed, referred to as "HDR display mapping".
// HDR display mapping maps values that are brighter than what a HDR TV can display, into the upper brightness range of the TV's capabilities, so that
// details in the very bright areas of the scene won't get clipped.
//
// The sample implements the following visualizations
//  -HDR display mapping
//  -HDR to SDR tone mapping
//  -Highlight values brighter than the TV max brightness, i.e. those that will naturally be clipped by TV
//
// Note, these shaders are not optimized, the goal is simply to explore different methods of HDR display mappings
//
// Refer to the white paper "HDR Display Mapping", http://aka.ms/hdr-display-mapping 
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HDRCommon.hlsli"
#include "FullScreenQuad.hlsli"

// HDR to SDR tone mapping
static const int TonemappingModeNone              = 0;  // HDR values above 1.0 will simply be clipped
static const int TonemappingModeReinhard          = 1;  // Simple Reinhard tone mapping

// HDR display mapping
static const int DisplayMappingModeNone           = 0;  // No display mapping, send the full range of HDR scene values to the HDR TV
static const int DisplayMappingModeNoClipping     = 1;  // Reduce HDR values brighter than max TV brightness using a 3 point Bezier curve, using a soft shoulder, TV max brightness and max HDR scene brightness
static const int DisplayMappingModeSomeClipping   = 2;  // Same as NoClipping, but allow for some clipping to retain brightness, a compromise between brightness and details


// Constant buffer with display mapping data
cbuffer DisplayMappingData : register(b0)
{
    // Display mapping data
    float PaperWhiteNits;                               // Defining how bright paper white is, can be used to control the overall brightness of the scene
    float SoftShoulderStart2084;                        // Threshold at beginning of soft shoulder for HDR display mapping, normalized non-linear ST.2084 value
    float MaxBrightnessOfTV2084;                        // Max perceived brightness of TV, nits converted to normalized non-linear ST.2084 value 
    float MaxBrightnessOfHDRScene2084;                  // Max perceived brightness of HDR scene, nits converted to normalized non-linear ST.2084 value 

    // Visualization data
    int   RenderAsSDR;                                  // Display the HDR image as if it were rendered on a SDR TV
    int   TonemappingMode;                              // Which HDR to SDR tone mapping method to use
    int   DisplayMappingMode;                           // Which HDR display mapping method to use
    int   IndicateValuesBrighterThanTV;                 // Highlight which values are brighter than TV max brightness, i.e. those that will naturally be clipped by your eye
    float MaxBrightnessOfTV2084ForVisualization;        // Visualization needs to know the actual max nits and the max nits used with clipping
};


// HDR to SDR tone mapping using a simple Reinhard operator
float3 TonemapReinhard(float3 HDRSceneValue)
{
    return HDRSceneValue / (1.0f + HDRSceneValue);
}


// Display colors as gray scale, except for values that are rendered brighter than the TV can display. Colors brighter
// than the TV will be displayed as red, and colors which are just as bright as the TV will be displayed in purple.
// Useful to inspect how different methods handle mapping the HDR values into a range which can be displayed by the TV.
float3 HighlightValuesBrighterThanTV(float3 hdrSceneValue, float3 normalizedLinearValue, bool applyToHDRValues)
{
    float threshold;
    float maxValue = max(max(normalizedLinearValue.r, normalizedLinearValue.g), normalizedLinearValue.b);

    if (applyToHDRValues == 1)
    {
        threshold = MaxBrightnessOfTV2084ForVisualization; // HDR values will be compared in normalized non-linear ST.2084 space
    }
    else
    {
        threshold = 1.0f;                   // SDR values will be compared in normalized linear space
    }

    if (maxValue > threshold)
    {
        return float3(1.0f, 0.0f, 0.0f);    // Indicate values brighter than TV can handle as Red
    }
    else if (maxValue == threshold)
    {
        return float3(1.0f, 0.0f, 1.0f);    // Indicate values just as bright as TV can handle as Purple
    }
    else                                    // Indicate values that TV can display as gray scale
    {
        float3 clippedValue = saturate(normalizedLinearValue);
        float luminance = dot(clippedValue.rgb, float3(0.3f, 0.59f, 0.11f));
        return luminance;
    }
}


// HDR to SDR tone mapping
float3 TonemapHDR2SDR(float3 hdrSceneValue)
{
    float3 tonemappedValue = hdrSceneValue;

    if (TonemappingMode == TonemappingModeReinhard)
    {
        tonemappedValue = TonemapReinhard(hdrSceneValue);
    }
    else if (TonemappingMode == TonemappingModeNone)
    {
        tonemappedValue = saturate(hdrSceneValue);
    }

    if (IndicateValuesBrighterThanTV == 1)
    {
        tonemappedValue = HighlightValuesBrighterThanTV(hdrSceneValue, tonemappedValue, false);
    }

    return tonemappedValue;
}


// Prepare the HDR swapchain buffer as HDR10. This means the buffer has to contain data which uses
//  - Rec.2020 color primaries
//  - Quantized using ST.2084 curve
//  - 10-bit per channel
float4 PrepareHDR10(float4 hdrSceneValue)
{
    // Rotate Rec.709 color primaries into Rec.2020 color primaries
    hdrSceneValue.rgb = mul(from709to2020, hdrSceneValue.rgb);

    float4 HDR10 = hdrSceneValue;
    float3 normalizedLinearValue = NormalizeHDRSceneValue(hdrSceneValue.rgb, PaperWhiteNits);       // Normalize using paper white nits to prepare for ST.2084

    if (RenderAsSDR == 1)                                                                           // Should look the same as displayed on SDR TV wrt the amount of detail seen, but values should just go to white, not bright
    {
        float3 tonemappedHDRSceneValue = TonemapHDR2SDR(hdrSceneValue.rgb);                         // Tonemap HDR to SDR values, i.e. HDR scene values are now mapped to [0..1]
        normalizedLinearValue = NormalizeHDRSceneValue(tonemappedHDRSceneValue, PaperWhiteNits);    // Normalize using paper white nits to prepare for ST.2084
        HDR10.rgb = LinearToST2084(normalizedLinearValue.rgb);                                      // Don't apply to alpha
    }
    else if (DisplayMappingMode == DisplayMappingModeNone)
    {
        HDR10.rgb = LinearToST2084(normalizedLinearValue);                                          // No display mapping, i.e. just apply the ST.2084 curve
    }
    else
    {
        HDR10.rgb = MapHDRSceneToDisplayCapabilities(normalizedLinearValue, SoftShoulderStart2084, MaxBrightnessOfTV2084, MaxBrightnessOfHDRScene2084); // Map HDR scene values to TV capabilities, to avoid clipping of bright detail
    }

    if (IndicateValuesBrighterThanTV == 1)
    {
        HDR10.rgb = HighlightValuesBrighterThanTV(hdrSceneValue.rgb, HDR10.rgb, true);              // Debug visualization to indicate which bright values will be clipped by the eye since they are brighter than the TV can display
    }

    return HDR10;
}


// Prepare SDR swapchain as using Rec.709 color primaries with sRGB gamma curve
float4 PrepareGameDVR(float4 hdrSceneValue)
{
    return float4(TonemapHDR2SDR(hdrSceneValue.rgb), hdrSceneValue.a);
}


struct PSOut
{
    float4 HDR10    : SV_Target0;       // HDR10 buffer using Rec.2020 color primaries with ST.2084 curve
    float4 GameDVR  : SV_Target1;       // GameDVR buffer using Rec.709 color primaries with sRGB gamma curve
};

// Takes as input the HDR scene values and outputs HDR10 and GameDVR
[RootSignature(FullScreenQuadRS)]
PSOut main(Interpolators In)
{
    PSOut output;

    // Input is linear values using sRGB / Rec.709 color primaries
    float4 hdrSceneValues = Texture.Sample(PointSampler, In.TexCoord);

    output.HDR10 = PrepareHDR10(hdrSceneValues);        // HDR signal using Rec.2020 color primaries with ST.2084 curve
    output.GameDVR = PrepareGameDVR(hdrSceneValues);    // SDR signal using Rec.709 color primaries with sRGB gamma curve

    return output;
}
