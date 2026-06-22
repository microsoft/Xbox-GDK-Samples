//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

// square pattern is more expensive than rect
#ifndef CSQUADCOORD_SQUARE
# define CSQUADCOORD_SQUARE                                 0
#endif

// XB1 got support for QuadReadAcross in October 2022 GDK, but it only equates to this anyway
// On Scarlett QuadReadAcross uses DPP rather than ds_swizzle, but I actually found this to be 1%s slower (timed over a long average)
#if defined __XBOX_ONE || defined __XBOX_SCARLETT
# define USE_LANE_SWIZZLE                                   1
#endif

#if USE_LANE_SWIZZLE != 0
// only legal values for either parameter are 0,1,2,3
# define __XB_CROSSBAR4_READ_THREAD(dst, src)                ((src) << ((dst) << 1))
// bit 15 enables QDMode rather than bit mode
# define __XB_CROSSBAR4_SET_READS(src0, src1, src2, src3)    (0x8000 | __XB_CROSSBAR4_READ_THREAD(0, src0) | __XB_CROSSBAR4_READ_THREAD(1, src1) | __XB_CROSSBAR4_READ_THREAD(2, src2) | __XB_CROSSBAR4_READ_THREAD(3, src3))

# define FastReadAcrossX(A)						            __XB_LaneSwizzle(A, __XB_CROSSBAR4_SET_READS(1, 0, 3, 2))
# define FastReadAcrossY(A)						            __XB_LaneSwizzle(A, __XB_CROSSBAR4_SET_READS(2, 3, 0, 1))
# define FastReadAcrossD(A)						            __XB_LaneSwizzle(A, __XB_CROSSBAR4_SET_READS(3, 2, 1, 0))
#else
# define FastReadAcrossX(A)						            QuadReadAcrossX(A)
# define FastReadAcrossY(A)						            QuadReadAcrossY(A)
# define FastReadAcrossD(A)						            QuadReadAcrossDiagonal(A)
#endif

// for using the 4x lane crossbar, we need to change the thread ordering so 4 threads in linear order
// cover a 2x2 pixel region this means constructing 4x4 quads of 2x2 pixels each
uint2 GetCSQuadCoord_Wave64_88(uint threadIndex)
{
    uint2 threadId;

    // could load from buffer instead, depending on what shader bottlenecks are
#if CSQUADCOORD_SQUARE != 0
    /* Sightly more expensive square version produces:

     0  1  4  5 16 17 20 21
     2  3  6  7 18 19 22 23
     8  9 12 13 24 25 28 29
    10 11 14 15 26 27 30 31
    32 33 36 37 48 49 52 53
    34 35 38 39 50 51 54 55
    40 41 44 45 56 57 60 61
    42 43 46 47 58 59 62 63

    // 3 periods of tiling, non optimised for clarity:
    x = ( index & 1      ) + ((index & 4) >> 1) + ((index & 16) >> 2);
    y = ((index & 2) >> 1) + ((index & 8) >> 2) + ((index & 32) >> 3);
    */

    // duplicate index in top 16 bits, but pre-shifted right by 1
    threadIndex |= threadIndex << 15;

    // two bitwise ANDS for the price of one
    threadId.x = (threadIndex) & 0x10001;
    threadId.x |= (threadIndex >> 1) & 0x20002;
    threadId.x |= (threadIndex >> 2) & 0x40004;

    threadId.y = threadId.x >> 16;
    threadId.x &= 0x7;
#else
    /* Cheap rectangular version produces:

     0  1  4  5  8  9 12 13
     2  3  6  7 10 11 14 15
    16 17 20 21 24 25 28 29
    18 19 22 23 26 27 30 31
    32 33 36 37 40 41 44 45
    34 35 38 39 42 43 46 47
    48 49 52 53 56 57 60 61
    50 51 54 55 58 59 62 63

    // 2 periods of tiling, non optimised for clarity:
    x = (index & 1) + ((index & 12) >> 1);
    y = ((index & 2) >> 1) + ((index & 48) >> 3);

    */
    uint indexSHR1 = threadIndex >> 1;
    threadId.x = (threadIndex & 1) + (indexSHR1 & 6);
    threadId.y = (indexSHR1 & 1) + ((threadIndex & 48) >> 3);
#endif
    return threadId;
}


uint2 GetCSQuadCoord_Wave32_84(uint threadIndex)
{
    return GetCSQuadCoord_Wave64_88(threadIndex);
}


uint2 GetCSQuadCoord_Wave64_88()
{
    return GetCSQuadCoord_Wave64_88(WaveGetLaneIndex());
}


uint2 GetCSQuadCoord_Wave32_84()
{
    return GetCSQuadCoord_Wave32_84(WaveGetLaneIndex());
}
