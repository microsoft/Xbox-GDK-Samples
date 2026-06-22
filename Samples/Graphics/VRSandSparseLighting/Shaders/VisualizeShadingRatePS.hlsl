//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FullScreenTriangleVS.hlsl"


[RootSignature(GlobalRS)]
float4 VisualizeShadingRatePS(VS_OUTPUT input) : SV_Target
{
    uint shadingRate = 0;

    // this would perhaps be cleaner with bindless, however I felt the code would be clearer if we had explicit names for each map, rather than an array index
    // .Load/[] is faster than point sampling on Scarlett consoles
    if(rootConstantCB1 == 1)
        shadingRate = ShadingRateImage2x2_SRV[int2(input.uv * debugSurfaceSizeF.xy)].x;
    else
        shadingRate = ShadingRateImage8x8_SRV[int2(input.uv * debugSurfaceSizeF.xy)].x;

    float4 colour;
    switch (shadingRate)
    {
    case D3D12_SHADING_RATE_1X1:    colour = float4(1.0, 0.0, 0.0, 1.0);   break;
    case D3D12_SHADING_RATE_1X2:    colour = float4(1.0, 1.0, 0.0, 1.0);   break;
    case D3D12_SHADING_RATE_2X1:    colour = float4(0.0, 1.0, 1.0, 1.0);   break;
    case D3D12_SHADING_RATE_2X2:    colour = float4(0.0, 0.0, 0.0, 0.0);   break;
    default:                        colour = float4(1.0, 1.0, 1.0, 1.0);   break;  // should never happen!
    }
    return 0.25 * colour;
}

