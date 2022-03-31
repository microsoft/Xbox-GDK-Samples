//--------------------------------------------------------------------------------------
// FinalHDRShaderCS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HDRCommon.hlsli"

static const float PaperWhiteNits       = 200.0f;   // Define how bright white should be
static const float GamutExpansionStart  = 1.0f;     // When color gamut expansion starts
static const float GamutExpansionStop   = 5.0f;     // When color gamut expanion ends

#define ROOT_SIGNATURE \
    "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL), \
     DescriptorTable(UAV(u1, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)"

RWTexture2D<float4> HDRSceneTexture : register(u0);
RWTexture2D<float4> SwapBuffer      : register(u1);

[numthreads(8, 8, 1)]
[RootSignature(ROOT_SIGNATURE)]
void main(uint3 id : SV_DispatchThreadID)
{
    float3 output = 0.0f;

    // Original linear HDR scene values in Rec.709 color space
    float3 Rec709 = HDRSceneTexture[id.xy].rgb;

    // Linear HDR scene values expanded into P3-D65 color space. This is optional, but recommended, otherwise you under utilize the display's capabilities
    float3 P3 = ExpandColorGamut(Rec709, GamutExpansionStart, GamutExpansionStop);

#ifdef __XBOX_SCARLETT
    // For Scarlett, we recommend presenting linear P3-D65 colors and let the display hardware convert to Rec.2020 and apply the ST.2084 gamma curve
    float3 normalizedLinearValue = NormalizeHDRSceneValue(P3, PaperWhiteNits);          // Normalize using paper white nits to prepare for ST.2084     
    output = normalizedLinearValue;
#else
    // For Xbox One, the swap chain has to contain HDR10 values, i.e. non-linear values in Rec.2020 with ST.2084 gamma curve
    float3 Rec2020 = mul(fromP3_D65to2020, P3);                                         // Rotate P3-D65 color space into Rec.2020 color space
    float3 normalizedLinearValue = NormalizeHDRSceneValue(Rec2020, PaperWhiteNits);     // Normalize using paper white nits to prepare for ST.2084     
    float3 HDR10 = LinearToST2084(normalizedLinearValue);                               // Apply ST.2084 curve
    output = HDR10;
#endif

    SwapBuffer[id.xy] = float4(output, 1.0f);
}
