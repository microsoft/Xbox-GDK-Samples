//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#include "SSAORS.hlsli"
#include "Constants.h"

#if USE_FP_16
#define FLOAT_TYPE float16_t
#define FLOAT2_TYPE float16_t2
#define FLOAT4_TYPE float16_t4
#else
#define FLOAT_TYPE float
#define FLOAT2_TYPE float2
#define FLOAT4_TYPE float4
#endif

#if __XBOX_ONE
#define __XBOX_REGALLOC_VGPR_LIMIT 32
#endif

#ifndef INTERLEAVE_RESULT
#define WIDE_SAMPLING 1
#endif

#ifdef INTERLEAVE_RESULT
Texture2DArray<FLOAT_TYPE> DepthTex : register(t0);
#else
Texture2D<FLOAT_TYPE> DepthTex : register(t0);
#endif
RWTexture2D<FLOAT_TYPE> Occlusion : register(u0);
SamplerState LinearBorderSampler : register(s1);
ConstantBuffer<SSOARenderConstants> CB1 : register(b1);

#if WIDE_SAMPLING
// 32x32 cache size:  the 16x16 in the center forms the area of focus with the 8-pixel perimeter used for wide gathering.
#define TILE_DIM 32
#define THREAD_COUNT_X 16
#define THREAD_COUNT_Y 16
#else
// 16x16 cache size:  the 8x8 in the center forms the area of focus with the 4-pixel perimeter used for gathering.
#define TILE_DIM 16
#define THREAD_COUNT_X 8
#define THREAD_COUNT_Y 8
#endif

groupshared FLOAT_TYPE DepthSamples[TILE_DIM * TILE_DIM];


#if USE_FP_16
float16_t Clamp(float16_t a, float16_t b, float16_t c)
{
    return __XB_Med3_F16(a, b, c);
}
#else
float Clamp(float a, float b, float c)
{
    return __XB_Med3_F32(a, b, c);
}
#endif

FLOAT2_TYPE TestSamplePair2(FLOAT_TYPE frontDepth, FLOAT_TYPE invRange, uint base, int offset1, int offset2)
{
    // "Disocclusion" measures the penetration distance of the depth sample within the sphere.
    // Disocclusion < 0 (full occlusion) -> the sample fell in front of the sphere
    // Disocclusion > 1 (no occlusion) -> the sample fell behind the sphere
    FLOAT4_TYPE depthSample = FLOAT4_TYPE(DepthSamples[base + offset1], DepthSamples[base + offset2], DepthSamples[base - offset1], DepthSamples[base - offset2]);
    FLOAT4_TYPE disocclusion = depthSample * invRange - frontDepth;

    FLOAT4_TYPE pseudoDisocclusion = saturate(FLOAT_TYPE(CB1.gRejectFadeoff) * disocclusion);

    return
        FLOAT2_TYPE(Clamp(disocclusion.x, pseudoDisocclusion.z, 1.0), Clamp(disocclusion.y, pseudoDisocclusion.w, 1.0)) +
        FLOAT2_TYPE(Clamp(disocclusion.z, pseudoDisocclusion.x, 1.0), Clamp(disocclusion.w, pseudoDisocclusion.y, 1.0)) -
        pseudoDisocclusion.xy * pseudoDisocclusion.zw;
}

#ifdef SAMPLE_EXHAUSTIVELY
FLOAT_TYPE TestSamples(uint centerIdx, uint x, uint y, FLOAT_TYPE invDepth, FLOAT_TYPE invThickness)
{
#if WIDE_SAMPLING
    x <<= 1;
    y <<= 1;
#endif

    FLOAT_TYPE invRange = invThickness * invDepth;
    FLOAT_TYPE frontDepth = invThickness - 0.5;

    if (y == 0)
    {
        // Axial
        FLOAT2_TYPE res = TestSamplePair2(frontDepth, invRange, centerIdx, x, x * TILE_DIM);
        return 0.5 * (res.x + res.y);
    }
    else if (x == y)
    {
        // Diagonal
        FLOAT2_TYPE res = TestSamplePair2(frontDepth, invRange, centerIdx, x * TILE_DIM - x, x * TILE_DIM + x);
        return 0.5 * (res.x + res.y);
    }
    else
    {
        // L-Shaped
        FLOAT2_TYPE res1 = TestSamplePair2(frontDepth, invRange, centerIdx, y * TILE_DIM + x, y * TILE_DIM - x);
        FLOAT2_TYPE res2 = TestSamplePair2(frontDepth, invRange, centerIdx, x * TILE_DIM + y, x * TILE_DIM - y);
        res1 += res2;
        return 0.25 * (res1.x + res1.y);
    }
}
#endif

