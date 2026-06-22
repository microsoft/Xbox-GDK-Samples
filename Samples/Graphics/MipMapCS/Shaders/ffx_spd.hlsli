//_____________________________________________________________/\_______________________________________________________________
//==============================================================================================================================
//
//                                         [FFX SPD] Single Pass Downsampler 2.0
//
//==============================================================================================================================
// LICENSE
// =======
// Copyright (c) 2017-2020 Advanced Micro Devices, Inc. All rights reserved.
// -------
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// -------
// The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
// Software.
// -------
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
// WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//------------------------------------------------------------------------------------------------------------------------------
// INTEGRATION SUMMARY FOR CPU
// ===========================
// // you need to provide as constants:
// // number of mip levels to be computed (maximum is 12)
// // number of total thread groups: ((widthInPixels+63)>>6) * ((heightInPixels+63)>>6)
// // workGroupOffset -> by default 0, if you only downsample a rectancle within the source texture use SpdSetup function to calculate correct offset
// ...
// // Dispatch the shader such that each thread group works on a 64x64 sub-tile of the source image
// // for Cube Textures or Texture2DArray, use the z dimension
// vkCmdDispatch(cmdBuf,(widthInPixels+63)>>6,(heightInPixels+63)>>6, slices);

// // you can also use the SpdSetup function:
// //on top of your cpp file:
// #define A_CPU
// #include "ffx_a.h"
// #include "ffx_spd.h"
// // before your dispatch call, use SpdSetup function to get your constants
// varAU2(dispatchThreadGroupCountXY); // output variable
// varAU2(workGroupOffset);  // output variable, this constants are required if Left and Top are not 0,0
// varAU2(numWorkGroupsAndMips); // output variable
// // input information about your source texture:
// // left and top of the rectancle within your texture you want to downsample
// // width and height of the rectancle you want to downsample
// // if complete source texture should get downsampled: left = 0, top = 0, width = sourceTexture.width, height = sourceTexture.height
// varAU4(rectInfo) = initAU4(0, 0, m_Texture.GetWidth(), m_Texture.GetHeight()); // left, top, width, height
// SpdSetup(dispatchThreadGroupCountXY, workGroupOffset, numWorkGroupsAndMips, rectInfo);
// ...
// // constants:
// data.numWorkGroupsPerSlice = numWorkGroupsAndMips[0];
// data.mips = numWorkGroupsAndMips[1];
// data.workGroupOffset[0] = workGroupOffset[0];
// data.workGroupOffset[1] = workGroupOffset[1];
// ...
// uint32_t dispatchX = dispatchThreadGroupCountXY[0];
// uint32_t dispatchY = dispatchThreadGroupCountXY[1];
// uint32_t dispatchZ = m_CubeTexture.GetArraySize(); // slices - for 2D Texture this is 1, for cube texture 6
// vkCmdDispatch(cmd_buf, dispatchX, dispatchY, dispatchZ);

//------------------------------------------------------------------------------------------------------------------------------
// INTEGRATION SUMMARY FOR GPU
// ===========================

// // Setup pre-portability-header defines (sets up GLSL/HLSL path, etc)
#define A_GPU 1
#define A_HLSL 1

// // Include the portability header (or copy it in without an include).
#include "ffx_a.h"

// [SAMPLER] - if you want to use a sampler with linear filtering for loading the source image
// follow additionally the instructions marked with [SAMPLER]
// add following define:
#define SPD_LINEAR_SAMPLER
// this is recommended, as using one sample() with linear filter to reduce 2x2 is faster
// than 4x load() plus manual averaging

#if defined(A_HALF)
// // source image
Texture2D<AH4> imgSrc :register(t0);

// // destination -> 12 is the maximum number of mips supported by SPD
globallycoherent RWTexture2D<AH4> imgDst[12] :register(u1);
#else
// // source image
Texture2D<float4> imgSrc :register(t0);

// // destination -> 12 is the maximum number of mips supported by SPD
globallycoherent RWTexture2D<float4> imgDst[12] :register(u1);
#endif

// // global atomic counter - MUST be initialized to 0
// // SPD resets the counter back after each run by calling SpdResetAtomicCounter(slice)
// // if you have more than 1 slice (== if you downsample a cube texture or a texture2Darray)
// // you have an array of counters: counter[6] -> if you have 6 slices for example
struct SpdGlobalAtomicBuffer
{
    uint counter;
};
globallycoherent RWStructuredBuffer<SpdGlobalAtomicBuffer> spdGlobalAtomic : register(u0);

// // [SAMPLER] add sampler
SamplerState srcSampler :register(s0);

// // constants - either push constant or constant buffer
// // or calculate within shader
// // [SAMPLER] when using sampler add inverse source image size
cbuffer spdConstants : register(b5)
{
    uint mips;
    uint numWorkGroups;
    float2 invInputSize;
};

// // Define LDS variables
#if !defined(A_HALF)
 groupshared AF4 spdIntermediate[16][16];
#else
// // PACKED version
groupshared AH4 spdIntermediate[16][16];
#endif

groupshared AU1 spdCounter;

// // if subgroup operations are not supported / can't use SM6.0
// #define SPD_NO_WAVE_OPERATIONS

// // Define the fetch function(s) and the reduction function
// // if non-power-of-2 textures, add border controls to the load and store functions
// // to make sure the borders of the mip level look as you want it
// // if you don't add border controls you'll read zeros past the border
// // if you load with a sampler, this is obv. handled by your sampler :)
// // this is also the place where you need to do color space transformation if needed
// // E.g. if your texture format is SRGB/UNORM and you use the UAV load and store functions
// // no automatic to/from linear conversions are happening
// // there is to/from linear conversions when using a sampler and render target approach
// // conversion to linear (load function): x*x
// // conversion from linear (store function): sqrt()

