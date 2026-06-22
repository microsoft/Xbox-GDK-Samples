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
    GTAOConstants       g_GTAOConsts;
}

#include "XeGTAO.hlsli"
#include "GTAORS.hlsli"

Texture2D<float>        g_srcRawDepth           : register(t0);   // source depth buffer data (in NDC space in DirectX)
RWTexture2D<float3>     g_outNormalmap          : register(u0);   // output viewspace normals if generating from depth
SamplerState            g_samplerPointClamp     : register(s0);

groupshared float depths[10][10];
groupshared float viewPosX[10][10];
groupshared float viewPosY[10][10];
#define BORDER_PAD 1
// Generic viewspace normal generate pass
void XeGTAO_ComputeViewspaceNormal(const uint2 pixCoord, const uint2 gtid, const uint gidx, const GTAOConstants consts, Texture2D<float> sourceNDCDepth, SamplerState depthSampler, RWTexture2D<float3> outNormalmap)
{
    float2 normalizedScreenPos = (pixCoord + 0.5.xx) * consts.ViewportPixelSize;
    uint2 pixCoordBottom = pixCoord + uint2(0, 1);
    float2 normalizedScreenPosBottom = (pixCoordBottom + 0.5.xx) * consts.ViewportPixelSize;

    float centerTop = sourceNDCDepth.Load(uint3(pixCoord, 0));
    float centerBottom = sourceNDCDepth.Load(uint3(pixCoordBottom, 0));

    // viewspace Z at the center
    float viewspaceZTop = XeGTAO_ScreenSpaceToViewSpaceDepth(centerTop, consts); //sourceViewspaceDepth.SampleLevel( depthSampler, normalizedScreenPos, 0 ).x; 
    float viewspaceZBottom = XeGTAO_ScreenSpaceToViewSpaceDepth(centerBottom, consts);
    depths[gtid.x + BORDER_PAD][gtid.y + BORDER_PAD] = viewspaceZTop;
    depths[gtid.x + BORDER_PAD][gtid.y + 1 + BORDER_PAD] = viewspaceZBottom;
    float3 CENTER = XeGTAO_ComputeViewspacePosition(normalizedScreenPos, viewspaceZTop, consts);
    float3 BOTTOM = XeGTAO_ComputeViewspacePosition(normalizedScreenPosBottom, viewspaceZBottom, consts);
    viewPosX[gtid.x + BORDER_PAD][gtid.y + BORDER_PAD] = CENTER.x;
    viewPosY[gtid.x + BORDER_PAD][gtid.y + BORDER_PAD] = CENTER.y;
    viewPosX[gtid.x + BORDER_PAD][gtid.y + 1 + BORDER_PAD] = BOTTOM.x;
    viewPosY[gtid.x + BORDER_PAD][gtid.y + 1 + BORDER_PAD] = BOTTOM.y;
      
    // each thread fills in one border pixel
    { 
        uint2 corner = WaveReadLaneFirst(pixCoord);
        float2 cornerScreenPos = WaveReadLaneFirst(normalizedScreenPos);
        int zeroToSeven = gtid.x;
        int negOneOrEight = (gtid.y & 0x2) ? 8 : -1;
        bool xOrY = (gtid.y >> 0x2);
        int2 offset = int2(xOrY ? zeroToSeven : negOneOrEight, xOrY ? negOneOrEight : zeroToSeven);

        float border = sourceNDCDepth.Load(uint3(corner + offset, 0));
        float borderZ = XeGTAO_ScreenSpaceToViewSpaceDepth(border, consts);
        depths[offset.x + BORDER_PAD][offset.y + BORDER_PAD] = borderZ;
        float3 viewPos = XeGTAO_ComputeViewspacePosition(cornerScreenPos + (float2) offset * consts.ViewportPixelSize, borderZ, consts);
        viewPosX[offset.x + BORDER_PAD][offset.y + BORDER_PAD] = viewPos.x;
        viewPosY[offset.x + BORDER_PAD][offset.y + BORDER_PAD] = viewPos.y;
    }

    GroupMemoryBarrierWithGroupSync();

    // viewspace Zs left top right bottom
    const float pixLZ = depths[gtid.x - 1 + BORDER_PAD][gtid.y + BORDER_PAD];
    const float pixTZ = depths[gtid.x + BORDER_PAD][gtid.y - 1 + BORDER_PAD];
    const float pixRZ = depths[gtid.x + 1 + BORDER_PAD][gtid.y + BORDER_PAD];
    const float pixBZ = viewspaceZBottom;
    const float pixRZBottom = depths[gtid.x + 1 + BORDER_PAD][gtid.y + 1 + BORDER_PAD];
    const float pixLZBottom = depths[gtid.x - 1 + BORDER_PAD][gtid.y + 1 + BORDER_PAD];
    const float pixBZBottom = depths[gtid.x + BORDER_PAD][gtid.y + 2 + BORDER_PAD];

    lpfloat4 edgesLRTB = XeGTAO_CalculateEdges((lpfloat) viewspaceZTop, (lpfloat) pixLZ, (lpfloat) pixRZ, (lpfloat) pixTZ, (lpfloat) pixBZ);
    lpfloat4 edgesLRTBBottom = XeGTAO_CalculateEdges((lpfloat) viewspaceZBottom, (lpfloat) pixLZBottom, (lpfloat) pixRZBottom, (lpfloat) viewspaceZTop, (lpfloat) pixBZBottom);

    float3 LEFT = float3(viewPosX[gtid.x - 1 + BORDER_PAD][gtid.y + BORDER_PAD], viewPosY[gtid.x - 1 + BORDER_PAD][gtid.y + BORDER_PAD], pixLZ);
    float3 RIGHT = float3(viewPosX[gtid.x + 1 + BORDER_PAD][gtid.y + BORDER_PAD], viewPosY[gtid.x + 1 + BORDER_PAD][gtid.y + BORDER_PAD], pixRZ);
    float3 TOP = float3(viewPosX[gtid.x + BORDER_PAD][gtid.y - 1 + BORDER_PAD], viewPosY[gtid.x + BORDER_PAD][gtid.y - 1 + BORDER_PAD], pixTZ);

    float3 viewSpaceNormal = XeGTAO_CalculateNormal(edgesLRTB, CENTER, LEFT, RIGHT, TOP, BOTTOM);
    outNormalmap[pixCoord] = saturate(viewSpaceNormal * 0.5 + 0.5);

    float3 LEFTBOTTOM = float3(viewPosX[gtid.x - 1 + BORDER_PAD][gtid.y + 1 + BORDER_PAD], viewPosY[gtid.x - 1 + BORDER_PAD][gtid.y + 1 + BORDER_PAD], pixLZBottom);
    float3 RIGHTBOTTOM = float3(viewPosX[gtid.x + 1 + BORDER_PAD][gtid.y + 1 + BORDER_PAD], viewPosY[gtid.x + 1 + BORDER_PAD][gtid.y + 1 + BORDER_PAD], pixRZBottom);
    float3 BOTTOMBOTTOM = float3(viewPosX[gtid.x + BORDER_PAD][gtid.y + 2 + BORDER_PAD], viewPosY[gtid.x + BORDER_PAD][gtid.y + 2 + BORDER_PAD], pixBZBottom);
    float3 viewSpaceNormalBottom = XeGTAO_CalculateNormal(edgesLRTBBottom, BOTTOM, LEFTBOTTOM, RIGHTBOTTOM, CENTER, BOTTOMBOTTOM);
    outNormalmap[pixCoordBottom] = saturate(viewSpaceNormalBottom * 0.5 + 0.5);
}

// Optional screen space viewspace normals from depth generation
[RootSignature(GTAO_RootSig)]
[numthreads(XE_GTAO_NUMTHREADS_X, XE_GTAO_NUMTHREADS_Y_WAVE32, 1)]
void main(const uint2 pixCoord : SV_DispatchThreadID, uint2 gtid : SV_GroupThreadID, uint gidx : SV_GroupIndex)
{
    XeGTAO_ComputeViewspaceNormal(pixCoord * uint2(1, 2), gtid * uint2(1, 2), gidx, g_GTAOConsts, g_srcRawDepth, g_samplerPointClamp, g_outNormalmap);
}
