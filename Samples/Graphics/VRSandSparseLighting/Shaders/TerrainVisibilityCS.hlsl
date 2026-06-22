//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "TerrainGlobal.hlsli"


#define VIS_NONE            uint(-1)
#define VIS_HIGH_RES        3
#define VIS_MEDIUM_RES      2
#define VIS_LOW_RES         1
#define VIS_VERY_LOW_RES    0


uint EncodeTile(uint tileX, uint tileY)
{
	return (tileY << 16) | tileX;
}


float3 GetMinMax(const Plane plane, const AABB box)
{
    return float3(
        plane.normal.x >= 0.0 ? box.maxX : box.minX,
        plane.normal.y >= 0.0 ? box.maxY : box.minY,
        plane.normal.z >= 0.0 ? box.maxZ : box.minZ
        );
}


bool IntersectAABB(const Frustum frustum, const AABB box)
{
    for (int i = 0; i < 5; i++)
    {
        Plane plane = frustum.planes[i];

        float3 minMax = GetMinMax(plane, box);

        if (dot(plane.normal, minMax) < -plane.D)
            return false;
    }
    return true;
}


uint CullTerrainTile(uint2 tileId)
{
	uint srcAddress = TerrainAABBAddress(tileId, terrainTileCountPerAxis);

    AABB bounds;

    bounds.minY = TerrainComparisonUintToFloat(AABBTileMinMaxSRV[srcAddress + 0]);
    bounds.maxY = TerrainComparisonUintToFloat(AABBTileMinMaxSRV[srcAddress + 1]);

    int terrainTileSize = 1u << terrainTileSizeLog2;

    bounds.minX = float(tileId.x << terrainTileSizeLog2);
    bounds.minZ = float(tileId.y << terrainTileSizeLog2);
    bounds.maxX = bounds.minX + float(terrainTileSize);
    bounds.maxZ = bounds.minZ + float(terrainTileSize);

    bounds.minX = (bounds.minX * worldScale) + xzTranslation;
    bounds.maxX = (bounds.maxX * worldScale) + xzTranslation;

    bounds.minZ = (bounds.minZ * worldScale) + xzTranslation;
    bounds.maxZ = (bounds.maxZ * worldScale) + xzTranslation;

    // ((tileId.x ^ tileId.y) & 1) ? true : false;
    if (!IntersectAABB(viewFrustum, bounds))
    {
        return VIS_NONE;
    }

    float yDelta = (bounds.maxY - bounds.minY) * TERRAIN_TILE_Y_DELTA_SCALE;
    uint lod = VIS_HIGH_RES;

    if (bounds.maxY < TERRAIN_TILE_HEIGHT_ABOVE_WHICH_LOD_BUMP)
    {
        if (yDelta <= TERRAIN_TILE_CONSIDERD_FLAT_Y_DELTA_VERY_LOW)
        {
            return VIS_VERY_LOW_RES;
        }
    }
    {
        float3 center = float3( bounds.minX + 0.5 * (bounds.maxX - bounds.minX),
                                bounds.minY + 0.5 * yDelta,
                                bounds.minZ + 0.5 * (bounds.maxZ - bounds.minZ));

        float3 v = center - cameraPos;
        float dist = dot(v, v);

        if (bounds.maxY > TERRAIN_TILE_HEIGHT_ABOVE_WHICH_EXTRA_WEIGHT)
            dist *= 0.5f;

        if (dist > (yDelta * yDelta * TERRAIN_VERY_LOW_LOD_DISTANCE * TERRAIN_VERY_LOW_LOD_DISTANCE))
        {
            lod = VIS_VERY_LOW_RES;
        }
        else if (dist > (yDelta * yDelta * TERRAIN_LOW_LOD_DISTANCE * TERRAIN_LOW_LOD_DISTANCE))
        {
            lod = VIS_LOW_RES;
        }
        else if (dist > (yDelta * yDelta * TERRAIN_MED_LOD_DISTANCE * TERRAIN_MED_LOD_DISTANCE))
        {
            lod = VIS_MEDIUM_RES;
        }
    }
    if (bounds.maxY < TERRAIN_TILE_HEIGHT_ABOVE_WHICH_EXTRA_WEIGHT)
    {
        if (yDelta < TERRAIN_TILE_CONSIDERD_FLAT_Y_DELTA_LOW)
        {
            return min(lod, VIS_LOW_RES);
        }
        if (yDelta < TERRAIN_TILE_CONSIDERD_FLAT_Y_DELTA_MED)
        {
            return min(lod, VIS_MEDIUM_RES);
        }
    }
    if (bounds.maxY >= TERRAIN_TILE_HEIGHT_ABOVE_WHICH_LOD_BUMP)
    {
        lod = min(lod + 1, VIS_HIGH_RES);
    }
    return lod;
}