#if !defined (A_HALF)
// AU1 slice parameter is for Cube textures and texture2DArray
// if downsampling Texture2D you can ignore this parameter, otherwise use it to access correct slice
// // Load from source image
#if !defined(SPD_LINEAR_SAMPLER)
AF4 SpdLoadSourceImage(ASU2 tex, AU1 slice){return imgSrc[tex];}
#else
AF4 SpdLoadSourceImage(ASU2 p, AU1 slice){
    AF2 textureCoord = p * invInputSize + invInputSize;
    return imgSrc.SampleLevel(srcSampler, textureCoord, 0);
}
#endif

// // SpdLoad() takes a 32-bit signed integer 2D coordinate and loads color.
// // Loads the 5th mip level, each value is computed by a different thread group
// // last thread group will access all its elements and compute the subsequent mips
// // reminder: if non-power-of-2 textures, add border controls if you do not want to read zeros past the border
AF4 SpdLoad(ASU2 tex, AU1 slice){return imgDst[5][tex];}

// Define the store function
void SpdStore(ASU2 pix, AF4 value, AU1 mip, AU1 slice){imgDst[mip][pix] = value;}

// // Define the atomic counter increase function
// // each slice only reads and stores to its specific slice counter
// // so, if you have several slices it's
// // InterlockedAdd(spdGlobalAtomic[0].counter[slice], 1, spdCounter);
void SpdIncreaseAtomicCounter(AU1 slice){InterlockedAdd(spdGlobalAtomic[0].counter, 1, spdCounter);}
AU1 SpdGetAtomicCounter(){return spdCounter;}
void SpdResetAtomicCounter(AU1 slice){spdGlobalAtomic[0].counter = 0;}

// // Define the LDS load and store functions
AF4 SpdLoadIntermediate(AU1 x, AU1 y){return spdIntermediate[x][y];}
void SpdStoreIntermediate(AU1 x, AU1 y, AF4 value){spdIntermediate[x][y] = value;}

// // Define your reduction function: takes as input the four 2x2 values and returns 1 output value
// Example below: computes the average value
AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3){return (v0+v1+v2+v3)*0.25;}

#else
// // PACKED VERSION
// Load from source image
#if !defined(SPD_LINEAR_SAMPLER)
AH4 SpdLoadSourceImageH(ASU2 tex, AU1 slice){return AH4(imgSrc[tex]);}
#else
AH4 SpdLoadSourceImageH(ASU2 p, AU1 slice){
    AF2 textureCoord = p * invInputSize + invInputSize;
    return AH4(imgSrc.SampleLevel(srcSampler, textureCoord, 0));
}
#endif

// // SpdLoadH() takes a 32-bit signed integer 2D coordinate and loads color.
// // Loads the 5th mip level, each value is computed by a different thread group
// // last thread group will access all its elements and compute the subsequent mips
AH4 SpdLoadH(ASU2 tex, AU1 slice){return AH4(imgDst[5][tex]);}

// Define the store function
void SpdStoreH(ASU2 pix, AH4 value, AU1 index, AU1 slice){imgDst[index][pix] = value;}

// // Define the atomic counter increase function
void SpdIncreaseAtomicCounter(AU1 slice){InterlockedAdd(spdGlobalAtomic[0].counter, 1, spdCounter);}
AU1 SpdGetAtomicCounter(){return spdCounter;}
void SpdResetAtomicCounter(AU1 slice) { spdGlobalAtomic[0].counter = 0; }

// // Define the LDS load and store functions
AH4 SpdLoadIntermediateH(AU1 x, AU1 y){return spdIntermediate[x][y];}
void SpdStoreIntermediateH(AU1 x, AU1 y, AH4 value){spdIntermediate[x][y] = value;}

// // Define your reduction function: takes as input the four 2x2 values and returns 1 output value
// Example below: computes the average value
AH4 SpdReduce4H(AH4 v0, AH4 v1, AH4 v2, AH4 v3){return (v0+v1+v2+v3)*AH1(0.25);}
#endif

//_____________________________________________________________/\_______________________________________________________________

void SpdWorkgroupShuffleBarrier() {
    GroupMemoryBarrierWithGroupSync();
}

