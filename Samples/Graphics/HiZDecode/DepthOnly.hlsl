//--------------------------------------------------------------------------------------
// DepthOnly.hlsl
//
// Depth-only vertex shader to draw a full-screen quad
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

[RootSignature(FullScreenQuadRS)]
float4 main(uint vI : SV_VertexId) : SV_Position
{
    // We use the 'big triangle' optimization so you only Draw 3 verticies instead of 4.
    float2 texcoord = float2((vI << 1) & 2, vI & 2);
    return float4(texcoord.x * 2 - 1, -texcoord.y * 2 + 1, 1 /* This is the clear value! */, 1);
}
