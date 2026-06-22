//--------------------------------------------------------------------------------------
// ReconstructHDRAndConvertToHDR10PS.hlsl
//
// This sample shows how HDR can be reconstructed from an already tonemapped SDR image as
// a simple postprocessing technique.The technique is useful for adding HDR to a game without
// disrupting the render pipeline, keeping the same artistic intent as the tonemapped image,
// and can also be applied to SDR videos and UI splash screens. The sample has a toggle to
// easily compare the difference between SDR and reconstructed HDR.
//
// One challenge with HDR is that videos and UI splash screens are mostly saved as SDR, and
// will look dull compared to the actual rendered scene. This technique can HDR'ify the SDR
// content automatically. Another challenge is that when you simply remove the game's tonemap
// operator to retain HDR scene values, the artistic intent of the image might get lost,
// especially if the tonemap operator is combined with operations like color grading, brightness
// and contrast. A good short-term solution is to reconstruct HDR scene values from the already
// tonemapped and color graded final image of the game, using an inverse tonemapper, thus keeping
// the artistic intent of the SDR image. We refer to this as SDR mastered, and cannot utilize
// HDR as an HDR mastered image, but it's still a good short-term solution with which games
// have already shipped. The pixel shader can easily be optimized using a 3D lookup table (LUT).
//
// Pros
//      -SDR mastered, i.e. the SDR artistic intent stays the same for HDR
//      -Postprocessing stays the same, e.g. color grading, tone mapping, AA
//      -Cut scene videos stay the same
//      -Simple to implement as a single postprocessing technique
// Cons
//      -Colors close to white look emissive, e.g. fog, particles
//      -Noise like film grain gets exaggerated
//      -Loss of precision, FP11:11:10 -> 10:10:10:2
//      -Not as good as HDR mastered
//
// NOTE: The sample uses several 8-bit compressed SDR images to show the result, so compression
// artifacts and banding might be visible, which is not caused by the HDR reconstruction technique.
//
// Refer to the Xfest 2017 presentation "HDR Tips and Tricks from the Trenches" http://aka.ms/XF17022
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HDRCommon.hlsli"
#include "FullScreenQuad.hlsli"


// Constant buffer data for pixel shader
cbuffer HDRData : register(b0)
{
    // Values will be reconstructed as follows:
    //      SDR             HDR
    //      0.0     ->      0 nits
    //      0.5     ->      Paper white nits, e.g. 200 nits
    //      1.0     ->      MaxReconstructedNits, e.g. 1000 nits

    // HDR reconstruction constants
    float MaxReconstructedNits;             // How bright (in nits) should the tonemapped SDR value of 1.0 be
    float ReconstructedColorSaturation;     // Lerping between per luma and per color channel reconstruction, where 0 = only use per luma reconstruction, 1 = use only per color channel reconstruction
    int bUseGamutExpansion;                 // When TRUE, use a custom color space that is slightly bigger than Rec.709 to produce more saturated colors, if FALSE, just use Rec.709
    int bApplyReconstruction;               // Toggle between SDR/HDR

    // HDR10 constants
    float DisplayGamma;                     // Simple pow() adjustment for display gamma / contrast
    float PaperWhiteNits;                   // Defines how bright white is (in nits), which controls how bright the SDR range in the image will be

};

// Calc luminance
float RGBToLuminance(float3 x)
{
    return dot(x, float3(0.212671, 0.715160, 0.072169)); // Defined for sRGB/Rec.709 gamut
}

// Inverse tonemap based on Reinhard, bending the curve so that the value of 0.5 returns paper white nits, and the value of 1.0f
// returns a specified max nits value, e.g. 1000 nits. Refer to slides 12-15 of the Xfest 2017 presentation "HDR Tips and Tricks from the Trenches" http://aka.ms/XF17022
float4 InverseToneMap(float4 x, float maxReconstructedNits)
{
    // SDR range, [0..0.5] -> [0..PaperWhiteNits]
    float4 sdr = x / (1.0001f - x); // Some post-tonemap operation might take x to 1.0, so use 1.0001f

    // HDR range, [0.5..1.0] -> [PaperWhiteNits..MaxReconstructedNits]
    float HDRMaxSceneValue = maxReconstructedNits / PaperWhiteNits;
    HDRMaxSceneValue += 0.0001f;
    float b = HDRMaxSceneValue / (HDRMaxSceneValue - 1.0f);
    float a = 1.0f - ((0.5f * b) / (b - 0.5f));
    float4 hdr = a + ((b * x) / (b - x));

    return select((x > 0.5f), hdr, sdr);
}