// Only last active workgroup should proceed
bool SpdExitWorkgroup(AU1 numWorkGroups, AU1 localInvocationIndex, AU1 slice)
{
    // global atomic counter
    if (localInvocationIndex == 0)
    {
        SpdIncreaseAtomicCounter(slice);
    }
    SpdWorkgroupShuffleBarrier();
    return (SpdGetAtomicCounter() != (numWorkGroups - 1));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if !defined(A_HALF)
// User defined: AF4 SpdReduce4(AF4 v0, AF4 v1, AF4 v2, AF4 v3);

AF4 SpdReduceQuad(AF4 v)
{
#if !defined(SPD_NO_WAVE_OPERATIONS)
    // Changed from original code to use Xbox swizzle intrinsics
    AF4 v0 = v;
    AF4 v1, v2, v3;
    [unroll]
    for (uint i = 0; i < 4; ++i)
    {
        AF1 vi = v[i];
        v1[i] = __XB_LaneSwizzle(vi, 0x081F);
        v2[i] = __XB_LaneSwizzle(vi, 0x0C1F);
        v3[i] = __XB_LaneSwizzle(vi, 0x041F);
    }
    return SpdReduce4(v0, v1, v2, v3);
#endif
    return v;
}

AF4 SpdReduceIntermediate(AU2 i0, AU2 i1, AU2 i2, AU2 i3)
{
    AF4 v0 = SpdLoadIntermediate(i0.x, i0.y);
    AF4 v1 = SpdLoadIntermediate(i1.x, i1.y);
    AF4 v2 = SpdLoadIntermediate(i2.x, i2.y);
    AF4 v3 = SpdLoadIntermediate(i3.x, i3.y);
    return SpdReduce4(v0, v1, v2, v3);
}

AF4 SpdReduceLoad4(AU2 i0, AU2 i1, AU2 i2, AU2 i3, AU1 slice)
{
    AF4 v0 = SpdLoad(ASU2(i0), slice);
    AF4 v1 = SpdLoad(ASU2(i1), slice);
    AF4 v2 = SpdLoad(ASU2(i2), slice);
    AF4 v3 = SpdLoad(ASU2(i3), slice);
    return SpdReduce4(v0, v1, v2, v3);
}

AF4 SpdReduceLoad4(AU2 base, AU1 slice)
{
    return SpdReduceLoad4(
        AU2(base + AU2(0, 0)),
        AU2(base + AU2(0, 1)),
        AU2(base + AU2(1, 0)),
        AU2(base + AU2(1, 1)),
        slice);
}

AF4 SpdReduceLoadSourceImage4(AU2 i0, AU2 i1, AU2 i2, AU2 i3, AU1 slice)
{
    AF4 v0 = SpdLoadSourceImage(ASU2(i0), slice);
    AF4 v1 = SpdLoadSourceImage(ASU2(i1), slice);
    AF4 v2 = SpdLoadSourceImage(ASU2(i2), slice);
    AF4 v3 = SpdLoadSourceImage(ASU2(i3), slice);
    return SpdReduce4(v0, v1, v2, v3);
}

AF4 SpdReduceLoadSourceImage(AU2 base, AU1 slice)
{
#ifdef SPD_LINEAR_SAMPLER
    return SpdLoadSourceImage(ASU2(base), slice);
#else
    return SpdReduceLoadSourceImage4(
        AU2(base + AU2(0, 0)),
        AU2(base + AU2(0, 1)),
        AU2(base + AU2(1, 0)),
        AU2(base + AU2(1, 1)),
        slice);
#endif
}

void SpdDownsampleMips_0_1_Intrinsics(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
    AF4 v[4];

    ASU2 tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2, y * 2);
    ASU2 pix = ASU2(workGroupID.xy * 32) + ASU2(x, y);
    v[0] = SpdReduceLoadSourceImage(tex, slice);
    SpdStore(pix, v[0], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2 + 32, y * 2);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x + 16, y);
    v[1] = SpdReduceLoadSourceImage(tex, slice);
    SpdStore(pix, v[1], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2, y * 2 + 32);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x, y + 16);
    v[2] = SpdReduceLoadSourceImage(tex, slice);
    SpdStore(pix, v[2], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2 + 32, y * 2 + 32);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x + 16, y + 16);
    v[3] = SpdReduceLoadSourceImage(tex, slice);
    SpdStore(pix, v[3], 0, slice);

    if (mip <= 1)
        return;

    v[0] = SpdReduceQuad(v[0]);
    v[1] = SpdReduceQuad(v[1]);
    v[2] = SpdReduceQuad(v[2]);
    v[3] = SpdReduceQuad(v[3]);

    if ((localInvocationIndex % 4) == 0)
    {
        SpdStore(ASU2(workGroupID.xy * 16) +
            ASU2(x / 2, y / 2), v[0], 1, slice);
        SpdStoreIntermediate(
            x / 2, y / 2, v[0]);

        SpdStore(ASU2(workGroupID.xy * 16) +
            ASU2(x / 2 + 8, y / 2), v[1], 1, slice);
        SpdStoreIntermediate(
            x / 2 + 8, y / 2, v[1]);

        SpdStore(ASU2(workGroupID.xy * 16) +
            ASU2(x / 2, y / 2 + 8), v[2], 1, slice);
        SpdStoreIntermediate(
            x / 2, y / 2 + 8, v[2]);

        SpdStore(ASU2(workGroupID.xy * 16) +
            ASU2(x / 2 + 8, y / 2 + 8), v[3], 1, slice);
        SpdStoreIntermediate(
            x / 2 + 8, y / 2 + 8, v[3]);
    }
}

void SpdDownsampleMips_0_1_LDS(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
    AF4 v[4];

    ASU2 tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2, y * 2);
    ASU2 pix = ASU2(workGroupID.xy * 32) + ASU2(x, y);
    v[0] = SpdReduceLoadSourceImage(tex, slice);
    SpdStore(pix, v[0], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2 + 32, y * 2);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x + 16, y);
    v[1] = SpdReduceLoadSourceImage(tex, slice);
    SpdStore(pix, v[1], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2, y * 2 + 32);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x, y + 16);
    v[2] = SpdReduceLoadSourceImage(tex, slice);
    SpdStore(pix, v[2], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2 + 32, y * 2 + 32);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x + 16, y + 16);
    v[3] = SpdReduceLoadSourceImage(tex, slice);
    SpdStore(pix, v[3], 0, slice);

    if (mip <= 1)
        return;

    for (int i = 0; i < 4; i++)
    {
        SpdStoreIntermediate(x, y, v[i]);
        SpdWorkgroupShuffleBarrier();
        if (localInvocationIndex < 64)
        {
            v[i] = SpdReduceIntermediate(
                AU2(x * 2 + 0, y * 2 + 0),
                AU2(x * 2 + 1, y * 2 + 0),
                AU2(x * 2 + 0, y * 2 + 1),
                AU2(x * 2 + 1, y * 2 + 1)
            );
            SpdStore(ASU2(workGroupID.xy * 16) + ASU2(x + (i % 2) * 8, y + (i / 2) * 8), v[i], 1, slice);
        }
        SpdWorkgroupShuffleBarrier();
    }

    if (localInvocationIndex < 64)
    {
        SpdStoreIntermediate(x + 0, y + 0, v[0]);
        SpdStoreIntermediate(x + 8, y + 0, v[1]);
        SpdStoreIntermediate(x + 0, y + 8, v[2]);
        SpdStoreIntermediate(x + 8, y + 8, v[3]);
    }
}

