//--------------------------------------------------------------------------------------
// SMAABlendWeightPS.hlsl
//
// Pixel shader for SMAA pass 2 blend weights
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------

#include "FullScreenQuad.hlsli"

#include "SMAA.hlsli"

[RootSignature(FullScreenQuadRS)]
float4 main(VSSmaaBlendingWeightOut In) : SV_Target0
{
    return SMAABlendingWeightCalculationPS(In);
}
