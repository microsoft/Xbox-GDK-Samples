         //--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"


float GetHeight(float2 pos)
{
	return Fbm(pos, 16, 1.0);
}


[RootSignature(GlobalRS)]
[numthreads(8, 8, 1)]
void TerrainGenerate(uint3 threadId : SV_DispatchThreadID)
{
    float2 offset = float2(10.0, 0.0);
    float scale = 0.0025;
    
    float height = GetHeight(offset + float2(threadId.xy) * scale * sourceToHeightMapSizeMultiplier);
    height *= 0.5;
    height *= height;
    height *= worldScaleY;

    SourceHeightMapUAV0[threadId.xy] = max(0.01, height + 0.01);   // zero height is used as an optimisation to denotes out of range
}

