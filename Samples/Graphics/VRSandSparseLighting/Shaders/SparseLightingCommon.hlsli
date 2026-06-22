//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

#include "Global.hlsli"

#ifndef TITLE_USES_VRS_TIER1
# define TITLE_USES_VRS_TIER1                           0
#endif

#ifndef SPARSE_LIGHTING
# define SPARSE_LIGHTING                                1
#endif

/*
    (256-32) = 224 entries per tile

    This is a 1/8th memory saving (~1.75mb at 1440p) vs having memory for 256 coordinates per 16x16 tile. Since if we have
    more than 224 coordinates, we are not saving any wave execution, so we light every pixel and don't read any coordinates
*/
#define MAX_COORDINATES_PER_SCREEN_TILE                 224

// Saved 45us Lockhart and 40us Anaconda (time saving is in deferred lighting pass, about 3.5% reduction, no extra cost in depth decompress)
#ifndef SPARSE_LIGHTING_USE_21BIT_UNORM_PACKED_DEPTH
# define SPARSE_LIGHTING_USE_21BIT_UNORM_PACKED_DEPTH   1
#endif

#define QUAD_COVERAGE_SHARE_VERTICAL_0                  1
#define QUAD_COVERAGE_SHARE_HORIZONTAL_0                2
#define QUAD_COVERAGE_SHARE_VERTICAL_1                  4
#define QUAD_COVERAGE_SHARE_HORIZONTAL_1                8

/*
    Sparse coordinate consists of
    4bits x, 4bits y (tile relatively coordinate)
    3bits for copying end result: horizontally, vertically, diagonally
    21 bit unorm linearZ
*/
#define SPARSE_DATA_GET_X(sparseData)                   (sparseData & 0xf)
#define SPARSE_DATA_GET_Y(sparseData)                   __XB_UBFE(4, 4, sparseData)      // (((sparseData) >> 4) & 0xf)
#define SPARSE_DATA_COPY_H(sparseData)                  ((sparseData) & 0x100)
#define SPARSE_DATA_COPY_V(sparseData)                  ((sparseData) & 0x200)
#define SPARSE_DATA_COPY_D(sparseData)                  ((sparseData) & 0x400)
#if SPARSE_LIGHTING_USE_21BIT_UNORM_PACKED_DEPTH == 1
# define SPARSE_DATA_GET_DEPTH21(sparseData)            (float(((sparseData) >> 11)) / float((1U << 21) - 1))
#endif 


uint BuildSparseData(uint2 localCoord, uint copyFlags, uint depth21Bits)
{
#if SPARSE_LIGHTING_USE_21BIT_UNORM_PACKED_DEPTH == 1
    return localCoord.x | (localCoord.y << 4) | (copyFlags << 8) | (depth21Bits << 11);
#else
    return localCoord.x | (localCoord.y << 4) | (copyFlags << 8);
#endif
}



// Uses 24bit vector instruction
uint GetSparseLightingTileLinearIndexVector(uint2 tile)
{
    return __XB_MadU24(tile.y, sparseLightingBufferWidth, tile.x);
}


// Shader compiler doesn't switch from __XB_MadU24 vector instruction to scalar instructions when presented with scalar input
uint GetSparseLightingTileLinearIndexScalar(uint2 tile)
{
    return tile.y * sparseLightingBufferWidth + tile.x;
}


uint GetSparseLightingCountVector(uint2 tile)
{
    return SparseLightingCountSRV[GetSparseLightingTileLinearIndexVector(tile)];
}


uint GetSparseLightingCountScalar(uint2 tile)
{
    return SparseLightingCountSRV[GetSparseLightingTileLinearIndexScalar(tile)];
}


void WriteSparseLightingCount(uint2 tile, uint count)
{
    SparseLightingCountUAV[GetSparseLightingTileLinearIndexScalar(tile)] = count;
}


/*
    (256-32) = 224 entries per tile

    This is a 1/8th memory saving (~1.75mb at 1440p) vs having memory for 256 coordinates per 16x16 tile. Since if we have
    more than 224 coordinates, we are not saving any wave execution, so we light every pixel and don't read any coordinates
*/
uint TileIDToLinearIndexVector(uint2 tileID)
{
    return __XB_MulU24(GetSparseLightingTileLinearIndexVector(tileID), MAX_COORDINATES_PER_SCREEN_TILE);
}


uint TileIDToLinearIndexScalar(uint2 tileID)
{
    return GetSparseLightingTileLinearIndexScalar(tileID) * MAX_COORDINATES_PER_SCREEN_TILE;
}