void SpdDownsampleMips_0_1(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    SpdDownsampleMips_0_1_LDS(x, y, workGroupID, localInvocationIndex, mip, slice);
#else
    SpdDownsampleMips_0_1_Intrinsics(x, y, workGroupID, localInvocationIndex, mip, slice);
#endif
}


void SpdDownsampleMip_2(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    if (localInvocationIndex < 64)
    {
        AF4 v = SpdReduceIntermediate(
            AU2(x * 2 + 0, y * 2 + 0),
            AU2(x * 2 + 1, y * 2 + 0),
            AU2(x * 2 + 0, y * 2 + 1),
            AU2(x * 2 + 1, y * 2 + 1)
        );
        SpdStore(ASU2(workGroupID.xy * 8) + ASU2(x, y), v, mip, slice);
        // store to LDS, try to reduce bank conflicts
        // x 0 x 0 x 0 x 0 x 0 x 0 x 0 x 0
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // 0 x 0 x 0 x 0 x 0 x 0 x 0 x 0 x
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // x 0 x 0 x 0 x 0 x 0 x 0 x 0 x 0
        // ...
        // x 0 x 0 x 0 x 0 x 0 x 0 x 0 x 0
        SpdStoreIntermediate(x * 2 + y % 2, y * 2, v);
    }
#else
    AF4 v = SpdLoadIntermediate(x, y);
    v = SpdReduceQuad(v);
    // quad index 0 stores result
    if (localInvocationIndex % 4 == 0)
    {
        SpdStore(ASU2(workGroupID.xy * 8) + ASU2(x / 2, y / 2), v, mip, slice);
        SpdStoreIntermediate(x + (y / 2) % 2, y, v);
    }
#endif
}

void SpdDownsampleMip_3(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    if (localInvocationIndex < 16)
    {
        // x 0 x 0
        // 0 0 0 0
        // 0 x 0 x
        // 0 0 0 0
        AF4 v = SpdReduceIntermediate(
            AU2(x * 4 + 0 + 0, y * 4 + 0),
            AU2(x * 4 + 2 + 0, y * 4 + 0),
            AU2(x * 4 + 0 + 1, y * 4 + 2),
            AU2(x * 4 + 2 + 1, y * 4 + 2)
        );
        SpdStore(ASU2(workGroupID.xy * 4) + ASU2(x, y), v, mip, slice);
        // store to LDS
        // x 0 0 0 x 0 0 0 x 0 0 0 x 0 0 0
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // 0 x 0 0 0 x 0 0 0 x 0 0 0 x 0 0
        // ...
        // 0 0 x 0 0 0 x 0 0 0 x 0 0 0 x 0
        // ...
        // 0 0 0 x 0 0 0 x 0 0 0 x 0 0 0 x
        // ...
        SpdStoreIntermediate(x * 4 + y, y * 4, v);
    }
#else
    if (localInvocationIndex < 64)
    {
        AF4 v = SpdLoadIntermediate(x * 2 + y % 2, y * 2);
        v = SpdReduceQuad(v);
        // quad index 0 stores result
        if (localInvocationIndex % 4 == 0)
        {
            SpdStore(ASU2(workGroupID.xy * 4) + ASU2(x / 2, y / 2), v, mip, slice);
            SpdStoreIntermediate(x * 2 + y / 2, y * 2, v);
        }
    }
#endif
}

void SpdDownsampleMip_4(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    if (localInvocationIndex < 4)
    {
        // x 0 0 0 x 0 0 0
        // ...
        // 0 x 0 0 0 x 0 0
        AF4 v = SpdReduceIntermediate(
            AU2(x * 8 + 0 + 0 + y * 2, y * 8 + 0),
            AU2(x * 8 + 4 + 0 + y * 2, y * 8 + 0),
            AU2(x * 8 + 0 + 1 + y * 2, y * 8 + 4),
            AU2(x * 8 + 4 + 1 + y * 2, y * 8 + 4)
        );
        SpdStore(ASU2(workGroupID.xy * 2) + ASU2(x, y), v, mip, slice);
        // store to LDS
        // x x x x 0 ...
        // 0 ...
        SpdStoreIntermediate(x + y * 2, 0, v);
    }
#else
    if (localInvocationIndex < 16)
    {
        AF4 v = SpdLoadIntermediate(x * 4 + y, y * 4);
        v = SpdReduceQuad(v);
        // quad index 0 stores result
        if (localInvocationIndex % 4 == 0)
        {
            SpdStore(ASU2(workGroupID.xy * 2) + ASU2(x / 2, y / 2), v, mip, slice);
            SpdStoreIntermediate(x / 2 + y, 0, v);
        }
    }
#endif
}

