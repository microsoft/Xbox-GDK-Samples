//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Global.hlsli"
#include "BuildSparseBuffersCommon.hlsli"

#ifndef VRS_SUPER_SAMPLING_SUPPORT_IN_HTILE
# define VRS_SUPER_SAMPLING_SUPPORT_IN_HTILE    1      // driver default
#endif

#ifndef DEPTH_UTIL_DEPTH_EXPANDED_CLEAR_MODE
# define DEPTH_UTIL_DEPTH_EXPANDED_CLEAR_MODE   1     // driver default
#endif

#include "DepthDecompressUtility.hlsli"

// Does the g-buffer contain SV_Coverage information?
#ifndef COVERAGE_IN_GBUFFER
# define COVERAGE_IN_GBUFFER                    1
#endif

/*
    Pack data about a depth sample into a single VGPR

    32 bits split into

    2 flags, clear and/or decomrpessed
    9 bits for plane indices (only 16 indices, so this is far more than we need)
    21 bit linear Z
*/
#define DEPTHDATA_CLEAR                                     uint(0x80000000)
#define DEPTHDATA_DECOMPRESSED                              uint(0x40000000)
#define DEPTHDATA_DEPTH_MASK                                ((1 << 21) - 1)
#define DEPTHDATA_PLANE_INDEX_MASK                          (~(DEPTHDATA_CLEAR | DEPTHDATA_DECOMPRESSED | DEPTHDATA_DEPTH_MASK))
#define DEPTHDATA_CREATE_PLANE_INDEX(index)                 ((index) << 21)
#define DEPTHDATA_GET_PLANE_INDEX(data)                     (((data) & DEPTHDATA_PLANE_INDEX_MASK) >> 21)

/*
    If HTile is in the super sampling mode, i.e. with support for 4x4 mode (which is the driver default)
    then shading rate is stored using bits: 6,7,10,11
    otherwise in non super sampling rate, bits: 10,11

    0 value means high rate
    1 value maens low rate in that axis
*/
#if VRS_SUPER_SAMPLING_SUPPORT_IN_HTILE
# define HTILE_SHADING_RATE_MASK                            0xcc0
# define HTILE_GET_YAXIS_SHADING_RATE_RAW(hTile)            ((hTile) & 0xc00)
# define HTILE_GET_XAXIS_SHADING_RATE_RAW(hTile)            ((hTile) & 0x0c0)
#else
# define HTILE_SHADING_RATE_MASK                            0xc00
# define HTILE_GET_YAXIS_SHADING_RATE_RAW(hTile)            ((hTile) & 0x400)
# define HTILE_GET_XAXIS_SHADING_RATE_RAW(hTile)            ((hTile) & 0x800)
#endif

groupshared uint g_DepthData[16][16];




uint GetZMaskAndRawShadingRate(out uint rawShadingRate, ByteAddressBuffer HTileBuffer, uint HTileInfo, uint2 TileCoord)
{
    uint HTileValue = HTileBuffer.Load(GetHTileAddress(TileCoord, HTileInfo));
    rawShadingRate = HTileValue & HTILE_SHADING_RATE_MASK;
    return __XB_UBFE(4, 0, HTileValue);
}


float GetZValue(ZDataLayout layout, uint zDataOffset, uint2 globalPixel, uint flattenedIndex, out uint planeIndex)
{
    GroupMemoryBarrierWithGroupSync();

    planeIndex = 0;
    int2 pixelPos = globalPixel & 7;

    [branch] // scalar branch
    if (layout.planeCount > 1)
    {
        uint element = flattenedIndex;
        uint bitOffset = __XB_MulU24(element, layout.indexBits);
        uint indexOffset = zDataOffset + layout.pMaskOffset + (bitOffset >> 5);

        bitOffset &= 31;

        planeIndex = sharedDepthData[indexOffset] >> bitOffset;
        uint bitsRead = 32 - bitOffset;
        if (layout.indexBits > bitsRead)
            planeIndex |= sharedDepthData[indexOffset + 1] << bitsRead;

        planeIndex &= __XB_BFM(layout.indexBits, 0);
    }
    uint planeOffset = __XB_MadU24(planeIndex, 3, zDataOffset);
    uint3 plane = uint3(sharedDepthData[planeOffset], sharedDepthData[planeOffset + 1], sharedDepthData[planeOffset + 2]);

    // 4-bit pixel centers are {-7, -5, -3, -1, +1, +3, +5, +7}
    pixelPos = pixelPos * 2 - 7;
    return GetDepthFromZPlane(plane, pixelPos);
}