/*
    Insert at the very start of your shader, determines whether the given wave has any work to do, and what coordinates
    it has to go process

    returns false if nothing for thread to do
*/
#ifdef __XBOX_ENABLE_WAVE32        // make sure we are calling this with correct wave size
bool SparseLightingPrefix84(in out uint2 Gid, uint2 GTid, out uint2 coord, out float depth01, out bool sparseLightingEnabled, out uint sparseData, bool rotateGid)
{
    // keep shader compiler happy with its warnings, although these values cannot be used uninitialized
    sparseLightingEnabled = false;
    depth01 = 0.0;
    sparseData = 0;

    /*
        By default we were processing all sky tiles first, which created ~60us of VALU downtime while we spin
        clear tiles. Inverting Y to process sky tiles last had a similar result, just at the end of the dispatch.

        Instead dispatch.x/y are swapped, so we are processing the screen left to right instead of top
        to bottom. This puts sky tiles in the mix with work items which is a lot better

        We could build a seperate list of tiles to process and use an ExecuteIndirect, but this is
        unlikely to be quicker, and a gaurenteed net loss when there is no sky

        We could also use a 1D dispatch and use a divide to turn into a 2D tile coordinate, but integer divides
        are nasty, so this trick of swapping x/y around seems to work well enough
    */
    if (rotateGid)
    {
        Gid.xy = Gid.yx;
    }
#if SPARSE_LIGHTING == 1
    // there are 8 wave32s per 16x16 sparse lighting tile, calculate which 16x16 tile we're dealing with
    uint2 tileId                = uint2(Gid.x / 2, Gid.y / 4); 
    uint tileIndex              = GetSparseLightingTileLinearIndexScalar(tileId);
    uint sparseLightingCount    = SparseLightingCountSRV[tileIndex];

    if (!sparseLightingCount)       // scalar
        return false;

    // in the case of a full rate tile, we didn't write any coordinates / it would be pointless reading them
    sparseLightingEnabled       = sparseLightingCount <= (256 - 32);

    if (sparseLightingEnabled)      // scalar
    {
        // this is scalar, so avoid __XB_MadU24 and __XB_MulU24 which are vector instructions
        uint tileWaveIndex      = ((Gid.x & 1) * 4 + (Gid.y & 3));                  // [0..7]
        uint lightCoordIndex    = tileWaveIndex * 32 + WaveGetLaneIndex();

        if (lightCoordIndex >= sparseLightingCount)
            return false;

        sparseData              = SparseLightingCoordsSRV[tileIndex * MAX_COORDINATES_PER_SCREEN_TILE + lightCoordIndex];
        coord.x                 = tileId.x * 16 + SPARSE_DATA_GET_X(sparseData);
        coord.y                 = tileId.y * 16 + SPARSE_DATA_GET_Y(sparseData);
# if SPARSE_LIGHTING_USE_21BIT_UNORM_PACKED_DEPTH == 1
        depth01                 = SPARSE_DATA_GET_DEPTH21(sparseData);                              
# else
        depth01                 = LinearDepthSRV[coord];
# endif
    }
    else
#endif
    {
        // Make new SV_DispatchThreadID
        coord = Gid * uint2(8, 4) + GTid;
        depth01 = LinearDepthSRV[coord];

        if (depth01 < 0.0)       // negative values used to indicate clear
            return false;
    }
    return true;
}
#endif


// Writes colour to UAV between 1 and 4 times
void SparseLightingPostfix(RWTexture2D<float3> UAV, bool sparseLightingEnabled, uint sparseData, uint2 coord, float3 colour)
{
    UAV[coord] = colour;

#if SPARSE_LIGHTING == 1
    if (!sparseLightingEnabled)      // scalar
        return;

    uint2 copyCoord = coord.xy ^ 0x1;

    if (SPARSE_DATA_COPY_H(sparseData))
    {
        UAV[uint2(copyCoord.x, coord.y)] = colour;
    }
    if (SPARSE_DATA_COPY_V(sparseData))
    {
        UAV[uint2(coord.x, copyCoord.y)] = colour;
    }
    if (SPARSE_DATA_COPY_D(sparseData))
    {
        UAV[uint2(copyCoord.x, copyCoord.y)] = colour;
    }
#endif
}


/*
    Create a 4bit mask of whether pixels have a shared broadcast copy under VRS

    Which coverage bits are which pixels depends on shading rate
    See 1x MSAA coverage mask in spec
    https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html

    If we are using Tier2 only, then SV_ShadingRate will be the same for all pixels
    in each 8x8 tile, so within a 2x2 block of pixels, we will always have the same
    shading rate and therefore we don't care how the different coverage values for
    different shading rates inter-relate. Unfortunately if we cannot gaurentee the
    same shading rate within a 2x2 block of pixels (i.e. tier1 is in effect as well)
    then we have to make sure the coverage data stored in the g-buffer is consistent
    between the different shading rates. Unfortunately SV_Coverage repurposes the
    same bits for different shading rates meaning if we just stored SV_Coverage, we
    couldn't tell the shading rates apart later. So if a title is using tier1, we
    need to transform SV_Coverage and write time according to SV_Coverage
*/
uint GetQuadCoverage(uint svCoverage, uint svShadingRate)
{    
#if TITLE_USES_VRS_TIER1 == 1
    uint coverage = 0;

    if ((svShadingRate & D3D12_SHADING_RATE_2X2) == D3D12_SHADING_RATE_2X2)
    {
        if ((svCoverage & 0xf) == 0xf)
        {
            coverage = QUAD_COVERAGE_SHARE_VERTICAL_0 | QUAD_COVERAGE_SHARE_HORIZONTAL_0 | QUAD_COVERAGE_SHARE_VERTICAL_1 | QUAD_COVERAGE_SHARE_HORIZONTAL_1;
        }
        else
        {
            if ((svCoverage & 0xc) == 0xc) coverage  = QUAD_COVERAGE_SHARE_HORIZONTAL_0;
            if ((svCoverage & 0x3) == 0x3) coverage |= QUAD_COVERAGE_SHARE_HORIZONTAL_1;
            if ((svCoverage & 0xa) == 0xa) coverage |= QUAD_COVERAGE_SHARE_VERTICAL_0;
            if ((svCoverage & 0x5) == 0x5) coverage |= QUAD_COVERAGE_SHARE_VERTICAL_1;
        }
    }
    else if (svCoverage == 0x3) // bottom 2 bits set, but this could be 1x2 or 2x1
    {
        if (svShadingRate & D3D12_SHADING_RATE_1X2)         coverage = QUAD_COVERAGE_SHARE_VERTICAL_0;
        else if (svShadingRate & D3D12_SHADING_RATE_2X1)    coverage = QUAD_COVERAGE_SHARE_HORIZONTAL_0;
    }
    return coverage;
#else
    return svCoverage;
#endif
}
