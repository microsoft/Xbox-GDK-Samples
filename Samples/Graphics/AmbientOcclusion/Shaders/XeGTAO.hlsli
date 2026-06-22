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

#define __XBOX_DISABLE_UAV_WRITE_TO_UNALLOCATED_CHANNELS 1

#if defined( XE_GTAO_SHOW_NORMALS ) || defined( XE_GTAO_SHOW_EDGES ) || defined( XE_GTAO_SHOW_BENT_NORMALS )
RWTexture2D<float4>         g_outputDbgImage    : register( u2 );
#endif

#include "XeGTAO.h"

#define XE_GTAO_PI               	(3.1415926535897932384626433832795)
#define XE_GTAO_PI_HALF             (1.5707963267948966192313216916398)

#ifndef XE_GTAO_USE_HALF_FLOAT_PRECISION
#define XE_GTAO_USE_HALF_FLOAT_PRECISION 0
#endif

#if defined(XE_GTAO_FP32_DEPTHS) && XE_GTAO_USE_HALF_FLOAT_PRECISION
#error Using XE_GTAO_USE_HALF_FLOAT_PRECISION with 32bit depths is not supported yet unfortunately (it is possible to apply fp16 on parts not related to depth but this has not been done yet)
#endif 


#if (XE_GTAO_USE_HALF_FLOAT_PRECISION != 0)
#if 0 // old fp16 approach (<SM6.2)
    typedef min16float      lpfloat; 
    typedef min16float2     lpfloat2;
    typedef min16float3     lpfloat3;
    typedef min16float4     lpfloat4;
    typedef min16float3x3   lpfloat3x3;
#else // new fp16 approach (requires SM6.2 and -enable-16bit-types) - WARNING: perf degradation noticed on some HW, while the old (min16float) path is mostly at least a minor perf gain so this is more useful for quality testing
    typedef float16_t       lpfloat; 
    typedef float16_t2      lpfloat2;
    typedef float16_t3      lpfloat3;
    typedef float16_t4      lpfloat4;
    typedef float16_t3x3    lpfloat3x3;
