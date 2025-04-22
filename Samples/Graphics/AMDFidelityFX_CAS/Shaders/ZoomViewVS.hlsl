//--------------------------------------------------------------------------------------
// ZoomViewVS.hlsl
//
// Vertex shader for zoomed view.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

[RootSignature(FullScreenQuadRS)]
Interpolators main(uint vI : SV_VertexId)
{
    Interpolators output;

    float2 scale = float2(0.06, 0.06);

    // We use the 'big triangle' optimization so you only Draw 3 verticies instead of 4.
    float2 texcoord = float2((vI << 1) & 2, vI & 2);

    output.TexCoord = texcoord * scale + float2(0.5, 0.5) - scale / 2.0;
    output.Position = float4(texcoord.x * 2 - 1, -texcoord.y * 2 + 1, 0, 1);
    return output;
}
