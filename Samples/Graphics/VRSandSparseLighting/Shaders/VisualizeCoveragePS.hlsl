//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FullScreenTriangleVS.hlsl"


[RootSignature(GlobalRS)]
float4 VisualizeCoveragePS(VS_OUTPUT input) : SV_Target
{
    if (LinearDepthSRV[int2(input.uv * float2(renderTargetDims.xy))] < 0.0)
        return float4(0.0, 0.0, 0.2, 1.0);

    uint shadingRate = 0;

    // this would perhaps be cleaner with bindless, however I felt the code would be clearer if we had explicit names for each map, rather than an array index
    // .Load/[] is faster than point sampling on Scarlett consoles
    if(rootConstantCB1 == 1)
        shadingRate = ShadingRateImage2x2_SRV[int2(input.uv * debugSurfaceSizeF.xy)].x;
    else
        shadingRate = ShadingRateImage8x8_SRV[int2(input.uv * debugSurfaceSizeF.xy)].x;

    float coverage = GBufferDebugAndShadingRateSRV[int2(input.uv * float2(renderTargetDims.xy))].w;

    float4 colour;
    switch (shadingRate)
    {
    case D3D12_SHADING_RATE_1X1:    colour = float4(1.0, 0.0, 0.0, 1.0);   break;
    case D3D12_SHADING_RATE_1X2:    colour = float4(1.0, 1.0, 0.0, 1.0);   break;
    case D3D12_SHADING_RATE_2X1:    colour = float4(0.0, 1.0, 1.0, 1.0);   break;
    case D3D12_SHADING_RATE_2X2:    colour = float4(0.0, 0.5, 0.0, 1.0);   break;
    default:                        colour = float4(1.0, 1.0, 1.0, 1.0);   break;  // should never happen!
    }

    // 1x1 rate has a coverage of zero-
    if (coverage > 0.0)
    {
        colour.xyz *= coverage;
    }
    colour.w *= 0.25;
    return colour;
}

