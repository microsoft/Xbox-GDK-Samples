//--------------------------------------------------------------------------------------
// FullScreenQuadGammaPS.hlsl
//
// A simple pixel shader to render a texture in gamma space
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

float4 main11(Interpolators In) : SV_Target0
{
    float4 linearColor = Texture.Sample(PointSampler, In.TexCoord);
    return pow(linearColor, 1.0f / 2.2f);
}

[RootSignature(FullScreenQuadRS)]
float4 main(Interpolators In) : SV_Target0
{
    return main11(In);
}
