//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Global.hlsli"

#ifndef DEPTH_UTIL_DEPTH_EXPANDED_CLEAR_MODE
# define DEPTH_UTIL_DEPTH_EXPANDED_CLEAR_MODE 1     // driver default
#endif

#include "DepthDecompressUtility.hlsli"




void ProcessTile(bool tileClear, uint2 tile, uint zMask, uint2 localCoord, uint threadIndex)
{
    uint2 pixelCoord = (tile << 3) | localCoord;
    float linearZ = -1.0;           // got sign bit, might as well use it to indicate clear

    if (!tileClear)
    {
        float depth = DecompressDepth(DepthSRV, zMask, pixelCoord, threadIndex);

        if (depth != 0.0)           // reverseZ, 0 is clear
        {            
            linearZ = LinearizeAndForwardZ(depth);
        }
    }
    LinearDepthUAV[pixelCoord] = linearZ;
#ifdef HALF_RES_Z
    if (((pixelCoord.x | pixelCoord.y) & 1) == 0)
    {
        LinearDepthHalfResUAV[pixelCoord >> 1] = linearZ;
    }
#endif
}


[RootSignature(GlobalRS)]
[numthreads(8, 8, 1)]           // each group processes 16x16 pixels
void DepthDecompress(uint2 Gid : SV_GroupID, uint threadIndex : SV_GroupIndex, uint2 GTid : SV_GroupThreadID)
{
    // scalar start (this is a huge scalar only prefix!)
    uint2 tile = Gid.xy << 1;   // 16x16 tile
    uint2 tile00 = tile | uint2(0, 0);
    uint2 tile01 = tile | uint2(0, 1);
    uint2 tile10 = tile | uint2(1, 0);
    uint2 tile11 = tile | uint2(1, 1);

    uint zMask00 = GetZMask(HTileSRV, htileInfo, tile00);
    uint zMask01 = GetZMask(HTileSRV, htileInfo, tile01);
    uint zMask10 = GetZMask(HTileSRV, htileInfo, tile10);
    uint zMask11 = GetZMask(HTileSRV, htileInfo, tile11);

    bool clear00 = IsTileDepthClear(zMask00);
    bool clear01 = IsTileDepthClear(zMask01);
    bool clear10 = IsTileDepthClear(zMask10);
    bool clear11 = IsTileDepthClear(zMask11);
    // scalar end

    // decompress depth
    uint2 localCoord = SwizzleLinearIndex(threadIndex); // shared work, one swizzle for 4x tiles

    ProcessTile(clear00, tile00, zMask00, localCoord, threadIndex);
    ProcessTile(clear01, tile01, zMask01, localCoord, threadIndex);
    ProcessTile(clear10, tile10, zMask10, localCoord, threadIndex);
    ProcessTile(clear11, tile11, zMask11, localCoord, threadIndex);
}