float DecompressDepth2(Texture2D<float> depthSrc, uint zMask, uint2 st, uint threadIndex, in out uint depthData)
{
    // Uncompressed
    if (zMask == 15)
    {
        depthData |= DEPTHDATA_DECOMPRESSED;
        return depthSrc[st];
    }
    ZDataLayout ZLayout = GetZDataLayout(zMask, 0);
    uint zDataOffset = GetZDataOffset(ZLayout, st);
    uint maxLoadIndex = zDataOffset + ZLayout.planeDataWords;

    if (threadIndex < maxLoadIndex)
    {
        sharedDepthData[threadIndex] = asuint(depthSrc[st]);
    }
    uint planeIndex;
    float depth = GetZValue(ZLayout, zDataOffset, st, threadIndex, planeIndex);
    depthData |= DEPTHDATA_CREATE_PLANE_INDEX(planeIndex);
    return depth;
}


void ProcessTile(bool tileClear, uint2 tile, uint zMask, uint2 localCoord, uint threadIndex)
{
    uint2 pixelCoord = (tile << 3) | localCoord;
    uint depthData = DEPTHDATA_CLEAR;
    float linearZ = -1.0;           // got sign bit, might as well use it to indicate clear

    if (!tileClear)
    {
        float depth = DecompressDepth2(DepthSRV, zMask, pixelCoord, threadIndex, depthData);

        if (depth != 0.0)           // reverseZ, 0 is clear
        {            
            linearZ = LinearizeAndForwardZ(depth);

            depthData &= ~DEPTHDATA_CLEAR;

#if SPARSE_LIGHTING_USE_21BIT_UNORM_PACKED_DEPTH == 1
            depthData |= Build21BitDepth(linearZ);
#endif
        }
    }
    uint2 LDSCoord = pixelCoord & 15;
    g_DepthData[LDSCoord.x][LDSCoord.y] = depthData;

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
void DepthDecompressAndBuildSparseBuffersCS(uint2 Gid : SV_GroupID, uint threadIndex : SV_GroupIndex, uint2 GTid : SV_GroupThreadID)
{
    // scalar start (this is a huge scalar only prefix!)
    bool shadingRatefromHTile = rootConstantCB1 ? false : true;

    uint2 tile = Gid.xy << 1;   // 16x16 tile
    uint2 tile00 = tile | uint2(0, 0);
    uint2 tile01 = tile | uint2(0, 1);
    uint2 tile10 = tile | uint2(1, 0);
    uint2 tile11 = tile | uint2(1, 1);

    uint rawShadingRate00, rawShadingRate01, rawShadingRate10, rawShadingRate11;
    uint zMask00 = GetZMaskAndRawShadingRate(rawShadingRate00, HTileSRV, htileInfo, tile00);
    uint zMask01 = GetZMaskAndRawShadingRate(rawShadingRate01, HTileSRV, htileInfo, tile01);
    uint zMask10 = GetZMaskAndRawShadingRate(rawShadingRate10, HTileSRV, htileInfo, tile10);
    uint zMask11 = GetZMaskAndRawShadingRate(rawShadingRate11, HTileSRV, htileInfo, tile11);

    bool tileClear00 = IsTileDepthClear(zMask00);
    bool tileClear01 = IsTileDepthClear(zMask01);
    bool tileClear10 = IsTileDepthClear(zMask10);
    bool tileClear11 = IsTileDepthClear(zMask11);
    bool allTilesClear = tileClear00 && tileClear01 && tileClear10 & tileClear11;

    // normal deferred lighting wastes processing at the edge of the screen if the render target is not
    // a multiple of the tile size, we simply have threads reading and writing harmlessly outside of the
    // surface. Here we can do better, and reduce the lighting count for pixels off render target
    bool tileClipsRenderTarget = any((Gid.xy * 16 + 16) > renderTargetDims.xy);

    // if all tiles are full rate we light every pixel and don't bother with doing an examination work
    // except that if the tile clips the render target, we may still be able to reduce the light count
    bool allTilesHighRate = false;

    if (shadingRatefromHTile && (!tileClipsRenderTarget))
    {
        allTilesHighRate = (0 == (rawShadingRate00 | rawShadingRate01 | rawShadingRate10 | rawShadingRate11));
    }
    // scalar end

    // decompress depth
    uint2 localCoord = SwizzleLinearIndex(threadIndex); // shared work, one swizzle for 4x tiles

    ProcessTile(tileClear00, tile00, zMask00, localCoord, threadIndex);
    ProcessTile(tileClear01, tile01, zMask01, localCoord, threadIndex);
    ProcessTile(tileClear10, tile10, zMask10, localCoord, threadIndex);
    ProcessTile(tileClear11, tile11, zMask11, localCoord, threadIndex);

    // build sparse lighting data
    uint pixelsToLightCount;

    [branch]
    if (allTilesClear || allTilesHighRate)    // allTilesHighRate will be false if tileClipsRenderTarget
    {
        // these are special cases where we don't need to output any coordinates
        // uses as R8, so write 255 instead of 256
        pixelsToLightCount = allTilesClear ? 0 : 255;
    }
    else
    {
        // each thread examines 2x2 pixels
        uint2 tileLocalCoord = GTid.xy << 1;
        uint2 pixelCoord = Gid * 16 + tileLocalCoord;

        uint depthData00 = g_DepthData[tileLocalCoord.x + 0][tileLocalCoord.y + 0];
        uint depthData01 = g_DepthData[tileLocalCoord.x + 0][tileLocalCoord.y + 1];
        uint depthData10 = g_DepthData[tileLocalCoord.x + 1][tileLocalCoord.y + 0];
        uint depthData11 = g_DepthData[tileLocalCoord.x + 1][tileLocalCoord.y + 1];

        uint clearCode;
        clearCode  = (depthData00 >> 31) << CLEAR_CODE_SHIFT_00;
        clearCode |= (depthData01 >> 31) << CLEAR_CODE_SHIFT_01;
        clearCode |= (depthData10 >> 31) << CLEAR_CODE_SHIFT_10;
        clearCode |= (depthData11 >> 31) << CLEAR_CODE_SHIFT_11;

        // select from the four HTiles for this thread and extract shading rate
        uint rawShadingRate;
        if(shadingRatefromHTile)
        {
            uint rawShadingRateY0   = tileLocalCoord.x < 8 ? rawShadingRate00 : rawShadingRate10;
            uint rawShadingRateY1   = tileLocalCoord.x < 8 ? rawShadingRate01 : rawShadingRate11;
            rawShadingRate          = tileLocalCoord.y < 8 ? rawShadingRateY0 : rawShadingRateY1;
        }
        else
        {
            // this is a shading rate per 2x2 pixels, so one read gives us the shading rate for a single thread :)
            rawShadingRate          = ShadingRateImage2x2_SRV[Gid.xy * 8 + GTid.xy];
        }
        bool removeClearPixels = true;
        uint writeCode;

        [branch]
#if COVERAGE_IN_GBUFFER != 0
        if (rawShadingRate == 0)
#else
        if ((rawShadingRate == 0) || (depthData00 & DEPTHDATA_DECOMPRESSED))
#endif
        {
            // 1x1 rate shading, light all pixels
            writeCode = BUILD_WRITE_CODE_LIGHT_ALL4_PIXELS;
        }
        else
        {
            bool equalHoriz0, equalHoriz1, equalVert0, equalVert1;

#if COVERAGE_IN_GBUFFER != 0
            [branch]
            if (depthData00 & DEPTHDATA_DECOMPRESSED)
            {
                /*
                    Depth tile is decompressed, which means we have no plane index data

                    Fall back to reading quad coverage from g-buffer
                    Quad coverage only needs 4 bits, but here I'm using an 8bit channel
                    unorm, so twice as much storage as needed.... (but no bitmask operations)

                    GatherAlpha might be interest here, except you have to calculate the uv
                    and this shader is VALU limited, not TEX limited

                    Some points of interest. Firstly, coverage data already respects shading
                    rate, if we had 1x1 rate, every coverage value would be zero. Secondly because
                    I am only checking for equality, no need to unpack as an integer.

                    If you don't have coverage data from a g-buffer, the only thing to do is
                    fall back to full rate
                */ 
                float coverage00 = GBufferDebugAndShadingRateSRV[pixelCoord + uint2(0, 0)].w;
                float coverage01 = GBufferDebugAndShadingRateSRV[pixelCoord + uint2(0, 1)].w;
                float coverage10 = GBufferDebugAndShadingRateSRV[pixelCoord + uint2(1, 0)].w;
                float coverage11 = GBufferDebugAndShadingRateSRV[pixelCoord + uint2(1, 1)].w;

                equalHoriz0 = (coverage00 == coverage10);
                equalHoriz1 = (coverage01 == coverage11);
                equalVert0  = (coverage00 == coverage01);
                equalVert1  = (coverage10 == coverage11);
            }
            else
#endif
            {
                // all four pixels still compressed :) and non-full rate shading :)
                
                // get the plane index and clear bit
#if SPARSE_LIGHTING_USE_21BIT_UNORM_PACKED_DEPTH == 1
                // and out 21bit depth as we don't want this in the comparison
                uint planeIndex00 = depthData00 & (DEPTHDATA_PLANE_INDEX_MASK | DEPTHDATA_CLEAR);
                uint planeIndex01 = depthData01 & (DEPTHDATA_PLANE_INDEX_MASK | DEPTHDATA_CLEAR);
                uint planeIndex10 = depthData10 & (DEPTHDATA_PLANE_INDEX_MASK | DEPTHDATA_CLEAR);
                uint planeIndex11 = depthData11 & (DEPTHDATA_PLANE_INDEX_MASK | DEPTHDATA_CLEAR);
#else
                // bitwise AND not needed in this case as the depth data only contains plane index and clear bit
# define        planeIndex00 depthData00
# define        planeIndex01 depthData01
# define        planeIndex10 depthData10
# define        planeIndex11 depthData11
#endif
                // clear / not clear pixels will fail this equality test , however both clear will pass which means we still need to call ModifyEqualityForNonClearPixels               
                equalHoriz0 = planeIndex00 == planeIndex10;
                equalHoriz1 = planeIndex01 == planeIndex11;
                equalVert0  = planeIndex00 == planeIndex01;
                equalVert1  = planeIndex10 == planeIndex11;
            }
            // non zero if low rate
            uint shadingRateX, shadingRateY;
            if (shadingRatefromHTile)
            {
                shadingRateX = HTILE_GET_XAXIS_SHADING_RATE_RAW(rawShadingRate);
                shadingRateY = HTILE_GET_YAXIS_SHADING_RATE_RAW(rawShadingRate);
            }
            else
            {
                shadingRateX = rawShadingRate & D3D12_SHADING_RATE_2X1;
                shadingRateY = rawShadingRate & D3D12_SHADING_RATE_1X2;
            }
            writeCode = BuildWriteCode(removeClearPixels, clearCode, shadingRateX, shadingRateY, equalHoriz0, equalHoriz1, equalVert0, equalVert1);
        }
        [branch]
        if (removeClearPixels)
        {
            writeCode = RemoveClearPixels(clearCode, writeCode);
        }
        [branch]
        if (tileClipsRenderTarget)              // scalar branch
        {
            writeCode = RemoveOffRTPixels(writeCode, pixelCoord);
        }
        // all threads have to execute this, since we have cross lane intrinsics involved
        pixelsToLightCount = WriteSparseCoordinates(writeCode, Gid.xy, tileLocalCoord,
            depthData00 & DEPTHDATA_DEPTH_MASK,
            depthData01 & DEPTHDATA_DEPTH_MASK,
            depthData10 & DEPTHDATA_DEPTH_MASK,
            depthData11 & DEPTHDATA_DEPTH_MASK);
    }
    if (threadIndex == 0)
    {
        WriteSparseLightingCount(Gid, pixelsToLightCount);
    }
}
