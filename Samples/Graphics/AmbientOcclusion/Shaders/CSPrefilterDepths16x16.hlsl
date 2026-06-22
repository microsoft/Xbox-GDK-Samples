///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2016-2021, Intel Corporation 
// 
// SPDX-License-Identifier: MIT
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XeGTAO is based on GTAO/GTSO "Jimenez et al. / Practical Real-Time Strategies for Accurate Indirect Occlusion", 
// https://www.activision.com/cdn/research/Practical_Real_Time_Strategies_for_Accurate_Indirect_Occlusion_NEW%20VERSION_COLOR.pdf
// 
// Implementation:  Filip Strugar (filip.strugar@intel.com), Steve Mccalla <stephen.mccalla@intel.com>         (\_/)
// Version:         (see XeGTAO.h)                                                                            (='.'=)
// Details:         https://github.com/GameTechDev/XeGTAO                                                     (")_(")
//
// Version history: see XeGTAO.h
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __XBOX_SCARLETT
#if defined(__XBOX_ENABLE_WAVE32) && (__XBOX_ENABLE_WAVE32 != 0)
#error "This shader requires wave64 for the wave intrinsics to work correctly."
#endif
#endif

#ifndef __INTELLISENSE__    // avoids some pesky intellisense errors
#include "XeGTAO.h"
#endif

cbuffer GTAOConstantBuffer                      : register(b0)
{
    GTAOConstants               g_GTAOConsts;
}

#include "XeGTAO.hlsli"
#include "GTAORS.hlsli"

// input output textures for the first pass (XeGTAO_PrefilterDepths16x16)
Texture2D<float>            g_srcRawDepth           : register(t0);   // source depth buffer data (in NDC space in DirectX)
RWTexture2D<lpfloat>        g_outWorkingDepthMIP0   : register(u0);   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
RWTexture2D<lpfloat>        g_outWorkingDepthMIP1   : register(u1);   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
RWTexture2D<lpfloat>        g_outWorkingDepthMIP2   : register(u2);   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
RWTexture2D<lpfloat>        g_outWorkingDepthMIP3   : register(u3);   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
RWTexture2D<lpfloat>        g_outWorkingDepthMIP4   : register(u4);   // output viewspace depth MIP (these are views into g_srcWorkingDepth MIP levels)
SamplerState g_samplerPointClamp : register(s0);

// weighted average depth filter
lpfloat XeGTAO_DepthMIPFilter(lpfloat depth0, lpfloat depth1, lpfloat depth2, lpfloat depth3, const GTAOConstants consts)
{
    lpfloat maxDepth = max(max(depth0, depth1), max(depth2, depth3));

    // fadeout precompute optimisation
    const lpfloat falloffMul = consts.DepthFalloffMul;
    const lpfloat falloffAdd = consts.DepthFalloffAdd;

    lpfloat weight0 = saturate((maxDepth - depth0) * falloffMul + falloffAdd);
    lpfloat weight1 = saturate((maxDepth - depth1) * falloffMul + falloffAdd);
    lpfloat weight2 = saturate((maxDepth - depth2) * falloffMul + falloffAdd);
    lpfloat weight3 = saturate((maxDepth - depth3) * falloffMul + falloffAdd);

    lpfloat weightSum = weight0 + weight1 + weight2 + weight3;
    return (weight0 * depth0 + weight1 * depth1 + weight2 * depth2 + weight3 * depth3) / weightSum;
}

// This is also a good place to do non-linear depth conversion for cases where one wants the 'radius' (effectively the threshold between near-field and far-field GI), 
// is required to be non-linear (i.e. very large outdoors environments).
lpfloat XeGTAO_ClampDepth(float depth)
{
#ifdef XE_GTAO_USE_HALF_FLOAT_PRECISION
    return (lpfloat) clamp(depth, 0.0, 65504.0);
#else
    return clamp( depth, 0.0, 3.402823466e+38 );
#endif
}

