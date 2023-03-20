//--------------------------------------------------------------------------------------
// GenerateMipsComputeShader.hlsl
//
// Generate Mips for a cube map
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#include "ConstantBuffer.h"


// Generate mips using a Compute Shader without using the LDS
// This shader can generate 4 mips in one dispatch.
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


ConstantBuffer<GenerateMipsConstants> constants : register(b0);
Texture2DArray<float4> texIn : register(t0);
RWTexture2DArray<float4> texOut[MIPS_IN_ONE_SHADER] : register(u0);
SamplerState samp : register(s0);

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
            vOut[i] *= 0.25f;
            break;
        case 2:
            // Read values from neighboring threads and average the value
            //v0 = vIn[i];                           // 0123
            vOut[i] += __XB_LaneSwizzle(v0, 0x081f); // 1032 (xor_mask = 00010, or_mask = 00000, and_mask = 11111)
            vOut[i] += __XB_LaneSwizzle(v0, 0x401f); // 2301 (xor_mask = 10000, or_mask = 00000, and_mask = 11111)
            vOut[i] += __XB_LaneSwizzle(v0, 0x481f); // 3210 (xor_mask = 10010, or_mask = 00000, and_mask = 11111) 
            vOut[i] *= 0.25f;
            break;
        case 4:
            // Each thread reads from the same lane and then calculates average
            vOut[i] = __XB_ReadLane(v0, 0);
            vOut[i] += __XB_ReadLane(v0, 4);
            vOut[i] += __XB_ReadLane(v0, 32);
            vOut[i] += __XB_ReadLane(v0, 36);
            vOut[i] *= 0.25f;
            break;
        }

    }
    return float4(vOut);
}

[numthreads(8, 8, 1)]
[RootSignature(
    "RootConstants(num32BitConstants=4, b0),"
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
    float2 uv = (id.xy + 0.5f) * constants.InvOutTexelSize;
    float4 vIn = texIn.SampleLevel(samp, float3(uv, id.z), constants.SrcMipIndex);
    uint iDstTex = 0;
    texOut[iDstTex][id] = vIn;

    for (uint i = 0; i < constants.numMips - 1; ++i)
    {
        ++iDstTex;
        id.xy >>= 1;
        vIn = Downsample2x2(vIn, 1U << i);
        texOut[iDstTex][id] = vIn;
    }
}
