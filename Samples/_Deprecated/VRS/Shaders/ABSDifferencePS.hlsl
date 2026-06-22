//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FullScreenTriangleVS.hlsl"


[RootSignature(GlobalRS)]
float4 ABSDifferencePS(VS_OUTPUT input) : SV_Target
{
    float3 c0 = RenderedImage1x1SRV.SampleLevel(PointClampSampler, input.uv, 0).xyz;
    float3 c1 = RenderedImageVRSSRV.SampleLevel(PointClampSampler, input.uv, 0).xyz;

    return float4(abs(c0 - c1), 1.0);
}

