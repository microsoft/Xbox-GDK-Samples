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

groupshared float g_heights[LDS_DIM][LDS_DIM];



float GetHeight(int2 coord)
{
   return g_heights[coord.x][coord.y];
}



float GetHeight(int2 coord, float safeHeight)
{
    float height = g_heights[coord.x][coord.y];

    return (0.0 == height) ? safeHeight : height;        // read off edge of texture?
}



[RootSignature(GlobalRS)]
[numthreads(GROUP_DIM, GROUP_DIM, 1)]
void TerrainSmooth(
    uint2 Gid : SV_GroupID,
    uint3 GTid : SV_GroupThreadID
    )
{
    if ((GTid.x < HALF_LDS_DIM) && (GTid.y < HALF_LDS_DIM))
    {
        uint2 dst = 2 * GTid.xy;
        uint2 src = (Gid * GROUP_DIM + dst) - 1;
        float4 heights;

        if (sourceBufferIndex)      // TODO bindless
        {
            heights.w = SourceHeightMapUAV1[int2(src.x + 0, src.y + 0)];
            heights.z = SourceHeightMapUAV1[int2(src.x + 1, src.y + 0)];
            heights.x = SourceHeightMapUAV1[int2(src.x + 0, src.y + 1)];
            heights.y = SourceHeightMapUAV1[int2(src.x + 1, src.y + 1)];
        }
        else
        {
            heights.w = SourceHeightMapUAV0[int2(src.x + 0, src.y + 0)];
            heights.z = SourceHeightMapUAV0[int2(src.x + 1, src.y + 0)];
            heights.x = SourceHeightMapUAV0[int2(src.x + 0, src.y + 1)];
            heights.y = SourceHeightMapUAV0[int2(src.x + 1, src.y + 1)];
        }
        g_heights[dst.x + 0][dst.y + 0] = heights.w;
        g_heights[dst.x + 1][dst.y + 0] = heights.z;
        g_heights[dst.x + 0][dst.y + 1] = heights.x;
        g_heights[dst.x + 1][dst.y + 1] = heights.y;
    }
    GroupMemoryBarrierWithGroupSync();

    int2 coord = int2(GTid.xy) + 1;
	float h[9];
	
    h[4] = GetHeight(coord + int2( 0, 0));

    h[0] = GetHeight(coord + int2(-1, 1), h[4]);
	h[1] = GetHeight(coord + int2( 0, 1), h[4]);
	h[2] = GetHeight(coord + int2( 1, 1), h[4]);
	h[3] = GetHeight(coord + int2(-1, 0), h[4]);
	h[5] = GetHeight(coord + int2( 1, 0), h[4]);
	h[6] = GetHeight(coord + int2(-1,-1), h[4]);
	h[7] = GetHeight(coord + int2( 0,-1), h[4]);
	h[8] = GetHeight(coord + int2( 1,-1), h[4]);
	/*
		[6][7][8]
		[3][4][5]
		[0][1][2]
	*/
    float height = h[4];

    float maxCap = TERRAIN_SMOOTHING_HEIGHT_CAP;

    if (height < maxCap)
    {
        float delta;
        delta =     abs(h[0] - height);
        delta = max(abs(h[1] - height), delta);
        delta = max(abs(h[2] - height), delta);
        delta = max(abs(h[3] - height), delta);

        delta = max(abs(h[5] - height), delta);
        delta = max(abs(h[6] - height), delta);
        delta = max(abs(h[7] - height), delta);
        delta = max(abs(h[8] - height), delta);

        float blendFactor = (maxCap - height) / maxCap;
        float maxDelta = blendFactor * 3.0;
        if (delta < maxDelta)
        {
            blendFactor *= 1.0 - (delta / maxDelta);

            float averageHeight = (h[0] + h[1] + h[2] + h[3] + h[4] + h[5] + h[6] + h[7] + h[8]) / 9.0;
            height = lerp(height, averageHeight, blendFactor);
        }
    }
    uint2 dst = Gid * GROUP_DIM + GTid.xy;

    if (sourceBufferIndex)
    {
        SourceHeightMapUAV0[dst] = height;
    }
    else
    {
        SourceHeightMapUAV1[dst] = height;
    }
}

