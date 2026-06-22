//--------------------------------------------------------------------------------------
// TranslucentMLAB_ROV_PS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define USE_HLSL
#include "SharedDefinitions.h"
#include "HLSLCommon.hlsli"

// To test differences in performance from activating ROVs.
#define USE_ROV

// Descriptors.
ConstantBuffer<ObjectConstants> objCts                      : register(b1);
ConstantBuffer<BlendPassConstants> cts                      : register(b2);

Texture2D<float4> diffuse                                   : register(t0);

#ifdef USE_ROV
RasterizerOrderedTexture2D<uint> clearMask                  : register(u0);
RasterizerOrderedStructuredBuffer<NodeFragments> nodeList   : register(u1);
#else
RWTexture2D<uint> clearMask                                 : register(u0);
RWStructuredBuffer<NodeFragments> nodeList                  : register(u1);
#endif

sampler texSampler                                          : register(s0);

[earlydepthstencil]
[RootSignature(MAIN_RS)]
void main(Interpolators IN)
{
    // Get current pixel's coordinate
    uint2 pixelCoords = uint2(IN.position.x, IN.position.y);

    // Array (to be initialized) to store the nodes.
    TransparentFragment fragments[NODE_COUNT_MLAB];

    // Index into the resource with the node elements
    uint flattenedIndex = pixelCoords.x + pixelCoords.y * cts.resolution.x;

    float4 color = float4(objCts.diffuseColor, TRANSLUCENT_ALPHA);

    // Create the entry for the current pixel.
    uint entryColor = PackRGBA(ToRGBE(float4(color.rgb * color.w, 1.0f)));
    float entryTransmission = 1.0f - color.w;
    float normalizedDepth = IN.viewDepth / cts.cameraFar;
    uint entryDepth = uint(normalizedDepth * DEPTH_MAX);

    uint clear = clearMask[pixelCoords];

    if (clear == 0u)
    {
        clearMask[pixelCoords] = 1u;

        fragments[0].color = entryColor;
        fragments[0].transmission = (uint)(entryTransmission * TRANSMISSION_MAX);
        fragments[0].depth = entryDepth;
        
        for (int i = 1; i < NODE_COUNT_MLAB; ++i)
        {
            fragments[i].color = 0;
            fragments[i].transmission = TRANSMISSION_MAX;
            fragments[i].depth = DEPTH_MAX;
        }
    }
    else
    {
        // split into arrays
        uint colors[NODE_COUNT_MLAB + 1];
        float transmissions[NODE_COUNT_MLAB + 1];
        uint depths[NODE_COUNT_MLAB + 1];
        
        fragments = nodeList[flattenedIndex].frags;

        for (int i = 0; i < NODE_COUNT_MLAB; ++i)
        {
            colors[i]        = fragments[i].color;
            depths[i]        = fragments[i].depth;
            transmissions[i] = (fragments[i].transmission / (float)TRANSMISSION_MAX);
        }

        // Find correct spot for new fragment, and push the rest
        uint insertPos = 0;
        float prevTransmission = 1.0f;

        for (int i = 0; i < NODE_COUNT_MLAB; ++i)
        {
            if (entryDepth > depths[i])
            {
                insertPos++;
                prevTransmission = transmissions[i];
            }
        }

        // Push everything to make room.
        for (int i = NODE_COUNT_MLAB - 1; i >= 0; --i)
        {
            colors[i + 1] = colors[i];
            depths[i + 1] = depths[i];
            transmissions[i + 1] = transmissions[i] * entryTransmission;

            if (i == insertPos) break;
        }

        colors[insertPos]        = entryColor;
        depths[insertPos]        = entryDepth;
        transmissions[insertPos] = entryTransmission * prevTransmission;

        // Fuse last two elements.
        if (depths[NODE_COUNT_MLAB] != DEPTH_MAX)
        {
            // Mix these two colors
            float3 color1 = FromRGBE(UnpackRGBA(colors[NODE_COUNT_MLAB])).rgb;
            float3 color2 = FromRGBE(UnpackRGBA(colors[NODE_COUNT_MLAB - 1])).rgb;

            color2 = color2 + color1 * transmissions[NODE_COUNT_MLAB - 1] * rcp(transmissions[NODE_COUNT_MLAB - 2]);

            colors[NODE_COUNT_MLAB - 1]          = PackRGBA(ToRGBE(float4(color2.rgb, 1.0f)));
            depths[NODE_COUNT_MLAB - 1]          = depths[NODE_COUNT_MLAB - 1];
            transmissions[NODE_COUNT_MLAB - 1]   = transmissions[NODE_COUNT_MLAB - 1];
        }

        // prepare data to copy to structure buffer.
        for (int i = 0; i < NODE_COUNT_MLAB; ++i)
        {
            fragments[i].transmission = (uint)(transmissions[i] * TRANSMISSION_MAX);
            fragments[i].depth        = depths[i];
            fragments[i].color        = colors[i];
        }
    }

    nodeList[flattenedIndex].frags = fragments;
    return;
}
