//--------------------------------------------------------------------------------------
// ConvertToHDR10PS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "HDRCommon.hlsli"

#define ROOT_SIGNATURE \
    "DescriptorTable(SRV(t0, numDescriptors=1)), \
     StaticSampler(s0, \
             filter = FILTER_MIN_MAG_MIP_POINT, \
             addressU = TEXTURE_ADDRESS_CLAMP, \
             addressV = TEXTURE_ADDRESS_CLAMP, \
             addressW = TEXTURE_ADDRESS_CLAMP, \
             visibility = SHADER_VISIBILITY_PIXEL)"

struct Interpolators
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
};

static const float g_PaperWhiteNits = 250.0f;

Texture2D<float4> Texture : register(t0);
SamplerState PointSampler : register(s0);

// Takes as input the HDR scene values and outputs HDR10 values. Note that this shader only outputs
// HDR10 values, it does not ouput SDR values for GameDVR, since those are rendered by the auto tone mapper
[RootSignature(ROOT_SIGNATURE)]
float4 main(Interpolators In) : SV_Target0
{
    float4 hdrSceneValues = Texture.Sample(PointSampler, In.TexCoord);
    float4 HDR10 = ConvertToHDR10(hdrSceneValues, g_PaperWhiteNits);

    return HDR10;
}
