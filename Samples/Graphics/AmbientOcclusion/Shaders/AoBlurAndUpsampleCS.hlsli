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

#define __XBOX_DISABLE_UAV_WRITE_TO_UNALLOCATED_CHANNELS 1

#ifdef USE_FP_16
#define FLOAT_TYPE float16_t
#define FLOAT2_TYPE float16_t2
#define FLOAT4_TYPE float16_t4
#else
#define FLOAT_TYPE float
#define FLOAT2_TYPE float2
#define FLOAT4_TYPE float4
#endif

Texture2D<FLOAT_TYPE> LoResDB : register(t0);
Texture2D<FLOAT_TYPE> HiResDB : register(t1);
Texture2D<FLOAT_TYPE> LoResAO1 : register(t2);
#ifdef COMBINE_LOWER_RESOLUTIONS
Texture2D<FLOAT_TYPE> LoResAO2 : register(t3);
#endif
#ifdef BLEND_WITH_HIGHER_RESOLUTION
Texture2D<FLOAT_TYPE> HiResAO : register(t4);
#endif

RWTexture2D<FLOAT_TYPE> AoResult : register(u0);

SamplerState LinearSampler : register(s0);

ConstantBuffer<BlurAndUpsampleConstants> CB1 : register(b1);

groupshared FLOAT_TYPE DepthCache[196];
groupshared FLOAT_TYPE AOCache1[196];
groupshared FLOAT_TYPE AOCache2[196];

void PrefetchData(uint index, float2 uv)
{
    FLOAT4_TYPE AO1 = LoResAO1.Gather(LinearSampler, uv);

#ifdef COMBINE_LOWER_RESOLUTIONS
    AO1 = min(AO1, LoResAO2.Gather(LinearSampler, uv));
#endif

    AOCache1[index] = AO1.w;
    AOCache1[index + 1] = AO1.z;
    AOCache1[index + 14] = AO1.x;
    AOCache1[index + 15] = AO1.y;

    FLOAT4_TYPE ID = 1.0 / LoResDB.Gather(LinearSampler, uv);
    DepthCache[index] = ID.w;
    DepthCache[index + 1] = ID.z;
    DepthCache[index + 14] = ID.x;
    DepthCache[index + 15] = ID.y;
}

FLOAT_TYPE SmartBlur(FLOAT_TYPE a, FLOAT_TYPE b, FLOAT_TYPE c, FLOAT_TYPE d, FLOAT_TYPE e, bool Left, bool Middle, bool Right)
{
    b = Left | Middle ? b : c;
    a = Left ? a : b;
    d = Right | Middle ? d : c;
    e = Right ? e : d;
    return ((a + e) * 0.5 + b + c + d) * 0.25;
}

bool CompareDeltas(FLOAT_TYPE d1, FLOAT_TYPE d2, FLOAT_TYPE l1, FLOAT_TYPE l2)
{
    float temp = d1 * d2 + CB1.StepSize;
    return temp * temp > l1 * l2 * CB1.kBlurTolerance;
}

