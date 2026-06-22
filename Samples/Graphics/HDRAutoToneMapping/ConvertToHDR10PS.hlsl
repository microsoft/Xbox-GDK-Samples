//--------------------------------------------------------------------------------------
// ConvertToHDR10PS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HDRCommon.hlsli"
#include "FullScreenQuad.hlsli"

static const float g_PaperWhiteNits = 200.0f;

// Takes as input the HDR scene values and outputs HDR10 values. Note that this shader only outputs
// HDR10 values, it does not ouput SDR values for GameDVR, since those are rendered by the auto tone mapper
[RootSignature(FullScreenQuadRS)]
float4 main(Interpolators In) : SV_Target0
{
    float4 hdrSceneValues = Texture.Sample(PointSampler, In.TexCoord);
    float4 HDR10 = ConvertToHDR10(hdrSceneValues, g_PaperWhiteNits);

    return HDR10;
}