// Convert linear SDR values to linear HDR values. Per luminance can look a bit desaturated, but per color channel can look too saturated, so interpolate between them
float3 ReconstructHDR(float3 sdr, float maxReconstructedNits)
{
    float luma = RGBToLuminance(sdr);
    float4 invToneMapped = InverseToneMap(float4(sdr.rgb, luma), maxReconstructedNits);
    float3 perChannel = invToneMapped.rgb;
    float3 perLuma = sdr / (luma + 0.0001f) * invToneMapped.a;

    // Interpolate per luminance and per color channel, 0.25 works well, but it's nice to have control over this
    float3 reconstructedHDR = ((1.0f - ReconstructedColorSaturation) * perLuma) + (ReconstructedColorSaturation * perChannel);

    return reconstructedHDR;
}

// Convert to HDR10
float4 ConvertToHDR10(float4 hdrSceneValue)
{
    // Rotate colors into Rec.2020 from Rec.709
    float3 rec2020 = mul(from709to2020, hdrSceneValue.rgb);

    // Expand colors into the P3 color space. We do seperate expansions for the SDR and HDR range and just interpolate between the two.
    //  SDR: Cannot expand to the full P3 color space, because it will change skin tone too much, so we use a custom color space wider than Rec.709, but smaller than P3
    //  HDR: Expand to the full P3 color space. This helps keep color saturation in brights, e.g. a bright blue sky will still be blue and not lose color the brighter it gets
    if (bUseGamutExpansion == 1)
    {       
        float3 expandedSDR = mul(fromExpanded709to2020, hdrSceneValue.rgb); // Expand SDR, using a custom rotation matrix instead of fromRec709to2020
        float3 expandedHDR = mul(fromP3_D65to2020, hdrSceneValue.rgb);      // Expand HDR into the full P3 D65 space, using the color rotation matrix fromP3to2020 instead of fromRec709to2020

        // Interpolate between expanded SDR/HDR
        float lum = max(max(hdrSceneValue.r, hdrSceneValue.g), hdrSceneValue.b);
        float start = 1.0f;                                                 // At this HDR value, we get the expanded SDR value
        float stop = 2.0f;                                                  // At this HDR value, we get the expanded HDR value
        float t = saturate((lum - start) / (stop - start));
        rec2020 = lerp(expandedSDR, expandedHDR, t);                        // Lerp between expanded Rec.709 and P3
    }

    float3 normalizedLinearValue = NormalizeHDRSceneValue(rec2020, PaperWhiteNits);
    float3 HDR10 = LinearToST2084(normalizedLinearValue);

    return float4(HDR10.rgb, hdrSceneValue.a);
}

struct PSOut
{
    float4 HDR10    : SV_Target0;       // HDR10 buffer using Rec.2020 color primaries with ST.2084 curve
    float4 GameDVR  : SV_Target1;       // GameDVR buffer using Rec.709 color primaries with sRGB gamma curve
};

// Takes as input the HDR scene values and outputs HDR10 and GameDVR. This shader can be optimized by using a 32x32x32 3D LUT
[RootSignature(FullScreenQuadRS)]
PSOut main(Interpolators In)
{
    PSOut output;

    float4 tonemappedSDR = Texture.Sample(PointSampler, In.TexCoord);       // The final tonemapped SDR image of the game
    tonemappedSDR.rgb = pow(abs(tonemappedSDR.rgb), DisplayGamma);          // Apply contrast adjutment with a display gamma
    
    float3 hdr = tonemappedSDR.rgb;

    if (bApplyReconstruction)
    {
        hdr = ReconstructHDR(tonemappedSDR.rgb, MaxReconstructedNits);
    }

    output.HDR10 = ConvertToHDR10(float4(hdr, tonemappedSDR.a));
    output.GameDVR = pow(abs(tonemappedSDR), 1.0f/2.2f); // Do gamma correction in shader, because the sample's GameDVR swapchain buffer is 10:10:10:2

    return output;
}