void BlurHorizontally(uint leftMostIndex)
{
    FLOAT_TYPE a0 = AOCache1[leftMostIndex];
    FLOAT_TYPE a1 = AOCache1[leftMostIndex + 1];
    FLOAT_TYPE a2 = AOCache1[leftMostIndex + 2];
    FLOAT_TYPE a3 = AOCache1[leftMostIndex + 3];
    FLOAT_TYPE a4 = AOCache1[leftMostIndex + 4];
    FLOAT_TYPE a5 = AOCache1[leftMostIndex + 5];
    FLOAT_TYPE a6 = AOCache1[leftMostIndex + 6];
#ifdef BLUR_5_PER_THREAD
    FLOAT_TYPE a7 = AOCache1[leftMostIndex + 7];
    FLOAT_TYPE a8 = AOCache1[leftMostIndex + 8];
#endif

    FLOAT_TYPE d0 = DepthCache[leftMostIndex];
    FLOAT_TYPE d1 = DepthCache[leftMostIndex + 1];
    FLOAT_TYPE d2 = DepthCache[leftMostIndex + 2];
    FLOAT_TYPE d3 = DepthCache[leftMostIndex + 3];
    FLOAT_TYPE d4 = DepthCache[leftMostIndex + 4];
    FLOAT_TYPE d5 = DepthCache[leftMostIndex + 5];
    FLOAT_TYPE d6 = DepthCache[leftMostIndex + 6];
#ifdef BLUR_5_PER_THREAD
    FLOAT_TYPE d7 = DepthCache[leftMostIndex + 7];
    FLOAT_TYPE d8 = DepthCache[leftMostIndex + 8];
#endif

    FLOAT_TYPE d01 = d1 - d0;
    FLOAT_TYPE d12 = d2 - d1;
    FLOAT_TYPE d23 = d3 - d2;
    FLOAT_TYPE d34 = d4 - d3;
    FLOAT_TYPE d45 = d5 - d4;
    FLOAT_TYPE d56 = d6 - d5;
#ifdef BLUR_5_PER_THREAD
    FLOAT_TYPE d67 = d7 - d6;
    FLOAT_TYPE d78 = d8 - d7;
#endif

    FLOAT_TYPE l01 = FLOAT_TYPE(d01 * d01 + CB1.StepSize);
    FLOAT_TYPE l12 = FLOAT_TYPE(d12 * d12 + CB1.StepSize);
    FLOAT_TYPE l23 = FLOAT_TYPE(d23 * d23 + CB1.StepSize);
    FLOAT_TYPE l34 = FLOAT_TYPE(d34 * d34 + CB1.StepSize);
    FLOAT_TYPE l45 = FLOAT_TYPE(d45 * d45 + CB1.StepSize);
    FLOAT_TYPE l56 = FLOAT_TYPE(d56 * d56 + CB1.StepSize);
#ifdef BLUR_5_PER_THREAD
    FLOAT_TYPE l67 = FLOAT_TYPE(d67 * d67 + CB1.StepSize);
    FLOAT_TYPE l78 = FLOAT_TYPE(d78 * d78 + CB1.StepSize);
#endif

    bool c02 = CompareDeltas(d01, d12, l01, l12);
    bool c13 = CompareDeltas(d12, d23, l12, l23);
    bool c24 = CompareDeltas(d23, d34, l23, l34);
    bool c35 = CompareDeltas(d34, d45, l34, l45);
    bool c46 = CompareDeltas(d45, d56, l45, l56);
#ifdef BLUR_5_PER_THREAD
    bool c57 = CompareDeltas(d56, d67, l56, l67);
    bool c68 = CompareDeltas(d67, d78, l67, l78);
#endif

    AOCache2[leftMostIndex] = SmartBlur(a0, a1, a2, a3, a4, c02, c13, c24);
    AOCache2[leftMostIndex + 1] = SmartBlur(a1, a2, a3, a4, a5, c13, c24, c35);
    AOCache2[leftMostIndex + 2] = SmartBlur(a2, a3, a4, a5, a6, c24, c35, c46);
#ifdef BLUR_5_PER_THREAD
    AOCache2[leftMostIndex + 3] = SmartBlur(a3, a4, a5, a6, a7, c35, c46, c57);
    AOCache2[leftMostIndex + 4] = SmartBlur(a4, a5, a6, a7, a8, c46, c57, c68);
#endif
}

void BlurVertically(uint topMostIndex)
{
    FLOAT_TYPE a0 = AOCache2[topMostIndex];
    FLOAT_TYPE a1 = AOCache2[topMostIndex + 14];
    FLOAT_TYPE a2 = AOCache2[topMostIndex + 28];
    FLOAT_TYPE a3 = AOCache2[topMostIndex + 42];
    FLOAT_TYPE a4 = AOCache2[topMostIndex + 56];
    FLOAT_TYPE a5 = AOCache2[topMostIndex + 70];
    FLOAT_TYPE a6 = AOCache2[topMostIndex + 84];

    FLOAT_TYPE d0 = DepthCache[topMostIndex + 2];
    FLOAT_TYPE d1 = DepthCache[topMostIndex + 16];
    FLOAT_TYPE d2 = DepthCache[topMostIndex + 30];
    FLOAT_TYPE d3 = DepthCache[topMostIndex + 44];
    FLOAT_TYPE d4 = DepthCache[topMostIndex + 58];
    FLOAT_TYPE d5 = DepthCache[topMostIndex + 72];
    FLOAT_TYPE d6 = DepthCache[topMostIndex + 86];

    FLOAT_TYPE d01 = d1 - d0;
    FLOAT_TYPE d12 = d2 - d1;
    FLOAT_TYPE d23 = d3 - d2;
    FLOAT_TYPE d34 = d4 - d3;
    FLOAT_TYPE d45 = d5 - d4;
    FLOAT_TYPE d56 = d6 - d5;

    FLOAT_TYPE l01 = FLOAT_TYPE(d01 * d01 + CB1.StepSize);
    FLOAT_TYPE l12 = FLOAT_TYPE(d12 * d12 + CB1.StepSize);
    FLOAT_TYPE l23 = FLOAT_TYPE(d23 * d23 + CB1.StepSize);
    FLOAT_TYPE l34 = FLOAT_TYPE(d34 * d34 + CB1.StepSize);
    FLOAT_TYPE l45 = FLOAT_TYPE(d45 * d45 + CB1.StepSize);
    FLOAT_TYPE l56 = FLOAT_TYPE(d56 * d56 + CB1.StepSize);

    bool c02 = CompareDeltas(d01, d12, l01, l12);
    bool c13 = CompareDeltas(d12, d23, l12, l23);
    bool c24 = CompareDeltas(d23, d34, l23, l34);
    bool c35 = CompareDeltas(d34, d45, l34, l45);
    bool c46 = CompareDeltas(d45, d56, l45, l56);

    FLOAT_TYPE aoResult1 = SmartBlur(a0, a1, a2, a3, a4, c02, c13, c24);
    FLOAT_TYPE aoResult2 = SmartBlur(a1, a2, a3, a4, a5, c13, c24, c35);
    FLOAT_TYPE aoResult3 = SmartBlur(a2, a3, a4, a5, a6, c24, c35, c46);

    AOCache1[topMostIndex] = aoResult1;
    AOCache1[topMostIndex + 14] = aoResult2;
    AOCache1[topMostIndex + 28] = aoResult3;
}

