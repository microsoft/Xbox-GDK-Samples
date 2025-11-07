//--------------------------------------------------------------------------------------
// TemporalResolve.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define HLSL_INCLUDE
#include "../SharedDataTypes.h"
#include "RootSignatures.hlsli"

#define     VARIANCE_INTERSECTION_MAX_T     100

float2 GetMotionVector(uint2 DTid);
float4 SampleHistoryBuffer(float2 previousFrameScreenSpace);
float4 NeighbourClamping(in float4 previousColor, in float3 minAABB, in float3 maxAABB, in float3 m1, in float3 m2, in float3 currentColor);
float4 ClipAABB(in float4 historyColor, in float3 currentColor, in float3 minColor, in float3 maxColor);

ConstantBuffer<TemporalResolveCB> constants : register(b0);

Texture2D<float4> colorBuffer               : register (t0);
Texture2D<float> currentDepth               : register (t1);
Texture2D<float4> historyBuffer             : register (t2);
Texture2D<float2> motionVectors             : register (t3);

RWTexture2D<float4> outResolved             : register (u0);

sampler linearSampler                       : register (s0);

// TAA accumulation weight.
static float temporalWeight = 0.9f;

// Entry point.
[RootSignature(TemporalResolveRS)]
[numthreads(TEMPORAL_RESOLVE_THREAD_X, TEMPORAL_RESOLVE_THREAD_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint2 GTid : SV_GroupThreadId)
{
    // Sample current color buffer.
    float4 currentFrameColor = colorBuffer.Load(uint3(DTid.xy, 0u));

    // Early exit if we are not using TAA.
    uint use_TAA = (constants.flags & (uint)TAA_FLAG_ENABLE_TAA);
    if (use_TAA == 0u)
    {
        outResolved[DTid.xy] = currentFrameColor;
        return;
    }

    float3 minAABB = min(1.0f, currentFrameColor.xyz);
    float3 maxAABB = max(0.0f, currentFrameColor.xyz);
    float3 moment1 = currentFrameColor.xyz;
    float3 moment2 = currentFrameColor.xyz * currentFrameColor.xyz;

    // Gather the information used for the color clipping algorithms later on.
    [unroll]
    for (int i = -1; i <= 1; ++i)
    {
        [unroll]
        for (int j = -1; j <= 1; ++j)
        {
            if (i == 0 && j == 0) continue;
            float3 neighbour = colorBuffer.Load(uint3(DTid.x + i, DTid.y + j, 0u)).rgb;
            minAABB = min(minAABB, neighbour);
            maxAABB = max(maxAABB, neighbour);
            moment1 += neighbour;
            moment2 += (neighbour * neighbour);
        }
    }

    // Motion vectors.
    float2 motionVector = GetMotionVector(DTid.xy);

    float2 currentPixelCenter = float2(DTid.xy) + 0.5f;
    float2 finalScreenCoordsForPreviousFrame = currentPixelCenter + motionVector;

    // Sample History buffer.
    float4 historyBufferColor = SampleHistoryBuffer(finalScreenCoordsForPreviousFrame);

    // Color clamping. This gets rid of ghosting.
    historyBufferColor = NeighbourClamping(historyBufferColor, minAABB, maxAABB, moment1, moment2, currentFrameColor.xyz);

    // Weight the current and history contributions with temporalWeight.
    float4 accumulatedContribution = (temporalWeight) * historyBufferColor;
    float4 currentFrameContribution = (1.0f - temporalWeight) * currentFrameColor;
    float4 finalColor = accumulatedContribution + currentFrameContribution;

    // Write into the final texture.
    outResolved[DTid.xy] = finalColor;
}

float2 GetMotionVector(uint2 DTid)
{
    float2 motionVector = 0.0f;
    float2 uv = (float2(DTid) + 0.5f) * constants.resolution.zw;

    uint dilationNone = (constants.flags & (uint)TAA_FLAG_USE_DILATION_NONE);
    uint dilationClosestDepth = (constants.flags & (uint)TAA_FLAG_USE_DILATION_CLOSEST_DEPTH);
    uint dilationGreatesVelocity = (constants.flags & (uint)TAA_FLAG_USE_DILATION_GREATEST_VELOCITY);

    if (dilationNone != 0u)
    {
        motionVector = motionVectors.SampleLevel(linearSampler, uv, 0);
    }
    else if (dilationClosestDepth != 0) // Closest depth.
    {
        float closestDepth = 100.0f;
        float2 closestUVOffset = 0.0f;
        for (int j = -1; j <= 1; ++j)
        {
            for (int i = -1; i <= 1; ++i)
            {
                float2 uvOffset = float2(i, j) * constants.resolution.zw;
                float neighborDepth = currentDepth.SampleLevel(linearSampler, uv + uvOffset, 0);
                if (neighborDepth < closestDepth)
                {
                    closestUVOffset = uvOffset;
                    closestDepth = neighborDepth;
                }
            }
        }

        motionVector = motionVectors.SampleLevel(linearSampler, uv + closestUVOffset, 0);
    }
    else // Greatest velocity.
    {
        float maxVelocitySq = 0.0f;
        float2 chosenVelocity = 0.0f;
        for (int j = -1; j <= 1; ++j)
        {
            for (int i = -1; i <= 1; ++i)
            {
                float2 uvOffset = float2(i, j) * constants.resolution.zw;
                float2 neighborVelocity = motionVectors.SampleLevel(linearSampler, uv + uvOffset, 0);
                float velocityMagnitudeSq = dot(neighborVelocity, neighborVelocity);
                if (velocityMagnitudeSq > maxVelocitySq)
                {
                    maxVelocitySq = velocityMagnitudeSq;
                    motionVector = neighborVelocity;
                }
            }
        }
    }

    return motionVector;
}

float4 SampleHistoryBuffer(float2 previousFrameScreenSpace)
{
    float4 historyBufferColor = 0.0f;

    uint useBicubic5taps = (constants.flags & (uint)TAA_FLAG_USE_BICUBIC_FILTER_5TAPS);
    uint useBicubic9taps = (constants.flags & (uint)TAA_FLAG_USE_BICUBIC_FILTER_9TAPS);

    // Use Bilinear.
    if (useBicubic9taps == 0u && useBicubic5taps == 0u)
    {
        float2 uv = float2(previousFrameScreenSpace.xy) * constants.resolution.zw;

        historyBufferColor = historyBuffer.SampleLevel(linearSampler, uv, 0);
    }
    else // Use Bicubic.
    {
        float2 screenCoordsFloor = floor(previousFrameScreenSpace);
        float2 fractionPart = previousFrameScreenSpace - screenCoordsFloor;

        float2 offset = 0.0f;
        if (fractionPart.x < 0.5f)
        {
            offset.x = -1.0f;
        }
        if (fractionPart.y < 0.5f)
        {
            offset.y = -1.0f;
        }

        float2 t1 = frac(fractionPart + 0.5f);
        float2 t2 = t1 * t1;
        float2 t3 = t2 * t1;

        float2 Q0 = (-t3 + 2.0f * t2 - t1) * 0.5f;
        float2 Q1 = (3.0f * t3 - 5.0f * t2 + 2.0f) * 0.5f;
        float2 Q2 = (-3.0f * t3 + 4.0f * t2 + t1) * 0.5f;
        float2 Q3 = (t3 - t2) * 0.5f;

        float2 s12 = Q1 + Q2;
        float2 f12 = Q2 / (Q1 + Q2);

        // Center pixel coord for pixel in position [1,1] in zero indexed 4x4 tile.
        // Corrected by offset, based on the fractional position of the reprojected position.
        float2 tc1 = (screenCoordsFloor + 0.5f) + offset;

        // Pixel center coords for upper left (tc0), bottom right (tc3), and bilinear evaluation (tc12).
        float2 tc0 = tc1 - 1.0f;
        float2 tc3 = tc1 + 2.0f;
        float2 tc12 = tc1 + f12;

        // UV space coords of the previous positions.
        float2 uv0 = (tc0) * constants.resolution.zw;
        float2 uv3 = (tc3) * constants.resolution.zw;
        float2 uv12 = (tc12) * constants.resolution.zw;

        if (useBicubic9taps != 0u)
        {
            float3 A0 = historyBuffer.SampleLevel(linearSampler, float2(uv0.x, uv0.y), 0).rgb;   // Upper-Left    (point sample)
            float3 A1 = historyBuffer.SampleLevel(linearSampler, float2(uv12.x, uv0.y), 0).rgb;  // Upper-Center  (bilinear, two samples horizontal)
            float3 A2 = historyBuffer.SampleLevel(linearSampler, float2(uv3.x, uv0.y), 0).rgb;   // Upper-Right   (point sample)

            float3 A3 = historyBuffer.SampleLevel(linearSampler, float2(uv0.x, uv12.y), 0).rgb;  // Center-Left   (bilinear, two samples vertical)
            float3 A4 = historyBuffer.SampleLevel(linearSampler, float2(uv12.x, uv12.y), 0).rgb; // Center-Center (Bilinear, four samples)
            float3 A5 = historyBuffer.SampleLevel(linearSampler, float2(uv3.x, uv12.y), 0).rgb;  // Center-Right  (bilinear, two samples vertical)

            float3 A6 = historyBuffer.SampleLevel(linearSampler, float2(uv0.x, uv3.y), 0).rgb;   // Bottom-Left   (point sample)
            float3 A7 = historyBuffer.SampleLevel(linearSampler, float2(uv12.x, uv3.y), 0).rgb;  // Bottom-Center (bilinear, two samples horizontal)
            float3 A8 = historyBuffer.SampleLevel(linearSampler, float2(uv3.x, uv3.y), 0).rgb;   // Bottom-Right  (point sample)

            float3 color =
                (A0 * (Q0.x) + A1 * (s12.x) + A2 * (Q3.x)) * (Q0.y) +
                (A3 * (Q0.x) + A4 * (s12.x) + A5 * (Q3.x)) * (s12.y) +
                (A6 * (Q0.x) + A7 * (s12.x) + A8 * (Q3.x)) * (Q3.y);

            historyBufferColor = float4(color, 1.0f);
        }
        else
        {
            float3 ATC = historyBuffer.SampleLevel(linearSampler, float2(uv12.x, uv0.y), 0).rgb;  // Upper-Center  (bilinear, two samples horizontal)
            float3 ACL = historyBuffer.SampleLevel(linearSampler, float2(uv0.x, uv12.y), 0).rgb;  // Center-Left   (bilinear, two samples vertical)
            float3 ACC = historyBuffer.SampleLevel(linearSampler, float2(uv12.x, uv12.y), 0).rgb; // Center-Center (Bilinear, four samples)
            float3 ACR = historyBuffer.SampleLevel(linearSampler, float2(uv3.x, uv12.y), 0).rgb;  // Center-Right  (bilinear, two samples vertical)
            float3 ABC = historyBuffer.SampleLevel(linearSampler, float2(uv12.x, uv3.y), 0).rgb;  // Bottom-Center (bilinear, two samples horizontal)

            float3 color =
                ((0.5f * (ACL + ATC)) * (Q0.x) + ATC * (s12.x) + (0.5f * (ACR + ATC)) * (Q3.x)) * (Q0.y) +
                (ACL * (Q0.x) + ACC * (s12.x) + ACR * (Q3.x)) * (s12.y) +
                ((0.5f * (ACL + ABC)) * (Q0.x) + ABC * (s12.x) + (0.5f * (ACR + ABC)) * (Q3.x)) * (Q3.y);

            historyBufferColor = float4(color, 1.0f);
        }
    }

    return historyBufferColor;
}

float4 NeighbourClamping(in float4 previousColor, in float3 minAABB, in float3 maxAABB, in float3 m1, in float3 m2, in float3 currentColor)
{
    uint modeRGBClamp = (constants.flags & (uint)TAA_FLAG_USE_RGB_CLAMP);
    uint modeRGBClipping = (constants.flags & (uint)TAA_FLAG_USE_RGB_CLIPPING);
    uint modeVarianceClipping = (constants.flags & (uint)TAA_FLAG_USE_VARIANCE_CLIPPING);

    if (modeRGBClamp != 0u)
    {
        previousColor = float4(clamp(previousColor.xyz, minAABB, maxAABB), 1.0f);
    }
    else if (modeRGBClipping != 0u)
    {
        previousColor = ClipAABB(previousColor, currentColor, minAABB, maxAABB);
    }
    else if (modeVarianceClipping != 0u)
    {
        float3 mean = m1 * constants.varianceNeighbourCountRCP;
        float3 sigma = sqrt((m2 * constants.varianceNeighbourCountRCP) - (mean * mean));
        float3 gamma = 1.0f;
        float3 maxColor = mean + gamma * sigma;
        float3 minColor = mean - gamma * sigma;
        previousColor = float4(clamp(previousColor.rgb, minColor, maxColor), 1.0f);
    }

    // If no mode was selected, then this returns the previous color with no change.
    return previousColor;
}

float4 ClipAABB(in float4 historyColor, in float3 currentColor, in float3 minColor, in float3 maxColor)
{
    const float3 direction = currentColor - historyColor.xyz;

    float3 AABBCenter = 0.5f * (minColor + maxColor);
    float3 AABBExtents = AABBCenter - minColor;

    // calculate intersection for the closest slabs from the center of the AABB in HistoryColour direction.
    const float3 intersection = ((AABBCenter - sign(direction) * AABBExtents) - historyColor.xyz) / direction;

    // clip unexpected T values.
    const float3 possibleT = select(intersection > 0.0f.xxx, intersection, VARIANCE_INTERSECTION_MAX_T + 1.0f);
    const float t = min(VARIANCE_INTERSECTION_MAX_T, min(possibleT.x, min(possibleT.y, possibleT.z)));

    // final history color.
    return float4(select(t < VARIANCE_INTERSECTION_MAX_T, historyColor.xyz + direction * t, historyColor.xyz), 1.0f);
}