void SpdDownsampleMip_5(AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    if (localInvocationIndex < 1)
    {
        // x x x x 0 ...
        // 0 ...
        AF4 v = SpdReduceIntermediate(
            AU2(0, 0),
            AU2(1, 0),
            AU2(2, 0),
            AU2(3, 0)
        );
        SpdStore(ASU2(workGroupID.xy), v, mip, slice);
    }
#else
    if (localInvocationIndex < 4)
    {
        AF4 v = SpdLoadIntermediate(localInvocationIndex, 0);
        v = SpdReduceQuad(v);
        // quad index 0 stores result
        if (localInvocationIndex % 4 == 0)
        {
            SpdStore(ASU2(workGroupID.xy), v, mip, slice);
        }
    }
#endif
}

void SpdDownsampleMips_6_7(AU1 x, AU1 y, AU1 mips, AU1 slice)
{
    ASU2 tex = ASU2(x * 4 + 0, y * 4 + 0);
    ASU2 pix = ASU2(x * 2 + 0, y * 2 + 0);
    AF4 v0 = SpdReduceLoad4(tex, slice);
    SpdStore(pix, v0, 6, slice);

    tex = ASU2(x * 4 + 2, y * 4 + 0);
    pix = ASU2(x * 2 + 1, y * 2 + 0);
    AF4 v1 = SpdReduceLoad4(tex, slice);
    SpdStore(pix, v1, 6, slice);

    tex = ASU2(x * 4 + 0, y * 4 + 2);
    pix = ASU2(x * 2 + 0, y * 2 + 1);
    AF4 v2 = SpdReduceLoad4(tex, slice);
    SpdStore(pix, v2, 6, slice);

    tex = ASU2(x * 4 + 2, y * 4 + 2);
    pix = ASU2(x * 2 + 1, y * 2 + 1);
    AF4 v3 = SpdReduceLoad4(tex, slice);
    SpdStore(pix, v3, 6, slice);

    if (mips <= 7) return;
    // no barrier needed, working on values only from the same thread

    AF4 v = SpdReduce4(v0, v1, v2, v3);
    SpdStore(ASU2(x, y), v, 7, slice);
    SpdStoreIntermediate(x, y, v);
}

void SpdDownsampleNextFour(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 baseMip, AU1 mips, AU1 slice)
{
    if (mips <= baseMip) return;
    SpdWorkgroupShuffleBarrier();
    SpdDownsampleMip_2(x, y, workGroupID, localInvocationIndex, baseMip, slice);

    if (mips <= baseMip + 1) return;
    SpdWorkgroupShuffleBarrier();
    SpdDownsampleMip_3(x, y, workGroupID, localInvocationIndex, baseMip + 1, slice);

    if (mips <= baseMip + 2) return;
    SpdWorkgroupShuffleBarrier();
    SpdDownsampleMip_4(x, y, workGroupID, localInvocationIndex, baseMip + 2, slice);

    if (mips <= baseMip + 3) return;
    SpdWorkgroupShuffleBarrier();
    SpdDownsampleMip_5(workGroupID, localInvocationIndex, baseMip + 3, slice);
}