// We essentially want 5 weights:  4 for each low-res pixel and 1 to blend in when none of the 4 really
// match.  The filter strength is 1 / DeltaZTolerance.  So a tolerance of 0.01 would yield a strength of 100.
// Note that a perfect match of low to high depths would yield a weight of 10^6, completely superceding any
// noise filtering.  The noise filter is intended to soften the effects of shimmering when the high-res depth
// buffer has a lot of small holes in it causing the low-res depth buffer to inaccurately represent it.
FLOAT_TYPE BilateralUpsample(FLOAT_TYPE HiDepth, FLOAT_TYPE HiAO, FLOAT4_TYPE LowDepths, FLOAT4_TYPE LowAO)
{
    float4 weights = float4(9, 3, 1, 3) / (abs(HiDepth - LowDepths) + CB1.kUpsampleTolerance);
    float TotalWeight = dot(weights, 1) + CB1.NoiseFilterStrength;
    float WeightedSum = dot(LowAO, weights) + CB1.NoiseFilterStrength; // * HiAO;
    return FLOAT_TYPE(HiAO * WeightedSum / TotalWeight);
}

[RootSignature(SSAO_RootSig)]
[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint GI : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    //
    // Load 4 pixels per thread into LDS to fill the 16x16 LDS cache with depth and AO
    // Only 13x13 values are needed for the blur and LDS is arranged as 14x14 so skip the last two rows and columns
    //
    if (GTid.x < 7 && GTid.y < 7)
    {
        PrefetchData((GTid.x << 1) + (GTid.y * 28), int2(DTid.xy + GTid.xy - 2) * CB1.InvLowResolution);
    }
    GroupMemoryBarrierWithGroupSync();

    // Goal:  End up with a 9x9 patch that is blurred so we can upsample.  Blur radius is 2 pixels, so start with 13x13 area.

    //
    // Horizontally blur the pixels.	13x13 -> 9x13
    //
#ifdef BLUR_5_PER_THREAD
    if (GI < 26)
    {
#ifdef USE_FP_16
        // Blur 5 per thread but allow one to overlap (offset of 4) to make LDS reads aligned
        // since only 9 values are needed per row instead of 10
        BlurHorizontally((GI / 2) * 14 + (GI % 2) * 4);
#else
        BlurHorizontally((GI / 2) * 14 + (GI % 2) * 5);
#endif
    }
#else
    if (GI < 39)
    {
        BlurHorizontally((GI / 3) * 14 + (GI % 3) * 3);
    }
#endif
    GroupMemoryBarrierWithGroupSync();

    //
    // Vertically blur the pixels.		9x13 -> 9x9
    //
    if (GI < 27)
    {
        BlurVertically((GI / 9) * 42 + GI % 9);
    }
    GroupMemoryBarrierWithGroupSync();

    //
    // Bilateral upsample
    //
    uint Idx0 = GTid.x + GTid.y * 14;
    FLOAT4_TYPE LoSSAOs = FLOAT4_TYPE(AOCache1[Idx0 + 14], AOCache1[Idx0 + 15], AOCache1[Idx0 + 1], AOCache1[Idx0]);

    // We work on a quad of pixels at once because then we can gather 4 each of high and low-res depth values
    float2 UV0 = DTid.xy * CB1.InvLowResolution;
    float2 UV1 = DTid.xy * 2 * CB1.InvHighResolution;

#ifdef BLEND_WITH_HIGHER_RESOLUTION
    FLOAT4_TYPE HiSSAOs = HiResAO.Gather(LinearSampler, UV1);
#else
    FLOAT4_TYPE HiSSAOs = 1.0;
#endif
    FLOAT4_TYPE LoDepths = LoResDB.Gather(LinearSampler, UV0);
    FLOAT4_TYPE HiDepths = HiResDB.Gather(LinearSampler, UV1);

    int2 OutST = DTid.xy << 1;
    AoResult[OutST + int2(-1, 0)] = BilateralUpsample(HiDepths.x, HiSSAOs.x, LoDepths.xyzw, LoSSAOs.xyzw);
    AoResult[OutST + int2(0, 0)] = BilateralUpsample(HiDepths.y, HiSSAOs.y, LoDepths.yzwx, LoSSAOs.yzwx);
    AoResult[OutST + int2(0, -1)] = BilateralUpsample(HiDepths.z, HiSSAOs.z, LoDepths.zwxy, LoSSAOs.zwxy);
    AoResult[OutST + int2(-1, -1)] = BilateralUpsample(HiDepths.w, HiSSAOs.w, LoDepths.wxyz, LoSSAOs.wxyz);
}
