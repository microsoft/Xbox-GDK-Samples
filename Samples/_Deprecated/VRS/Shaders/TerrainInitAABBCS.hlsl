//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"

#define GROUP_DIM		8



[RootSignature(GlobalRS)]
[numthreads(GROUP_DIM * GROUP_DIM, 1, 1)]
void InitAABBMinMax(uint DTid : SV_DispatchThreadID)
{
	uint destAddress = DTid.x * 2;

	AABBTileMinMaxUAV[destAddress + 0] = 0xffffffff;    // min
	AABBTileMinMaxUAV[destAddress + 1] = 0;             // max
}
