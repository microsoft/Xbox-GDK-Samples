//--------------------------------------------------------------------------------------
// SDRScene.hlsl
//
// Simple shader to tonemap an HDR image
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

float3 SimpleToneMap(float3 hdr)
{
    return hdr / (hdr + 1.0f);
}

[RootSignature(FullScreenQuadRS)]
float4 main(Interpolators In) : SV_Target0
{
    float4 scene = Texture.Sample(LinearSampler, In.TexCoord);
    float3 sdr = SimpleToneMap(scene.rgb);

    return float4(sdr, 1.0f);
}
