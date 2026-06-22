//--------------------------------------------------------------------------------------
// EQAAConstants.h
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

using namespace DirectX;

#if !ENABLE_EQAA
#error Must have EQAA_ENABLED defined in order to access EQAASamplePositions
#endif

// These must be kept in sync with the numbers in FMask.hlsl
#define MAX_NUM_FRAGMENTS 8
#define MAX_LOG_NUM_FRAGMENTS 3
#define MAX_NUM_SAMPLES 16 
#define MAX_LOG_NUM_SAMPLES 4 
#define MAX_QUALITY MAX_LOG_NUM_SAMPLES 

// This is to offset 1f EQAA samples so that the only anchor is at the pixel centre
#define MSAASHIFT(v,s) (((v) + 8 + 16 + (s)) % 16 - 8)

// Now that the shader compiler has been modified to (correctly) not expose EQAA sample positions,
// we must hard-code them. If the driver (or the title) changes these selections, then this array
// must be manually updated.
typedef XMVECTORI32 _int2;   // HLSL int2 array will be padded to int4's
typedef _int2 EQAASamplePositions[MAX_LOG_NUM_FRAGMENTS + 1][MAX_QUALITY + 1][1 << MAX_LOG_NUM_SAMPLES];
struct EQAASamplePositionsStruct
{
    EQAASamplePositions SamplePositions;
};
__declspec(selectany) EQAASamplePositionsStruct g_EQAASamplePositions =
{
    {
        // LOG_NUM_FRAGMENTS = 0, NUM_FRAGMENTS = 1
        {
            // LOG_NUM_SAMPLES = 0, NUM_SAMPLES = 1, QUALITY = 0
            {
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
            },
            // LOG_NUM_SAMPLES = 1, NUM_SAMPLES = 2, QUALITY = 1
            {
                { MSAASHIFT(4,-4),MSAASHIFT(4,-4) },
                { MSAASHIFT(-4,-4),MSAASHIFT(-4,-4) },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 },
                { 0, 0 }
            },
    // LOG_NUM_SAMPLES = 2, NUM_SAMPLES = 4, QUALITY = 2
    {
        { MSAASHIFT(-6,6),MSAASHIFT(-6,6) },
        { MSAASHIFT(6,6),MSAASHIFT(6,6) },
        { MSAASHIFT(-2,6),MSAASHIFT(2,6) },
        { MSAASHIFT(2,6),MSAASHIFT(-2,6) },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 3, NUM_SAMPLES = 8, QUALITY = 3
    {
        { MSAASHIFT(7,-7),MSAASHIFT(6,-6) },
        { MSAASHIFT(-7,-7),MSAASHIFT(-8,-6) },
        { MSAASHIFT(-5,-7),MSAASHIFT(5,-6) },
        { MSAASHIFT(1,-7),MSAASHIFT(-5,-6) },
        { MSAASHIFT(3,-7),MSAASHIFT(7,-6) },
        { MSAASHIFT(-3,-7),MSAASHIFT(-7,-6) },
        { MSAASHIFT(-1,-7),MSAASHIFT(1,-6) },
        { MSAASHIFT(5,-7),MSAASHIFT(-1,-6) },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 4, NUM_SAMPLES = 16, QUALITY = 4
    {
        { MSAASHIFT(7, -7),MSAASHIFT(6,-6) },
        { MSAASHIFT(-7, -7),MSAASHIFT(-8,-6) },
        { MSAASHIFT(-5, -7),MSAASHIFT(5,-6) },
        { MSAASHIFT(1, -7),MSAASHIFT(-5,-6) },
        { MSAASHIFT(3, -7),MSAASHIFT(7,-6) },
        { MSAASHIFT(-3, -7),MSAASHIFT(-7,-6) },
        { MSAASHIFT(-1, -7),MSAASHIFT(1,-6) },
        { MSAASHIFT(5, -7),MSAASHIFT(-1,-6) },
        { MSAASHIFT(4, -7),MSAASHIFT(2,-6) },
        { MSAASHIFT(-8, -7),MSAASHIFT(-6,-6) },
        { MSAASHIFT(-2, -7),MSAASHIFT(3,-6) },
        { MSAASHIFT(2, -7),MSAASHIFT(-3,-6) },
        { MSAASHIFT(0, -7),MSAASHIFT(4,-6) },
        { MSAASHIFT(-4, -7),MSAASHIFT(-2,-6) },
        { MSAASHIFT(-6, -7),MSAASHIFT(0,-6) },
        { MSAASHIFT(6, -7),MSAASHIFT(-4,-6) }
    },
},

// LOG_NUM_FRAGMENTS = 1, NUM_FRAGMENTS = 2
{
    // LOG_NUM_SAMPLES = 1, NUM_SAMPLES = 2, QUALITY = 0
    {
        { 4, 4 },
        { -4,-4 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 1, NUM_SAMPLES = 2, QUALITY = 1
    {
        { 4, 4 },
        { -4,-4 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 2, NUM_SAMPLES = 4, QUALITY = 2
    {
        { -6,-6 },
        { 6, 6 },
        { -2, 2 },
        { 2,-2 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 3, NUM_SAMPLES = 8, QUALITY = 3
    {
        { 7, 6 },
        { -7,-8 },
        { -5, 5 },
        { 1,-5 },
        { 3, 7 },
        { -3,-7 },
        { -1, 1 },
        { 5,-1 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 4, NUM_SAMPLES = 16, QUALITY = 4
    {
        { 7, 6 },
        { -7,-8 },
        { -5, 5 },
        { 1,-5 },
        { 3, 7 },
        { -3,-7 },
        { -1, 1 },
        { 5,-1 },
        { 4, 2 },
        { -8,-6 },
        { -2, 3 },
        { 2,-3 },
        { 0, 4 },
        { -4,-2 },
        { -6, 0 },
        { 6,-4 }
    },
},

// LOG_NUM_FRAGMENTS = 2, NUM_FRAGMENTS = 4
{
    // LOG_NUM_SAMPLES = 2, NUM_SAMPLES = 4, QUALITY = 0
    {
        { -2,-6 },
        { 6,-2 },
        { -6, 2 },
        { 2, 6 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 2, NUM_SAMPLES = 4, QUALITY = 1
    {
        { -6,-6 },
        { 6, 6 },
        { -2, 2 },
        { 2,-2 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 2, NUM_SAMPLES = 4, QUALITY = 2
    {
        { -6,-6 },
        { 6, 6 },
        { -2, 2 },
        { 2,-2 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 3, NUM_SAMPLES = 8, QUALITY = 3
    {
        { 7, 6 },
        { -7,-8 },
        { -5, 5 },
        { 1,-5 },
        { 3, 7 },
        { -3,-7 },
        { -1, 1 },
        { 5,-1 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 4, NUM_SAMPLES = 16, QUALITY = 4
    {
        { 7, 6 },
        { -7,-8 },
        { -5, 5 },
        { 1,-5 },
        { 3, 7 },
        { -3,-7 },
        { -1, 1 },
        { 5,-1 },
        { 4, 2 },
        { -8,-6 },
        { -2, 3 },
        { 2,-3 },
        { 0, 4 },
        { -4,-2 },
        { -6, 0 },
        { 6,-4 }
    },
},

// LOG_NUM_FRAGMENTS = 3, NUM_FRAGMENTS = 8
{
    // LOG_NUM_SAMPLES = 3, NUM_SAMPLES = 8, QUALITY = 0
    {
        { 1,-3 },
        { -1, 3 },
        { 5, 1 },
        { -3,-5 },
        { -5, 5 },
        { -7,-1 },
        { 3, 7 },
        { 7,-7 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 3, NUM_SAMPLES = 8, QUALITY = 1
    {
        { 7, 6 },
        { -7,-8 },
        { -5, 5 },
        { 1,-5 },
        { 3, 7 },
        { -3,-7 },
        { -1, 1 },
        { 5,-1 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 3, NUM_SAMPLES = 8, QUALITY = 2
    {
        { 7, 6 },
        { -7,-8 },
        { -5, 5 },
        { 1,-5 },
        { 3, 7 },
        { -3,-7 },
        { -1, 1 },
        { 5,-1 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 3, NUM_SAMPLES = 8, QUALITY = 3
    {
        { 7, 6 },
        { -7,-8 },
        { -5, 5 },
        { 1,-5 },
        { 3, 7 },
        { -3,-7 },
        { -1, 1 },
        { 5,-1 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 },
        { 0, 0 }
    },
    // LOG_NUM_SAMPLES = 4, NUM_SAMPLES = 16, QUALITY = 4
    {
        { 7, 6 },
        { -7,-8 },
        { -5, 5 },
        { 1,-5 },
        { 3, 7 },
        { -3,-7 },
        { -1, 1 },
        { 5,-1 },
        { 4, 2 },
        { -8,-6 },
        { -2, 3 },
        { 2,-3 },
        { 0, 4 },
        { -4,-2 },
        { -6, 0 },
        { 6,-4 }
    },
},
},
};

