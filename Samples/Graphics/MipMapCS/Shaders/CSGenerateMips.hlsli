//--------------------------------------------------------------------------------------
// CSGenerateMips.hlsli
////
// Generate mips using a Compute Shader without using the LDS
// This shader can generate 4 mips in one dispatch.
// If the generated mip has odd dimensions (except 1) in any direction, it is better
// to start a new dispatch with that mip, otherwise we don't consider all the values while generating the mip.
// Check the C++ code to see how dispatches are generated.
//
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
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "Shared.h"

#ifndef ODD_DIMS_Y_X
	#define ODD_DIMS_Y_X 0
#endif

// Root Signature Constant
cbuffer CB : register(b5)
{
	float2 g_vInvTexDims;
	uint   g_iSrcSlice;
	uint   g_iNumMips;
};

#ifdef USE_FP_16
typedef float16_t4 TextureFloatType;
#define POINT_25 0.25
#define POINT_5 0.5
#else
typedef float4 TextureFloatType;
#define POINT_25 0.25f
#define POINT_5 0.5f
#endif

Texture2D<TextureFloatType> texIn : register(t0);
RWTexture2D<TextureFloatType> texOut[MIPS_IN_ONE_SHADER] : register(u0);
SamplerState samp : register(s0);

#ifdef USE_FP_16
float16_t4 Downsample2x2(float16_t4 v, uniform uint iStride)
{
    float16_t4 vOut = v;
    float rg = __XB_V_PACK_B32_F16(v.r, v.g);
    float ba = __XB_V_PACK_B32_F16(v.b, v.a);

    switch (iStride)
    {
    case 1:
    {
        // Read values from neighboring threads and average the value
        float rg1 = __XB_LaneSwizzle(rg, 0x041f); // 1032 (xor_mask = 00001, or_mask = 00000, and_mask = 11111)
        float rg2 = __XB_LaneSwizzle(rg, 0x201f); // 2301 (xor_mask = 01000, or_mask = 00000, and_mask = 11111)
        float rg3 = __XB_LaneSwizzle(rg, 0x241f); // 3210 (xor_mask = 01001, or_mask = 00000, and_mask = 11111)
        float ba1 = __XB_LaneSwizzle(ba, 0x041f); // 1032 (xor_mask = 00001, or_mask = 00000, and_mask = 11111)
        float ba2 = __XB_LaneSwizzle(ba, 0x201f); // 2301 (xor_mask = 01000, or_mask = 00000, and_mask = 11111)
        float ba3 = __XB_LaneSwizzle(ba, 0x241f); // 3210 (xor_mask = 01001, or_mask = 00000, and_mask = 11111)
        float16_t4 v1 = float16_t4(__XB_AsHalf(rg1), __XB_AsHalf(ba1));
        float16_t4 v2 = float16_t4(__XB_AsHalf(rg2), __XB_AsHalf(ba2));
        float16_t4 v3 = float16_t4(__XB_AsHalf(rg3), __XB_AsHalf(ba3));
        vOut = POINT_25 * (v + v1 + v2 + v3);
    }
        break;
    case 2:
    {
        // Read values from neighboring threads and average the value
        float rg1 = __XB_LaneSwizzle(rg, 0x081f); // 1032 (xor_mask = 00001, or_mask = 00000, and_mask = 11111)
        float rg2 = __XB_LaneSwizzle(rg, 0x401f); // 2301 (xor_mask = 01000, or_mask = 00000, and_mask = 11111)
        float rg3 = __XB_LaneSwizzle(rg, 0x481f); // 3210 (xor_mask = 01001, or_mask = 00000, and_mask = 11111)
        float ba1 = __XB_LaneSwizzle(ba, 0x081f); // 1032 (xor_mask = 00001, or_mask = 00000, and_mask = 11111)
        float ba2 = __XB_LaneSwizzle(ba, 0x401f); // 2301 (xor_mask = 01000, or_mask = 00000, and_mask = 11111)
        float ba3 = __XB_LaneSwizzle(ba, 0x481f); // 3210 (xor_mask = 01001, or_mask = 00000, and_mask = 11111)
        float16_t4 v1 = float16_t4(__XB_AsHalf(rg1), __XB_AsHalf(ba1));
        float16_t4 v2 = float16_t4(__XB_AsHalf(rg2), __XB_AsHalf(ba2));
        float16_t4 v3 = float16_t4(__XB_AsHalf(rg3), __XB_AsHalf(ba3));
        vOut = POINT_25 * (v + v1 + v2 + v3);
    }
        break;
    case 4:
        // Each thread reads from the same lane and then calculates average
        float rg0 = __XB_ReadLane(rg, 0);
        float rg4 = __XB_ReadLane(rg, 4);
        float rg32 = __XB_ReadLane(rg, 32);
        float rg36 = __XB_ReadLane(rg, 36);
        float ba0 = __XB_ReadLane(ba, 0);
        float ba4 = __XB_ReadLane(ba, 4);
        float ba32 = __XB_ReadLane(ba, 32);
        float ba36 = __XB_ReadLane(ba, 36);
        float16_t4 v0 = float16_t4(__XB_AsHalf(rg0), __XB_AsHalf(ba0));
        float16_t4 v4 = float16_t4(__XB_AsHalf(rg4), __XB_AsHalf(ba4));
        float16_t4 v32 = float16_t4(__XB_AsHalf(rg32), __XB_AsHalf(ba32));
        float16_t4 v36 = float16_t4(__XB_AsHalf(rg36), __XB_AsHalf(ba36));
        vOut = POINT_25 * (v0 + v4 + v32 + v36);
        break;
    }

    return vOut;
}

