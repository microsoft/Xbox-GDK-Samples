//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainRenderVS.hlsl"
#include "TerrainRaycast.hlsli"


[RootSignature(GlobalRS)]
float4 TerrainRenderPS(VS_OUTPUT input) : SV_Target
{
    float3 normal = LoadAndDecodeNormal(input.worldPos);

    // AO
    float3 aoDebug;
    float3 colour = CalculateAO(input.worldPos, normal, aoDebug);

#if DEBUG_LODS == 1
    switch (input.debug)
    {
    case 0: // very low res, keep green
        colour.xz = 0.0;
        break;
    case 1: // low res, keep blue
        colour.xy = 0.0;
        break;
    case 2: // med res, keep yellow
        colour.z = 0.0;
        break;
    case 3: // high res, keep red
        colour.yz = 0.0;
        break;
    }
#endif
    return float4(colour, 1.0);
}

