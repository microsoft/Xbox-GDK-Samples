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
#define __XBOX_ENABLE_WAVE32 1
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

// input output textures for the third pass (XeGTAO_Denoise)
#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
Texture2D<uint>             g_srcWorkingAOTerm      : register(t0);   // coming from previous pass
#else
Texture2D<float>            g_srcWorkingAOTerm      : register(t0);   // coming from previous pass
#endif
Texture2D<uint>             g_srcWorkingEdges       : register(t1);   // coming from previous pass
#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
RWTexture2D<uint>           g_outFinalAOTerm        : register(u0);   // final AO term - 'visibility + bent normals'
#else
RWTexture2D<float>          g_outFinalAOTerm        : register(u0);   // final AO term - just 'visibility'
#endif
SamplerState                g_samplerPointClamp     : register(s0);


#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
typedef lpfloat4 AOTermType;    // .xyz is bent normal, .w is visibility term
#else
typedef lpfloat  AOTermType;    // .x is visibility term
#endif

void XeGTAO_AddSample(AOTermType ssaoValue, lpfloat edgeValue, inout AOTermType sum, inout lpfloat sumWeight)
{
    lpfloat weight = edgeValue;

    sum += (weight * ssaoValue);
    sumWeight += weight;
}

#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
void XeGTAO_Output(uint2 pixCoord, RWTexture2D<uint> outputTexture, AOTermType outputValue, const uniform bool finalApply)
#else
void XeGTAO_Output(uint2 pixCoord, RWTexture2D<float> outputTexture, AOTermType outputValue, const uniform bool finalApply)
#endif
{
#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
    lpfloat visibility = outputValue.w * ((finalApply) ? ((lpfloat) XE_GTAO_OCCLUSION_TERM_SCALE) : 1);
    lpfloat3 bentNormal = normalize(outputValue.xyz);
    outputTexture[pixCoord.xy] = XeGTAO_EncodeVisibilityBentNormal(visibility, bentNormal);
#else
    outputValue *= (finalApply) ? ((lpfloat) XE_GTAO_OCCLUSION_TERM_SCALE) : 1;
    outputTexture[pixCoord.xy] = outputValue;
#endif
}

#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
void XeGTAO_DecodeGatherPartial(const uint4 packedValue, out AOTermType outDecoded[4])
#else
void XeGTAO_DecodeGatherPartial(const float4 packedValue, out AOTermType outDecoded[4])
#endif
{
    for (int i = 0; i < 4; i++)
#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
        XeGTAO_DecodeVisibilityBentNormal(packedValue[i], outDecoded[i].w, outDecoded[i].xyz);
#else
        outDecoded[i] = lpfloat(packedValue[i]);
#endif
}

