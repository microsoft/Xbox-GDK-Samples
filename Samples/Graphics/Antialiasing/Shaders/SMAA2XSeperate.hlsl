//--------------------------------------------------------------------------------------
// SMAA2XSeperate.hlsl
//
// Pixel shader for splitting SMAA2X multisampled buffers
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

struct PSSmaaSeparateOut
{
    float4 color0 : SV_TARGET0;
    float4 color1 : SV_TARGET1;
};

Texture2DMS<float4, 2> colorTexMS : register(t0);

[RootSignature(FullScreenQuadRS)]
PSSmaaSeparateOut main(Interpolators In)
{
    PSSmaaSeparateOut psOut;

    int2 pos = int2(In.Position.xy);
    psOut.color0 = colorTexMS.Load(pos, 0);
    psOut.color1 = colorTexMS.Load(pos, 1);
    return psOut;
}