void SpdDownsample(
    AU2 workGroupID,
    AU1 localInvocationIndex,
    AU1 mips,
    AU1 numWorkGroups,
    AU1 slice
) {
    AU2 sub_xy = ARmpRed8x8(localInvocationIndex % 64);
    AU1 x = sub_xy.x + 8 * ((localInvocationIndex >> 6) % 2);
    AU1 y = sub_xy.y + 8 * ((localInvocationIndex >> 7));
    SpdDownsampleMips_0_1(x, y, workGroupID, localInvocationIndex, mips, slice);

    SpdDownsampleNextFour(x, y, workGroupID, localInvocationIndex, 2, mips, slice);

    if (mips <= 6) return;

    if (SpdExitWorkgroup(numWorkGroups, localInvocationIndex, slice)) return;

    SpdResetAtomicCounter(slice);

    // After mip 6 there is only a single workgroup left that downsamples the remaining up to 64x64 texels.
    SpdDownsampleMips_6_7(x, y, mips, slice);

    SpdDownsampleNextFour(x, y, AU2(0, 0), localInvocationIndex, 8, mips, slice);
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//==============================================================================================================================
//                                                       PACKED VERSION
//==============================================================================================================================

#ifdef A_HALF

AH4 SpdReduceQuadH(AH4 v)
{
#if !defined(SPD_NO_WAVE_OPERATIONS)
    // Changed from original code to use Xbox swizzle intrinsics and manual packing
    AH4 v0 = v;
    AH4 v1, v2, v3;

    AF1 rg = __XB_V_PACK_B32_F16(v.r, v.g);
    AF1 ba = __XB_V_PACK_B32_F16(v.b, v.a);
    AF1 rg1 = __XB_LaneSwizzle(rg, 0x081F);
    AF1 rg2 = __XB_LaneSwizzle(rg, 0x0C1F);
    AF1 rg3 = __XB_LaneSwizzle(rg, 0x041F);
    AF1 ba1 = __XB_LaneSwizzle(ba, 0x081F);
    AF1 ba2 = __XB_LaneSwizzle(ba, 0x0C1F);
    AF1 ba3 = __XB_LaneSwizzle(ba, 0x041F);

    v1 = AH4(__XB_AsHalf(rg1), __XB_AsHalf(ba1));
    v2 = AH4(__XB_AsHalf(rg2), __XB_AsHalf(ba2));
    v3 = AH4(__XB_AsHalf(rg3), __XB_AsHalf(ba3));

    return SpdReduce4H(v0, v1, v2, v3);
#endif
    return AH4(0.0, 0.0, 0.0, 0.0);

}

AH4 SpdReduceIntermediateH(AU2 i0, AU2 i1, AU2 i2, AU2 i3)
{
    AH4 v0 = SpdLoadIntermediateH(i0.x, i0.y);
    AH4 v1 = SpdLoadIntermediateH(i1.x, i1.y);
    AH4 v2 = SpdLoadIntermediateH(i2.x, i2.y);
    AH4 v3 = SpdLoadIntermediateH(i3.x, i3.y);
    return SpdReduce4H(v0, v1, v2, v3);
}

AH4 SpdReduceLoad4H(AU2 i0, AU2 i1, AU2 i2, AU2 i3, AU1 slice)
{
    AH4 v0 = SpdLoadH(ASU2(i0), slice);
    AH4 v1 = SpdLoadH(ASU2(i1), slice);
    AH4 v2 = SpdLoadH(ASU2(i2), slice);
    AH4 v3 = SpdLoadH(ASU2(i3), slice);
    return SpdReduce4H(v0, v1, v2, v3);
}

AH4 SpdReduceLoad4H(AU2 base, AU1 slice)
{
    return SpdReduceLoad4H(
        AU2(base + AU2(0, 0)),
        AU2(base + AU2(0, 1)),
        AU2(base + AU2(1, 0)),
        AU2(base + AU2(1, 1)),
        slice);
}

AH4 SpdReduceLoadSourceImage4H(AU2 i0, AU2 i1, AU2 i2, AU2 i3, AU1 slice)
{
    AH4 v0 = SpdLoadSourceImageH(ASU2(i0), slice);
    AH4 v1 = SpdLoadSourceImageH(ASU2(i1), slice);
    AH4 v2 = SpdLoadSourceImageH(ASU2(i2), slice);
    AH4 v3 = SpdLoadSourceImageH(ASU2(i3), slice);
    return SpdReduce4H(v0, v1, v2, v3);
}

AH4 SpdReduceLoadSourceImageH(AU2 base, AU1 slice)
{
#ifdef SPD_LINEAR_SAMPLER
    return SpdLoadSourceImageH(ASU2(base), slice);
#else
    return SpdReduceLoadSourceImage4H(
        AU2(base + AU2(0, 0)),
        AU2(base + AU2(0, 1)),
        AU2(base + AU2(1, 0)),
        AU2(base + AU2(1, 1)),
        slice);
#endif
}

void SpdDownsampleMips_0_1_IntrinsicsH(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mips, AU1 slice)
{
    AH4 v[4];

    ASU2 tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2, y * 2);
    ASU2 pix = ASU2(workGroupID.xy * 32) + ASU2(x, y);
    v[0] = SpdReduceLoadSourceImageH(tex, slice);
    SpdStoreH(pix, v[0], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2 + 32, y * 2);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x + 16, y);
    v[1] = SpdReduceLoadSourceImageH(tex, slice);
    SpdStoreH(pix, v[1], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2, y * 2 + 32);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x, y + 16);
    v[2] = SpdReduceLoadSourceImageH(tex, slice);
    SpdStoreH(pix, v[2], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2 + 32, y * 2 + 32);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x + 16, y + 16);
    v[3] = SpdReduceLoadSourceImageH(tex, slice);
    SpdStoreH(pix, v[3], 0, slice);

    if (mips <= 1)
        return;

    v[0] = SpdReduceQuadH(v[0]);
    v[1] = SpdReduceQuadH(v[1]);
    v[2] = SpdReduceQuadH(v[2]);
    v[3] = SpdReduceQuadH(v[3]);

    if ((localInvocationIndex % 4) == 0)
    {
        SpdStoreH(ASU2(workGroupID.xy * 16) + ASU2(x / 2, y / 2), v[0], 1, slice);
        SpdStoreIntermediateH(x / 2, y / 2, v[0]);

        SpdStoreH(ASU2(workGroupID.xy * 16) + ASU2(x / 2 + 8, y / 2), v[1], 1, slice);
        SpdStoreIntermediateH(x / 2 + 8, y / 2, v[1]);

        SpdStoreH(ASU2(workGroupID.xy * 16) + ASU2(x / 2, y / 2 + 8), v[2], 1, slice);
        SpdStoreIntermediateH(x / 2, y / 2 + 8, v[2]);

        SpdStoreH(ASU2(workGroupID.xy * 16) + ASU2(x / 2 + 8, y / 2 + 8), v[3], 1, slice);
        SpdStoreIntermediateH(x / 2 + 8, y / 2 + 8, v[3]);
    }
}

void SpdDownsampleMips_0_1_LDSH(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mips, AU1 slice)
{
    AH4 v[4];

    ASU2 tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2, y * 2);
    ASU2 pix = ASU2(workGroupID.xy * 32) + ASU2(x, y);
    v[0] = SpdReduceLoadSourceImageH(tex, slice);
    SpdStoreH(pix, v[0], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2 + 32, y * 2);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x + 16, y);
    v[1] = SpdReduceLoadSourceImageH(tex, slice);
    SpdStoreH(pix, v[1], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2, y * 2 + 32);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x, y + 16);
    v[2] = SpdReduceLoadSourceImageH(tex, slice);
    SpdStoreH(pix, v[2], 0, slice);

    tex = ASU2(workGroupID.xy * 64) + ASU2(x * 2 + 32, y * 2 + 32);
    pix = ASU2(workGroupID.xy * 32) + ASU2(x + 16, y + 16);
    v[3] = SpdReduceLoadSourceImageH(tex, slice);
    SpdStoreH(pix, v[3], 0, slice);

    if (mips <= 1)
        return;

    for (int i = 0; i < 4; i++)
    {
        SpdStoreIntermediateH(x, y, v[i]);
        SpdWorkgroupShuffleBarrier();
        if (localInvocationIndex < 64)
        {
            v[i] = SpdReduceIntermediateH(
                AU2(x * 2 + 0, y * 2 + 0),
                AU2(x * 2 + 1, y * 2 + 0),
                AU2(x * 2 + 0, y * 2 + 1),
                AU2(x * 2 + 1, y * 2 + 1)
            );
            SpdStoreH(ASU2(workGroupID.xy * 16) + ASU2(x + (i % 2) * 8, y + (i / 2) * 8), v[i], 1, slice);
        }
        SpdWorkgroupShuffleBarrier();
    }

    if (localInvocationIndex < 64)
    {
        SpdStoreIntermediateH(x + 0, y + 0, v[0]);
        SpdStoreIntermediateH(x + 8, y + 0, v[1]);
        SpdStoreIntermediateH(x + 0, y + 8, v[2]);
        SpdStoreIntermediateH(x + 8, y + 8, v[3]);
    }
}

