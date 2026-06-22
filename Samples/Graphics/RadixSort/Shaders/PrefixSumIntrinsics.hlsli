//--------------------------------------------------------------------------------------
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#ifndef PREFIX_SUM_INTRINSICS
#define PREFIX_SUM_INTRINSICS

#ifndef WAVE_SIZE
    #error WAVE_SIZE must be defined
#endif

uint PrefixSum_Inclusive(uint x)
{
    uint WavePrefix = x;

    uint Tmp = 0;
    Tmp = __XB_LaneSwizzle(WavePrefix, 0x001e);

    // Explicitly activate every odd lane
    if (__XB_V_CNDMASK_B32(0xaaaaaaaa, uint2(0, 1))) WavePrefix += Tmp;

    Tmp = __XB_LaneSwizzle(WavePrefix, 0x003c);

    // Explicitly activate every odd group of 2 lanes
    if (__XB_V_CNDMASK_B32(0xcccccccc, uint2(0, 1))) WavePrefix += Tmp;

    Tmp = __XB_LaneSwizzle(WavePrefix, 0x0078);

    // Explicitly activate every odd group of 4 lanes
    if (__XB_V_CNDMASK_B32(0xf0f0f0f0, uint2(0, 1))) WavePrefix += Tmp;

    Tmp = __XB_LaneSwizzle(WavePrefix, 0x00f0);

    // Explicitly activate every odd group of 8 lanes
    if (__XB_V_CNDMASK_B32(0xff00ff00, uint2(0, 1))) WavePrefix += Tmp;

    Tmp = __XB_LaneSwizzle(WavePrefix, 0x01e0);

    // Explicitly activate every odd group of 16 lanes
    if (__XB_V_CNDMASK_B32(0xffff0000, uint2(0, 1))) WavePrefix += Tmp;

#if WAVE_SIZE == 32
    // Do nothing
#elif WAVE_SIZE == 64
    // Swizzle is done only within 32 lanes, so read prefix sum of first 31 lanes
    Tmp = __XB_ReadLane(WavePrefix, 31);

    // Explicitly activate upper group of 32 lanes
    if (__XB_V_CNDMASK_B32(uint2(0, 0xffffffff), uint2(0, 1))) WavePrefix += Tmp;
#else
    #error WAVE_SIZE must be 32 or 64
#endif
    return WavePrefix;
}

#endif // PREFIX_SUM_INTRINSICS
