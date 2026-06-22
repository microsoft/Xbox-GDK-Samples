//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"


[RootSignature(GlobalRS)]
[numthreads(8, 8, 1)]
void TerrainCompute8x8maxY(
    uint2 DTid : SV_DispatchThreadID,
    uint groupThreadIndex : SV_GroupIndex)
{
    float boundingBoxMax = WaveActiveMax(HeightMapSRV[DTid]);

    if (groupThreadIndex == 0)
    {
        LowResMaxHeightUAV[DTid >> 3] = boundingBoxMax;
    }
}