#endif
#else
typedef float lpfloat;
typedef float2 lpfloat2;
typedef float3 lpfloat3;
typedef float4 lpfloat4;
typedef float3x3 lpfloat3x3;
#endif


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// R11G11B10_UNORM <-> float3
float3 XeGTAO_R11G11B10_UNORM_to_FLOAT3(uint packedInput)
{
    float3 unpackedOutput;
    unpackedOutput.x = (float) ((packedInput) & 0x000007ff) / 2047.0f;
    unpackedOutput.y = (float) ((packedInput >> 11) & 0x000007ff) / 2047.0f;
    unpackedOutput.z = (float) ((packedInput >> 22) & 0x000003ff) / 1023.0f;
    return unpackedOutput;
}
// 'unpackedInput' is float3 and not float3 on purpose as half float lacks precision for below!
uint XeGTAO_FLOAT3_to_R11G11B10_UNORM(float3 unpackedInput)
{
    uint packedOutput;
    packedOutput = ((uint(saturate(unpackedInput.x) * 2047 + 0.5f)) |
        (uint(saturate(unpackedInput.y) * 2047 + 0.5f) << 11) |
        (uint(saturate(unpackedInput.z) * 1023 + 0.5f) << 22));
    return packedOutput;
}
//
lpfloat4 XeGTAO_R8G8B8A8_UNORM_to_FLOAT4(uint packedInput)
{
    lpfloat4 unpackedOutput;
    unpackedOutput.x = (lpfloat) (packedInput & 0x000000ff) / (lpfloat) 255;
    unpackedOutput.y = (lpfloat) (((packedInput >> 8) & 0x000000ff)) / (lpfloat) 255;
    unpackedOutput.z = (lpfloat) (((packedInput >> 16) & 0x000000ff)) / (lpfloat) 255;
    unpackedOutput.w = (lpfloat) (packedInput >> 24) / (lpfloat) 255;
    return unpackedOutput;
}
//
uint XeGTAO_FLOAT4_to_R8G8B8A8_UNORM(lpfloat4 unpackedInput)
{
    return ((uint(saturate(unpackedInput.x) * (lpfloat) 255 + (lpfloat) 0.5)) |
            (uint(saturate(unpackedInput.y) * (lpfloat) 255 + (lpfloat) 0.5) << 8) |
            (uint(saturate(unpackedInput.z) * (lpfloat) 255 + (lpfloat) 0.5) << 16) |
            (uint(saturate(unpackedInput.w) * (lpfloat) 255 + (lpfloat) 0.5) << 24));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Inputs are screen XY and viewspace depth, output is viewspace position
float3 XeGTAO_ComputeViewspacePosition(const float2 screenPos, const float viewspaceDepth, const GTAOConstants consts)
{
    float3 ret;
    ret.xy = (consts.NDCToViewMul * screenPos.xy + consts.NDCToViewAdd) * viewspaceDepth;
    ret.z = viewspaceDepth;
    return ret;
}

float XeGTAO_ScreenSpaceToViewSpaceDepth(const float screenDepth, const GTAOConstants consts)
{
    float depthLinearizeMul = consts.DepthUnpackConsts.x;
    float depthLinearizeAdd = consts.DepthUnpackConsts.y;
    // Optimised version of "-cameraClipNear / (cameraClipFar - projDepth * (cameraClipFar - cameraClipNear)) * cameraClipFar"
    return depthLinearizeMul / (depthLinearizeAdd - screenDepth);
}

lpfloat4 XeGTAO_CalculateEdges(const lpfloat centerZ, const lpfloat leftZ, const lpfloat rightZ, const lpfloat topZ, const lpfloat bottomZ)
{ 
    lpfloat2 edgesRB = lpfloat2(rightZ, bottomZ) - centerZ;
    lpfloat2 edgesLT = lpfloat2(leftZ, topZ) - centerZ;
    lpfloat2 slope = (edgesRB - edgesLT) * 0.5;
    lpfloat2 edgesSlopeAdjustedRB = edgesRB - slope;
    lpfloat2 edgesSlopeAdjustedLT = edgesLT + slope;
    lpfloat4 edgesLRTB = lpfloat4(
        min(abs(edgesLT.x), abs(edgesSlopeAdjustedLT.x)),
        min(abs(edgesRB.x), abs(edgesSlopeAdjustedRB.x)),
        min(abs(edgesLT.y), abs(edgesSlopeAdjustedLT.y)),
        min(abs(edgesRB.y), abs(edgesSlopeAdjustedRB.y)));

    return lpfloat4(saturate((1.25 - edgesLRTB / (centerZ * 0.011))));
}

// packing/unpacking for edges; 2 bits per edge mean 4 gradient values (0, 0.33, 0.66, 1) for smoother transitions!
uint XeGTAO_PackEdges(lpfloat4 edgesLRTB)
{
    // integer version:
     edgesLRTB = saturate(edgesLRTB) * 2.9.xxxx + 0.5.xxxx;
     return (((uint)edgesLRTB.x) << 6) + (((uint)edgesLRTB.y) << 4) + (((uint)edgesLRTB.z) << 2) + (((uint)edgesLRTB.w));
    // 
    // optimized, should be same as above
    //edgesLRTB = round(saturate(edgesLRTB) * 2.9);
    //return dot(edgesLRTB, lpfloat4(64.0 / 255.0, 16.0 / 255.0, 4.0 / 255.0, 1.0 / 255.0));
}

lpfloat4 XeGTAO_UnpackEdges(uint _packedVal)
{
    uint packedVal = _packedVal;
    lpfloat4 edgesLRTB;
    edgesLRTB.x = lpfloat((packedVal >> 6) & 0x03) / 3.0; // there's really no need for mask (as it's an 8 bit input) but I'll leave it in so it doesn't cause any trouble in the future
    edgesLRTB.y = lpfloat((packedVal >> 4) & 0x03) / 3.0;
    edgesLRTB.z = lpfloat((packedVal >> 2) & 0x03) / 3.0;
    edgesLRTB.w = lpfloat((packedVal >> 0) & 0x03) / 3.0;

    return edgesLRTB;
}

float3 XeGTAO_CalculateNormal(const float4 edgesLRTB, float3 pixCenterPos, float3 pixLPos, float3 pixRPos, float3 pixTPos, float3 pixBPos)
{
    // Get this pixel's viewspace normal
    float4 acceptedNormals = saturate(float4(edgesLRTB.x * edgesLRTB.z, edgesLRTB.z * edgesLRTB.y, edgesLRTB.y * edgesLRTB.w, edgesLRTB.w * edgesLRTB.x) + 0.01);

    pixLPos = normalize(pixLPos - pixCenterPos);
    pixRPos = normalize(pixRPos - pixCenterPos);
    pixTPos = normalize(pixTPos - pixCenterPos);
    pixBPos = normalize(pixBPos - pixCenterPos);

    float3 pixelNormal = acceptedNormals.x * cross(pixLPos, pixTPos) +
                        +acceptedNormals.y * cross(pixTPos, pixRPos) +
                        +acceptedNormals.z * cross(pixRPos, pixBPos) +
                        +acceptedNormals.w * cross(pixBPos, pixLPos);
    pixelNormal = normalize(pixelNormal);

    return pixelNormal;
}

float3 DisplayNormalSRGB(float3 normal)
{
    return pow(abs(normal * 0.5 + 0.5), 2.2);
}

// http://h14s.p5r.org/2012/09/0x5f3759df.html, [Drobot2014a] Low Level Optimizations for GCN, https://blog.selfshadow.com/publications/s2016-shading-course/activision/s2016_pbs_activision_occlusion.pdf slide 63
lpfloat XeGTAO_FastSqrt(lpfloat x)
{
    // It is faster to use sqrt than the approximation on Scarlett
#ifdef __XBOX_SCARLETT
    return sqrt(x);
#else
    return (lpfloat) (asfloat(0x1fbd1df5 + (asint(x) >> 1)));
#endif
}
// input [-1, 1] and output [0, PI], from https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
lpfloat XeGTAO_FastACos(lpfloat inX)
{
    const lpfloat PI = 3.141593;
    const lpfloat HALF_PI = 1.570796;
    lpfloat x = abs(inX);
    lpfloat res = -0.156583 * x + HALF_PI;
    res *= XeGTAO_FastSqrt(1.0 - x);
    return (inX >= 0) ? res : PI - res;
}

// http://h14s.p5r.org/2012/09/0x5f3759df.html, [Drobot2014a] Low Level Optimizations for GCN, https://blog.selfshadow.com/publications/s2016-shading-course/activision/s2016_pbs_activision_occlusion.pdf slide 63
lpfloat2 XeGTAO_FastSqrt(lpfloat2 x)
{
    // It is faster to use sqrt than the approximation on Scarlett
#ifdef __XBOX_SCARLETT
    return sqrt(x);
#else
    return (lpfloat2) (asfloat(0x1fbd1df5 + (asint(x) >> 1)));
#endif
}

// input [-1, 1] and output [0, PI], from https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
lpfloat2 XeGTAO_FastACos(lpfloat2 inX)
{
    const lpfloat PI = 3.141593;
    const lpfloat HALF_PI = 1.570796;
    lpfloat2 x = abs(inX);
    lpfloat2 res = -0.156583 * x + HALF_PI;
    res *= XeGTAO_FastSqrt(1.0 - x);
    return select(inX >= 0, res, PI - res);
}

uint XeGTAO_EncodeVisibilityBentNormal(lpfloat visibility, lpfloat3 bentNormal)
{
    return XeGTAO_FLOAT4_to_R8G8B8A8_UNORM(lpfloat4(bentNormal * 0.5 + 0.5, visibility));
}

void XeGTAO_DecodeVisibilityBentNormal(const uint packedValue, out lpfloat visibility, out lpfloat3 bentNormal)
{
    lpfloat4 decoded = XeGTAO_R8G8B8A8_UNORM_to_FLOAT4(packedValue);
    bentNormal = decoded.xyz * 2.0.xxx - 1.0.xxx; // could normalize - don't want to since it's done so many times, better to do it at the final step only
    visibility = decoded.w;
}
