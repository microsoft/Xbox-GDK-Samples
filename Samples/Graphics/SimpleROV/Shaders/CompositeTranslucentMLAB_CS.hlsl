//--------------------------------------------------------------------------------------
// CompositeTranslucentMLAB_CS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define USE_HLSL
#include "SharedDefinitions.h"
#include "HLSLCommon.hlsli"

ConstantBuffer<BlendPassConstants> cts      : register(b0);

Texture2D<uint> ClearMask                   : register(t0);
StructuredBuffer<NodeFragments> NodeList    : register(t1);

RWTexture2D<float4> renderTarget            : register(u0);

[RootSignature(COMPOSITE_RS)]
[numthreads(TILE_SIZE_X, TILE_SIZE_Y, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint const flattenedIndex = DTid.x + DTid.y * cts.resolution.x;

    uint isPixelClear = ClearMask[DTid.xy];
    if (isPixelClear != 0)
    {
        TransparentFragment fragments[NODE_COUNT_MLAB] = NodeList.Load(flattenedIndex).frags;

        // Get current RT color.
        float3 currentRTColor = renderTarget[DTid.xy].rgb;
        float3 accumColor = 0.0f;

        // Iterate through list and apply colors.
        float transmission = 1.0f;
        for (int i = 0; i < NODE_COUNT_MLAB && fragments[i].depth < DEPTH_MAX; ++i)
        {
            float3 color = FromRGBE(UnpackRGBA(fragments[i].color)).rgb;
            accumColor += transmission * color;
            transmission = (fragments[i].transmission / (float)TRANSMISSION_MAX);
        }

        accumColor += currentRTColor * transmission;
        renderTarget[DTid.xy] = float4(accumColor.rgb, 1.0f);
    }
}
