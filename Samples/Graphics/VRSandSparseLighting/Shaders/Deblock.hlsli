//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Global.hlsli"

#ifndef __XBOX_ENABLE_WAVE32
# error deblocker runs best with wave32
#endif


// recommended, good quality increase for low cost
#ifndef DEBLOCK_SHARPEN_FILTER
# define DEBLOCK_SHARPEN_FILTER                     1
#endif

#define DEPTH_TOLERANCE                             0.01
#define EQUALITY_TOLERANCE                          0.00125
#define SHARPEN_TOLERANCE                           0.25                        // careful not to set this too high or you get 'ringing'
#define INV_SHARPEN_TOLERANCE                       (1.0 / SHARPEN_TOLERANCE)   // change this define to a pre-computed reciprocal if moving to constant buffer)
#define SHARPEN_STRENGTH                            1.5
#define DISSIMILAR_TOLERANCE                        0.05
#define DISSIMILAR_TOLERANCE_INVOKING_DEPTH_TEST    (DISSIMILAR_TOLERANCE * 3.0)

// normal colour if no blend, or blend with neighbour with very close colour
// green - pixels colours were too dissimilar to blend
// blue - depth test was performed and depths were too different to blend
// red - depth test performed and depths were similar, blend performed
//#define DEBUG_BLEND_TEST

groupshared float3 g_colour[18][18];        // with borders, though 4x corner pixels are unused


uint2 BuildLDSCoord(uint2 coord)
{
    return coord + uint2(1, 1);
}


void LoadToLDSBorderL(Texture2D<float3> inputColourSRV, uint threadIndex, uint2 tileXY)
{
    uint2 ldsCoord = uint2(0, threadIndex + 1);

    // reading off the render target will return black which would be pretty disasterous, so clamp reads
    uint2 coord = uint2(tileXY.x ? tileXY.x - 1 : 0, tileXY.y + threadIndex);

    g_colour[ldsCoord.x][ldsCoord.y] = inputColourSRV[coord].xyz;
}


void LoadToLDSBorderR(Texture2D<float3> inputColourSRV, uint threadIndex, uint2 tileXY)
{
    uint2 ldsCoord = uint2(17, threadIndex + 1);

    // reading off the render target will return black which would be pretty disasterous, so clamp reads
    uint2 coord = uint2(tileXY.x + 16, tileXY.y + threadIndex);
    coord       = min(coord, renderTargetDims.xy - 1);

    g_colour[ldsCoord.x][ldsCoord.y] = inputColourSRV[coord].xyz;
}


void LoadToLDSBorderT(Texture2D<float3> inputColourSRV, uint threadIndex, uint2 tileXY)
{
    uint2 ldsCoord = uint2(threadIndex + 1, 0);

    // reading off the render target will return black which would be pretty disasterous, so clamp reads
    uint2 coord = uint2(tileXY.x + threadIndex, tileXY.y ? tileXY.y - 1 : 0);

    g_colour[ldsCoord.x][ldsCoord.y] = inputColourSRV[coord].xyz;
}


void LoadToLDSBorderB(Texture2D<float3> inputColourSRV, uint threadIndex, uint2 tileXY)
{
    uint2 ldsCoord = uint2(threadIndex + 1, 17);

    // reading off the render target will return black which would be pretty disasterous, so clamp reads
    uint2 coord = uint2(tileXY.x + threadIndex, tileXY.y + 16);
    coord       = min(coord, renderTargetDims.xy - 1);

    g_colour[ldsCoord.x][ldsCoord.y] = inputColourSRV[coord].xyz;
}


void LoadToLDS(Texture2D<float3> inputColourSRV, uint2 dstCoord, uint2 srcCoord)
{
    uint2 ldsCoord = BuildLDSCoord(dstCoord);

    // reading off the render target will return black which would be pretty disasterous, so clamp reads
    srcCoord = min(srcCoord.xy, renderTargetDims.xy - 1);

    g_colour[ldsCoord.x][ldsCoord.y] = inputColourSRV[srcCoord.xy].xyz;
}


