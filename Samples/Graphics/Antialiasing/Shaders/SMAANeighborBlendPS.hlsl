//--------------------------------------------------------------------------------------
// SMAANeighborBlendPS.hlsl
//
// Pixel shader for SMAA pass 3 neighborhood blending
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

#include "SMAA.hlsli"

[RootSignature(FullScreenQuadRS)]
float4 main(VSSmaaNeighborhoodBlendingOut In) : SV_Target0
{
    return SMAANeighborhoodBlendingPS(In);
}
