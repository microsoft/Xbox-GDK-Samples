//--------------------------------------------------------------------------------------
// TemporalHelper.hlsli
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

float4 ProjToViewSpace(float3 projSpacePt, float4x4 invProj)
{
    float4 temp = mul(float4(projSpacePt, 1.0f), invProj);
    temp = temp / temp.w;
    return temp;
}

float4 ViewToWorldSpace(float3 viewSpacePt, float4x4 invView)
{
    return mul(float4(viewSpacePt, 1.0f), invView);
}

float4 getWorldSpaceFromPixelCoordAndDepth(in float2 pixelCoord, in float4x4 invView, in float4x4 invProj, in float2 rcpResolution, in float ndcDepth)
{
    // First, we need to go from screen space to NDC. We start from pixel centers (dtid + 0.5f).
    float3 ndcCoords = 0.0f;

    // Flipping Y (since origin is at top left in directx)
    ndcCoords.x = (pixelCoord.x) * rcpResolution.x * 2.0f - 1.0f;
    ndcCoords.y = (pixelCoord.y) * rcpResolution.y * -2.0f + 1.0f;
    ndcCoords.z = ndcDepth;

    // NDC to view space transform.
    float4 viewSpaceCoord = ProjToViewSpace(ndcCoords, invProj);

    // View to world space transform.
    float4 worldSpaceCoord = ViewToWorldSpace(viewSpaceCoord.xyz, invView);

    return worldSpaceCoord;
}