#if DEBLOCK_SHARPEN_FILTER == 1
bool Blend(Texture2D<float> linearDepthSRV, inout float3 colour, float3 c, uint2 c1, uint2 c2, float diff)
#else
bool Blend(Texture2D<float> linearDepthSRV, inout float3 colour, float3 c, uint2 c1, uint2 c2)
#endif
{
#if DEBLOCK_SHARPEN_FILTER == 0
    float diff = TestEquality(colour, c);
#endif
    if (diff > DISSIMILAR_TOLERANCE)
    {
        if (diff > DISSIMILAR_TOLERANCE_INVOKING_DEPTH_TEST)
        {
            // too dissimilar, don't do blend
#ifdef DEBUG_BLEND_TEST
            colour = float3(0.0, 1.0, 0.0);      
#endif
            return false;
        }
        // only blend midly dissimilarly if within a depth tolerance
        float d1 = linearDepthSRV[c1];
        float d2 = linearDepthSRV[c2];

#ifdef DEBUG_BLEND_TEST
        if (abs(d1 - d2) > DEPTH_TOLERANCE)
        {
            colour = float3(0.0, 0.0, 1.0);
            return false;
        }
        colour = float3(1.0, 0.0, 0.0);
        return true;
#else
        if (abs(d1 - d2) > DEPTH_TOLERANCE)
            return false;
#endif
    }
    colour = 0.5 * (colour + c);
    return true;
}


#if DEBLOCK_SHARPEN_FILTER == 1
bool Deblock(Texture2D<float> linearDepthSRV, inout float3 colour, float3 c1, float3 c2, uint2 coord, uint2 coordC1, uint2 coordC2, float diffC1, float diffC2)
{
    if (TestEquality(colour, c1, EQUALITY_TOLERANCE))
    {
        return Blend(linearDepthSRV, colour, c2, coord, coordC2, diffC2);
    }
    if (TestEquality(colour, c2, EQUALITY_TOLERANCE))
    {
        return Blend(linearDepthSRV, colour, c1, coord, coordC1, diffC1);
    }
    return false;
}
#else
bool Deblock(Texture2D<float> linearDepthSRV, inout float3 colour, float3 c1, float3 c2, uint2 coord, uint2 coordC1, uint2 coordC2)
{
    if (TestEquality(colour, c1, EQUALITY_TOLERANCE))
    {
        return Blend(linearDepthSRV, colour, c2, coord, coordC2);
    }
    if (TestEquality(colour, c2, EQUALITY_TOLERANCE))
    {
        return Blend(linearDepthSRV, colour, c1, coord, coordC1);
    }
    return false;
}
#endif


#if DEBLOCK_SHARPEN_FILTER == 1
void SharpenSample(float diff, float3 colour, inout float3 sum, inout float weight)
{
    if (diff >= SHARPEN_TOLERANCE)
        return;

    float t = 1.0 - diff * INV_SHARPEN_TOLERANCE;
    sum += t * colour;
    weight += t;
}
#endif


float3 Deblock(Texture2D<float> linearDepthSRV, uint2 coord, uint2 localCoord)
{
    uint2 ldsCoord = BuildLDSCoord(localCoord);
    float3 colour = g_colour[ldsCoord.x][ldsCoord.y];

    // horizontal deblock
    uint2 coordC1 = coord + uint2(1, 0);
    uint2 coordC2 = coord - uint2(1, 0);
    float3 c1 = g_colour[ldsCoord.x + 1][ldsCoord.y];
    float3 c2 = g_colour[ldsCoord.x - 1][ldsCoord.y];
    float3 r1 = colour;
#if DEBLOCK_SHARPEN_FILTER == 1
    float diffC1 = TestEquality(colour, c1);
    float diffC2 = TestEquality(colour, c2);
    bool deblockedH = Deblock(linearDepthSRV, r1, c1, c2, coord, coordC1, coordC2, diffC1, diffC2);
#else
    bool deblockedH = Deblock(linearDepthSRV, r1, c1, c2, coord, coordC1, coordC2);
#endif
    // vertical deblock (we can't feed the new deblocked colour into this, or it wouldn't pass the equality test)
    uint2 coordC3 = coord + uint2(0, 1);
    uint2 coordC4 = coord - uint2(0, 1);
    float3 r2 = colour;
    float3 c3 = g_colour[ldsCoord.x][ldsCoord.y + 1];
    float3 c4 = g_colour[ldsCoord.x][ldsCoord.y - 1];
#if DEBLOCK_SHARPEN_FILTER == 1
    float diffC3 = TestEquality(colour, c3);
    float diffC4 = TestEquality(colour, c4);
    bool deblockedV = Deblock(linearDepthSRV, r2, c3, c4, coord, coordC3, coordC4, diffC3, diffC4);
#else
    bool deblockedV = Deblock(linearDepthSRV, r2, c3, c4, coord, coordC3, coordC4);
#endif
    if (!(deblockedH || deblockedV))
        return colour;

    colour = 0.5 * (r1 + r2);

    // This system of variable weights per sample and excluding samples is a great help in increasing quality and reducing 'ringing', though obviously
    // comes at some expense. Might be interesting to add in diagonals to this computation, however we would need to populate 4x corners of LDS memory
    // (likely very cheap) however the cost of sharpen operation would obviously go up with 4x more LDS loads and all the associated weights
#if DEBLOCK_SHARPEN_FILTER == 1
    float3 sum = float3(0.0, 0.0, 0.0);
    float weight = 0.0;

    SharpenSample(diffC1, c1, sum, weight);
    SharpenSample(diffC2, c2, sum, weight);
    SharpenSample(diffC3, c3, sum, weight);
    SharpenSample(diffC4, c4, sum, weight);

    colour *= 1.0 + SHARPEN_STRENGTH;
    colour -= SHARPEN_STRENGTH * (sum / weight);
#endif
    return colour;
}


