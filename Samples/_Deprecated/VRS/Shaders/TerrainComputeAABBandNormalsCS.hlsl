//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"


#define GROUP_DIM		8
#define GROUP_SIZE		(GROUP_DIM * GROUP_DIM)
#define LDS_DIM			(GROUP_DIM + 2)
#define HALF_LDS_DIM	(LDS_DIM >> 1)
#define LDS_SIZE		(LDS_DIM * LDS_DIM)



groupshared float g_lds[LDS_SIZE];


float GetHeight(int2 coord)
{
	return g_lds[coord.x + (coord.y * LDS_DIM)];
}


void StoreNormal(int2 coord, float2 normal)
{
    uint2 n = uint2(saturate((normal * 0.5) + 0.5) * 65535.0);
    uint nPacked = n.x | (n.y << 16);
    g_lds[coord.x + (coord.y * LDS_DIM)] = asfloat(nPacked);
}


float2 GetNormal(int2 coord)
{
    uint nPacked = asuint(g_lds[coord.x + (coord.y * LDS_DIM)]);
    uint2 n = uint2(nPacked & 0xffff, (nPacked >> 16) & 0xffff);
    return (float2(n.xy) / 32767.5) - 1.0;
}



[RootSignature(GlobalRS)]
[numthreads(GROUP_DIM, GROUP_DIM, 1)]
void TerrainComputeAABBAndNormals(
	uint2 Gid : SV_GroupID,
	uint3 GTid : SV_GroupThreadID,
    uint groupThreadIndex : SV_GroupIndex)
{
    float thisMin, thisMax;

	if ((GTid.x < HALF_LDS_DIM) && (GTid.y < HALF_LDS_DIM))
	{
        float2 uv = ((Gid * GROUP_DIM) + (2 * GTid.xy)) * invSourceTextureSize + halfTexelOffsetSourceSize;

		float4 heights = SourceHeightMapSRV0.Gather(LinearClampSampler, uv);

		uint destIdx = GTid.x * 2 + GTid.y * 2 * LDS_DIM;

		g_lds[destIdx              ] = heights.w;
		g_lds[destIdx + 1          ] = heights.z;
		g_lds[destIdx + LDS_DIM    ] = heights.x;
		g_lds[destIdx + LDS_DIM + 1] = heights.y;

        thisMin = min(heights.x, heights.y);
        thisMin = min(thisMin, heights.z);
        thisMin = min(thisMin, heights.w);

        thisMax = max(heights.x, heights.y);
        thisMax = max(thisMax, heights.z);
        thisMax = max(thisMax, heights.w);
	}
	GroupMemoryBarrierWithGroupSync();

	int2 coord = int2(GTid.xy) + 1;
	float h[9];
	
	h[0] = GetHeight(coord + int2(-1, 1));
	h[1] = GetHeight(coord + int2( 0, 1));
	h[2] = GetHeight(coord + int2( 1, 1));
	h[3] = GetHeight(coord + int2(-1, 0));
	h[4] = GetHeight(coord + int2( 0, 0));
	h[5] = GetHeight(coord + int2( 1, 0));
	h[6] = GetHeight(coord + int2(-1,-1));
	h[7] = GetHeight(coord + int2( 0,-1));
	h[8] = GetHeight(coord + int2( 1,-1));
	/*
		[6][7][8]
		[3][4][5]
		[0][1][2]
	*/	
	float2 normal;
	normal.x = (h[2] - h[0]) + 2.0 * (h[5] - h[3]) + (h[8] - h[6]);
	normal.y = (h[6] - h[0]) + 2.0 * (h[7] - h[1]) + (h[8] - h[2]);
    normal.x = -normal.x;

	uint2 destCoord = (Gid.xy * GROUP_DIM) + GTid.xy;

    float2 normalScaled = normal * NORMAL_ENCODING_SCALE;

    NormalMapUAV[0][destCoord] = normalScaled;

    // write per tile min/mayY using interlocked instructions
    // thisMin/thisMax needs setting on threads which were not involved in loading data
    if ((GTid.x >= HALF_LDS_DIM) || (GTid.y >= HALF_LDS_DIM))
    {
        thisMin = h[4];
        thisMax = h[4];
    }
    float boundingBoxMin = WaveActiveMin(thisMin);
    float boundingBoxMax = WaveActiveMax(thisMax);

    if (WaveIsFirstLane()) 
    {
        // This is calculating the tile min/max from the full res height field, would be less ops to do this from the lower
        uint destAddress = TerrainAABBAddress(destCoord >> sourceToHeightMapSizeLog2, tileSizeLog2, tileCountPerAxis);

        InterlockedMin(AABBTileMinMaxUAV[destAddress + 0], TerrainFloatToComparisonUint(boundingBoxMin));
        InterlockedMax(AABBTileMinMaxUAV[destAddress + 1], TerrainFloatToComparisonUint(boundingBoxMax));
    }
    // write low res height
    uint mask = (1U << sourceToHeightMapSizeLog2) - 1;
    if (((GTid.x | GTid.y) & mask) == 0)
    {
        HeightMapUAV[((Gid * GROUP_DIM) + GTid.xy) >> sourceToHeightMapSizeLog2] = max(0.01, h[4]); // zero height denotes out of range
    }
    // repurposing LDS now for normal data, now that we're done with heights
    GroupMemoryBarrierWithGroupSync();

    StoreNormal(GTid.xy, normalScaled);

    // Write to smaller mip levels
    for (uint i = 1; i < 4; ++i)
    {
        GroupMemoryBarrierWithGroupSync();

        uint mask = (1U << i) - 1;

        if (((GTid.x | GTid.y) & mask) == 0)
        {
            int2 ldsCoord = GTid.xy >> (i - 1);
            float2 n1 = GetNormal(ldsCoord + int2(0, 0));
            float2 n2 = GetNormal(ldsCoord + int2(0, 1));
            float2 n3 = GetNormal(ldsCoord + int2(1, 0));
            float2 n4 = GetNormal(ldsCoord + int2(1, 1));
            float2 n = (n1 + n2 + n3 + n4) / 4.0;

            NormalMapUAV[i][((Gid * GROUP_DIM) + GTid.xy) >> i] = n;

            GroupMemoryBarrierWithGroupSync();

            StoreNormal(GTid.xy >> i, n);
        }
    }
}