#else

float4 Downsample2x2(float4 v, uniform uint iStride)
{
	float vOut[4] = { v.x, v.y, v.z, v.w, };

	[unroll]
	for (uint i = 0; i < 4; ++i)
	{
		float v0 = vOut[i];
		switch (iStride)
		{
		case 1:
			// Read values from neighboring threads and average the value
			//v0 = vIn[i];                           // 0123
			vOut[i] += __XB_LaneSwizzle(v0, 0x041f); // 1032 (xor_mask = 00001, or_mask = 00000, and_mask = 11111)
			vOut[i] += __XB_LaneSwizzle(v0, 0x201f); // 2301 (xor_mask = 01000, or_mask = 00000, and_mask = 11111)
			vOut[i] += __XB_LaneSwizzle(v0, 0x241f); // 3210 (xor_mask = 01001, or_mask = 00000, and_mask = 11111) 
			vOut[i] *= POINT_25;
			break;
		case 2:
			// Read values from neighboring threads and average the value
			//v0 = vIn[i];                           // 0123
			vOut[i] += __XB_LaneSwizzle(v0, 0x081f); // 1032 (xor_mask = 00010, or_mask = 00000, and_mask = 11111)
			vOut[i] += __XB_LaneSwizzle(v0, 0x401f); // 2301 (xor_mask = 10000, or_mask = 00000, and_mask = 11111)
			vOut[i] += __XB_LaneSwizzle(v0, 0x481f); // 3210 (xor_mask = 10010, or_mask = 00000, and_mask = 11111) 
			vOut[i] *= POINT_25;
			break;
		case 4:
			// Each thread reads from the same lane and then calculates average
			vOut[i]  = __XB_ReadLane(v0, 0);
			vOut[i] += __XB_ReadLane(v0, 4);
			vOut[i] += __XB_ReadLane(v0, 32);
			vOut[i] += __XB_ReadLane(v0, 36);
			vOut[i] *= POINT_25;
			break;
		}
		
	}
	return float4(vOut);
}
#endif

[numthreads(8, 8, 1)]
[RootSignature(
    "RootConstants(num32BitConstants=4, b5),"
    "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"
    "DescriptorTable(UAV(u0, numDescriptors=4), visibility=SHADER_VISIBILITY_ALL),"
    "StaticSampler(s0,"
    "addressU = TEXTURE_ADDRESS_CLAMP,"
    "addressV = TEXTURE_ADDRESS_CLAMP,"
    "addressW = TEXTURE_ADDRESS_CLAMP,"
    "comparisonFunc = COMPARISON_NEVER,"
    "borderColor=STATIC_BORDER_COLOR_OPAQUE_BLACK,"
    "maxLOD = 3.402823466e+38f,"
    "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT )"
    )
]
void main(uint3 id : SV_DispatchThreadID)
{
#if (ODD_DIMS_Y_X == 0)
    TextureFloatType vIn = texIn.SampleLevel(samp, (id.xy + 0.5f) * g_vInvTexDims.xy, g_iSrcSlice);
#endif

#if (ODD_DIMS_Y_X == 1)
	// > 2:1 downsize in X dimension
	// Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
	float2 loc1 = id.xy + float2(0.25f, 0.5f);
	float2 loc2 = loc1 + float2(0.5f, 0);
#endif

#if (ODD_DIMS_Y_X == 2)
	// > 2:1 downsize in Y dimension
	// Use 2 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
	float2 loc1 = id.xy + float2(0.5f, 0.25f);
	float2 loc2 = loc1 + float2(0, 0.5f);
#endif

#if (ODD_DIMS_Y_X == 1) || (ODD_DIMS_Y_X == 2)
	loc1 *= g_vInvTexDims.xy;
	loc2 *= g_vInvTexDims.xy;

    TextureFloatType vIn = texIn.SampleLevel(samp, loc1, g_iSrcSlice);
    vIn += texIn.SampleLevel(samp, loc2, g_iSrcSlice);
    vIn *= POINT_5;
#endif

#if (ODD_DIMS_Y_X == 3)
	// > 2:1 downsize in both dimensions
	// Use 4 bilinear samples to guarantee we don't undersample when downsizing by more than 2x
	// in both directions.
	float2 loc1 = id.xy + float2(0.25f, 0.25f);
	float2 loc2 = (loc1 + float2(0.0f, 0.5f)) * g_vInvTexDims.xy;
	float2 loc3 = (loc1 + float2(0.5f, 0.0f)) * g_vInvTexDims.xy;
	float2 loc4 = (loc1 + float2(0.5f, 0.5f)) * g_vInvTexDims.xy;
	loc1 *= g_vInvTexDims.xy;

    TextureFloatType vIn = texIn.SampleLevel(samp, loc1, g_iSrcSlice);
    vIn += texIn.SampleLevel(samp, loc2, g_iSrcSlice);
    vIn += texIn.SampleLevel(samp, loc3, g_iSrcSlice);
    vIn += texIn.SampleLevel(samp, loc4, g_iSrcSlice);
    vIn *= POINT_25;
#endif

	uint iDstTex = 0;
	texOut[iDstTex][id.xy] = vIn;

	for (uint i = 0; i < g_iNumMips-1; ++i)
	{
		++iDstTex;
		id.xy >>= 1;
		vIn = Downsample2x2(vIn, 1U << i);
		texOut[iDstTex][id.xy] = vIn;
	}
}
