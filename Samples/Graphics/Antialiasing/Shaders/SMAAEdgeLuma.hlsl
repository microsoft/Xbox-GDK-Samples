//--------------------------------------------------------------------------------------
// SMAAEdgeLuma.hlsl
//
// Pixel shader for SMAA pass 1 edge detection using luma
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

#include "SMAA.hlsli"

[RootSignature(FullScreenQuadRS)]
float4 main(VSSmaaEdgeOut In) : SV_Target0
{
    return SMAALumaEdgeDetectionPS(In);
}