void SpdDownsampleMips_0_1H(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mips, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    SpdDownsampleMips_0_1_LDSH(x, y, workGroupID, localInvocationIndex, mips, slice);
#else
    SpdDownsampleMips_0_1_IntrinsicsH(x, y, workGroupID, localInvocationIndex, mips, slice);
#endif
}


void SpdDownsampleMip_2H(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    if (localInvocationIndex < 64)
    {
        AH4 v = SpdReduceIntermediateH(
            AU2(x * 2 + 0, y * 2 + 0),
            AU2(x * 2 + 1, y * 2 + 0),
            AU2(x * 2 + 0, y * 2 + 1),
            AU2(x * 2 + 1, y * 2 + 1)
        );
        SpdStoreH(ASU2(workGroupID.xy * 8) + ASU2(x, y), v, mip, slice);
        // store to LDS, try to reduce bank conflicts
        // x 0 x 0 x 0 x 0 x 0 x 0 x 0 x 0
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // 0 x 0 x 0 x 0 x 0 x 0 x 0 x 0 x
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // x 0 x 0 x 0 x 0 x 0 x 0 x 0 x 0
        // ...
        // x 0 x 0 x 0 x 0 x 0 x 0 x 0 x 0
        SpdStoreIntermediateH(x * 2 + y % 2, y * 2, v);
    }
#else
    AH4 v = SpdLoadIntermediateH(x, y);
    v = SpdReduceQuadH(v);
    // quad index 0 stores result
    if (localInvocationIndex % 4 == 0)
    {
        SpdStoreH(ASU2(workGroupID.xy * 8) + ASU2(x / 2, y / 2), v, mip, slice);
        SpdStoreIntermediateH(x + (y / 2) % 2, y, v);
    }
#endif
}

void SpdDownsampleMip_3H(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    if (localInvocationIndex < 16)
    {
        // x 0 x 0
        // 0 0 0 0
        // 0 x 0 x
        // 0 0 0 0
        AH4 v = SpdReduceIntermediateH(
            AU2(x * 4 + 0 + 0, y * 4 + 0),
            AU2(x * 4 + 2 + 0, y * 4 + 0),
            AU2(x * 4 + 0 + 1, y * 4 + 2),
            AU2(x * 4 + 2 + 1, y * 4 + 2)
        );
        SpdStoreH(ASU2(workGroupID.xy * 4) + ASU2(x, y), v, mip, slice);
        // store to LDS
        // x 0 0 0 x 0 0 0 x 0 0 0 x 0 0 0
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
        // 0 x 0 0 0 x 0 0 0 x 0 0 0 x 0 0
        // ...
        // 0 0 x 0 0 0 x 0 0 0 x 0 0 0 x 0
        // ...
        // 0 0 0 x 0 0 0 x 0 0 0 x 0 0 0 x
        // ...
        SpdStoreIntermediateH(x * 4 + y, y * 4, v);
    }
#else
    if (localInvocationIndex < 64)
    {
        AH4 v = SpdLoadIntermediateH(x * 2 + y % 2, y * 2);
        v = SpdReduceQuadH(v);
        // quad index 0 stores result
        if (localInvocationIndex % 4 == 0)
        {
            SpdStoreH(ASU2(workGroupID.xy * 4) + ASU2(x / 2, y / 2), v, mip, slice);
            SpdStoreIntermediateH(x * 2 + y / 2, y * 2, v);
        }
    }
#endif
}

void SpdDownsampleMip_4H(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    if (localInvocationIndex < 4)
    {
        // x 0 0 0 x 0 0 0
        // ...
        // 0 x 0 0 0 x 0 0
        AH4 v = SpdReduceIntermediateH(
            AU2(x * 8 + 0 + 0 + y * 2, y * 8 + 0),
            AU2(x * 8 + 4 + 0 + y * 2, y * 8 + 0),
            AU2(x * 8 + 0 + 1 + y * 2, y * 8 + 4),
            AU2(x * 8 + 4 + 1 + y * 2, y * 8 + 4)
        );
        SpdStoreH(ASU2(workGroupID.xy * 2) + ASU2(x, y), v, mip, slice);
        // store to LDS
        // x x x x 0 ...
        // 0 ...
        SpdStoreIntermediateH(x + y * 2, 0, v);
    }
#else
    if (localInvocationIndex < 16)
    {
        AH4 v = SpdLoadIntermediateH(x * 4 + y, y * 4);
        v = SpdReduceQuadH(v);
        // quad index 0 stores result
        if (localInvocationIndex % 4 == 0)
        {
            SpdStoreH(ASU2(workGroupID.xy * 2) + ASU2(x / 2, y / 2), v, mip, slice);
            SpdStoreIntermediateH(x / 2 + y, 0, v);
        }
    }
#endif
}

