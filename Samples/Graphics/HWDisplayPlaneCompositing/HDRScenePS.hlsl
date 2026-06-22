//--------------------------------------------------------------------------------------
// HDRScenePS.hlsl
//
// Shader to render an HDR image with color gamut expansion
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"
#include "HDRCommon.hlsli"

static const float PaperWhiteNits = 200.0f;         // Define how bright white should be
static const float GamutExpansionStart = 0.0f;      // When color gamut expansion starts
static const float GamutExpansionStop = 5.0f;       // When color gamut expanion ends

[RootSignature(FullScreenQuadRS)]
float4 main(Interpolators In) : SV_Target0
{
    // Linear HDR in Rec.709
    float4 hdrScene = Texture.Sample(LinearSampler, In.TexCoord);

    // Present linear P3-D65 colors and let the display hardware convert to Rec.2020 and apply the ST.2084 gamma curve
    float3 hdrSceneInP3 = ExpandColorGamut(hdrScene.rgb, GamutExpansionStart, GamutExpansionStop);
    float3 normalizedLinearValue = NormalizeHDRSceneValue(hdrSceneInP3, PaperWhiteNits);

    return float4(normalizedLinearValue, 1.0f);
}