// The shader reads values from other threads in the same threadgroup using the swizzle instruction
// __XB_LaneSwizzle which corresponds to ds_swizzle_b32 instruction.
// ds_swizzle_b32 enables sharing data between groups of 32 threads
// From the AMD Instruction Set:
// ds_swizzle_b32:
//			// full data sharing within 4 consecutive threads
//			if (offset[15]) {
//				for (i = 0; i < 32; i += 4) {
//					thread_out[i + 0] = thread_valid[i + offset[1:0]] ?
//						thread_in[i + offset[1:0]] : 0;
//					thread_out[i + 1] = thread_valid[i + offset[3:2]] ?
//						thread_in[i + offset[3:2]] : 0;
//					thread_out[i + 2] = thread_valid[i + offset[5:4]] ?
//						thread_in[i + offset[5:4]] : 0;
//					thread_out[i + 3] = thread_valid[i + offset[7:6]] ?
//						thread_in[i + offset[7:6]] : 0;
//				}
//			}
//			// limited data sharing within 32 consecutive threads
//			else {
//				and_mask = offset[4:0];
//				or_mask = offset[9:5];
//				xor_mask = offset[14:10];
//				for (i = 0; i < 32; i++) {
//					j = ((i & and_mask) | or_mask) ^ xor_mask;
//					thread_out[i] = thread_valid[j] ? thread_in[j] : 0;
//
// 8x8 threads are used in a threadgroup
// Threads (first 31 threads in a 8x8 threadgroup):
//    0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |
//    8  |  9  |  10 |  11 |  12 |  13 |  14 |  15 |
//    16 |  17 |  18 |  19 |  20 |  21 |  22 |  23 |
//    24 |  25 |  26 |  27 |  28 |  29 |  30 |  31 |
//
// 0th Pass: Sample from previous mip level
// 1st Pass threads: 0, 1, 8, 9
// 2nd Pass threads: 0, 2, 16, 17
// 3rd pass threads: 0, 4, 32, 36
// The final pass uses __XB_ReadLane to read from the threads
// which cannot be read using the __XB_LaneSwizzle instruction
void XeGTAO_PrefilterDepths16x16(uint2 dispatchThreadID /*: SV_DispatchThreadID*/, const GTAOConstants consts, Texture2D<float> sourceNDCDepth, SamplerState depthSampler, RWTexture2D<lpfloat> outDepth0, RWTexture2D<lpfloat> outDepth1, RWTexture2D<lpfloat> outDepth2, RWTexture2D<lpfloat> outDepth3, RWTexture2D<lpfloat> outDepth4)
{
    // MIP 0
    uint2 baseCoord = dispatchThreadID;
    const uint2 pixCoord = baseCoord * 2;
    float4 depths4 = sourceNDCDepth.GatherRed(depthSampler, float2(pixCoord * consts.ViewportPixelSize), int2(1, 1));
    lpfloat depth0 = XeGTAO_ClampDepth(XeGTAO_ScreenSpaceToViewSpaceDepth(depths4.w, consts));
    lpfloat depth1 = XeGTAO_ClampDepth(XeGTAO_ScreenSpaceToViewSpaceDepth(depths4.z, consts));
    lpfloat depth2 = XeGTAO_ClampDepth(XeGTAO_ScreenSpaceToViewSpaceDepth(depths4.x, consts));
    lpfloat depth3 = XeGTAO_ClampDepth(XeGTAO_ScreenSpaceToViewSpaceDepth(depths4.y, consts));
    outDepth0[pixCoord + uint2(0, 0)] = (lpfloat) depth0;
    outDepth0[pixCoord + uint2(1, 0)] = (lpfloat) depth1;
    outDepth0[pixCoord + uint2(0, 1)] = (lpfloat) depth2;
    outDepth0[pixCoord + uint2(1, 1)] = (lpfloat) depth3;

    // MIP 1
    lpfloat dm1 = XeGTAO_DepthMIPFilter(depth0, depth1, depth2, depth3, consts);
    outDepth1[baseCoord] = (lpfloat) dm1;
    lpfloat inTL = dm1;
    lpfloat inTR = __XB_LaneSwizzle(dm1, 0x041f); // 1032 (xor_mask = 00001, or_mask = 00000, and_mask = 11111)
    lpfloat inBL = __XB_LaneSwizzle(dm1, 0x201f); // 2301 (xor_mask = 01000, or_mask = 00000, and_mask = 11111)
    lpfloat inBR = __XB_LaneSwizzle(dm1, 0x241f); // 3210 (xor_mask = 01001, or_mask = 00000, and_mask = 11111)
    lpfloat dm2 = XeGTAO_DepthMIPFilter(inTL, inTR, inBL, inBR, consts);

    // MIP 2
    baseCoord >>= 1;
    outDepth2[baseCoord] = (lpfloat) dm2;

    inTL = dm2;
    inTR = __XB_LaneSwizzle(dm2, 0x081f); // 1032 (xor_mask = 00010, or_mask = 00000, and_mask = 11111)
    inBL = __XB_LaneSwizzle(dm2, 0x401f); // 2301 (xor_mask = 10000, or_mask = 00000, and_mask = 11111)
    inBR = __XB_LaneSwizzle(dm2, 0x481f); // 3210 (xor_mask = 10010, or_mask = 00000, and_mask = 11111)   
    lpfloat dm3 = XeGTAO_DepthMIPFilter(inTL, inTR, inBL, inBR, consts);

    // MIP 3
    baseCoord >>= 1;
    outDepth3[baseCoord] = (lpfloat) dm3;

    inTL = __XB_ReadLane(dm3, 0);
    inTR = __XB_ReadLane(dm3, 4);
    inBL = __XB_ReadLane(dm3, 32);
    inBR = __XB_ReadLane(dm3, 36);
    lpfloat dm4 = XeGTAO_DepthMIPFilter(inTL, inTR, inBL, inBR, consts);

    // MIP 4
    baseCoord >>= 1;
    outDepth4[baseCoord] = (lpfloat) dm4;
}


// Engine-specific entry point for the first pass
[RootSignature(GTAO_RootSig)]
[numthreads(8, 8, 1)]   // <- hard coded to 8x8; each thread computes 2x2 blocks so processing 16x16 block: Dispatch needs to be called with (width + 16-1) / 16, (height + 16-1) / 16
void main(uint2 dispatchThreadID : SV_DispatchThreadID)
{
    XeGTAO_PrefilterDepths16x16(dispatchThreadID, g_GTAOConsts, g_srcRawDepth, g_samplerPointClamp, g_outWorkingDepthMIP0, g_outWorkingDepthMIP1, g_outWorkingDepthMIP2, g_outWorkingDepthMIP3, g_outWorkingDepthMIP4);
}