#ifdef XE_GTAO_COMPUTE_BENT_NORMALS
void XeGTAO_Denoise(const uint2 pixCoordBase, const uniform bool finalApply)
#else
void XeGTAO_Denoise(const uint2 pixCoordBase, const uniform bool finalApply)
#endif
{
    const lpfloat blurAmount = (finalApply) ? ((lpfloat) g_GTAOConsts.DenoiseBlurBeta) : ((lpfloat) g_GTAOConsts.DenoiseBlurBeta / (lpfloat) 5.0);
    const lpfloat diagWeight = 0.85 * 0.5;

    // gather edge and visibility quads, used later
    const float2 gatherCenter = float2(pixCoordBase.x, pixCoordBase.y) * g_GTAOConsts.ViewportPixelSize;
    uint4 edgesQ0 = g_srcWorkingEdges.GatherRed(g_samplerPointClamp, gatherCenter, int2(0, 0));
    uint4 edgesQ1 = g_srcWorkingEdges.GatherRed(g_samplerPointClamp, gatherCenter, int2(2, 0));
    uint4 edgesQ2 = g_srcWorkingEdges.GatherRed(g_samplerPointClamp, gatherCenter, int2(0, 2));
    uint4 edgesQ3 = g_srcWorkingEdges.GatherRed(g_samplerPointClamp, gatherCenter, int2(2, 2));
    
    [unroll]
    for (int side = 0; side < 4; side++)
    {
        const int2 pixCoord = int2(pixCoordBase.x + (side & 0x1), pixCoordBase.y + (side >> 1));

        lpfloat4 edgesL_LRTB = XeGTAO_UnpackEdges((side == 0) ? (edgesQ0.x) : (side == 1) ? (edgesQ0.y) : (side == 2) ? edgesQ2.w : edgesQ2.z);
        lpfloat4 edgesT_LRTB = XeGTAO_UnpackEdges((side == 0) ? (edgesQ0.z) : (side == 1) ? (edgesQ1.w) : (side == 2) ? edgesQ0.y : edgesQ1.x);
        lpfloat4 edgesR_LRTB = XeGTAO_UnpackEdges((side == 0) ? (edgesQ1.x) : (side == 1) ? (edgesQ1.y) : (side == 2) ? edgesQ3.w : edgesQ3.z);
        lpfloat4 edgesB_LRTB = XeGTAO_UnpackEdges((side == 0) ? (edgesQ2.z) : (side == 1) ? (edgesQ3.w) : (side == 2) ? edgesQ2.y : edgesQ3.x);

        lpfloat4 edgesC_LRTB = XeGTAO_UnpackEdges((side == 0) ? (edgesQ0.y) : (side == 1) ? (edgesQ1.x) : (side == 2) ? edgesQ2.z : edgesQ3.w);

        // Edges aren't perfectly symmetrical: edge detection algorithm does not guarantee that a left edge on the right pixel will match the right edge on the left pixel (although
        // they will match in majority of cases). This line further enforces the symmetricity, creating a slightly sharper blur. Works real nice with TAA.
        edgesC_LRTB *= lpfloat4(edgesL_LRTB.y, edgesR_LRTB.x, edgesT_LRTB.w, edgesB_LRTB.z);

#if 1   // this allows some small amount of AO leaking from neighbours if there are 3 or 4 edges; this reduces both spatial and temporal aliasing
        const lpfloat leak_threshold = 2.5;
        const lpfloat leak_strength = 0.5;
        lpfloat edginess = (saturate(4.0 - leak_threshold - dot(edgesC_LRTB, 1.xxxx)) / (4 - leak_threshold)) * leak_strength;
        edgesC_LRTB = saturate(edgesC_LRTB + edginess);
#endif

#ifdef XE_GTAO_SHOW_EDGES
        g_outputDbgImage[pixCoord] = 1.0 - lpfloat4( edgesC_LRTB.x, edgesC_LRTB.y * 0.5 + edgesC_LRTB.w * 0.5, edgesC_LRTB.z, 1.0 );
        //g_outputDbgImage[pixCoord] = 1 - float4( edgesC_LRTB[side].z, edgesC_LRTB[side].w , 1, 0 );
        //g_outputDbgImage[pixCoord] = edginess.xxxx;
#endif

        // for diagonals; used by first and second pass
        lpfloat weightTL = diagWeight * (edgesC_LRTB.x * edgesL_LRTB.z + edgesC_LRTB.z * edgesT_LRTB.x);
        lpfloat weightTR = diagWeight * (edgesC_LRTB.z * edgesT_LRTB.y + edgesC_LRTB.y * edgesR_LRTB.z);
        lpfloat weightBL = diagWeight * (edgesC_LRTB.w * edgesB_LRTB.x + edgesC_LRTB.x * edgesL_LRTB.w);
        lpfloat weightBR = diagWeight * (edgesC_LRTB.y * edgesR_LRTB.w + edgesC_LRTB.w * edgesB_LRTB.y);

        // put these texture samples inside the unrolled loop because it allows the compiler to do better scheduling of the image samples
        AOTermType visQ0[4];
        XeGTAO_DecodeGatherPartial(g_srcWorkingAOTerm.GatherRed(g_samplerPointClamp, gatherCenter, int2(0, 0)), visQ0);
        AOTermType visQ1[4];
        XeGTAO_DecodeGatherPartial(g_srcWorkingAOTerm.GatherRed(g_samplerPointClamp, gatherCenter, int2(2, 0)), visQ1);
        AOTermType visQ2[4];
        XeGTAO_DecodeGatherPartial(g_srcWorkingAOTerm.GatherRed(g_samplerPointClamp, gatherCenter, int2(0, 2)), visQ2);
        AOTermType visQ3[4];
        XeGTAO_DecodeGatherPartial(g_srcWorkingAOTerm.GatherRed(g_samplerPointClamp, gatherCenter, int2(2, 2)), visQ3);

        // first pass
        AOTermType ssaoValue = (side == 0) ? (visQ0[1]) : (side == 1) ? (visQ1[0]) : (side == 2) ? visQ2[2] : visQ3[3];
        AOTermType ssaoValueL = (side == 0) ? (visQ0[0]) : (side == 1) ? (visQ0[1]) : (side == 2) ? visQ2[3] : visQ2[2];
        AOTermType ssaoValueT = (side == 0) ? (visQ0[2]) : (side == 1) ? (visQ1[3]) : (side == 2) ? visQ0[1] : visQ1[0];
        AOTermType ssaoValueR = (side == 0) ? (visQ1[0]) : (side == 1) ? (visQ1[1]) : (side == 2) ? visQ3[3] : visQ3[2];
        AOTermType ssaoValueB = (side == 0) ? (visQ2[2]) : (side == 1) ? (visQ3[3]) : (side == 2) ? visQ2[1] : visQ3[0];
        AOTermType ssaoValueTL = (side == 0) ? (visQ0[3]) : (side == 1) ? (visQ0[2]) : (side == 2) ? visQ0[0] : visQ0[1];
        AOTermType ssaoValueBR = (side == 0) ? (visQ3[3]) : (side == 1) ? (visQ3[2]) : (side == 2) ? visQ3[0] : visQ3[1];
        AOTermType ssaoValueTR = (side == 0) ? (visQ1[3]) : (side == 1) ? (visQ1[2]) : (side == 2) ? visQ1[0] : visQ1[1];
        AOTermType ssaoValueBL = (side == 0) ? (visQ2[3]) : (side == 1) ? (visQ2[2]) : (side == 2) ? visQ2[0] : visQ2[1];

        lpfloat sumWeight = blurAmount;
        AOTermType sum = ssaoValue * sumWeight;

        XeGTAO_AddSample(ssaoValueL, edgesC_LRTB.x, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueR, edgesC_LRTB.y, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueT, edgesC_LRTB.z, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueB, edgesC_LRTB.w, sum, sumWeight);

        XeGTAO_AddSample(ssaoValueTL, weightTL, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueTR, weightTR, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueBL, weightBL, sum, sumWeight);
        XeGTAO_AddSample(ssaoValueBR, weightBR, sum, sumWeight);

        AOTermType aoTerm = sum / sumWeight;

        XeGTAO_Output(pixCoord, g_outFinalAOTerm, aoTerm, finalApply);

#ifdef XE_GTAO_SHOW_BENT_NORMALS
        if (finalApply)
        {
            g_outputDbgImage[pixCoord] = float4( DisplayNormalSRGB( aoTerm.xyz /** aoTerm.www*/ ), 1 );
        }
#endif

    }
}
