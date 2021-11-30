//--------------------------------------------------------------------------------------
// SMAABlendWeightVS.hlsl
//
// Vertex shader for SMAA pass 2 blend weights
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

#include "SMAA.hlsli"

[RootSignature(FullScreenQuadRS)]
VSSmaaBlendingWeightOut main(uint vI : SV_VertexId)
{
    // We use the 'big triangle' optimization so you only Draw 3 verticies instead of 4.
    float2 texcoord = float2((vI << 1) & 2, vI & 2);

    VSSmaaBlendingWeightIn vsIn;
    vsIn.tex = texcoord;
    vsIn.pos = float4(texcoord.x * 2 - 1, -texcoord.y * 2 + 1, 0, 1);

    return SMAABlendingWeightCalculationVS(vsIn);
}