void AppendTile(uint2 threadId, uint visCode, uint waveAppendOffset, uint bucket)
{
    uint threadAppendOffset = WaveReadLaneFirst(waveAppendOffset);
    threadAppendOffset += WavePrefixCountBits(visCode == bucket);

    if (visCode == bucket)
    {
        InstanceBufferUAV[visCode][threadAppendOffset] = EncodeTile(threadId.x, threadId.y);
    }
}


[RootSignature(GlobalRS)]
[numthreads(8, 8, 1)]
void main(uint2 threadId : SV_DispatchThreadID, uint groupThreadIndex : SV_GroupIndex)
{
    if (!(threadId.x | threadId.y))
    {
        // cheap zero, only one thread on entire dispatch will come in here
        // saves firing off another GPU task to clear the count (but does require double buffering of args)
        DrawArgs[(frameIndex ^ 1)    ].InstanceCount = 0;   // very low res args
        DrawArgs[(frameIndex ^ 1) + 2].InstanceCount = 0;   // low res args
        DrawArgs[(frameIndex ^ 1) + 4].InstanceCount = 0;   // med res args
        DrawArgs[(frameIndex ^ 1) + 6].InstanceCount = 0;   // high res args
    }
    uint visCode = CullTerrainTile(threadId.xy);
    uint appendCountHighRes     = WaveActiveCountBits(visCode == VIS_HIGH_RES);
    uint appendCountMediumRes   = WaveActiveCountBits(visCode == VIS_MEDIUM_RES);
    uint appendCountLowRes      = WaveActiveCountBits(visCode == VIS_LOW_RES);
    uint appendCountVeryLowRes  = WaveActiveCountBits(visCode == VIS_VERY_LOW_RES);

    if (appendCountLowRes + appendCountHighRes + appendCountMediumRes + appendCountVeryLowRes)
    {
        uint waveAppendOffsetVeryLowRes = 0;
        uint waveAppendOffsetLowRes = 0;
        uint waveAppendOffsetMediumRes = 0;
        uint waveAppendOffsetHighRes = 0;
        if(WaveIsFirstLane())
        {
            if (appendCountHighRes)     InterlockedAdd(DrawArgs[frameIndex + 6].InstanceCount, appendCountHighRes, waveAppendOffsetHighRes);
            if (appendCountMediumRes)   InterlockedAdd(DrawArgs[frameIndex + 4].InstanceCount, appendCountMediumRes, waveAppendOffsetMediumRes);
            if (appendCountLowRes)      InterlockedAdd(DrawArgs[frameIndex + 2].InstanceCount, appendCountLowRes, waveAppendOffsetLowRes);
            if (appendCountVeryLowRes)  InterlockedAdd(DrawArgs[frameIndex + 0].InstanceCount, appendCountVeryLowRes, waveAppendOffsetVeryLowRes);
        }
        if (appendCountHighRes)         AppendTile(threadId, visCode, waveAppendOffsetHighRes, VIS_HIGH_RES);
        if (appendCountMediumRes)       AppendTile(threadId, visCode, waveAppendOffsetMediumRes, VIS_MEDIUM_RES);
        if (appendCountLowRes)          AppendTile(threadId, visCode, waveAppendOffsetLowRes, VIS_LOW_RES);
        if (appendCountVeryLowRes)      AppendTile(threadId, visCode, waveAppendOffsetVeryLowRes, VIS_VERY_LOW_RES);
    }
}