float3 Deblock1616(Texture2D<float3> inputColourSRV, Texture2D<float> linearDepthSRV, uint2 Gid, uint2 GTid, uint2 DTid, uint groupThreadIndex)
{
    // load colour to inner 16x16 of groupshared
    LoadToLDS(inputColourSRV, GTid, DTid);

    // rather than clamping the reads to the render target, but always performing a read. I tried reading back from LDS
    // on out of bounds read with a [branch], which worked fine but required an extra group sync after the initial read
    // to LDS, obviously to make sure the inner 16x16 of LDS was actually fully populated before processing the border
    // pixels. Doing reads with clamped bounds was 3us slower than copy LDS, until I removed the group sync, then it
    // became 5us faster
    //GroupMemoryBarrierWithGroupSync();

    uint2 tileXY = Gid * 16;

    // The theory here is SV_GroupThreadID is distributed in such a way only one wave enters each function
    // which means that the following 4x function calls are run in parallel by different waves
    // (unfortunately, no SV_GroupWaveIndex which would be ideal!!!)
#ifdef __XBOX_ENABLE_WAVE32
    // letting the first 4 waves32 in to process the borders instead of waves 0,2,4,6 (wave32's when running the wave64 case) saved 10us
    // this is because the last 4 waves dispatches of the 8 total don't get any borders to process, so they get to the sync early
    // The 10us is the time between wave3 completing a border and getting to the sync, vs wave64 completing a border and getting to the sync
    if (groupThreadIndex < 16)                                          LoadToLDSBorderL(inputColourSRV, groupThreadIndex, tileXY);
    if ((groupThreadIndex >= 32) && (groupThreadIndex < 32 + 16))       LoadToLDSBorderR(inputColourSRV, groupThreadIndex & 15, tileXY);
    if ((groupThreadIndex >= 64) && (groupThreadIndex < 64 + 16))       LoadToLDSBorderT(inputColourSRV, groupThreadIndex & 15, tileXY);
    if ((groupThreadIndex >= 96) && (groupThreadIndex < 96 + 16))       LoadToLDSBorderB(inputColourSRV, groupThreadIndex & 15, tileXY);
#else
    if (groupThreadIndex < 16)                                          LoadToLDSBorderL(inputColourSRV, groupThreadIndex, tileXY);
    if ((groupThreadIndex >=  64) && (groupThreadIndex <  64 + 16))     LoadToLDSBorderR(inputColourSRV, groupThreadIndex & 15, tileXY);
    if ((groupThreadIndex >= 128) && (groupThreadIndex < 128 + 16))     LoadToLDSBorderT(inputColourSRV, groupThreadIndex & 15, tileXY);
    if ((groupThreadIndex >= 196) && (groupThreadIndex < 196 + 16))     LoadToLDSBorderB(inputColourSRV, groupThreadIndex & 15, tileXY);
#endif
    GroupMemoryBarrierWithGroupSync();

    return Deblock(linearDepthSRV, DTid, GTid);
}
