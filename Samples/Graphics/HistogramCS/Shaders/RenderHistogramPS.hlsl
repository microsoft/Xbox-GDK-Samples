//--------------------------------------------------------------------------------------
// RenderHistogramPS.hlsl
//
// A simple pixel shader to render a texture 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

cbuffer PSHData : register(b0)
{
    uint NumBins;
    float Scale;
};

Buffer<uint> Counts : register(t0);

[RootSignature(FullScreenQuadRS)]
float4 main(Interpolators In) : SV_Target0
{
    float x = In.TexCoord.x * NumBins * 2;
    clip((float)((int)x & 1) - 0.5f);
    int count = Counts[(int)(x / 2)];
    if ((1 - In.TexCoord.y) * Scale > (float)count)
        clip(-1);
    return 1;
}
