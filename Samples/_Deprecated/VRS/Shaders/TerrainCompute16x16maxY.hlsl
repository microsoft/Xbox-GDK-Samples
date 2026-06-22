//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"


[RootSignature(GlobalRS)]
[numthreads(8, 8, 1)]
void TerrainCompute16x16maxY(
    uint2 Gid : SV_GroupID,
    uint2 GTid : SV_GroupThreadID,
    uint2 DTid : SV_DispatchThreadID,
    uint groupThreadIndex : SV_GroupIndex)
{
    int2 coord = ((Gid * 8) + GTid) << 1;

    float boundingBoxMax1 = HeightMapSRV[coord + int2(0, 0)];
    float boundingBoxMax2 = HeightMapSRV[coord + int2(1, 0)];
    float boundingBoxMax3 = HeightMapSRV[coord + int2(0, 1)];
    float boundingBoxMax4 = HeightMapSRV[coord + int2(1, 1)];

    float boundingBoxMax = WaveActiveMax(max(max(boundingBoxMax1, boundingBoxMax2), max(boundingBoxMax3, boundingBoxMax4)));

    if (groupThreadIndex == 0)
    {
        LowResMaxHeightUAV[DTid >> 3] = boundingBoxMax;
    }
}
