//--------------------------------------------------------------------------------------
// PrepareSwapChainBuffersPS.hlsl
//
// Takes the final HDR back buffer with linear values using Rec.709 color primaries and
// outputs two signals, an HDR and SDR signal. The HDR siganl uses Rec.2020 color primaries
// with the ST.2084 curve, whereas the SDR signal uses Rec.709 color primaries which the
// hardware will apply the Rec.709 gamma curve.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HDRCommon.hlsli"
#include "FullScreenQuad.hlsli"

// Constant buffer with calibration data
cbuffer CalibrationDataCB : register(b0)
{
    float   PaperWhiteNits;                             // Defines how bright white is (in nits), which controls how bright the SDR range in the image will be
    float   DisplayGamma;                               // Simple pow() adjustment for display gamma / contrast
    float   ColorGamutExpansion;                        // Lerp between Rec.709 and expanded color gamut
    float   SoftShoulderStart2084;                      // Threshold at beginning of soft shoulder for HDR display mapping, normalized non-linear ST.2084 value
    float   MaxBrightnessOfTV2084;                      // Max perceived brightness of TV, nits converted to normalized non-linear ST.2084 value 
    float   MaxBrightnessOfHDRScene2084;                // Max perceived brightness of HDR scene, nits converted to normalized non-linear ST.2084 value 

    // When determining the max preceived brightness of the TV, we shouldn't apply display gamma or display mapping adjustments. These define the bounding box of where
    // that calibration image is rendered. E.g. when trying to establish the max perceived brightness of the TV with a high contrast setting, you will end up with the wrong value
    float   CalibrationImageBoundingBoxU1;              // Bounding box for calibration screens
    float   CalibrationImageBoundingBoxV1;
    float   CalibrationImageBoundingBoxU2;
    float   CalibrationImageBoundingBoxV2;
    float   PaperWhiteNitsForCalibrationScreen;         // Paperwhite nits to use within the calibration screens bounding box
    int     ApplyContrastToCalibrationScreen;           // Contrast adjustment should not be applied when determining the max perceived brightness of the TV
};


// Adjust HDR contrast. For SDR in the range [0..1], we can safely apply a simple pow() function, but for HDR, values higher than 1.0f will also be effected.
// This function interpolates between the power function and the linear HDR value to ensure that when reducing contrast, the maximum brightness in the scene
// is not getting dimmer.
float3 AdjustHDRContrast(float3 x)
{
    if (DisplayGamma >= 1.0f)
    {
        return pow(abs(x), DisplayGamma);
    }
    else
    {
        float3 t = saturate((2.0f - x) * (2.0f - x) * (2.0f - x));
        float3 y = (x * (1.0f - t)) + (pow(abs(x), DisplayGamma) * t);
        return y;
    }
}

// Prepare the HDR swapchain buffer as HDR10. This means the buffer has to contain data which uses
//  - Rec.2020 color primaries
//  - Quantized using ST.2084 curve
//  - 10-bit per channel
float4 PrepareHDR10(float4 hdrSceneValue, float2 UV)
{
    float3 HDR10;

    // Rotate into Rec.2020 color primaries, using color gamut expansion
    float3 expandedSDR = mul(fromExpanded709to2020, hdrSceneValue.rgb); // Expand SDR, using a custom rotation matrix instead of fromRec709to2020
    float3 expandedHDR = mul(fromP3_D65to2020, hdrSceneValue.rgb);      // Expand HDR into the full P3 D65 space, using the color rotation matrix fromP3to2020 instead of fromRec709to2020

    // Interpolate between expanded SDR/HDR
    float lum = max(max(hdrSceneValue.r, hdrSceneValue.g), hdrSceneValue.b);
    float start = 2.0f; // At this HDR value, we get the expanded SDR value
    float stop = 10.0f; // At this HDR value, we get the expanded HDR value
    float t = saturate((lum - start) / (stop - start));
    float3 expandedColor = lerp(expandedSDR, expandedHDR, t); // Lerp between expanded Rec.709 and P3
    hdrSceneValue.rgb = lerp(mul(from709to2020, hdrSceneValue.rgb), expandedColor, ColorGamutExpansion); // Lerp between expanded Rec.709 and P3

    // No contrast adjustment or display mapping inside the calibration images itself. E.g. when trying to establish the max
    // perceived brightness of the TV with a high contrast setting, you will end up with the wrong value
    if (UV.x >= CalibrationImageBoundingBoxU1 &&
        UV.x <= CalibrationImageBoundingBoxU2 &&
        UV.y >= CalibrationImageBoundingBoxV1 &&
        UV.y <= CalibrationImageBoundingBoxV2)
    {
        if (ApplyContrastToCalibrationScreen == 1)
        {
            hdrSceneValue.rgb = AdjustHDRContrast(hdrSceneValue.rgb);
        }      

        float3 normalizedLinearValue = NormalizeHDRSceneValue(hdrSceneValue.rgb, PaperWhiteNitsForCalibrationScreen);
        HDR10 = LinearToST2084(normalizedLinearValue);
    }
    else
    {
        hdrSceneValue.rgb = AdjustHDRContrast(hdrSceneValue.rgb);                                                                                   // Apply contrast adjustment
        float3 normalizedLinearValue = NormalizeHDRSceneValue(hdrSceneValue.rgb, PaperWhiteNits);                                                   // Normalize using paper white nits to prepare for ST.2084     
        HDR10 = MapHDRSceneToDisplayCapabilities(normalizedLinearValue, SoftShoulderStart2084, MaxBrightnessOfTV2084, MaxBrightnessOfHDRScene2084); // Map HDR scene values to TV capabilities, to avoid clipping of bright detail
    }

    return float4(HDR10.rgb, hdrSceneValue.a);
}

// Prepare SDR swapchain as using Rec.709 color primaries with sRGB gamma curve
float4 PrepareGameDVR(float4 hdrSceneValue, float2 UV)
{
    hdrSceneValue.rgb = pow(abs(hdrSceneValue.rgb), DisplayGamma);          // Apply contrast / display gamma adjustment
    float4 sdr = hdrSceneValue / (hdrSceneValue + 1.0f);                    // Simple Reinhard tone mapper

    // No tonemapping on calibration images within the calibration bounding box
    if (UV.x >= CalibrationImageBoundingBoxU1 &&
        UV.x <= CalibrationImageBoundingBoxU2 &&
        UV.y >= CalibrationImageBoundingBoxV1 &&
        UV.y <= CalibrationImageBoundingBoxV2)
    {
        sdr.rgb = hdrSceneValue.rgb;
    }

    return float4(sdr.rgb, hdrSceneValue.a);
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

    output.HDR10 = PrepareHDR10(hdrSceneValues, In.TexCoord.xy);
    output.GameDVR = PrepareGameDVR(hdrSceneValues, In.TexCoord.xy);

    return output;
}