FLOAT_TYPE TestSamplesAxial(uint centerIdx, uint x, uint y, FLOAT_TYPE invRange, FLOAT_TYPE frontDepth)
{
#if WIDE_SAMPLING
    x <<= 1;
    y <<= 1;
#endif

        // Axial
        FLOAT2_TYPE res = TestSamplePair2(frontDepth, invRange, centerIdx, x, x * TILE_DIM);
        return res.x + res.y;
}

FLOAT_TYPE TestSamplesDiagonal(uint centerIdx, uint x, uint y, FLOAT_TYPE invRange, FLOAT_TYPE frontDepth)
{
#if WIDE_SAMPLING
    x <<= 1;
    y <<= 1;
#endif

    // Diagonal
    FLOAT2_TYPE res = TestSamplePair2(frontDepth, invRange, centerIdx, x * TILE_DIM - x, x * TILE_DIM + x);
    return res.x + res.y;
}

FLOAT_TYPE TestSamplesL(uint centerIdx, uint x, uint y, FLOAT_TYPE invRange, FLOAT_TYPE frontDepth)
{
#if WIDE_SAMPLING
    x <<= 1;
    y <<= 1;
#endif

        // L-Shaped
        FLOAT2_TYPE res1 = TestSamplePair2(frontDepth, invRange, centerIdx, y * TILE_DIM + x, y * TILE_DIM - x);
        FLOAT2_TYPE res2 = TestSamplePair2(frontDepth, invRange, centerIdx, x * TILE_DIM + y, x * TILE_DIM - y);
        res1 += res2;
        return res1.x + res1.y;
}

[RootSignature(SSAO_RootSig)]
#if WIDE_SAMPLING
[numthreads(16, 16, 1)]
#else
[numthreads(8, 8, 1)]
#endif
void main(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
#if WIDE_SAMPLING
    float2 QuadCenterUV = int2(DTid.xy + GTid.xy - 7) * CB1.gInvSliceDimension;
#else
    float2 QuadCenterUV = int2(DTid.xy + GTid.xy - 3) * CB1.gInvSliceDimension;
#endif

    // Fetch four depths and store them in LDS
#ifdef INTERLEAVE_RESULT
    FLOAT4_TYPE depths = DepthTex.Gather(LinearBorderSampler, float3(QuadCenterUV, DTid.z));
#else
    FLOAT4_TYPE depths = DepthTex.Gather(LinearBorderSampler, QuadCenterUV);
#endif

    int destIdx = GTid.x * 2 + GTid.y * 2 * TILE_DIM;
    DepthSamples[destIdx] = depths.w;
    DepthSamples[destIdx + 1] = depths.z;
    DepthSamples[destIdx + TILE_DIM] = depths.x;
    DepthSamples[destIdx + TILE_DIM + 1] = depths.y;

    GroupMemoryBarrierWithGroupSync();

#if WIDE_SAMPLING
    uint thisIdx = GTid.x + GTid.y * TILE_DIM + 8 * TILE_DIM + 8;
#else
    uint thisIdx = GTid.x + GTid.y * TILE_DIM + 4 * TILE_DIM + 4;
#endif

    const FLOAT_TYPE invThisDepth = 1.0 / DepthSamples[thisIdx];

    FLOAT_TYPE ao = 0.0;

#ifdef SAMPLE_EXHAUSTIVELY
    // 68 samples:  sample all cells in *within* a circular radius of 5
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[0].x * TestSamples(thisIdx, 1, 0, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[0].x)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[0].y * TestSamples(thisIdx, 2, 0, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[0].y)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[0].z * TestSamples(thisIdx, 3, 0, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[0].z)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[0].w * TestSamples(thisIdx, 4, 0, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[0].w)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[1].x * TestSamples(thisIdx, 1, 1, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[1].x)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[2].x * TestSamples(thisIdx, 2, 2, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[2].x)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[2].w * TestSamples(thisIdx, 3, 3, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[2].w)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[1].y * TestSamples(thisIdx, 1, 2, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[1].y)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[1].z * TestSamples(thisIdx, 1, 3, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[1].z)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[1].w * TestSamples(thisIdx, 1, 4, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[1].w)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[2].y * TestSamples(thisIdx, 2, 3, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[2].y)));
    ao = (FLOAT_TYPE)(ao + CB1.gSampleWeightTable[2].z * TestSamples(thisIdx, 2, 4, invThisDepth, FLOAT_TYPE(CB1.gInvThicknessTable[2].z)));