void SpdDownsampleMip_5H(AU2 workGroupID, AU1 localInvocationIndex, AU1 mip, AU1 slice)
{
#ifdef SPD_NO_WAVE_OPERATIONS
    if (localInvocationIndex < 1)
    {
        // x x x x 0 ...
        // 0 ...
        AH4 v = SpdReduceIntermediateH(
            AU2(0, 0),
            AU2(1, 0),
            AU2(2, 0),
            AU2(3, 0)
        );
        SpdStoreH(ASU2(workGroupID.xy), v, mip, slice);
    }
#else
    if (localInvocationIndex < 4)
    {
        AH4 v = SpdLoadIntermediateH(localInvocationIndex, 0);
        v = SpdReduceQuadH(v);
        // quad index 0 stores result
        if (localInvocationIndex % 4 == 0)
        {
            SpdStoreH(ASU2(workGroupID.xy), v, mip, slice);
        }
    }
#endif
}

void SpdDownsampleMips_6_7H(AU1 x, AU1 y, AU1 mips, AU1 slice)
{
    ASU2 tex = ASU2(x * 4 + 0, y * 4 + 0);
    ASU2 pix = ASU2(x * 2 + 0, y * 2 + 0);
    AH4 v0 = SpdReduceLoad4H(tex, slice);
    SpdStoreH(pix, v0, 6, slice);

    tex = ASU2(x * 4 + 2, y * 4 + 0);
    pix = ASU2(x * 2 + 1, y * 2 + 0);
    AH4 v1 = SpdReduceLoad4H(tex, slice);
    SpdStoreH(pix, v1, 6, slice);

    tex = ASU2(x * 4 + 0, y * 4 + 2);
    pix = ASU2(x * 2 + 0, y * 2 + 1);
    AH4 v2 = SpdReduceLoad4H(tex, slice);
    SpdStoreH(pix, v2, 6, slice);

    tex = ASU2(x * 4 + 2, y * 4 + 2);
    pix = ASU2(x * 2 + 1, y * 2 + 1);
    AH4 v3 = SpdReduceLoad4H(tex, slice);
    SpdStoreH(pix, v3, 6, slice);

    if (mips < 8) return;
    // no barrier needed, working on values only from the same thread

    AH4 v = SpdReduce4H(v0, v1, v2, v3);
    SpdStoreH(ASU2(x, y), v, 7, slice);
    SpdStoreIntermediateH(x, y, v);
}

void SpdDownsampleNextFourH(AU1 x, AU1 y, AU2 workGroupID, AU1 localInvocationIndex, AU1 baseMip, AU1 mips, AU1 slice)
{
    if (mips <= baseMip) return;
    SpdWorkgroupShuffleBarrier();
    SpdDownsampleMip_2H(x, y, workGroupID, localInvocationIndex, baseMip, slice);

    if (mips <= baseMip + 1) return;
    SpdWorkgroupShuffleBarrier();
    SpdDownsampleMip_3H(x, y, workGroupID, localInvocationIndex, baseMip + 1, slice);

    if (mips <= baseMip + 2) return;
    SpdWorkgroupShuffleBarrier();
    SpdDownsampleMip_4H(x, y, workGroupID, localInvocationIndex, baseMip + 2, slice);

    if (mips <= baseMip + 3) return;
    SpdWorkgroupShuffleBarrier();
    SpdDownsampleMip_5H(workGroupID, localInvocationIndex, baseMip + 3, slice);
}

void SpdDownsampleH(
    AU2 workGroupID,
    AU1 localInvocationIndex,
    AU1 mips,
    AU1 numWorkGroups,
    AU1 slice
) {
    AU2 sub_xy = ARmpRed8x8(localInvocationIndex % 64);
    AU1 x = sub_xy.x + 8 * ((localInvocationIndex >> 6) % 2);
    AU1 y = sub_xy.y + 8 * ((localInvocationIndex >> 7));

    SpdDownsampleMips_0_1H(x, y, workGroupID, localInvocationIndex, mips, slice);

    SpdDownsampleNextFourH(x, y, workGroupID, localInvocationIndex, 2, mips, slice);

    if (mips < 7) return;

    if (SpdExitWorkgroup(numWorkGroups, localInvocationIndex, slice)) return;

    SpdResetAtomicCounter(slice);

    // After mip 6 there is only a single workgroup left that downsamples the remaining up to 64x64 texels.
    SpdDownsampleMips_6_7H(x, y, mips, slice);

    SpdDownsampleNextFourH(x, y, AU2(0, 0), localInvocationIndex, 8, mips, slice);
}

#endif // #ifdef A_HALF

[numthreads(256,1,1)]
[RootSignature(
    "RootConstants(num32BitConstants=4, b5),"
    "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL),"
    "DescriptorTable(UAV(u1, numDescriptors=12), visibility=SHADER_VISIBILITY_ALL),"
    "UAV(u0),"
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
void main(uint3 WorkGroupId : SV_GroupID, uint LocalThreadIndex : SV_GroupIndex) {
#if !defined(A_HALF)
    SpdDownsample(AU2(WorkGroupId.xy), AU1(LocalThreadIndex),
        AU1(mips), AU1(numWorkGroups), AU1(WorkGroupId.z));
#else
    SpdDownsampleH(AU2(WorkGroupId.xy), AU1(LocalThreadIndex),
    AU1(mips), AU1(numWorkGroups), AU1(WorkGroupId.z));
#endif
}
