//--------------------------------------------------------------------------------------
// CompositeTranslucentPPLL_CS.hlsl
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#define USE_HLSL
#include "SharedDefinitions.h"
#include "HLSLCommon.hlsli"

ConstantBuffer<BlendPassConstants> cts      : register(b0);

ByteAddressBuffer perPixelLinkedListHead    : register(t0);
StructuredBuffer<Fragment> linkedListBuffer : register(t1);

RWTexture2D<float4> renderTarget            : register(u0);

// Using insertion sort, which is not the most efficient, but works
// for this example since the list has 8 elements only.
inline void SortList(uint actualLength, inout Fragment list[NODE_COUNT_PPLL]);

[RootSignature(COMPOSITE_RS)]
[numthreads(TILE_SIZE_X, TILE_SIZE_Y, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    // ByteAddressBuffers (or raw buffers) use a byte value offset from the beginning of the buffer to
    // access data. The byte value must be a multiple of four so that it is DWORD aligned. If any other
    // value is provided, behavior is undefined.
    uint const address = 4 * (DTid.x + DTid.y * cts.resolution.x);
    uint currentIndex = perPixelLinkedListHead.Load(address);
    if (currentIndex == PPLL_CLEAR_VALUE)
    {
        return;
    }

    Fragment list[NODE_COUNT_PPLL];

    uint actualLength = 0;

    // Copy structured buffer into array for this pixel's linked list.
    for (uint i = 0; i < NODE_COUNT_PPLL; ++i)
    {
        list[i] = linkedListBuffer[currentIndex];
        currentIndex = list[i].next;
        actualLength++;

        if (currentIndex == PPLL_CLEAR_VALUE)
        {
            break;
        }
    }

    // Sort the linked list.
    SortList(actualLength, list);


    float4 opaqueColor = renderTarget[DTid.xy];

    // Compute final color.
    for (uint i = 0; i < actualLength; ++i)
    {
        Fragment current = list[i];

        float4 rgbeColor = UnpackRGBA(current.color);
        float4 rgbaColor = FromRGBE(rgbeColor);
        float transmission = (current.transmission / (float)TRANSMISSION_MAX);

        opaqueColor = rgbaColor + transmission * opaqueColor;
    }

    renderTarget[DTid.xy] = opaqueColor;
}

inline void SortList(uint actualLength, inout Fragment list[NODE_COUNT_PPLL])
{
    for (uint i = 0; i < actualLength; ++i)
    {
        for (uint j = i + 1; j < actualLength; ++j)
        {
            if (list[j].depth > list[i].depth)
            {
                Fragment temp = list[i];
                list[i] = list[j];
                list[j] = temp;
            }
        }
    }
}
