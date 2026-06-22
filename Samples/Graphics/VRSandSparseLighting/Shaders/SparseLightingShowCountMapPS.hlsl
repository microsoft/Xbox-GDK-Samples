//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "FullScreenTriangleVS.hlsl"
#include "SparseLightingCommon.hlsli"



[RootSignature(GlobalRS)]
float3 VisualiseCountMapPS(VS_OUTPUT input) : SV_Target
{
    float brightness = 0.7;

    uint count = GetSparseLightingCountVector(uint2(input.uv * debugSurfaceSizeF.xy));

    if (count == 0)
    {
        return float3(0.0, 0.0, 0.0);
    }
    uint numWaves = AlignUp(count, 32) / 32;

    if (numWaves == 8)
    {
        return float3(1.0, 0.0, 0.0) * brightness;
    }
    if (numWaves == 1)
    {
        return float3(0.0, 1.0, 0.0) * brightness;
    }
    if (numWaves == 2)
    {
        return float3(0.0, 0.5, 0.0) * brightness;
    }
    float t = float(numWaves - 2) / 5.0;

    return float3(t, t * 0.5 + 0.5, 1.0 - t) * brightness;
}