#else // SAMPLE_CHECKER
    // 36 samples:  sample every-other cell in a checker board pattern
    FLOAT2_TYPE constants = FLOAT2_TYPE(CB1.gInvThicknessTable[0].y, CB1.gInvThicknessTable[0].w);
    FLOAT2_TYPE invRangeP = constants * invThisDepth;
    FLOAT2_TYPE frontDepthP = constants - 0.5;

    FLOAT2_TYPE repack = 0.5 * FLOAT2_TYPE(TestSamplesAxial(thisIdx, 2, 0, invRangeP.x, frontDepthP.x), TestSamplesAxial(thisIdx, 4, 0, invRangeP.y, frontDepthP.y));
    ao = (FLOAT_TYPE)(ao + (CB1.gSampleWeightTable[0].y * repack.x));
    ao = (FLOAT_TYPE)(ao + (CB1.gSampleWeightTable[0].w * repack.y));

    constants = FLOAT2_TYPE(CB1.gInvThicknessTable[1].x, CB1.gInvThicknessTable[2].x);
    invRangeP = constants * invThisDepth;
    frontDepthP = constants - 0.5;
    repack = 0.5 * FLOAT2_TYPE(TestSamplesDiagonal(thisIdx, 1, 1, invRangeP.x, frontDepthP.x), TestSamplesDiagonal(thisIdx, 2, 2, invRangeP.y, frontDepthP.y));
    ao = (FLOAT_TYPE)(ao + (CB1.gSampleWeightTable[1].x * repack.x));
    ao = (FLOAT_TYPE)(ao + (CB1.gSampleWeightTable[2].x * repack.y));

    constants = FLOAT2_TYPE(CB1.gInvThicknessTable[1].z, CB1.gInvThicknessTable[2].z);
    invRangeP = constants * invThisDepth;
    frontDepthP = constants - 0.5;
    repack = 0.25 * FLOAT2_TYPE(TestSamplesL(thisIdx, 1, 3, invRangeP.x, frontDepthP.x), TestSamplesL(thisIdx, 2, 4, invRangeP.y, frontDepthP.y));
    ao = (FLOAT_TYPE)(ao + (CB1.gSampleWeightTable[1].z * repack.x));
    ao = (FLOAT_TYPE)(ao + (CB1.gSampleWeightTable[2].z * repack.y));

    FLOAT_TYPE constant = FLOAT_TYPE(CB1.gInvThicknessTable[2].w);
    FLOAT_TYPE invRange = constant * invThisDepth;
    FLOAT_TYPE frontDepth = constant - 0.5;
    ao = (FLOAT_TYPE)(ao + (CB1.gSampleWeightTable[2].w * 0.5 * TestSamplesDiagonal(thisIdx, 3, 3, invRange, frontDepth)));
#endif

#ifdef INTERLEAVE_RESULT
    uint2 OutPixel = DTid.xy << 2 | uint2(DTid.z & 3, DTid.z >> 2);
#else
    uint2 OutPixel = DTid.xy;
#endif
    Occlusion[OutPixel] = (FLOAT_TYPE)(ao * CB1.gRcpAccentuation);
}
